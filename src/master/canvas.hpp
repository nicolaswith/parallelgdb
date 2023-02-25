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
 * @file canvas.hpp
 *
 * @brief Header file for the UIDrawingArea class.
 *
 * This file is the header file for the UIDrawingArea class.
 */

#ifndef CANVAS_HPP
#define CANVAS_HPP

#include <gtkmm.h>

class UIWindow;

/**
 * @brief Draws the processes current location in the source files on the 
 * drawing area.
 * 
 * This class draws the processes current location in the source files on the
 * drawing area.
 */
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
	/// Draws the processes current location on the canvas.
	virtual bool on_draw(const Cairo::RefPtr<Cairo::Context> &c);

public:
	/// Default constructor.
	UIDrawingArea(const int num_processes, UIWindow *const window);
	/// Destructor.
	virtual ~UIDrawingArea() {}

	/// Sets the vertical offset for a process.
	/**
	 * This function sets the vertical offset for a process.
	 *
	 * @param rank The process rank to set the offset for.
	 *
	 * @param offset The offset in pixels.
	 */
	void set_y_offset(const int rank, const int offset)
	{
		m_y_offsets[rank] = offset;
	}

	/// Returns the radius of the dots.
	/**
	 * This function returns the radius of the dots.
	 */
	inline static int radius()
	{
		return s_radius;
	}

	/// Returns the spacing of the dots.
	/**
	 * This function returns the spacing of the dots.
	 */
	inline static int spacing()
	{
		return s_spacing;
	}
};

#endif /* CANVAS_HPP */
