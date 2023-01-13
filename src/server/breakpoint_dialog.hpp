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

#ifndef BREAKPOINT_DIALOG_HPP
#define BREAKPOINT_DIALOG_HPP

#include "defs.hpp"
#include "breakpoint.hpp"

class BreakpointDialog
{
	const int m_num_processes;
	const int m_max_buttons_per_row;

	Breakpoint *const m_breakpoint;

	bool *const m_button_states;

	Glib::RefPtr<Gtk::Builder> m_builder;
	Gtk::Dialog *m_dialog;
	Gtk::Grid *m_grid;

	void on_dialog_response(const int response_id);
	void toggle_all();

	template <class T>
	T *get_widget(const string &widget_name);

public:
	BreakpointDialog(const int num_processes, const int max_buttons_per_row, Breakpoint *const breakpoint, const bool init);
	~BreakpointDialog();

	inline int run()
	{
		return m_dialog->run();
	}

	inline const bool *get_button_states() const
	{
		return m_button_states;
	}
};

#endif /* BREAKPOINT_DIALOG_HPP */