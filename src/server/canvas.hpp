#ifndef CANVAS_HPP
#define CANVAS_HPP

#include "defs.hpp"
#include "mi_gdb.h"

class UIDrawingArea : public Gtk::DrawingArea
{
	static const Gdk::RGBA s_colors[];
	static const int s_radius;
	static const int s_spacing;

	const int m_num_processes;
	int *const m_y_offsets;

protected:
	virtual bool on_draw(const Cairo::RefPtr<Cairo::Context> &context);

public:
	UIDrawingArea(const int num_processes);
	virtual ~UIDrawingArea() {}

	void set_y_offset(const int rank, const int offset);

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
