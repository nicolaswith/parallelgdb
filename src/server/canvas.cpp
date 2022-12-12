#include <string>
#include "canvas.hpp"
#include "window.hpp"

const Gdk::RGBA UIDrawingArea::s_colors[] = {
	Gdk::RGBA("#ff0000"),
	Gdk::RGBA("#008000"),
	Gdk::RGBA("#0000ff"),
	Gdk::RGBA("#ffd700"),
	Gdk::RGBA("#c71585"),
	Gdk::RGBA("#00ff00"),
	Gdk::RGBA("#00ffff"),
	Gdk::RGBA("#1e90ff"),
};

const int UIDrawingArea::s_radius = 8;
const int UIDrawingArea::s_spacing = 4;

UIDrawingArea::UIDrawingArea(const int num_processes)
	: m_num_processes(num_processes),
	  m_y_offsets(new int[m_num_processes])
{
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		m_y_offsets[rank] = -3 * s_radius;
	}
}

void UIDrawingArea::set_y_offset(const int process_rank, const int offset)
{
	m_y_offsets[process_rank] = offset;
}

bool UIDrawingArea::on_draw(const Cairo::RefPtr<Cairo::Context> &context)
{
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		if (m_y_offsets[rank] > -2 * s_radius - 1)
		{
			context->save();
			context->arc(
				rank * (2 * s_radius + s_spacing) + s_radius + s_spacing,
				m_y_offsets[rank] + s_radius,
				s_radius,
				0.0,
				2.0 * M_PI);
			context->set_source_rgba(
				s_colors[rank].get_red(),
				s_colors[rank].get_green(),
				s_colors[rank].get_blue(),
				s_colors[rank].get_alpha());
			context->fill();
			context->restore();
		}
	}

	return true;
}