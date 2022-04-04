#include "vector.h"

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/kernel.h>

struct vector {
	vector_value *array;
	size_t capacity;
	size_t count;
};

struct vector *vector_new(size_t init_capacity) {
	struct vector *const v = kmalloc(sizeof(struct vector), GFP_KERNEL);
	if (!v) return NULL;
	if (init_capacity > 0) {
		v->array = kmalloc(sizeof(vector_value) * init_capacity, GFP_KERNEL);
		if (v->array == NULL) return NULL;
		v->capacity = init_capacity;
	} else {
		v->array = NULL;
		v->capacity = 0;
	}
	v->count = 0;
	return v;
}

void vector_destroy(struct vector** v) {
	if (!(*v)) return;
	kfree((*v)->array);
	kfree(*v);
	*v = NULL;
}

int vector_add(struct vector *const vector, const vector_value value) {
	if (vector->count == vector->capacity) {
		size_t new_capacity = vector->capacity;
		if (new_capacity == 0)
			new_capacity = 1;
		else
			new_capacity *= 2;
		vector_value *new_values;
		new_values = kmalloc(sizeof(vector_value) * new_capacity, GFP_KERNEL);
		if (!new_values) return 0;
		memcpy(new_values, vector->array, sizeof(vector_value) * vector->capacity);
		kfree(vector->array);
		vector->array = new_values;
		vector->capacity = new_capacity;
	}
	(*vector).array[vector->count] = value;
	vector->count += 1;
	return 1;
}

vector_value vector_get(const struct vector *const vector, const size_t i) {
	if (!vector || i > vector->count)
		return 0;
	return (*vector).array[i];
}

int vector_set(struct vector *const vector, const vector_value value, const size_t i) {
	if (!vector || i > vector->count)
		return 0;
	(*vector).array[i] = value;
	return 1;
}

size_t vector_value_print(char *const buf, const size_t buf_len, const vector_value val) {
	if (!buf) return 0;
	return snprintf(buf, buf_len, "%" PRI_VALUE, val);
}

size_t vector_print(char *buf, size_t buf_len, const struct vector *const vector) {	
	const size_t buf_len_old = buf_len;
	if (!vector || !buf) return 0;
	buf_len -= snprintf(buf, buf_len, "[");
	buf++;
		
	size_t i;
	for (i = 0; i < vector->count; i++) {
		if (buf_len <= 0)
			return buf_len_old - buf_len;
		size_t element_len = vector_value_print(buf, buf_len, vector_get(vector, i));
		if (i + 1 < vector->count) {
			snprintf(buf + element_len, buf_len - element_len, " ");
			element_len++;
		}
	
		buf_len -= element_len;
		buf += element_len;
	}
	buf_len -= snprintf(buf, buf_len, "]\n");
	
	return buf_len_old - buf_len;
}

size_t vector_count(const struct vector *const vector) {
	if (vector)
		return vector->count;
	else
		return 0;
}
 