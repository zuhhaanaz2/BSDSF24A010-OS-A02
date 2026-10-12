# OS Programming Assignment 02 - Report

## Feature 2: ls-v1.1.0 - Complete Long Listing Format

### Q1. What is the crucial difference between stat() and lstat()? When is lstat() more appropriate in ls?

Both system calls fill a `struct stat` with a file's metadata (type, permissions, owner, size, timestamps). They differ only when the path is a symbolic link. `stat()` follows the link and returns information about the file the link points to. `lstat()` does not follow it and returns information about the link itself.

`ls -l` should use `lstat()`. A symbolic link must be shown as a link: the type character is `l`, the size is the length of the stored target path, and the target is printed after `->`. With `stat()` a link would look like an ordinary file or directory, and a broken (dangling) link would cause an error because its target does not exist.

### Q2. How can bitwise operators and macros extract file type and permissions from st_mode?

`st_mode` is a single integer in which different bit groups carry different information. The upper bits hold the file type, and the lower bits hold the permission bits (owner, group, others) plus the special bits (setuid, setgid, sticky).

To get the file type, mask `st_mode` with `S_IFMT` and compare the result with a type constant: `(st_mode & S_IFMT) == S_IFDIR` tests for a directory. The mask is needed because the type constants share bits, so testing `st_mode & S_IFDIR` alone could give a wrong answer. The macros `S_ISDIR()`, `S_ISREG()` and `S_ISLNK()` do this correctly.

To get a permission, use bitwise AND with a single-bit constant. `st_mode & S_IRUSR` is non-zero if the owner has read permission, and `S_IWUSR`, `S_IXUSR`, `S_IRGRP`, `S_IWGRP`, `S_IXGRP`, `S_IROTH`, `S_IWOTH` and `S_IXOTH` work the same way. My `mode_to_string()` function checks these nine bits one by one to build the `rwxrwxrwx` string, and checks `S_ISUID`, `S_ISGID` and `S_ISVTX` to show `s`/`S` and `t`/`T` for the special bits.

## Feature 3: ls-v1.2.0 - Column Display (Down Then Across)

### Q1. Explain the general logic for printing items in a "down then across" columnar format. Why is a simple single loop through the filenames insufficient?

First all filenames are read into an array and the longest name is found. The column width is the longest name plus 2 spaces. The number of columns is `terminal_width / column_width` (at least 1), and the number of rows is `ceil(total_files / columns)`. The names are arranged so that they run down the first column, then down the second, and so on. To print this, the program loops over rows in the outer loop and over columns in the inner loop. The item printed for row `r` and column `c` is `names[c * rows + r]`.

A single loop through the array is not enough because the array order follows the columns, while the terminal prints line by line. Names that are neighbours in the array (for example index 0 and 1) belong in the same column, which means on different lines. The names on one printed line are therefore spread across the array, `rows` positions apart, and a single sequential loop cannot jump to them. The row-by-row loop with the index formula is needed.

### Q2. What is the purpose of the ioctl system call here? What are the limitations of a fixed 80-column fallback?

`ioctl` with the `TIOCGWINSZ` request asks the terminal driver for the window size and fills a `struct winsize`. The program reads `ws_col` to learn the current terminal width in columns, so the number of columns can adapt to the window.

If only a fixed width of 80 were used, the layout would ignore the real window. On a wide terminal, much of the screen would be wasted because too few columns would be printed. On a narrower terminal (fewer than 80 columns), lines would exceed the width and wrap in the middle of the output, breaking the alignment. The layout also would not change when the user resizes the window. The 80-column value is only used as a fallback when `ioctl` fails, for example when the output is redirected to a file or a pipe.
