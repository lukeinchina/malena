#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <check.h>

#include "naive_hash_table.h"

START_TEST(test_naive_hash_table)
{
	const static int CASE_NUM = 1000;
	size_t i;
	int    ret = 0;
	void *val = NULL;
	uint64_t cases[CASE_NUM];
	struct NaiveHashTable *table = ht_create(100);
	struct NaiveHashNode  *node  = NULL;
	for (i = 0 ; i < CASE_NUM; i++) {
		cases[i] = rand();
		val = (void *)(0XFF0000+cases[i]);
		if (NULL == ht_lookup(table, cases[i])) {
			ret = ht_insert(table, cases[i], val);
			ck_assert_int_eq(0, ret);
		} else {
			ret = ht_insert(table, cases[i], val);
			ck_assert_int_eq(-1, ret);
		}
	}
	for (i = 0 ; i < CASE_NUM; i++) {
		node = ht_lookup(table, cases[i]);
		ck_assert_ptr_ne(NULL, node);
		ck_assert(0XFF0000+cases[i] == (uint64_t)(node->val));
	}
	table = ht_destory(table);
	ck_assert_ptr_eq(NULL, table);
}
END_TEST

START_TEST(test_naive_ht_iter)
{
	const static unsigned long CASE_NUM = 10;
	const static unsigned long BUCKET_SIZE = 61;
	unsigned long i;
	// uint64_t cases[];

	/* hash table size = 61 */
	NaiveHashTable *table = ht_create(50);
	NaiveHashNode  *node  = NULL;
	NaiveHashIter  *iter  = ht_iter_new(table);
	iter = ht_iter_begin(iter);
	ck_assert_int_ne(0, ht_iter_isend(iter));

	/* locate in first bucket */
	for (i = 1; i <= CASE_NUM; i++) {
		ht_insert(table, i*BUCKET_SIZE, NULL);
	}

	i = 0LU;
	iter = ht_iter_begin(iter);
	ck_assert(0 == ht_iter_isend(iter));
	while (ht_iter_isend(iter) == 0) {
		node = ht_iter_entry(iter);
		iter = ht_iter_next(iter);
		i += 1;
	}
	ck_assert_msg(i == CASE_NUM, "%lu:%lu\n", i, CASE_NUM);


	/* locate in the last bucket */
	for (i = 1; i <= CASE_NUM; i++) {
		ht_insert(table, i*BUCKET_SIZE - 1, NULL);
	}
	i = 0LU;
	iter = ht_iter_begin(iter);
	ck_assert(0 == ht_iter_isend(iter));
	while (ht_iter_isend(iter) == 0) {
		node = ht_iter_entry(iter);
		iter = ht_iter_next(iter);
		i += 1;
	}
	ck_assert_msg(i == 2*CASE_NUM, "%lu:%lu\n", i, 2*CASE_NUM);

	/* locate in others */
	for (i = 1; i <= CASE_NUM; i++) {
		ht_insert(table, i*100, NULL);
	}
	i = 0LU;
	iter = ht_iter_begin(iter);
	ck_assert(0 == ht_iter_isend(iter));
	while (ht_iter_isend(iter) == 0) {
		node = ht_iter_entry(iter);
		iter = ht_iter_next(iter);
		i += 1;
	}
	ck_assert_msg(i == 3*CASE_NUM, "%lu:%lu\n", i, 3*CASE_NUM);

	table = ht_destory(table);
}
END_TEST

Suite * naive_hash_table_suite(void) {
	Suite *s;
	TCase *tc_core;
	TCase *tc_iter;

	s = suite_create("naive_hash_table");

	tc_core = tcase_create("hashtable core");
	tc_iter = tcase_create("hashtalbe iter");

	tcase_add_test(tc_core, test_naive_hash_table);
	suite_add_tcase(s, tc_core);

	tcase_add_test(tc_iter, test_naive_ht_iter);
	suite_add_tcase(s, tc_iter);

	return s;
}

int main(void) {
	int num_failed;
	Suite *s;
	SRunner *sr;

	srand(time(NULL));

	s = naive_hash_table_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_VERBOSE);
	num_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (num_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
