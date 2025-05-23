/*
 * MIT License
 *
 * Copyright (c) 2022-2024 Joey Castillo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesystem.h"
#include "watch.h"
#include "lfs.h"
#include "base64.h"
#include "delay.h"

#ifndef min
#define min(x, y) ((x) > (y) ? (y) : (x))
#endif

int lfs_storage_read(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
int lfs_storage_prog(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
int lfs_storage_erase(const struct lfs_config *cfg, lfs_block_t block);
int lfs_storage_sync(const struct lfs_config *cfg);

int lfs_storage_read(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
    (void) cfg;
    return !watch_storage_read(block, off, (void *)buffer, size);
}

int lfs_storage_prog(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    (void) cfg;
    return !watch_storage_write(block, off, (void *)buffer, size);
}

int lfs_storage_erase(const struct lfs_config *cfg, lfs_block_t block) {
    (void) cfg;
    return !watch_storage_erase(block);
}

int lfs_storage_sync(const struct lfs_config *cfg) {
    (void) cfg;
    return !watch_storage_sync();
}

const struct lfs_config watch_lfs_cfg = {
    // block device operations
    .read  = lfs_storage_read,
    .prog  = lfs_storage_prog,
    .erase = lfs_storage_erase,
    .sync  = lfs_storage_sync,

    // block device configuration
    .read_size = 16,
    .prog_size = NVMCTRL_PAGE_SIZE,
    .block_size = NVMCTRL_ROW_SIZE,
    .block_count = NVMCTRL_RWWEE_PAGES / 4,
    .cache_size = NVMCTRL_PAGE_SIZE,
    .lookahead_size = 16,
    .block_cycles = 100,
};

lfs_t eeprom_filesystem;
static lfs_file_t file;
static struct lfs_info info;

static int _traverse_df_cb(void *p, lfs_block_t block) {
    (void) block;
	uint32_t *nb = p;
	*nb += 1;
	return 0;
}

int32_t filesystem_get_free_space(void) {
	int err;

	uint32_t free_blocks = 0;
	err = lfs_fs_traverse(&eeprom_filesystem, _traverse_df_cb, &free_blocks);
	if(err < 0){
		return err;
	}

	uint32_t available = watch_lfs_cfg.block_count * watch_lfs_cfg.block_size - free_blocks * watch_lfs_cfg.block_size;

	return (int32_t)available;
}

static int filesystem_ls(lfs_t *lfs, const char *path) {
    lfs_dir_t dir;
    int err = lfs_dir_open(lfs, &dir, path);
    if (err < 0) {
        return err;
    }

    struct lfs_info info;
    while (true) {
        int res = lfs_dir_read(lfs, &dir, &info);
        if (res < 0) {
            return res;
        }

        if (res == 0) {
            break;
        }

        switch (info.type) {
            case LFS_TYPE_REG: printf("file "); break;
            case LFS_TYPE_DIR: printf("dir  "); break;
            default:           printf("?    "); break;
        }

        printf("%4ld bytes ", info.size);

        printf("%s\r\n", info.name);
    }

    err = lfs_dir_close(lfs, &dir);
    if (err < 0) {
        return err;
    }

    return 0;
}

bool filesystem_init(void) {
    int err = lfs_mount(&eeprom_filesystem, &watch_lfs_cfg);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err < 0) {
        printf("Ignore that error! Formatting filesystem...\r\n");
        err = lfs_format(&eeprom_filesystem, &watch_lfs_cfg);
        if (err < 0) return false;
        err = lfs_mount(&eeprom_filesystem, &watch_lfs_cfg) == LFS_ERR_OK;
        printf("Filesystem mounted with %ld bytes free.\r\n", filesystem_get_free_space());
    }

    return err == LFS_ERR_OK;
}

int _filesystem_format(void);
int _filesystem_format(void) {
    int err = lfs_unmount(&eeprom_filesystem);
    if (err < 0) {
        printf("Couldn't unmount - continuing to format, but you should reboot afterwards!\r\n");
    }

    err = lfs_format(&eeprom_filesystem, &watch_lfs_cfg);
    if (err < 0) return err;

    err = lfs_mount(&eeprom_filesystem, &watch_lfs_cfg);
    if (err < 0) return err;
    printf("Filesystem re-mounted with %ld bytes free.\r\n", filesystem_get_free_space());
    return 0;
}

bool filesystem_file_exists(char *filename) {
    info.type = 0;
    lfs_stat(&eeprom_filesystem, filename, &info);
    return info.type == LFS_TYPE_REG;
}

bool filesystem_rm(char *filename) {
    info.type = 0;
    lfs_stat(&eeprom_filesystem, filename, &info);
    if (filesystem_file_exists(filename)) {
        return lfs_remove(&eeprom_filesystem, filename) == LFS_ERR_OK;
    } else {
        printf("rm: %s: No such file\r\n", filename);
        return false;
    }
}

int32_t filesystem_get_file_size(char *filename) {
    if (filesystem_file_exists(filename)) {
        return info.size; // info struct was just populated by filesystem_file_exists
    }

    return -1;
}

bool filesystem_read_file(char *filename, char *buf, int32_t length) {
    memset(buf, 0, length);
    int32_t file_size = filesystem_get_file_size(filename);
    if (file_size > 0) {
        int err = lfs_file_open(&eeprom_filesystem, &file, filename, LFS_O_RDONLY);
        if (err < 0) return false;
        err = lfs_file_read(&eeprom_filesystem, &file, buf, min(length, file_size));
        if (err < 0) return false;
        return lfs_file_close(&eeprom_filesystem, &file) == LFS_ERR_OK;
    }

    return false;
}

bool filesystem_read_line(char *filename, char *buf, int32_t *offset, int32_t length) {
    memset(buf, 0, length + 1);
    int32_t file_size = filesystem_get_file_size(filename);
    if (file_size > 0) {
        int err = lfs_file_open(&eeprom_filesystem, &file, filename, LFS_O_RDONLY);
        if (err < 0) return false;
        err = lfs_file_seek(&eeprom_filesystem, &file, *offset, LFS_SEEK_SET);
        if (err < 0) return false;
        err = lfs_file_read(&eeprom_filesystem, &file, buf, min(length - 1, file_size - *offset));
        if (err < 0) return false;
        for(int i = 0; i < length; i++) {
            (*offset)++;
            if (buf[i] == '\n') {
                buf[i] = 0;
                break;
            }
        }
        return lfs_file_close(&eeprom_filesystem, &file) == LFS_ERR_OK;
    }

    return false;
}

static void filesystem_cat(char *filename) {
    info.type = 0;
    lfs_stat(&eeprom_filesystem, filename, &info);
    if (filesystem_file_exists(filename)) {
        if (info.size > 0) {
            char *buf = malloc(info.size + 1);
            filesystem_read_file(filename, buf, info.size);
            buf[info.size] = '\0';
            printf("%s\r\n", buf);
            free(buf);
        } else {
            printf("\r\n");
        }
    } else {
        printf("cat: %s: No such file\r\n", filename);
    }
}

bool filesystem_write_file(char *filename, char *text, int32_t length) {
    if (filesystem_get_free_space() <= 256) {
        printf("No free space!\n");
        return false;    
    }

    int err = lfs_file_open(&eeprom_filesystem, &file, filename, LFS_O_RDWR | LFS_O_CREAT | LFS_O_TRUNC);
    if (err < 0) return false;
    err = lfs_file_write(&eeprom_filesystem, &file, text, length);
    if (err < 0) return false;
    return lfs_file_close(&eeprom_filesystem, &file) == LFS_ERR_OK;
}

bool filesystem_append_file(char *filename, char *text, int32_t length) {
    if (filesystem_get_free_space() <= 256) {
        printf("No free space!\n");
        return false;    
    }

    int err = lfs_file_open(&eeprom_filesystem, &file, filename, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND);
    if (err < 0) return false;
    err = lfs_file_write(&eeprom_filesystem, &file, text, length);
    if (err < 0) return false;
    return lfs_file_close(&eeprom_filesystem, &file) == LFS_ERR_OK;
}

int filesystem_cmd_ls(int argc, char *argv[]) {
    if (argc >= 2) {
        filesystem_ls(&eeprom_filesystem, argv[1]);
    } else {
        filesystem_ls(&eeprom_filesystem, "/");
    }
    return 0;
}

int filesystem_cmd_cat(int argc, char *argv[]) {
    (void) argc;
    filesystem_cat(argv[1]);
    return 0;
}

int filesystem_cmd_b64encode(int argc, char *argv[]) {
    (void) argc;
    info.type = 0;
    lfs_stat(&eeprom_filesystem, argv[1], &info);
    if (filesystem_file_exists(argv[1])) {
        if (info.size > 0) {
            char *buf = malloc(info.size + 1);
            filesystem_read_file(argv[1], buf, info.size);
            // print a base 64 encoding of the file, 12 bytes at a time
            for (lfs_size_t i = 0; i < info.size; i += 12) {
                lfs_size_t len = min(12, info.size - i);
                char base64_line[17];
                b64_encode((unsigned char *)buf + i, len, (unsigned char *)base64_line);
                printf("%s\n", base64_line);
                delay_ms(10);
            }
            free(buf);
        } else {
            printf("\r\n");
        }
    } else {
        printf("b64encode: %s: No such file\r\n", argv[1]);
    }
    return 0;
}

int filesystem_cmd_df(int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    printf("free space: %ld bytes\r\n", filesystem_get_free_space());
    return 0;
}

int filesystem_cmd_rm(int argc, char *argv[]) {
    (void) argc;
    filesystem_rm(argv[1]);
    return 0;
}

int filesystem_cmd_format(int argc, char *argv[]) {
    (void) argc;
    if(strcmp(argv[1], "YES") == 0) {
        return _filesystem_format();
    }
    printf("usage: format YES\r\n");
    return 1;
}


int filesystem_cmd_echo(int argc, char *argv[]) {
    (void) argc;

    char *line = argv[1];
    size_t line_len = strlen(line);
    if (line[0] == '"' || line[0] == '\'') {
        line++;
        line_len -= 2;
        line[line_len] = '\0';
    }

    if (strchr(argv[3], '/')) {
        printf("subdirectories are not supported\r\n");
        return -2;
    }

    if (!strcmp(argv[2], ">")) {
        filesystem_write_file(argv[3], line, line_len);
        filesystem_append_file(argv[3], "\n", 1);
    } else if (!strcmp(argv[2], ">>")) {
        filesystem_append_file(argv[3], line, line_len);
        filesystem_append_file(argv[3], "\n", 1);
    } else {
        return -2;
    }

    return 0;
}
