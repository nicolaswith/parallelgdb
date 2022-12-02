#include <string>
#include "canvas.hpp"

UIDrawingArea::UIDrawingArea(const int a_num_processes, const int a_process_rank)
	: m_num_processes(a_num_processes),
	  m_process_rank(a_process_rank),
	  m_draw_pos(0)
{
	m_images = new Glib::RefPtr<Gdk::Pixbuf>[m_num_processes];
	string filename = string("./res/arrow.png");
	// string filename = string("./res/mark_") + std::to_string(m_process_rank) + string(".png");
	try
	{
		m_images[m_process_rank] = Gdk::Pixbuf::create_from_file(filename);
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << '\n';
	}
}

bool UIDrawingArea::on_draw(const Cairo::RefPtr<Cairo::Context> &a_context)
{
	Gdk::Cairo::set_source_pixbuf(a_context, m_images[m_process_rank], 0, m_draw_pos);
	a_context->paint();
	return true;
}