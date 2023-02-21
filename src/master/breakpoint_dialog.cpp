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
 * @file breakpoint_dialog.cpp
 *
 * @brief Contains the implementation of the BreakpointDialog class.
 *
 * This file contains the implementation of the BreakpointDialog class.
 */

#include <string>

#include "breakpoint_dialog.hpp"
#include "breakpoint.hpp"

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
T *BreakpointDialog::get_widget(const std::string &widget_name)
{
	T *widget;
	m_builder->get_widget<T>(widget_name, widget);
	return widget;
}

/**
 * This is the default constructor for the BreakpointDialog class. This
 * will generate a grid of checkbuttons that are, if the init flag is set, all
 * checked. Otherwise the state of the checkbuttons depends on the state of the
 * breakpoint at the corresponding process.
 *
 * @param num_processes The number of processes.
 *
 * @param max_buttons_per_row The number of buttons per grid row.
 *
 * @param[in] breakpoint A pointer to a Breakpoint object.
 *
 * @param init A flag indicating if the breakpoint if first created.
 */
BreakpointDialog::BreakpointDialog(const int num_processes,
								   const int max_buttons_per_row,
								   Breakpoint *const breakpoint,
								   const bool init)
	: m_num_processes(num_processes),
	  m_max_buttons_per_row(max_buttons_per_row),
	  m_breakpoint(breakpoint),
	  m_button_states(new bool[m_num_processes])
{
	// parse glade file
	m_builder = Gtk::Builder::create_from_resource("/pgdb/ui/breakpoint_dialog.glade");
	m_dialog = get_widget<Gtk::Dialog>("dialog");

	// generate checkbutton grid
	m_grid = get_widget<Gtk::Grid>("check-buttons-grid");
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::CheckButton *check_button =
			Gtk::manage(new Gtk::CheckButton(std::to_string(rank)));
		if (init)
		{
			check_button->set_active(true);
		}
		else
		{
			check_button->set_active(m_breakpoint->is_created(rank));
		}
		m_grid->attach(*check_button, rank % m_max_buttons_per_row,
					   rank / m_max_buttons_per_row);
	}

	// connect signal handlers
	get_widget<Gtk::CheckButton>("stop-all-checkbutton")
		->set_active(m_breakpoint->get_stop_all());
	get_widget<Gtk::Button>("toggle-all-button")
		->signal_clicked()
		.connect(sigc::mem_fun(*this, &BreakpointDialog::toggle_all));
	m_dialog->signal_response().connect(
		sigc::mem_fun(*this, &BreakpointDialog::on_dialog_response));

	m_dialog->add_button("Ok", Gtk::RESPONSE_OK);
	m_dialog->show_all();
}

/**
 * Closes the dialog and destroys the object.
 */
BreakpointDialog::~BreakpointDialog()
{
	delete[] m_button_states;
	delete m_dialog;
}

/**
 * This function gets called when the user clicks on the "Ok" button or the
 * close button. It populates the @ref m_button_states array with the state of
 * each checkbutton. This array will be used by the Breakpoint class to
 * set/delete breakpoints per process.
 *
 * @param response_id The response ID of the pressed button.
 */
void BreakpointDialog::on_dialog_response(const int response_id)
{
	if (Gtk::RESPONSE_OK != response_id)
	{
		return;
	}
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::CheckButton *check_button =
			dynamic_cast<Gtk::CheckButton *>(m_grid->get_child_at(
				rank % m_max_buttons_per_row, rank / m_max_buttons_per_row));
		m_button_states[rank] = check_button->get_active();
	}
	m_breakpoint->set_stop_all(
		get_widget<Gtk::CheckButton>("stop-all-checkbutton")->get_active());
}

/**
 * This function toggles all checkbuttons in the grid. If at least one is
 * active, all are turned of. If none is active, all are activated.
 */
void BreakpointDialog::toggle_all()
{
	// check if at least one is active
	bool one_checked = false;
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::CheckButton *check_button =
			dynamic_cast<Gtk::CheckButton *>(m_grid->get_child_at(
				rank % m_max_buttons_per_row, rank / m_max_buttons_per_row));
		if (check_button->get_active())
		{
			one_checked = true;
			break;
		}
	}
	// set the inverted state to all checkbuttons
	bool state = !one_checked;
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::CheckButton *check_button =
			dynamic_cast<Gtk::CheckButton *>(m_grid->get_child_at(
				rank % m_max_buttons_per_row, rank / m_max_buttons_per_row));
		check_button->set_active(state);
	}
}