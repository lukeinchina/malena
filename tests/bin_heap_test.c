#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <check.h>
#include "../bin_heap.h"

int greater(const void *left, const void *right) {
    const HeapElemType *l = (const HeapElemType *)left;
    const HeapElemType *r = (const HeapElemType *)right;
    if (l->key1 > r->key1
            || (l->key1 == r->key1 && l->key2 > r->key2)) {
        return 1;
    } else if (l->key1 == r->key1 && l->key2 == r->key2) {
        return 0;
    } else {
        return -1;
    }
}

START_TEST(test_bin_heap)
{
    HeapElemType nums[256];
    HeapElemType sorted_nums[256];
    HeapElemType e;

    int size = 10;
    int i;

    srand(time(NULL));
    size += rand() % 31;

    PriorityQueue h = pq_create(size, greater);
    e = pq_delete_min(h);
    ck_assert(ULLONG_MAX == e.key1);

    for (i = 0; i < size; i++) {
        nums[i].key1 = rand() % 1000 + 1;
        nums[i].key2 = 0LLU;
        nums[i].load = NULL;
        pq_insert(h, nums[i]);
    }
    for (i = 0; i < size; i++) {
        sorted_nums[i] = pq_delete_min(h);
    }
    for (i = 0; i < size; i++) {
        if (i > 0) {
            ck_assert_msg(sorted_nums[i-1].key1 <= sorted_nums[i-1].key1,
                    "%llu:%llu\n", sorted_nums[i].key1, sorted_nums[i-1].key1);
        }
    }
    e = pq_delete_min(h);
    ck_assert(ULLONG_MAX == e.key1);
}
END_TEST

START_TEST(test_heap_second_key)
{
    HeapElemType nums[256];
    HeapElemType sorted_nums[256];
    HeapElemType e;

    int size = 10;
    int i;

    srand(time(NULL));
    size += rand() % 31;

    PriorityQueue h = pq_create(size, greater);
    e = pq_delete_min(h);
    ck_assert(ULLONG_MAX == e.key1);

    for (i = 0; i < size; i++) {
        nums[i].key1 = rand() % 10 + 1;
        nums[i].key2 = rand() % 100 + 1;
        nums[i].load = NULL;
        pq_insert(h, nums[i]);
    }
    for (i = 0; i < size; i++) {
        sorted_nums[i] = pq_delete_min(h);
    }
    for (i = 0; i < size; i++) {
        if (i > 0) {
            ck_assert_msg(sorted_nums[i-1].key1 <= sorted_nums[i-1].key1,
                    "%llu:%llu\n", sorted_nums[i].key1, sorted_nums[i-1].key1);
            if (sorted_nums[i-1].key1 == sorted_nums[i].key1) {
                ck_assert_msg(sorted_nums[i-1].key2 <= sorted_nums[i-1].key2,
                        "%llu:%llu\n", sorted_nums[i].key2, sorted_nums[i-1].key2);
            }
        }
    }
    e = pq_delete_min(h);
    ck_assert(ULLONG_MAX == e.key1);
    printf("\n");
}
END_TEST

Suite * utf8_suite(void) {
	Suite *s;
	TCase *tc_bin_heap;
	TCase *tc_second_key;

	s = suite_create("bin_heap");

	/* Core test case */
	tc_bin_heap = tcase_create("BinaryHeap");
	tc_second_key = tcase_create("SecondKey");

	tcase_add_test(tc_bin_heap, test_bin_heap);
	tcase_add_test(tc_second_key, test_heap_second_key);
	suite_add_tcase(s, tc_bin_heap);
	suite_add_tcase(s, tc_second_key);

	return s;
}

int main(void) {
	int num_failed;
	Suite *s;
	SRunner *sr;

	s = utf8_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_VERBOSE);
	num_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (num_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

