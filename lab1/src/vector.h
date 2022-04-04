#ifndef VECTOR_H
#define VECTOR_H

#include <linux/types.h>

#define PRI_VALUE "lld"
#define VECTOR_VALUE_STR_LEN 21
typedef u64 vector_value;

struct vector;

struct vector *vector_new(size_t init_capacity);

void vector_destroy(struct vector **vector);

size_t vector_count(const struct vector *vector);

int vector_add(struct vector *vector, vector_value value);

size_t vector_value_print(char *buf, size_t buf_len, vector_value val);

size_t vector_print(char *buf, size_t buf_len, const struct vector *vector);

#endif
