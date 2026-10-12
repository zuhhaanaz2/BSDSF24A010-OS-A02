/*
* Programming Assignment 02: ls-v1.1.0
* Adds the -l (long listing) option to the base version.
* Usage:
*       $ ls
*       $ ls -l /home
*       $ ls -l /home/kali/   /etc/
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

extern int errno;

void do_ls(const char *dir);
void do_ls_long(const char *dir);
void mode_to_string(mode_t mode, char *str);

int main(int argc, char *argv[])
{
    int opt;
    int long_flag = 0;

    while ((opt = getopt(argc, argv, "l")) != -1)
    {
        switch (opt)
        {
        case 'l':
            long_flag = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [-l] [directory...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        if (long_flag)
            do_ls_long(".");
        else
            do_ls(".");
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            printf("Directory listing of %s : \n", argv[i]);
            if (long_flag)
                do_ls_long(argv[i]);
            else
                do_ls(argv[i]);
            puts("");
        }
    }
    return 0;
}

/* Reads all non-hidden names of a directory into a dynamic array.
 * Returns the array (NULL if the directory cannot be opened).
 * Sets *count to the number of names and *maxlen to the longest name length. */
static char **read_names(const char *dir, int *count, size_t *maxlen)
{
    struct dirent *entry;
    int capacity = 16;
    int n = 0;

    *count = 0;
    *maxlen = 0;

    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return NULL;
    }

    char **names = malloc(capacity * sizeof(char *));
    if (names == NULL)
    {
        perror("malloc");
        closedir(dp);
        return NULL;
    }

    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        /* array full: double its capacity */
        if (n == capacity)
        {
            capacity *= 2;
            char **tmp = realloc(names, capacity * sizeof(char *));
            if (tmp == NULL)
            {
                perror("realloc");
                break;
            }
            names = tmp;
        }

        names[n] = strdup(entry->d_name);
        if (names[n] == NULL)
        {
            perror("strdup");
            break;
        }

        size_t len = strlen(entry->d_name);
        if (len > *maxlen)
            *maxlen = len;
        n++;
    }

    if (errno != 0)
        perror("readdir failed");

    closedir(dp);
    *count = n;
    return names;
}

/* Terminal width in columns (80 if it cannot be determined) */
static int get_terminal_width(void)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
        return 80;
    return ws.ws_col;
}

/* Default display: "down then across" columns */
static void print_columns(char **names, int count, size_t maxlen)
{
    int term_width = get_terminal_width();
    int col_width = (int)maxlen + 2;
    int cols = term_width / col_width;
    if (cols < 1)
        cols = 1;
    int rows = (count + cols - 1) / cols;

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int idx = c * rows + r;
            if (idx >= count)
                break;
            printf("%-*s", col_width, names[idx]);
        }
        printf("\n");
    }
}

/* Default listing: read all names, then print them in columns */
void do_ls(const char *dir)
{
    int count;
    size_t maxlen;
    char **names = read_names(dir, &count, &maxlen);
    if (names == NULL)
        return;

    print_columns(names, count, maxlen);

    for (int i = 0; i < count; i++)
        free(names[i]);
    free(names);
}

/* Converts st_mode into a string like "drwxr-xr-x" */
void mode_to_string(mode_t mode, char *str)
{
    /* character 0: file type */
    if (S_ISDIR(mode))
        str[0] = 'd';
    else if (S_ISLNK(mode))
        str[0] = 'l';
    else if (S_ISCHR(mode))
        str[0] = 'c';
    else if (S_ISBLK(mode))
        str[0] = 'b';
    else if (S_ISFIFO(mode))
        str[0] = 'p';
    else if (S_ISSOCK(mode))
        str[0] = 's';
    else
        str[0] = '-';

    /* owner */
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';
    /* group */
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    /* others */
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';

    /* special bits: setuid, setgid, sticky */
    if (mode & S_ISUID)
        str[3] = (mode & S_IXUSR) ? 's' : 'S';
    if (mode & S_ISGID)
        str[6] = (mode & S_IXGRP) ? 's' : 'S';
    if (mode & S_ISVTX)
        str[9] = (mode & S_IXOTH) ? 't' : 'T';

    str[10] = '\0';
}

/* number of digits needed to print n */
static int num_digits(unsigned long n)
{
    int d = 1;
    while (n >= 10)
    {
        n /= 10;
        d++;
    }
    return d;
}

/* user name for a uid, or the number if there is no such user */
static void get_user_name(uid_t uid, char *buf, size_t size)
{
    struct passwd *pw = getpwuid(uid);
    if (pw != NULL)
        snprintf(buf, size, "%s", pw->pw_name);
    else
        snprintf(buf, size, "%u", (unsigned)uid);
}

/* group name for a gid, or the number if there is no such group */
static void get_group_name(gid_t gid, char *buf, size_t size)
{
    struct group *gr = getgrgid(gid);
    if (gr != NULL)
        snprintf(buf, size, "%s", gr->gr_name);
    else
        snprintf(buf, size, "%u", (unsigned)gid);
}

/* Long listing: one detailed, column-aligned line per file */
void do_ls_long(const char *dir)
{
    struct dirent *entry;
    struct stat st;
    char path[PATH_MAX];
    char perms[11];
    char user[64], group[64];

    int w_link = 1, w_user = 0, w_group = 0, w_size = 1;
    long long total_blocks = 0;

    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    /* ---- pass 1: find the widest value in each column ---- */
    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
        if (lstat(path, &st) == -1)
            continue;

        int n;
        n = num_digits((unsigned long)st.st_nlink);
        if (n > w_link)
            w_link = n;

        get_user_name(st.st_uid, user, sizeof(user));
        n = (int)strlen(user);
        if (n > w_user)
            w_user = n;

        get_group_name(st.st_gid, group, sizeof(group));
        n = (int)strlen(group);
        if (n > w_group)
            w_group = n;

        n = num_digits((unsigned long)st.st_size);
        if (n > w_size)
            w_size = n;

        total_blocks += st.st_blocks;
    }

    /* st_blocks counts 512-byte units; ls prints the total in 1K units */
    printf("total %lld\n", total_blocks / 2);

    /* ---- pass 2: print every line using those widths ---- */
    rewinddir(dp);
    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
        if (lstat(path, &st) == -1)
        {
            perror("lstat");
            continue;
        }

        mode_to_string(st.st_mode, perms);
        get_user_name(st.st_uid, user, sizeof(user));
        get_group_name(st.st_gid, group, sizeof(group));
        char *timestr = ctime(&st.st_mtime);

        /* real ls shows the year for files older than ~6 months (or in the future) */
        char timebuf[32];
        double age = difftime(time(NULL), st.st_mtime);
        if (age > 15778476 || age < 0)
            snprintf(timebuf, sizeof(timebuf), "%.6s  %.4s", timestr + 4, timestr + 20);
        else
            snprintf(timebuf, sizeof(timebuf), "%.12s", timestr + 4);

        printf("%s %*lu %-*s %-*s %*ld %s %s",
               perms,
               w_link, (unsigned long)st.st_nlink,
               w_user, user,
               w_group, group,
               w_size, (long)st.st_size,
               timebuf,
               entry->d_name);

        /* for a symbolic link, also show where it points */
        if (S_ISLNK(st.st_mode))
        {
            char linkbuf[PATH_MAX];
            ssize_t len = readlink(path, linkbuf, sizeof(linkbuf) - 1);
            if (len != -1)
            {
                linkbuf[len] = '\0';
                printf(" -> %s", linkbuf);
            }
        }
        printf("\n");
    }

    if (errno != 0)
    {
        perror("readdir failed");
    }

    closedir(dp);
}
