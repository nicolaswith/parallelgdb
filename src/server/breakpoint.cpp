#include "breakpoint.hpp"

Breakpoint::Breakpoint(const int num_processes, int line, string full_path)
	: m_num_processes(num_processes),
	  m_line(line),
	  m_full_path(full_path),
	  m_active_for_rank(new bool[m_num_processes])
{
}

Breakpoint::~Breakpoint()
{
	delete[] m_active_for_rank;
}

string Breakpoint::get_description() const
{
	return "0,1,2";
}

bool Breakpoint::create_breakpoint(const int rank)
{
	return true;
}

bool Breakpoint::delete_breakpoint(const int rank)
{
	return false;
}

bool Breakpoint::delete_breakpoints()
{
	bool all_deleted = true;
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		all_deleted &= delete_breakpoint(rank);
	}
	return all_deleted;
}

void Breakpoint::set_active(const int rank, const bool active)
{
}
