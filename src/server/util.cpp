#include <stdlib.h>
#include <memory.h>

#include "util.hpp"

void append_text(char **a_buffer, size_t *a_current_length, const char *const a_data, const size_t a_length)
{
	char *new_buffer = (char *)realloc(*a_buffer, (*a_current_length - 1 + a_length) * sizeof(char));
	memcpy(new_buffer + *a_current_length - 1, a_data, a_length);
	*a_buffer = new_buffer;
	*a_current_length = *a_current_length - 1 + a_length;
}
