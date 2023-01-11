#include <string>
#include <cmath>
#include "canvas.hpp"
#include "window.hpp"

const Gdk::RGBA UIDrawingArea::s_colors[NUM_COLORS] = {
	Gdk::RGBA("#ff0000"),
	Gdk::RGBA("#008000"),
	Gdk::RGBA("#0000bb"),
	Gdk::RGBA("#ffd700"),
	Gdk::RGBA("#c71585"),
	Gdk::RGBA("#00ff00"),
	Gdk::RGBA("#00ffff"),
	Gdk::RGBA("#1e90ff")};

const int UIDrawingArea::s_radius = 10;
const int UIDrawingArea::s_spacing = 4;

UIDrawingArea::UIDrawingArea(const int num_processes, UIWindow *const window)
	: m_num_processes(num_processes),
	  m_y_offsets(new int[m_num_processes]),
	  m_window(window)
{
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		m_y_offsets[rank] = -3 * s_radius;
	}
}

bool UIDrawingArea::on_draw(const Cairo::RefPtr<Cairo::Context> &c)
{
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		if (m_y_offsets[rank] > -2 * s_radius - 1 && m_window->target_state(rank) != TargetState::EXITED)
		{
			int x = rank * (2 * s_radius + s_spacing);
			int y = m_y_offsets[rank];

			c->save();

			c->set_source_rgba(
				s_colors[rank % NUM_COLORS].get_red(),
				s_colors[rank % NUM_COLORS].get_green(),
				s_colors[rank % NUM_COLORS].get_blue(),
				s_colors[rank % NUM_COLORS].get_alpha());

			c->arc(x + s_radius, y + s_radius, s_radius, 0.0, 2.0 * M_PI);
			c->fill();

			Pango::FontDescription font;
			font.set_family("Monospace");
			Glib::RefPtr<Pango::Layout> layout = create_pango_layout(std::to_string(rank));
			layout->set_font_description(font);

			int text_width;
			int text_height;
			layout->get_pixel_size(text_width, text_height);

			c->move_to(x + s_radius - (text_width / 2), y + pow(-1, rank) * (2 * s_radius + 1));
			layout->show_in_cairo_context(c);

			c->restore();
		}
	}

	return true;
}