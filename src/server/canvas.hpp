#ifndef CANVAS_HPP
#define CANVAS_HPP

#include "defs.hpp"
#include "mi_gdb.h"

class UIDrawingArea : public Gtk::DrawingArea
{
	const int m_num_processes;
	const int m_process_rank;
	int m_draw_pos;
	Glib::RefPtr<Gdk::Pixbuf> *m_images;

protected:
	virtual bool on_draw(const Cairo::RefPtr<Cairo::Context> &a_context);

public:
	UIDrawingArea(const int a_num_processes, const int a_process_rank);
	virtual ~UIDrawingArea() {}

	inline void draw_pos(const int a_draw_pos)
	{
		m_draw_pos = a_draw_pos;
	}

	inline int draw_pos() const
	{
		return m_draw_pos;
	}
};

#endif /* CANVAS_HPP */
