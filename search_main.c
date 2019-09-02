#include <stdio.h>
#include <getopt.h>

#include "log.h"
#include "type.h"
#include "define.h"
#include "naive_hash_table.h"
#include "merge.h"
#include "search.h"


void usage(void)
{
    printf("\t\n");
    printf("\t--help/-h print this message\n");
    printf("\t--dir=path/-d path specify inverted data path\n");
    printf("\t--verbose/-v output details\n");
    return;
}

int
main(int argc, char *argv[])
{
    const char *inv_dir = NULL;

    char ch;
    static struct option longopts[] = {
        { "dir",     required_argument, NULL, 'd' },
        { "verbose", no_argument,       NULL, 'v' },
        { "help",    no_argument,       NULL, 'h' },
        { NULL,      0,                 NULL,  0  }
    };
    while ((ch = getopt_long(argc, argv, "d:vh", longopts, NULL)) != -1) {
        switch (ch) {
            case 'd':
                inv_dir = optarg;
                break;
            case 'v':
                break;
            case 'h':
                usage();
                break;
            default:
                usage();
                break;
        }
    }
    argc -= optind;
    argv += optind;

    if (NULL == inv_dir) {
        printf("inverted data path is not set. exit(1)\n");
        exit(1);
    }
    NaiveHashTable *ht = load_invert_index(inv_dir);
    LOG(LOG_INFO, "load invert data finish.");
    do {
        ;
    }while(1);
    unload_invert_index(ht);
    return 0;
}
