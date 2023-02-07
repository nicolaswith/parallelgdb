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

/**
 * @file breakpoint.cpp
 *
 * @brief Contains the implementation of the Breakpoint class.
 *
 * This file contains the implementation of the Breakpoint class.
 */

#include <string>

#include "breakpoint.hpp"
#include "window.hpp"

using std::string;

/**
 * This is the default constructor for the Breakpoint class. It will allocate
 * a new array of @ref BreakpointState, storing the current state of the
 * breakpoint for each process.
 *
 * @param num_processes The total number of processes.
 *
 * @param line One-based source file line number.
 *
 * @param[in] full_path The full path to the source file.
 *
 * @param[in] window The pointer to the UIWindow object.
 *
 * @note
 * Gsv returns zero-based line numbers. So in the constructor call one must be
 * added!
 */
Breakpoint::Breakpoint(const int num_processes, const int line,
					   const string &full_path, UIWindow *const window)
	: m_num_processes(num_processes),
	  m_line(line),
	  m_full_path(full_path),
	  m_numbers(new int[m_num_processes]),
	  m_stop_all(false),
	  m_window(window),
	  m_breakpoint_state(new BreakpointState[m_num_processes])
{
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		m_numbers[rank] = -1;
		m_breakpoint_state[rank] = NO_ACTION;
	}
}

/**
 * Deletes the allocated @ref m_breakpoint_state array.
 */
Breakpoint::~Breakpoint()
{
	delete[] m_numbers;
	delete[] m_breakpoint_state;
}

/**
 * This function sets a breakpoint for a process. For this to work the process
 * MUST be in the stopped state. Otherwise GDB will not accept commands.
 *
 * @param rank The process rank to set the breakpoint for.
 *
 * @return @c true on success, @c false on error.
 *
 * @note
 * Breakpoints are ID'd by a number which is set by GDB. It is saved in the
 * @ref m_numbers array per process. The @ref m_numbers array is set by the
 * @ref set_number function which is called by the
 * @ref UIWindow::print_data_gdb function, when the corresponding GDB output has
 * been parsed.
 */
bool Breakpoint::create_breakpoint(const int rank)
{
	m_breakpoint_state[rank] = ERROR_CREATING;
	if (m_window->target_state(rank) != TargetState::STOPPED)
	{
		return false;
	}

	string cmd =
		"-break-insert " + m_full_path + ":" + std::to_string(m_line) + "\n";
	m_window->set_breakpoint(rank, this);
	m_window->send_data(m_window->get_conns_gdb(rank), cmd);

	m_breakpoint_state[rank] = CREATED;
	return true;
}

/**
 * This function deletes a breakpoint for a process. For this to work the
 * process MUST be in the stopped state. Otherwise GDB will not accept commands.
 *
 * @param rank The process rank for which to delete the breakpoint.
 *
 * @return @c true on success, @c false on error.
 *
 * @note
 * Breakpoints are ID'd by a number which is set by GDB. It is saved in the
 * @ref m_numbers array per process and used in this function to delete it.
 * The @ref m_numbers array is set by the @ref set_number function which is
 * called by the @ref UIWindow::print_data_gdb function, when the corresponding
 * GDB output has been parsed.
 */
bool Breakpoint::delete_breakpoint(const int rank)
{
	m_breakpoint_state[rank] = ERROR_DELETING;
	if (m_window->target_state(rank) != TargetState::STOPPED)
	{
		return false;
	}

	string cmd = "-break-delete " + std::to_string(m_numbers[rank]) + "\n";
	m_window->send_data(m_window->get_conns_gdb(rank), cmd);

	m_breakpoint_state[rank] = NO_ACTION;
	return true;
}

/**
 * This function deletes the breakpoints for all process. If not all of them
 * could be deleted, an error message dialog is shown.
 *
 * @return @c true when all breakpoints are deleted, @c false if at least one
 * could not be deleted.
 */
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
		Gtk::MessageDialog dialog(
			*m_window->root_window(),
			string("Could not delete Breakpoint for some Processes.\n"
				   "Not deleted for Process(es): ") +
				get_list(ERROR_DELETING),
			false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
		dialog.run();
	}
	update_states();
	return all_deleted;
}

/**
 * This function modifies the breakpoints according to @p button_states. It is
 * called when the user right-clicks the breakpoint symbol and accepts the
 * breakpoint dialog. The @p button_states array is owned and populated by
 * the BreakpointDialog object.
 *
 * @param[in] button_states The array describing the desired states of the
 * breakpoints. @c true meaning the breakpoint should exist, @c false meaning
 * it should not.
 */
void Breakpoint::update_breakpoints(const bool *const button_states)
{
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		if (!button_states[rank] &&
			m_breakpoint_state[rank] == BreakpointState::CREATED)
		{
			delete_breakpoint(rank);
		}
		if (button_states[rank] &&
			m_breakpoint_state[rank] != BreakpointState::CREATED)
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
		message += "Active Breakpoint for Process(es): " +
				   (created_list.empty() ? "<None>" : created_list);
		Gtk::MessageDialog info_dialog(*m_window->root_window(), message, false,
									   Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
		info_dialog.run();
	}
}

/**
 * This function generates a comma separated list of processes for which the
 * breakpoint is in the @p state state.
 *
 * @param state The looked for state.
 *
 * @return A comma separated list of processes for which the breakpoint is in
 * the looked for state.
 */
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

/**
 * This function checks if at least one breakpoint is existing.
 *
 * @return @c true if at least one breakpoint is existing, @c false otherwise.
 */
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

/**
 * This function resets the error states to their corresponding base state.
 *
 * If a breakpoint could not be deleted it is in the @c ERROR_DELETING state but
 * still existing, so reset to the @c CREATED state.
 *
 * If a breakpoint could not be set it is in the @c ERROR_CREATING state but
 * not existing, so reset to the @c NO_ACTION state.
 */
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