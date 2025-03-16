// Joey did this but no copyright is claimed on these dummy functions.
// We need these to silence linker warnings related to the -specs=nosys.specs flag.
// Note that we do actually implement some syscalls, like _read and _write (which redirect IO to the USB CDC).
// But these syscalls never get used, and as such they don't even take up space in the resulting binary.

int _close(int fd);
int _close(int fd) {
    (void) fd;
    return -1;
}
    
int _fstat(int fd, void *buf);
int _fstat(int fd, void *buf) {
    (void) fd;
    (void) buf;
    return -1;
}
    
int _getpid(void);
int _getpid(void) {
    return 0;
}
    
int _isatty(int fd);
int _isatty(int fd) {
    (void) fd;
    return 0;
}
    
int _kill(int pid, int sig);
int _kill(int pid, int sig) {
    (void) pid;
    (void) sig;
    return -1;
}
    
int _lseek(int fd, int offset, int whence);
int _lseek(int fd, int offset, int whence) {
    (void) fd;
    (void) offset;
    (void) whence;
    return -1;
}
    
int _open(const char *path, int flags);
int _open(const char *path, int flags) {
    (void) path;
    (void) flags;
    return -1;
}
