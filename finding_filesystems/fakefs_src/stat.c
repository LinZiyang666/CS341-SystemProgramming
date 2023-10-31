/**
 * finding_filesystems
 * CS 341 - Fall 2023
 */
/* START STAT */
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

/* Obtain the original definition of struct statx.  */
#undef __statx_defined
#define statx original_statx
#include <bits/types/struct_statx.h>
#undef statx

static int __set_errno(int n) {
    errno = n;
    return -1;
}

static int fstat_common(int fd, struct stat *buf, int *retval) {
    if (dictionary_contains(fd_map, &fd)) {
        fakefile *file = *(dictionary_at(fd_map, &fd).value);
        *retval = minixfs_stat(fs, file->path, buf);
        return 1;
    }
    return 0;
}

int __fxstat64(int ver, int fd, struct stat64 *buf) {
    if (disable_hooks)
        return orig_fstat64(ver, fd, buf);
    int retval;
    int hooked = fstat_common(fd, (struct stat *)buf, &retval);
    if (hooked)
        return retval;

    return orig_fstat64(ver, fd, buf);
}
int __lxstat64(int ver, const char *pathname, struct stat64 *buf) {
    fakefs_path_t paths = fakefs_realpath(pathname);
    auto_cleanup_path(paths);
    if (paths.base_path && paths.virtual_path) {
        int fd = open(pathname, O_RDONLY);
        if (fd == -1)
            return -1;
        int retval = __fxstat64(ver, fd, buf);
        close(fd);
        return retval;
    }
    return orig_lstat64(ver, pathname, buf);
}
int __xstat64(int ver, const char *pathname, struct stat64 *buf) {
    return __lxstat64(ver, pathname, buf);
}
int __fxstatat64(int ver, int dirfd, const char *pathname, struct stat64 *buf,
                 int flags) {
    if (disable_hooks)
        return orig_fstatat64(ver, dirfd, pathname, buf, flags);

    fakefs_path_t paths = fakefs_realpath(pathname);
    auto_cleanup_path(paths);
    if (paths.base_path && paths.virtual_path) {
        int fd = open(pathname, O_RDONLY);
        if (fd == -1)
            return -1;
        int retval = __fxstat64(ver, fd, buf);
        close(fd);
        return retval;
    }
    return orig_fstatat64(ver, dirfd, pathname, buf, flags);
}
int __fstatat64(int fd, const char *file, struct stat64 *buf, int flag) {
    return __fxstatat64(0, fd, file, buf, flag);
}
int __fxstat(int ver, int fd, struct stat *buf) {
    if (disable_hooks)
        return orig_fstat(0, fd, buf);
    int retval;
    int hooked = fstat_common(fd, buf, &retval);
    if (hooked)
        return retval;
    return orig_fstat(ver, fd, buf);
}
int __lxstat(int ver, const char *pathname, struct stat *buf) {
    int fd = open(pathname, O_RDONLY);
    if (fd == -1)
        return -1;
    int retval = __fxstat(ver, fd, buf);
    close(fd);
    return retval;
}
int __xstat(int ver, const char *pathname, struct stat *buf) {
    return __lxstat(ver, pathname, buf);
}
int __fxstatat(int ver, int dirfd, const char *pathname, struct stat *buf,
               int flags) {
    if (disable_hooks)
        return orig_fstatat(ver, dirfd, pathname, buf, flags);

    fakefs_path_t paths = fakefs_realpath(pathname);
    auto_cleanup_path(paths);
    if (paths.base_path && paths.virtual_path) {
        int fd = open(pathname, O_RDONLY);
        if (fd == -1)
            return -1;
        int retval = __fxstat(ver, fd, buf);
        close(fd);
        return retval;
    }

    return orig_fstatat(ver, dirfd, pathname, buf, flags);
}

int lstat(const char *pathname, struct stat *buf) {
    return __lxstat(0, pathname, buf);
}
int stat(const char *pathname, struct stat *buf) {
    return __xstat(0, pathname, buf);
}

static inline struct statx_timestamp
statx_convert_timestamp(struct timespec tv) {
    return (struct statx_timestamp){tv.tv_sec, tv.tv_nsec, 0};
}

static int statx_generic(int fd, const char *path, int flags,
                         __attribute__((unused)) unsigned int mask,
                         struct statx *buf) {
    static const int clear_flags = AT_STATX_SYNC_AS_STAT;

    /* Flags supported by our emulation.  */
    static const int supported_flags =
        AT_EMPTY_PATH | AT_NO_AUTOMOUNT | AT_SYMLINK_NOFOLLOW | clear_flags;

    if (__glibc_unlikely((flags & ~supported_flags) != 0)) {
        __set_errno(EINVAL);
        return -1;
    }

    struct stat64 st;
    int ret = __fstatat64(fd, path, &st, flags & ~clear_flags);
    if (ret != 0)
        return ret;

    /* The interface is defined in such a way that unused (padding)
        fields have to be cleared.  STATX_BASIC_STATS corresponds to the
        data which is available via fstatat64.  */
    struct original_statx obuf = {
        .stx_mask = STATX_BASIC_STATS,
        .stx_blksize = st.st_blksize,
        .stx_nlink = st.st_nlink,
        .stx_uid = st.st_uid,
        .stx_gid = st.st_gid,
        .stx_mode = st.st_mode,
        .stx_ino = st.st_ino,
        .stx_size = st.st_size,
        .stx_blocks = st.st_blocks,
        .stx_atime = statx_convert_timestamp(st.st_atim),
        .stx_ctime = statx_convert_timestamp(st.st_ctim),
        .stx_mtime = statx_convert_timestamp(st.st_mtim),
        .stx_rdev_major = major(st.st_rdev),
        .stx_rdev_minor = minor(st.st_rdev),
        .stx_dev_major = major(st.st_dev),
        .stx_dev_minor = minor(st.st_dev),
    };
    _Static_assert(sizeof(*buf) >= sizeof(obuf), "struct statx size");
    memcpy(buf, &obuf, sizeof(obuf));

    return 0;
}

int statx(int fd, const char *path, int flags, unsigned int mask,
          struct statx *buf) {
    return statx_generic(fd, path, flags, mask, buf);
}
int fstat(int fd, struct stat *buf) {
    return __fxstat(0, fd, buf);
}
/* END STAT */
