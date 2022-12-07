#include <string>
#include "canvas.hpp"
#include "window.hpp"

const Gdk::RGBA UIDrawingArea::colors[] = {
	Gdk::RGBA("#ff0000"),
	Gdk::RGBA("#008000"),
	Gdk::RGBA("#0000ff"),
	Gdk::RGBA("#ffd700"),
	Gdk::RGBA("#c71585"),
	Gdk::RGBA("#00ff00"),
	Gdk::RGBA("#00ffff"),
	Gdk::RGBA("#1e90ff"),
};

const int UIDrawingArea::m_radius = 8;
const int UIDrawingArea::m_spacing = 4;

UIDrawingArea::UIDrawingArea(const int a_num_processes, const int a_process_rank)
	: m_num_processes(a_num_processes),
	  m_process_rank(a_process_rank),
	  m_y_offsets(new int[m_num_processes])
{
}

void UIDrawingArea::set_y_offset(const int a_process_rank, const int a_offset)
{
	m_y_offsets[a_process_rank] = a_offset;
}

bool UIDrawingArea::on_draw(const Cairo::RefPtr<Cairo::Context> &a_context)
{
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		if (rank == m_process_rank)
		{
			continue;
		}
		if (m_y_offsets[rank] > -2 * m_radius - 1)
		{
			a_context->save();
			a_context->arc(
				rank * (2 * m_radius + m_spacing) + m_radius,
				m_y_offsets[rank] + m_radius,
				m_radius,
				0.0,
				2.0 * M_PI);
			a_context->set_source_rgba(
				colors[rank].get_red(),
				colors[rank].get_green(),
				colors[rank].get_blue(),
				colors[rank].get_alpha());
			a_context->fill();
			a_context->restore();
		}
	}

	return true;
}