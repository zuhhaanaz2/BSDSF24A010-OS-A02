# REPORT

## Feature 2: Long Listing (-l)
**Q1. stat() vs lstat()?**
stat() follows symbolic links and returns information about the file the link
points to. lstat() does not follow the link and returns information about the
link itself. ls -l should use lstat() so a symlink is shown as type 'l' with
its own size and its "-> target", instead of looking like the target file.

**Q2. Extracting information from st_mode?**
st_mode packs the file type in the high bits and the permissions in the low
bits. Use bitwise AND with a mask: (st_mode & S_IFMT) == S_IFDIR checks for a
directory (the S_ISDIR macro does this), and (st_mode & S_IRUSR) is non-zero if
the owner can read. Each permission bit has its own mask (S_IRUSR, S_IWUSR,
S_IXUSR, S_IRGRP ... S_IXOTH, plus S_ISUID, S_ISGID, S_ISVTX for special bits).

## Feature 3: Column Display
**Q1. Down-then-across logic? Why not a single loop?**
First compute colw = maxlen + 2, cols = term_width / colw, and
rows = ceil(n / cols). Then loop row by row; item (r, c) is names[r + c*rows].
Items fill the first column top to bottom, then the second, and so on. A
single loop prints names in array order (left to right), which fills across
instead of down, so each row needs items spaced `rows` apart.

**Q2. Purpose of ioctl? Limits of fixed 80 columns?**
ioctl(TIOCGWINSZ) asks the terminal for its current width in columns. With only
a fixed 80, the output would waste space in wide terminals and wrap/break the
alignment in narrow ones, and would not adapt when the window is resized.

## Feature 4: Horizontal (-x)
**Q1. Complexity: vertical vs horizontal?**
Down-then-across needs more pre-calculation: columns, then rows, and an index
formula (r + c*rows) before printing anything. Horizontal only needs the column
width and a running position counter; it prints in array order and wraps when
the next item would exceed the terminal width.

**Q2. How are display modes managed?**
A global variable display_mode (enum: MODE_DEFAULT, MODE_LONG, MODE_HORIZONTAL)
is set while parsing options with getopt. After reading the names, do_ls checks
this variable with if / else if and calls print_long, print_horizontal, or
print_columns.

## Feature 5: Alphabetical Sort
**Q1. Why read all entries before sorting? Drawbacks?**
Sorting needs every item available to compare, but readdir() returns entries
one at a time in arbitrary order. So all names must be stored first. For
directories with millions of files this uses a lot of memory and delays the
first output until everything has been read and sorted.

**Q2. qsort comparison function?**
Signature: int cmp(const void *a, const void *b). qsort is generic and works
on any type, so it passes untyped pointers to elements. Inside, cast them to the
real type: our elements are char*, so a and b are char** and we use
strcmp(*(char**)a, *(char**)b). It returns negative, zero, or positive to say
a < b, a == b, or a > b.

## Feature 6: Colors
**Q1. How do ANSI escape codes work? Green example?**
The terminal interprets the byte sequence ESC [ ... m as a formatting command
instead of text. Print the code, then the text, then a reset code. Green:
printf("\033[0;32m%s\033[0m", name);

**Q2. Which bits indicate executable?**
S_IXUSR (0100) for owner, S_IXGRP (0010) for group, S_IXOTH (0001) for others.
Check with st_mode & (S_IXUSR | S_IXGRP | S_IXOTH) to see if any of them is set.

## Feature 7: Recursive (-R)
**Q1. Base case?**
The base case is the condition that stops the recursion. In recursive ls, a
directory with no subdirectories makes the loop find nothing to recurse into
(and a directory that cannot be opened returns immediately). Skipping "." and
".." also prevents infinite recursion.

**Q2. Why a full path?**
readdir gives only the entry's name. Calling do_ls("subdir") would look for
"subdir" relative to the current working directory, not inside the parent, so
it would fail or open the wrong directory. Building "parent_dir/subdir" makes
opendir and lstat work from anywhere.
