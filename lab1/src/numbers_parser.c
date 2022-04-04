#include "numbers_parser.h"

#include <linux/types.h>
#include <linux/string.h>
#include <linux/kernel.h>

u64 parse_string(const char *str, size_t str_len) {
	u64 sum = 0;
	size_t i;
	u64 cur_int = 0;
	size_t digit_count = 0;
	for (i = 0; i < str_len; i++) {
		char cur_ch = str[i];
		if (cur_ch >= '0' && cur_ch <= '9') {
			if (digit_count > 0) {
				cur_int *= 10;
			}
			cur_int += (u64) (cur_ch - '0');
			digit_count++;
			
		} else if (digit_count > 0) {
			sum += cur_int;
			cur_int = 0;
			digit_count = 0;
	
		}
	}
	return sum;
}