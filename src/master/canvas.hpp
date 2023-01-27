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

#ifndef CANVAS_HPP
#define CANVAS_HPP

#include <gtkmm.h>

class UIWindow;

class UIDrawingArea : public Gtk::DrawingArea
{
	static const int s_radius;
	static const int s_spacing;

	const int m_num_processes;
	int *const m_y_offsets;
	
	UIWindow *const m_window;

public:
	static const Gdk::RGBA s_colors[];
	static const int NUM_COLORS;

protected:
	virtual bool on_draw(const Cairo::RefPtr<Cairo::Context> &c);

public:
	UIDrawingArea(const int num_processes, UIWindow *const window);
	virtual ~UIDrawingArea() {}

	void set_y_offset(const int rank, const int offset)
	{
		m_y_offsets[rank] = offset;
	}

	inline static int radius()
	{
		return s_radius;
	}

	inline static int spacing()
	{
		return s_spacing;
	}
};

#endif /* CANVAS_HPP */
