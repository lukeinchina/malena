/* use "Check" for unit test */
#include <string.h>
#include <stdlib.h>

#include <check.h>
#include "../md5.h"


START_TEST(test_md5_hash)
{
	size_t i, j, length;
	MD5_CTX ctx;
	unsigned char digest[32] = {0};
	char buff[64] = {0};

	const char *cases[] = {
		"123",
        "abc",
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
        "abcdefghijklmnopqrstuvwxyz0123456789"
	};
	const char *expect_results[] = {
		"202cb962ac59075b964b07152d234b70",
		"900150983cd24fb0d6963f7d28e17f72",
		"437bba8e0bf58337674f4539e75186ac",
		"6d2286301265512f019781cc0ce7a39f"
	};
	size_t size = sizeof(cases) / sizeof(cases[0]);
	ck_assert_int_eq(size, sizeof(expect_results) / sizeof(expect_results[0]));

	for (i = 0 ; i < size; i++) {
		length = strlen(cases[i]);
		md5_init(&ctx);
		md5_update(&ctx, cases[i], length);
		md5_final(digest, &ctx);

		memset(buff, 0, sizeof(buff));
		for (j = 0; j < 16; j++) {
			sprintf((char *)(buff+j*2), "%02x", digest[j]);
		}
		ck_assert(strcmp(buff, expect_results[i]) == 0);
	}
}
END_TEST

Suite * utf8_suite(void) {
	Suite *s;
	TCase *tc_core;

	s = suite_create("md5");

	/* Core test case */
	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_md5_hash);
	suite_add_tcase(s, tc_core);

	return s;
}

int main(void) {
	int num_failed;
	Suite *s;
	SRunner *sr;

	s = utf8_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	num_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (num_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
