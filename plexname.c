#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>
#include <fts.h>
#include <string.h>

int main(int argc, char ** argv) {
    char * directory = argv[1];
    // if (argc > 1) {
    //     for (int i = 1; i < argc; i++) {
    //         printf("%s\n", argv[i]);
    //     }
    // }

    // regex_t regex;
    // int result;

    // result = regcomp(&regex, "h\\w*\\s{3}w\\w*", 0);
    // if (result) {
    //     fprintf(stderr, "Could not compile regex.");
    //     exit(1);
    // }

    // result = regexec(&regex, "hello   world", 0, NULL, 0);
    // if (!result) puts("Match");
    // if (result == REG_NOMATCH) puts("No match");

    // regfree(&regex);

    char * paths[] = {directory, NULL};

    FTS * files = fts_open(paths, FTS_LOGICAL, NULL);
    fts_read(files);    // Because fts_children doesn't work right without this for some reason
    FTSENT * filelist = fts_children(files, 0);

    FTSENT * ptr = filelist;

    while (ptr != NULL) {
        printf("%s\n", ptr->fts_name);
        ptr = ptr->fts_link;
    }
    fts_close(files);
}