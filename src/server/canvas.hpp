#ifndef CANVAS_HPP
#define CANVAS_HPP

#include "defs.hpp"
#include "mi_gdb.h"

class UIDrawingArea : public Gtk::DrawingArea
{
	static const Gdk::RGBA colors[];
	static const int m_radius;
	static const int m_spacing;

	const int m_num_processes;
	int *const m_y_offsets;

protected:
	virtual bool on_draw(const Cairo::RefPtr<Cairo::Context> &a_context);

public:
	UIDrawingArea(const int a_num_processes);
	virtual ~UIDrawingArea() {}

	void set_y_offset(const int a_process_rank, const int a_offset);

	inline static int radius()
	{
		return m_radius;
	}

	inline static int spacing()
	{
		return m_spacing;
	}
};

#endif /* CANVAS_HPP */
