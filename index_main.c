#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#include <unistd.h>
#include <sys/stat.h>

#include "log.h"
#include "md5.h"
#include "util.h"
#include "index.h"

char *
fake_title(char *title, const char *text) {
    int count = 0;
    const char *begin = text;
    while ('\0' != *text && count < 20) {
        /* skip white space */
        for (;'\0' != *text && isspace(*text);text++) {
        }
        /* word */
        for (;'\0' != *text && !isspace(*text); text++) {
        }
        count += 1;
        assert(text - begin < 256);
        strncpy(title, begin, text-begin);
        title[text-begin] = '\0';
    }
    return title;
}

int
main(int argc, char *argv[]) {
	/*
	*/
    struct stat st;
	const char *path = NULL;
    const char *output_dir = NULL;
	if (argc != 3) {
        printf("usage:%s segment_dat  output_dir\n", argv[0]);
        exit(1);
	}
    path       = argv[1];
    output_dir = argv[2];

    if (access(argv[1], F_OK | R_OK) != 0) {
        LOG(LOG_FATAL, "file [%s] does not exist, or can not be read\n", argv[1]);
        perror("failed:");
        exit(1);
    }

    stat(argv[2], &st);
    if (!S_ISDIR(st.st_mode)) {
        LOG(LOG_FATAL, "dir [%s] ", argv[2]);
        perror("failed:");
        exit(1);
    }

    create_static_index(path, output_dir); 
	printf("\n");
	return 0;
}
