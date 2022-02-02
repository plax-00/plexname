#define __STDC_WANT_LIB_EXT2__ 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>
#include <fts.h>
#include <string.h>
#include <errno.h>
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

char * get_title() {

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
    int tflag = 0;
    char season[10];
    int sflag = 0;
    int vflag = 0;
    int option;

    while ((option = getopt(argc, argv, "t:s:v")) != -1) {
        switch (option) {
            case 't':
                strcpy(title, optarg);
                break;
            case 's':
                strcpy(season, optarg);
                sflag = 1;
                break;
            case 'v':
                vflag = 1;
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
    printf("Season: %s\n", season);
    // if (!tflag) strcpy(title, get_title());


    // Compiling regex to match episode number and file extension
    regex_t regex_ep;
    regex_t regex_ext;
    int result;

    result = regcomp(&regex_ep, "[sS][0-9]{2}\\W*[eE]([0-9]{2})", REG_EXTENDED);
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
        if(result) fprintf(stderr, "No match: %s\n", ptr->fts_name);
        
        result = matching_substring(&regex_ext, ptr->fts_name, file_ext);

        asprintf(&newname, "%s s%se%s.mkv", title, season, episode);
        rename(ptr->fts_name, newname);
        if (vflag) printf("Renamed %s to %s\n", ptr->fts_name, newname);

        ptr = ptr->fts_link;

        free(newname);
    }


    fts_close(ftsp);
    free(cwd);
    regfree(&regex_ep);
}