#ifndef BREAKPOINT_DIALOG_HPP
#define BREAKPOINT_DIALOG_HPP

#include "defs.hpp"
#include "breakpoint.hpp"

class BreakpointDialog
{
	const int m_num_processes;
	Breakpoint *const m_breakpoint;

	bool *const m_button_states;

	Glib::RefPtr<Gtk::Builder> m_builder;
	Gtk::Dialog *m_dialog;
	Gtk::Box *m_check_buttons_box;

	void on_dialog_response(const int response_id);
	void toggle_all();

	template <class T>
	T *get_widget(const string &widget_name);

public:
	BreakpointDialog(const int num_processes, Breakpoint *const breakpoint, const bool init);
	~BreakpointDialog();

	inline int run()
	{
		return m_dialog->run();
	}

	inline const bool *get_button_states() const
	{
		return m_button_states;
	}
};

#endif /* BREAKPOINT_DIALOG_HPP */