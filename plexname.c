#define __STDC_WANT_LIB_EXT2__ 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>
#include <fts.h>
#include <string.h>
#include <stdbool.h>
#include <libgen.h>


void print_usage() {
    puts("usage: plexname -t [title] -s [season #]");
}

int matching_substring(regex_t * regex, char * search_str, char * buffer) {
    // Searches search_str with the compiled regex and writes the
    // matching substring marked by () in regex into buffer
    // Not very robust but works for the needs of this program
    regmatch_t pmatch[2];
    int result;

    result = regexec(regex, search_str, 2, pmatch, 0);
    if (!result) {
        int size = pmatch[1].rm_eo - pmatch[1].rm_so;
        strncpy(buffer, &(search_str[pmatch[1].rm_so]), size);
        buffer[size] = '\0';
    } else {
        return -1;
    }

    return 0;
}

int get_title(char * buffer) {
    // Gets title from parent directory's parent directory's  
    // name and writes it to buffer
    char * cwd = getcwd(NULL, 0);
    char * title = basename(dirname(cwd));

    strcpy(buffer, title);
    free(cwd);
    
    return 0;
}

int get_season(char * buffer) {
    // Gets season number from parent directory name 
    // and writes it to buffer
    char * cwd = getcwd(NULL, 0);
    char * parent = basename(cwd);

    // Compiling regex to match season number
    regex_t regex_szn;
    int result;

    result = regcomp(&regex_szn, "season\\s*([0-9]+)$", REG_EXTENDED | REG_ICASE);
    if (result) {
        fprintf(stderr, "error: Could not compile regex.");
        exit(1);
    }

    result = matching_substring(&regex_szn, parent, buffer);

    free(cwd);
    regfree(&regex_szn);

    if (result) return -1;
    return 0;
}

int main(int argc, char * argv[]) {
    // Processing options
    char title[200]; 
    bool tflag = false;
    char season[10];
    bool sflag = false;
    bool eflag = false;
    char ep_pattern[100];
    bool vflag = false;
    bool mflag = false;
    int option;

    while ((option = getopt(argc, argv, "t:s:e:vm")) != -1) {
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
            case 'v':
                vflag = true;
                break;
            case 'm':
                mflag = true; 
                break;
            default:
                print_usage();
                exit(1);
        }
    }

    if (!sflag) {
        int result = get_season(season);
        if (result) {
            fprintf(stderr, "error: Unable to determine season number");
            exit(1);
        }
    }
    if (!tflag) get_title(title);

    if (mflag) {
        if (!(sflag && tflag)) {
            print_usage();
            exit(1);
        }

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

        asprintf(&newname, "%s s%se%s.%s", title, season, episode, file_ext);
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
    regfree(&regex_ep);
}