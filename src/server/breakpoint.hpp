#ifndef BREAKPOINT_HPP
#define BREAKPOINT_HPP

#include "defs.hpp"

#define RESPONSE_ID_OK 0

class Breakpoint
{
	const int m_num_processes;

	int m_line;
	string m_full_path;

	bool *m_active_for_rank;

public:
	Breakpoint(const int num_processes, int line, string full_path);
	~Breakpoint();

	bool create_breakpoint(const int rank);
	bool delete_breakpoint(const int rank);
	bool delete_breakpoints();
	void set_active(const int rank, const bool valid);
	string get_description() const;
};

#endif /* BREAKPOINT_HPP */