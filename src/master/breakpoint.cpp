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

#include <string>

#include "breakpoint.hpp"
#include "window.hpp"

using std::string;

Breakpoint::Breakpoint(const int num_processes, const int line, const string &full_path, UIWindow *const window)
	: m_num_processes(num_processes),
	  m_line(line),
	  m_full_path(full_path),
	  m_number(-1),
	  m_stop_all(false),
	  m_window(window),
	  m_breakpoint_state(new BreakpointState[m_num_processes])
{
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		m_breakpoint_state[rank] = NO_ACTION;
	}
}

Breakpoint::~Breakpoint()
{
	delete[] m_breakpoint_state;
}

bool Breakpoint::create_breakpoint(const int rank)
{
	m_breakpoint_state[rank] = ERROR_CREATING;
	if (m_window->target_state(rank) != TargetState::STOPPED)
	{
		return false;
	}

	string cmd = "-break-insert " + m_full_path + ":" + std::to_string(m_line) + "\n";
	m_window->set_breakpoint(rank, this);
	m_window->send_data(m_window->get_conns_gdb(rank), cmd);

	m_breakpoint_state[rank] = CREATED;
	return true;
}

bool Breakpoint::delete_breakpoint(const int rank)
{
	m_breakpoint_state[rank] = ERROR_DELETING;
	if (m_window->target_state(rank) != TargetState::STOPPED)
	{
		return false;
	}

	string cmd = "-break-delete " + std::to_string(m_number) + "\n";
	m_window->send_data(m_window->get_conns_gdb(rank), cmd);

	m_breakpoint_state[rank] = NO_ACTION;
	return true;
}

bool Breakpoint::delete_breakpoints()
{
	bool all_deleted = true;
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		if (m_breakpoint_state[rank] == CREATED)
		{
			all_deleted &= delete_breakpoint(rank);
		}
	}
	if (!all_deleted)
	{
		Gtk::MessageDialog dialog(*m_window->root_window(), string("Could not delete Breakpoint for some Processes.\nNot deleted for Process(es): ") + get_list(ERROR_DELETING), false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
		dialog.run();
	}
	update_states();
	return all_deleted;
}

void Breakpoint::update_breakpoints(const bool *const button_states)
{
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		if (!button_states[rank] && m_breakpoint_state[rank] == BreakpointState::CREATED)
		{
			delete_breakpoint(rank);
		}
		if (button_states[rank] && m_breakpoint_state[rank] != BreakpointState::CREATED)
		{
			create_breakpoint(rank);
		}
	}
	string error_creating_list = get_list(ERROR_CREATING);
	string error_deleting_list = get_list(ERROR_DELETING);
	update_states();
	if (!error_creating_list.empty() || !error_deleting_list.empty())
	{
		string message = "";
		if (!error_creating_list.empty())
		{
			message += "Could not create Breakpoint for some Processes. [ ";
			message += error_creating_list + "]\n";
		}
		if (!error_deleting_list.empty())
		{
			message += "Could not delete Breakpoint for some Processes. [ ";
			message += error_deleting_list + "]\n";
		}
		string created_list = get_list(CREATED);
		message += "Active Breakpoint for Process(es): " + (created_list.empty() ? "<None>" : created_list);
		Gtk::MessageDialog info_dialog(*m_window->root_window(), message, false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
		info_dialog.run();
	}
}

string Breakpoint::get_list(const BreakpointState state) const
{
	string str = "";
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		if (m_breakpoint_state[rank] == state)
		{
			if (!str.empty())
			{
				str += ", ";
			}
			str += std::to_string(rank);
		}
	}
	return str;
}

bool Breakpoint::one_created() const
{
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		if (m_breakpoint_state[rank] == CREATED)
		{
			return true;
		}
	}
	return false;
}

void Breakpoint::update_states()
{
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		if (m_breakpoint_state[rank] == ERROR_DELETING)
		{
			m_breakpoint_state[rank] = CREATED;
		}
		else if (m_breakpoint_state[rank] == ERROR_CREATING)
		{
			m_breakpoint_state[rank] = NO_ACTION;
		}
	}
}