#include <stdio.h>
#include <getopt.h>
#include <ctype.h>

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

void print_docid(const docid_t *docids, int size)
{
    int i;
    printf("total count: %d\n", size);
    for (i = 0; i < size; i++) {
        printf("%u%c", docids[i], i+1 == size ? '\n' : '\t');
    }
}

int
main(int argc, char *argv[])
{
    const char *inv_dir = NULL;
    char query[QUERY_LEN_MAX] = {0};
    const TermInvCell *query_terms[QUERY_TERM_MAX];
    docid_t common_docids[COMMON_DOCID_MAX];

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

    termid_t ids[QUERY_TERM_MAX] = {0};
    NaiveHashTable *ht = load_invert_index(inv_dir);
    int len   = 0;
    int count = 0;
    int inv_count = 0;
    int docid_count = 0;
    LOG(LOG_INFO, "load invert data finish.");
    do {
        printf("input query >>");
        fgets(query, sizeof(query), stdin);
        len = strlen(query);
        while (len > 0 && isspace(query[len-1])) {
            query[--len] = '\0';
        }
        if (strcmp(query, "quit") == 0 || strcmp(query, "exit") == 0) {
            break;
        }
        printf("%s\n", query);
        count = query_to_termids(query, ids, QUERY_TERM_MAX);
        inv_count = fetch_term_index(ht, ids, count, query_terms);
        if (inv_count != count) {
            LOG(LOG_ERROR, "[%s] find term index failed. expect = %d, return = %d",
                    query, count, inv_count);
            continue;
        }
        docid_count = common_docs(query_terms, count, common_docids, COMMON_DOCID_MAX);
        print_docid(common_docids, docid_count);
    }while(1);
    unload_invert_index(ht);
    return 0;
}
