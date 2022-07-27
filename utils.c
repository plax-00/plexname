#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

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
