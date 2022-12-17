#ifndef BREAKPOINT_HPP
#define BREAKPOINT_HPP

#include "defs.hpp"

class UIWindow;

enum BreakpointState
{
	NO_ACTION,
	CREATE,
	DELETE,
	CREATED,
	ERROR_CREATING,
	ERROR_DELETING
};

class Breakpoint
{
	const int m_num_processes;

	const int m_line;
	const string m_full_path;
	int m_number;

	UIWindow *const m_window;
	BreakpointState *const m_breakpoint_state;

	bool create_breakpoint(const int rank);
	bool delete_breakpoint(const int rank);
	string get_list(const BreakpointState state) const;
	void update_states();

public:
	Breakpoint(const int num_processes, const int line, const string &full_path, UIWindow *const window);
	~Breakpoint();

	void update_breakpoints(const bool *const button_states);
	bool delete_breakpoints();
	bool one_created() const;

	inline bool is_created(const int rank) const
	{
		return m_breakpoint_state[rank] == BreakpointState::CREATED;
	}

	inline void set_number(const int number)
	{
		m_number = number;
	}
};

#endif /* BREAKPOINT_HPP */