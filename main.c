#define __STDC_WANT_LIB_EXT2__ 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>
#include <fts.h>
#include <string.h>
#include <stdbool.h>
#include <libgen.h>
#include <sys/stat.h>

#include "utils.h"

void print_usage() {
    puts("usage: plexname -t [title] -s [season #] -m [TV directory]");
}

int main(int argc, char * argv[]) {
    char title[200];
    bool tflag = false;

    char season[10];
    bool sflag = false;

    char ep_pattern[100];
    bool eflag = false;

    char library_path[100];
    char * newpath;
    bool mflag = false;

    bool vflag = false;

    int option;

    // Processing options
    while (option = getopt(argc, argv, "t:s:e:m:v")) {
        switch (option) {
            case 't':
                strcpy(title, optarg);
                tflag = true;
                break;
            case 's':
                strcpy(season, optarg);
                sflag = true;
                break;
            case 'e':
                strcpy(ep_pattern, optarg);
                eflag = true;
                break;
            case 'm':
                strcpy(library_path, optarg);
                mflag = true;
                break;
            case 'v':
                vflag = true;
                break;
            case -1:
            default:
                print_usage();
                exit(1);
        }
    }

    if (!sflag) {
        int result = get_season(season);
        if (result) {
            fprintf(stderr, "Error: Unable to determine season number\n");
            exit(1);
        } else {
            sflag = true;
        }
    }

    if (!tflag) get_title(title);

    if (mflag) {
        if (!(sflag && tflag)) {
            puts("To use the -m option you must specify the title and season with -t and -s.");
            exit(1);
        }
        char * title_dir;
        asprintf(&title_dir, "%s/%s/", library_path, title);
        mkdir(title_dir, 0755);

        asprintf(&newpath, "%s/%s/Season %s/", library_path, title, season);
        mkdir(newpath, 0755);

        free(title_dir);
    } else {
        newpath = malloc(2);
        strcpy(newpath, "");
    }


    // Compiling regex to match episode number and file extension
    regex_t regex_ep;
    regex_t regex_ext;
    int result;

    if (!eflag) {
        strcpy(ep_pattern, "[sS][0-9]{2}\\W*[eE]([0-9]{2})");
    }
    result = regcomp(&regex_ep, ep_pattern, REG_EXTENDED);
    if (result) {
        fprintf(stderr, "error: Could not compile regex.");
        exit(1);
    }
    result = regcomp(&regex_ext, "\\.(\\w+)$", REG_EXTENDED);
    if (result) {
        fprintf(stderr, "error: Could not compile regex.");
        exit(1);
    }


    // Getting linked list of files in current working directory
    char * cwd = getcwd(NULL, 0);
    char * path_argv[] = {cwd, NULL};

    FTS * ftsp = fts_open(path_argv, FTS_LOGICAL, NULL);
    fts_read(ftsp);    // Because fts_children doesn't work right without this for some reason
    FTSENT * files = fts_children(ftsp, FTS_NAMEONLY);


    // Iterate through linked list and check if filenames match regex
    FTSENT * ptr = files;

    while (ptr != NULL) {
        char episode[10];
        char file_ext[10];
        char * newname;

        result = matching_substring(&regex_ep, ptr->fts_name, episode);
        if(result) {
            fprintf(stderr, "No match: %s\n", ptr->fts_name);
            ptr = ptr->fts_link;
            continue;
        }

        result = matching_substring(&regex_ext, ptr->fts_name, file_ext);

        asprintf(&newname, "%s%s s%se%s.%s", newpath, title, season, episode, file_ext);
        rename(ptr->fts_name, newname);
        if (vflag) {
            if (!strcmp(ptr->fts_name, newname)) {
                printf("No change: %s\n", ptr->fts_name);
            } else {
                printf("Renamed %s to %s\n", ptr->fts_name, newname);
            }
        }

        ptr = ptr->fts_link;

        free(newname);
    }


    fts_close(ftsp);
    free(cwd);
    free(newpath);
    regfree(&regex_ep);
}

