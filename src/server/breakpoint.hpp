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

	const UIWindow *const m_window;
	BreakpointState *const m_breakpoint_state;

	bool create_breakpoint(const int rank);
	bool delete_breakpoint(const int rank);
	string get_list(const BreakpointState state) const;

public:
	Breakpoint(const int num_processes, const int line, const string &full_path, const UIWindow *const window);
	~Breakpoint();

	void update_breakpoints(const bool *const button_states);
	bool delete_breakpoints();
	bool one_created() const;

	inline BreakpointState get_state(const int rank) const
	{
		return m_breakpoint_state[rank];
	}

	inline void set_state(const int rank, const BreakpointState state)
	{
		m_breakpoint_state[rank] = state;
	}
};

#endif /* BREAKPOINT_HPP */