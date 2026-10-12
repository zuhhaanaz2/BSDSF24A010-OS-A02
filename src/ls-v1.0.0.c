#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#define PATH_LEN 4096

enum { MODE_DEFAULT, MODE_LONG, MODE_HORIZONTAL };
int display_mode = MODE_DEFAULT;

void   do_ls(const char *dir);
char **read_names(const char *dir, int *count, int *maxlen);
int    cmp_names(const void *a, const void *b);
int    term_width(void);
void   print_columns(const char *dir, char **names, int n, int maxlen);
void   print_horizontal(const char *dir, char **names, int n, int maxlen);
void   print_long(const char *dir, char **names, int n);
void   show_long(const char *dir, const char *name);
void   print_name(const char *dir, const char *name);
void   mode_to_str(mode_t m, char *s);

int main(int argc, char *argv[])
{
    int opt;
    while ((opt = getopt(argc, argv, "lx")) != -1) {
        if (opt == 'l')
            display_mode = MODE_LONG;
        else if (opt == 'x')
            display_mode = MODE_HORIZONTAL;
        else {
            fprintf(stderr, "Usage: %s [-l] [-x] [dir...]\n", argv[0]);
            exit(1);
        }
    }

    if (optind == argc)
        do_ls(".");
    else {
        for (int i = optind; i < argc; i++) {
            if (argc - optind > 1)
                printf("%s:\n", argv[i]);
            do_ls(argv[i]);
        }
    }
    return 0;
}

void do_ls(const char *dir)
{
    int n, maxlen;
    char **names = read_names(dir, &n, &maxlen);
    if (names == NULL)
        return;

    qsort(names, n, sizeof(char *), cmp_names);   /* sort before displaying */

    if (display_mode == MODE_LONG)
        print_long(dir, names, n);
    else if (display_mode == MODE_HORIZONTAL)
        print_horizontal(dir, names, n, maxlen);
    else
        print_columns(dir, names, n, maxlen);

    for (int i = 0; i < n; i++)
        free(names[i]);
    free(names);
}

char **read_names(const char *dir, int *count, int *maxlen)
{
    DIR *dp = opendir(dir);
    if (dp == NULL) {
        fprintf(stderr, "Cannot open directory:%s\n", dir);
        return NULL;
    }
    int cap = 16, n = 0;
    char **names = malloc(cap * sizeof(char *));
    if (names == NULL) { perror("malloc"); exit(1); }
    *maxlen = 0;

    struct dirent *entry;
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;
        if (n == cap) {
            cap *= 2;
            names = realloc(names, cap * sizeof(char *));
            if (names == NULL) { perror("realloc"); exit(1); }
        }
        names[n] = strdup(entry->d_name);
        int len = strlen(entry->d_name);
        if (len > *maxlen)
            *maxlen = len;
        n++;
    }
    closedir(dp);
    *count = n;
    return names;
}

int cmp_names(const void *a, const void *b)
{
    const char *s1 = *(const char **)a;       /* a points to a char* element */
    const char *s2 = *(const char **)b;
    return strcmp(s1, s2);
}

int term_width(void)
{
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1 || w.ws_col == 0)
        return 80;
    return w.ws_col;
}

void print_columns(const char *dir, char **names, int n, int maxlen)
{
    if (n == 0)
        return;
    int colw = maxlen + 2;
    int cols = term_width() / colw;
    if (cols < 1) cols = 1;
    int rows = (n + cols - 1) / cols;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int i = r + c * rows;
            if (i >= n)
                break;
            print_name(dir, names[i]);
            printf("%*s", colw - (int)strlen(names[i]), "");
        }
        printf("\n");
    }
}

void print_horizontal(const char *dir, char **names, int n, int maxlen)
{
    int width = term_width();
    int colw  = maxlen + 2;
    int pos   = 0;

    for (int i = 0; i < n; i++) {
        if (pos > 0 && pos + colw > width) {
            printf("\n");
            pos = 0;
        }
        print_name(dir, names[i]);
        printf("%*s", colw - (int)strlen(names[i]), "");
        pos += colw;
    }
    if (n > 0)
        printf("\n");
}

void print_long(const char *dir, char **names, int n)
{
    long total = 0;
    struct stat st;
    char path[PATH_LEN];
    for (int i = 0; i < n; i++) {
        snprintf(path, PATH_LEN, "%s/%s", dir, names[i]);
        if (lstat(path, &st) == 0)
            total += st.st_blocks;
    }
    printf("total %ld\n", total / 2);
    for (int i = 0; i < n; i++)
        show_long(dir, names[i]);
}

void show_long(const char *dir, const char *name)
{
    char path[PATH_LEN], perm[11];
    struct stat st;

    snprintf(path, PATH_LEN, "%s/%s", dir, name);
    if (lstat(path, &st) == -1) {
        perror("lstat");
        return;
    }

    mode_to_str(st.st_mode, perm);
    struct passwd *pw = getpwuid(st.st_uid);
    struct group  *gr = getgrgid(st.st_gid);
    char *t = ctime(&st.st_mtime);

    printf("%s %3ld ", perm, (long)st.st_nlink);
    printf("%-8s ", pw ? pw->pw_name : "?");
    printf("%-8s ", gr ? gr->gr_name : "?");
    printf("%8ld ", (long)st.st_size);
    printf("%.12s ", t + 4);
    print_name(dir, name);

    if (S_ISLNK(st.st_mode)) {
        char target[PATH_LEN];
        ssize_t n = readlink(path, target, PATH_LEN - 1);
        if (n != -1) {
            target[n] = '\0';
            printf(" -> %s", target);
        }
    }
    printf("\n");
}

void print_name(const char *dir, const char *name)
{
    (void)dir;
    printf("%s", name);
}

void mode_to_str(mode_t m, char *s)
{
    strcpy(s, "----------");

    if      (S_ISDIR(m))  s[0] = 'd';
    else if (S_ISLNK(m))  s[0] = 'l';
    else if (S_ISCHR(m))  s[0] = 'c';
    else if (S_ISBLK(m))  s[0] = 'b';
    else if (S_ISFIFO(m)) s[0] = 'p';
    else if (S_ISSOCK(m)) s[0] = 's';

    if (m & S_IRUSR) s[1] = 'r';
    if (m & S_IWUSR) s[2] = 'w';
    if (m & S_IXUSR) s[3] = 'x';
    if (m & S_IRGRP) s[4] = 'r';
    if (m & S_IWGRP) s[5] = 'w';
    if (m & S_IXGRP) s[6] = 'x';
    if (m & S_IROTH) s[7] = 'r';
    if (m & S_IWOTH) s[8] = 'w';
    if (m & S_IXOTH) s[9] = 'x';

    if (m & S_ISUID) s[3] = (m & S_IXUSR) ? 's' : 'S';
    if (m & S_ISGID) s[6] = (m & S_IXGRP) ? 's' : 'S';
    if (m & S_ISVTX) s[9] = (m & S_IXOTH) ? 't' : 'T';
}
