# OS Programming Assignment 02 - Report

## Feature 2: ls-v1.1.0 - Complete Long Listing Format

### Q1. What is the crucial difference between stat() and lstat()? When is lstat() more appropriate in ls?

Both system calls fill a `struct stat` with a file's metadata (type, permissions, owner, size, timestamps). They differ only when the path is a symbolic link. `stat()` follows the link and returns information about the file the link points to. `lstat()` does not follow it and returns information about the link itself.

`ls -l` should use `lstat()`. A symbolic link must be shown as a link: the type character is `l`, the size is the length of the stored target path, and the target is printed after `->`. With `stat()` a link would look like an ordinary file or directory, and a broken (dangling) link would cause an error because its target does not exist.

### Q2. How can bitwise operators and macros extract file type and permissions from st_mode?

`st_mode` is a single integer in which different bit groups carry different information. The upper bits hold the file type, and the lower bits hold the permission bits (owner, group, others) plus the special bits (setuid, setgid, sticky).

To get the file type, mask `st_mode` with `S_IFMT` and compare the result with a type constant: `(st_mode & S_IFMT) == S_IFDIR` tests for a directory. The mask is needed because the type constants share bits, so testing `st_mode & S_IFDIR` alone could give a wrong answer. The macros `S_ISDIR()`, `S_ISREG()` and `S_ISLNK()` do this correctly.

To get a permission, use bitwise AND with a single-bit constant. `st_mode & S_IRUSR` is non-zero if the owner has read permission, and `S_IWUSR`, `S_IXUSR`, `S_IRGRP`, `S_IWGRP`, `S_IXGRP`, `S_IROTH`, `S_IWOTH` and `S_IXOTH` work the same way. My `mode_to_string()` function checks these nine bits one by one to build the `rwxrwxrwx` string, and checks `S_ISUID`, `S_ISGID` and `S_ISVTX` to show `s`/`S` and `t`/`T` for the special bits.
