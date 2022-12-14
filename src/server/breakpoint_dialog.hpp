#ifndef BREAKPOINT_DIALOG_HPP
#define BREAKPOINT_DIALOG_HPP

#include "defs.hpp"
#include "breakpoint.hpp"

#define RESPONSE_ID_OK 0

class BreakpointDialog : public Gtk::Dialog
{
	const int m_num_processes;
	Breakpoint *const m_breakpoint;

	void on_dialog_response(const int response_id);

public:
	BreakpointDialog(const int num_processes, Breakpoint *const breakpoint);
	~BreakpointDialog();
};

#endif /* BREAKPOINT_DIALOG_HPP */