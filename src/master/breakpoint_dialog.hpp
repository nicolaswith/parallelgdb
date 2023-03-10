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
 * @file breakpoint_dialog.hpp
 *
 * @brief Header file for the BreakpointDialog class.
 *
 * This is the header file for the BreakpointDialog class.
 */

#ifndef BREAKPOINT_DIALOG_HPP
#define BREAKPOINT_DIALOG_HPP

#include <gtkmm.h>
#include <iosfwd>

class Breakpoint;

/// A wrapper class for a Gtk::Dialog.
/**
 * This is a wrapper class for a Gtk::Dialog. It will generate a dialog for the
 * user to select the desired processes for which a breakpoint should be set or
 * deleted. This information is saved in the @ref m_button_states array.
 */
class BreakpointDialog
{
	const int m_num_processes;
	const int m_max_buttons_per_row;

	Breakpoint *const m_breakpoint;

	bool *const m_button_states;

	Glib::RefPtr<Gtk::Builder> m_builder;
	Gtk::Dialog *m_dialog;
	Gtk::Grid *m_grid;

	/// Signal handler for the accept/close action of the dialog.
	void on_dialog_response(const int response_id);
	/// Toggles all checkbuttons in the grid.
	void toggle_all();

	/// Wrapper for the Gtk::get_widget function.
	template <class T>
	T *get_widget(const std::string &widget_name);

public:
	/// Default constructor.
	BreakpointDialog(const int num_processes, const int max_buttons_per_row,
					 Breakpoint *const breakpoint, const bool init);
	/// Destructor.
	~BreakpointDialog();

	/// Runs the dialog.
	/**
	 * This function runs the dialog.
	 *
	 * @return The response ID.
	 */
	inline int run()
	{
		return m_dialog->run();
	}

	/// Returns a write protected pointer to the @ref m_button_states array.
	/**
	 * @return A write protected pointer to the @ref m_button_states array.
	 */
	inline const bool *get_button_states() const
	{
		return m_button_states;
	}
};

#endif /* BREAKPOINT_DIALOG_HPP */