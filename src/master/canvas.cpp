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
 * @file canvas.cpp
 *
 * @brief Contains the implementation of the UIDrawingArea class.
 *
 * This file contains the implementation of the UIDrawingArea class.
 */

#include <gtkmm.h>
#include <cmath>
#include <string>

#include "canvas.hpp"

using std::string;

const int UIDrawingArea::s_radius = 10;
const int UIDrawingArea::s_spacing = 4;

const int UIDrawingArea::NUM_COLORS = 8;
const Gdk::RGBA UIDrawingArea::s_colors[] = {
	Gdk::RGBA("#ff0000"),
	Gdk::RGBA("#008000"),
	Gdk::RGBA("#0000bb"),
	Gdk::RGBA("#ffd700"),
	Gdk::RGBA("#c71585"),
	Gdk::RGBA("#00ff00"),
	Gdk::RGBA("#00ffff"),
	Gdk::RGBA("#1e90ff")};

/**
 * This is the default constructor. It initializes the offsets of all processes
 * to be out of the visible area.
 *
 * @param num_processes The total number of processes.
 */
UIDrawingArea::UIDrawingArea(const int num_processes)
	: m_num_processes(num_processes),
	  m_y_offsets(new int[m_num_processes])
{
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		m_y_offsets[rank] = -3 * s_radius;
	}
}

/**
 * This function draws a dot for each processes at the through @ref m_y_offsets
 * specified location. The color of these dots cycle through the colors
 * defined in the @ref s_colors array.
 *
 * @param[in] c The Cairo context.
 *
 * @return @c true. The return value is used to indicate whether the event is
 * completely handled.
 */
bool UIDrawingArea::on_draw(const Cairo::RefPtr<Cairo::Context> &c)
{
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		if (m_y_offsets[rank] > -2 * s_radius - 1)
		{
			int x = rank * (2 * s_radius + s_spacing);
			int y = m_y_offsets[rank];

			c->save();

			c->set_source_rgba(s_colors[rank % NUM_COLORS].get_red(),
							   s_colors[rank % NUM_COLORS].get_green(),
							   s_colors[rank % NUM_COLORS].get_blue(),
							   s_colors[rank % NUM_COLORS].get_alpha());

			c->arc(x + s_radius, y + s_radius, s_radius, 0.0, 2.0 * M_PI);
			c->fill();

			Pango::FontDescription font;
			font.set_family("Monospace");
			Glib::RefPtr<Pango::Layout> layout =
				create_pango_layout(std::to_string(rank));
			layout->set_font_description(font);

			int text_width;
			int text_height;
			layout->get_pixel_size(text_width, text_height);

			c->move_to(x + s_radius - (text_width / 2),
					   y + pow(-1, rank) * (2 * s_radius + 1));
			layout->show_in_cairo_context(c);

			c->restore();
		}
	}

	return true;
}