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
	along with ParallelGDB.  If not, see <https://www.gnu.org/licenses/>.
*/

/**
 * @file breakpoint.hpp
 *
 * @brief Header file for the Breakpoint class.
 *
 * This is the header file for the Breakpoint class.
 */

#ifndef BREAKPOINT_HPP
#define BREAKPOINT_HPP

#include <gtkmm.h>
#include <iosfwd>

class UIWindow;

/// The states a Breakpoint can be in.
enum BreakpointState
{
	NO_ACTION,		/**< breakpoint is not set. */
	CREATE,			/**< breakpoint is marked to be created. */
	DELETE,			/**< breakpoint is marked to be deleted. */
	CREATED,		/**< breakpoint is set. */
	ERROR_CREATING, /**< breakpoint could not be created. */
	ERROR_DELETING	/**< breakpoint could not be deleted. */
};

/// Stores information about a breakpoint on a specific source file.
/**
 * This class stores information about a breakpoint on a specific source file
 * for all processes. Each process can either have this breakpoint enabled
 * or not. This is represented in the @ref m_breakpoint_state array using the
 * @ref BreakpointState states.
 */
class Breakpoint
{
	const int m_num_processes;

	const int m_line;
	const std::string m_full_path;
	int *m_numbers;

	bool m_stop_all;

	UIWindow *const m_window;
	BreakpointState *const m_breakpoint_state;

	/// Creates a breakpoint for a process.
	bool create_breakpoint(const int rank);
	/// Deletes a breakpoint for a process.
	bool delete_breakpoint(const int rank);
	/** @brief Generates a comma separated list of processes for which the
	 * breakpoint is in the @p state state.
	 */
	std::string get_list(const BreakpointState state) const;
	/// Resets the error states to their corresponding base state.
	void update_states();

public:
	/// Default constructor.
	Breakpoint(const int num_processes, const int line,
			   const std::string &full_path, UIWindow *const window);
	/// Destructor.
	~Breakpoint();

	/// Modifies the breakpoints according to @p button_states.
	void update_breakpoints(const bool *const button_states);
	/// Deletes the breakpoints for all process.
	bool delete_breakpoints();
	/// Checks if at least one breakpoint is existing.
	bool one_created() const;

	/// Returns wether the breakpoint for this process is created.
	/**
	 * This function returns wether the breakpoint for this process is created.
	 *
	 * @param rank The process rank to check for.
	 *
	 * @return @c true if the breakpoint is created, @c false otherwise.
	 */
	inline bool is_created(const int rank) const
	{
		return m_breakpoint_state[rank] == BreakpointState::CREATED;
	}

	/// Returns the line number of the associated source file.
	/**
	 * This function returns the line number of the associated source file.
	 *
	 * @return The line number of the associated source file.
	 *
	 * @note
	 * GDB expects the line numbers to be one-based, while the Gsv View returns
	 * zero-based line numbers. The @ref m_line MUST be one-based as it is used
	 * by GDB directly!
	 */
	inline int get_line_number() const
	{
		return m_line;
	}

	/// Sets the from GDB to this breakpoint assigned ID for a process.
	/**
	 * This function sets the from GDB to this breakpoint assigned ID for a
	 * process.
	 *
	 * @param rank The process rank.
	 *
	 * @param number The from GDB to this breakpoint assigned ID.
	 */
	inline void set_number(const int rank, const int number)
	{
		m_numbers[rank] = number;
	}

	/// Sets the stop all flag for a breakpoint.
	/**
	 * This function sets the stop all flag for a breakpoint.
	 *
	 * @param stop_all The state of the flag.
	 */
	inline void set_stop_all(const bool stop_all)
	{
		m_stop_all = stop_all;
	}

	/// Returns whether the stop all flag is set for this breakpoint.
	/**
	 * This function returns whether the stop all flag is set for this
	 * breakpoint.
	 *
	 * @return The state of the flag.
	 */
	inline bool get_stop_all() const
	{
		return m_stop_all;
	}
};

#endif /* BREAKPOINT_HPP */