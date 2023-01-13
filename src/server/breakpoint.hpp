/*
	This file is part of ParallelGDB.

	Copyright (c) 2023 by Nicolas With

	ParallelGDB is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	ParallelGDB is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with ParallelGDB.  If not, see <https://www.gnu.org/licenses/gpl-3.0.txt>.
*/

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

	bool m_stop_all;

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

	inline int get_line_number() const
	{
		return m_line;
	}

	inline void set_number(const int number)
	{
		m_number = number;
	}

	inline void set_stop_all(const bool stop_all)
	{
		m_stop_all = stop_all;
	}

	inline bool get_stop_all() const
	{
		return m_stop_all;
	}
};

#endif /* BREAKPOINT_HPP */