#include <stdlib.h>
#include <string.h>

#include <vector.h>

struct vector *newvectorsize(size_t membersize) {
	struct vector *ret;
	ret = malloc(sizeof *ret);
	if (ret == NULL)
		return NULL;
	ret->alloc = 20;
	ret->membersize = membersize;
	ret->data = malloc(ret->alloc * ret->membersize);
	if (ret->data == NULL) {
		free(ret);
		return NULL;
	}
	ret->len = 0;
	return ret;
}

int addvector(struct vector *vector, void *item) {
	if (vector->len >= vector->alloc) {
		void *newdata;
		size_t newalloc;
		newalloc = vector->alloc * 2;
		newdata = realloc(vector->data, newalloc * vector->alloc);
		if (newdata == NULL)
			return 1;
		vector->data = newdata;
		vector->alloc = newalloc;
	}
	memcpy((char *) vector->data + vector->len * vector->membersize, item,
			vector->membersize);
	++vector->len;
	return 0;
}

void freevector(struct vector *vector) {
	free(vector->data);
	free(vector);
}
