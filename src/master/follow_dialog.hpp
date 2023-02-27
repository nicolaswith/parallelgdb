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
 * @file follow_dialog.hpp
 *
 * @brief Header file for the FollowDialog class.
 *
 * This is the header file for the FollowDialog class.
 */

#ifndef FOLLOW_DIALOG_HPP
#define FOLLOW_DIALOG_HPP

#include <gtkmm.h>
#include <iosfwd>

#define FOLLOW_ALL -1

/// A wrapper class for a Gtk::Dialog.
/**
 * This is a wrapper class for a Gtk::Dialog. It will generate a dialog for the
 * user to select the processes to follow. 
 */
class FollowDialog
{
	const int m_num_processes;
	const int m_max_buttons_per_row;
	
	int m_follow_rank;

	Glib::RefPtr<Gtk::Builder> m_builder;
	Gtk::Dialog *m_dialog;

	Gtk::Grid *m_grid;
	Gtk::RadioButton *m_follow_all_radiobutton;

	/// Signal handler for the accept/close action of the dialog.
	void on_dialog_response(const int response_id);

	/// Wrapper for the Gtk::get_widget function.
	template <class T>
	T *get_widget(const std::string &widget_name);

public:
	/// Default constructor.
	FollowDialog(const int num_processes, const int max_buttons_per_row);
	/// Destructor.
	~FollowDialog();

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

	/// Returns the process rank to follow.
	/**
	 * @return The processes rank to follow.
	 */
	inline int follow_rank() const
	{
		return m_follow_rank;
	}
};

#endif /* FOLLOW_DIALOG_HPP */