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
 * @file follow_dialog.cpp
 *
 * @brief Contains the implementation of the FollowDialog class.
 *
 * This file contains the implementation of the FollowDialog class.
 */

#include <string>

#include "follow_dialog.hpp"

using std::string;

/**
 * This function is a wrapper for the Gtk::get_widget function.
 *
 * @tparam T The widget class.
 *
 * @param[in] widget_name The widget name.
 *
 * @return The pointer to the widget object on success, @c nullptr on error.
 */
template <class T>
T *FollowDialog::get_widget(const std::string &widget_name)
{
	T *widget;
	m_builder->get_widget<T>(widget_name, widget);
	return widget;
}

/**
 * This is the default constructor for the FollowDialog class. It will generate
 * a grid of radiobuttons for the user to select the process to follow.
 *
 * @param num_processes The number of processes.
 *
 * @param max_buttons_per_row The number of buttons per grid row.
 *
 * @param follow_rank The currently selected process(es) to follow.
 */
FollowDialog::FollowDialog(const int num_processes,
						   const int max_buttons_per_row,
						   const int follow_rank)
	: m_num_processes(num_processes),
	  m_max_buttons_per_row(max_buttons_per_row)
{
	// parse glade file
	m_builder =
		Gtk::Builder::create_from_resource("/pgdb/ui/follow_dialog.glade");
	m_dialog = get_widget<Gtk::Dialog>("dialog");

	// generate radiobuttons grid
	m_grid = get_widget<Gtk::Grid>("radiobuttons-grid");
	Gtk::RadioButton *first_radiobutton = Gtk::manage(new Gtk::RadioButton("0"));
	m_grid->attach(*first_radiobutton, 0, 0);
	Gtk::RadioButton::Group radiobutton_group = first_radiobutton->get_group();
	for (int rank = 1; rank < m_num_processes; ++rank)
	{
		Gtk::RadioButton *radiobutton =
			Gtk::manage(new Gtk::RadioButton(std::to_string(rank)));
		radiobutton->set_group(radiobutton_group);
		if (rank == follow_rank)
		{
			radiobutton->set_active(true);
		}
		m_grid->attach(*radiobutton, rank % m_max_buttons_per_row,
					   rank / m_max_buttons_per_row);
	}

	m_follow_all_radiobutton =
		get_widget<Gtk::RadioButton>("follow-all-radiobutton");
	m_follow_all_radiobutton->set_group(radiobutton_group);
	if (FOLLOW_ALL == follow_rank)
	{
		m_follow_all_radiobutton->set_active(true);
	}

	m_follow_none_radiobutton =
		get_widget<Gtk::RadioButton>("follow-none-radiobutton");
	m_follow_none_radiobutton->set_group(radiobutton_group);
	if (FOLLOW_NONE == follow_rank)
	{
		m_follow_none_radiobutton->set_active(true);
	}

	// connect signal handlers
	m_dialog->signal_response().connect(
		sigc::mem_fun(*this, &FollowDialog::on_dialog_response));

	m_dialog->add_button("Ok", Gtk::RESPONSE_OK);
	m_dialog->show_all();
}

/**
 * Closes the dialog and destroys the object.
 */
FollowDialog::~FollowDialog()
{
	delete m_dialog;
}

/**
 * This function gets called when the user clicks on the "Ok" button or the
 * close button. Checks which radiobutton is selected and saves the rank for
 * later use.
 *
 * @param response_id The response ID of the pressed button.
 */
void FollowDialog::on_dialog_response(const int response_id)
{
	if (Gtk::RESPONSE_OK != response_id)
	{
		return;
	}
	if (m_follow_all_radiobutton->get_active())
	{
		m_follow_rank = FOLLOW_ALL;
		return;
	}
	if (m_follow_none_radiobutton->get_active())
	{
		m_follow_rank = FOLLOW_NONE;
		return;
	}
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::RadioButton *radiobutton =
			dynamic_cast<Gtk::RadioButton *>(m_grid->get_child_at(
				rank % m_max_buttons_per_row, rank / m_max_buttons_per_row));
		if (radiobutton->get_active())
		{
			m_follow_rank = rank;
			break;
		}
	}
}
