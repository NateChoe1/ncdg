#ifndef HAVE_VECTOR
#define HAVE_VECTOR

#include <stddef.h>

struct vector {
	void *data;
	size_t len;
	size_t alloc;
	size_t membersize;
};

struct vector *newvectorsize(size_t membersize);
#define newvector(type) newvectorsize(sizeof(type))
#define getvector(vector, type, ind) (((type *) (vector)->data)[ind])
int addvector(struct vector *vector, void *item);
void freevector(struct vector *vector);

#endif
