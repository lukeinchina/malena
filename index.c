#include <ctype.h>

#include <vector>
#include <map>

#include "define.h"
#include "type.h"
#include "util.h"
#include "log.h"
#include "index.h"


typedef struct {
	termid_t id;
	occ_t    occ;
}Term;

int
text_to_term(const char *text, int offset, Term *terms, int size) {
	Term *ptr = terms;
	// char word[256];
	const uint8_t *start = (const uint8_t *)text;
	const uint8_t *end   = start;

	while (0 != *start) {
		for (; 0 != *start && isspace(*start); start++) {}
		end = start;
		for (; 0 != *end && !isspace(*end); end++) {}
		if (start == end) {
			break;
		}

		ptr->id  = termid(start, end);
		ptr->occ = offset;
		offset   += utf8_count(start, end);
		/*
		strncpy(word, (const char *)start, end-start);
		word[end-start] = 0;
		printf("[%s:%llu %d]  ", word, ptr->id, ptr->occ);
		*/
		ptr      += 1;

		if (ptr - terms >= size) {
			LOG(LOG_WARN, "term count[%ld] is overflow. size=[%d]", ptr - terms, size);
			break;
		}

		start = end;
	}
	return (int)(ptr - terms);
}

/*
 * @brief:
 * @param doc [in]:
 * @return:
 */
int
index_one_doc(const Doc *doc) {
	int i, n;
	Term terms[MAX_TERM_NUM];
	n = text_to_term(doc->title, 0, terms, MAX_TERM_NUM);
	/* check n */
	n += text_to_term(doc->content, MAX_TITLE_LEN, terms+n, MAX_TERM_NUM - n);

	/*
	std::map<termid_t, std::vector<occ_t> > inv;
	std::map<termid_t, std::vector<occ_t> >::iterator it;
	for (i = 0; i < n; i++) {
		it = inv.find(terms[i].id);
		if (it != inv.end()) {
			it->second.push_back(terms[i].occ);
		} else {
			std::vector<occ_t> vec;
			vec.push_back(terms[i].occ);
			inv.insert(make_pair(terms[i].id, vec) );
		}
	}
	for (it = inv.begin(); it != inv.end(); it++) {
		printf("[%llu] ", it->first);
		for (size_t i = 0; i < it->second.size(); i++) {
			printf("%d\t", (it->second)[i]);
		}
		printf("\n");
	}
	*/

	return 0;
}
