#ifndef UTIL_HPP
#define UTIL_HPP

inline bool src_is_gdb(const int a_port) 
{
	return (0 == (a_port & 0x4000));
}

inline int get_process_rank(const int a_port)
{
	return (0x3FFF & a_port);
}

void append_text(char **a_buffer, size_t *a_current_length, const char *const a_data, const size_t a_length);

#endif /* UTIL_HPP */