/*
 * MIT License
 *
 * Copyright (c) 2023 Edward Shin
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

#include "shell_cmd_list.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "filesystem.h"
#include "movement.h"
#include "watch.h"
#include "delay.h"

static int help_cmd(int argc, char *argv[]);
static int flash_cmd(int argc, char *argv[]);
static int settime_cmd(int argc, char *argv[]);
static int stress_cmd(int argc, char *argv[]);

shell_command_t g_shell_commands[] = {
    {
        .name = "?",
        .help = "print command list",
        .min_args = 0,
        .max_args = 0,
        .cb = help_cmd,
    },
    {
        .name = "help",
        .help = "print command list",
        .min_args = 0,
        .max_args = 0,
        .cb = help_cmd,
    },
    {
        .name = "flash",
        .help = "reboot to UF2 bootloader",
        .min_args = 0,
        .max_args = 0,
        .cb = flash_cmd,
    },
    {
        .name = "ls",
        .help = "usage: ls [PATH]",
        .min_args = 0,
        .max_args = 1,
        .cb = filesystem_cmd_ls,
    },
    {
        .name = "cat",
        .help = "usage: cat <PATH>",
        .min_args = 1,
        .max_args = 1,
        .cb = filesystem_cmd_cat,
    },
    {
        .name = "b64encode",
        .help = "usage: b64encode <PATH>",
        .min_args = 1,
        .max_args = 1,
        .cb = filesystem_cmd_b64encode,
    },
    {
        .name = "df",
        .help = "print filesystem free space",
        .min_args = 0,
        .max_args = 0,
        .cb = filesystem_cmd_df,
    },
    {
        .name = "rm",
        .help = "usage: rm [PATH]",
        .min_args = 1,
        .max_args = 1,
        .cb = filesystem_cmd_rm,
    },
    {
        .name = "format",
        .help = "usage: format YES",
        .min_args = 1,
        .max_args = 1,
        .cb = filesystem_cmd_format,
    },
    {
        .name = "echo",
        .help = "usage: echo TEXT {>,>>} FILE",
        .min_args = 3,
        .max_args = 3,
        .cb = filesystem_cmd_echo,
    },
    {
        .name = "settime",
        .help = "set the clock; usage: settime <unix_utc>",
        .min_args = 1,
        .max_args = 1,
        .cb = settime_cmd,
    },
    {
        .name = "stress",
        .help = "test CDC write; usage: stress [LEN] [DELAY_MS]",
        .min_args = 0,
        .max_args = 2,
        .cb = stress_cmd,
    },
};

const size_t g_num_shell_commands = sizeof(g_shell_commands) / sizeof(shell_command_t);

static int help_cmd(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    printf("Command List:\r\n");
    for (size_t i = 0; i < g_num_shell_commands; i++) {
        printf(" %s\t%s\r\n",
                g_shell_commands[i].name,
                (g_shell_commands[i].help) ? g_shell_commands[i].help : ""
        );
    }

    return 0;
}

static int flash_cmd(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    watch_reset_to_bootloader();
    return 0;
}

// Set the clock from a UTC Unix timestamp, e.g. from the host:
//   echo "settime $(date +%s)" > /dev/cu.usbmodem*
static int settime_cmd(int argc, char *argv[]) {
    (void) argc;

    char *end = NULL;
    uint32_t timestamp = strtoul(argv[1], &end, 10);
    if (end == argv[1] || *end != '\0') {
        printf("usage: settime <unix_utc>\r\n");
        return -1;
    }

    // The RTC epoch is 2020; earlier timestamps underflow into a garbage date.
    if (timestamp < 1577836800u /* 2020-01-01T00:00:00Z */) {
        printf("error: timestamp must be at or after 2020-01-01\r\n");
        return -1;
    }

    movement_set_utc_timestamp(timestamp);

    watch_date_time_t dt = movement_get_local_date_time();
    printf("time set: %04d-%02d-%02d %02d:%02d:%02d (local)\r\n",
           dt.unit.year + WATCH_RTC_REFERENCE_YEAR, dt.unit.month, dt.unit.day,
           dt.unit.hour, dt.unit.minute, dt.unit.second);
    return 0;
}

#define STRESS_CMD_MAX_LEN  (512)
static int stress_cmd(int argc, char *argv[]) {
    char test_str[STRESS_CMD_MAX_LEN+1] = {0};

    int max_len = 512;
    int delay = 0;

    if (argc >= 2) {
        if ((max_len = atoi(argv[1])) == 0) {
            return -1;
        }
        if (max_len > 512) {
            return -1;
        }
    }

    if (argc >= 3) {
        delay = atoi(argv[2]);
    }

    for (int i = 0; i < max_len; i++) {
        snprintf(&test_str[i], 2, "%u", (i+1)%10);
        printf("%u:\t%s\r\n", (i+1), test_str);
        if (delay > 0) {
            delay_ms(delay);
        }
    }

    return 0;
}
