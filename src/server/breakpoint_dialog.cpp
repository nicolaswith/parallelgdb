#include "breakpoint_dialog.hpp"

const char *const rank_id = "rank-id";

template <class T>
T *BreakpointDialog::get_widget(const string &widget_name)
{
	T *widget;
	m_builder->get_widget<T>(widget_name, widget);
	return widget;
}

BreakpointDialog::BreakpointDialog(const int num_processes, Breakpoint *const breakpoint, const bool init)
	: m_num_processes(num_processes),
	  m_breakpoint(breakpoint),
	  m_button_states(new bool[m_num_processes])
{
	m_builder = Gtk::Builder::create_from_file("./ui/breakpoint_dialog.glade");
	m_dialog = get_widget<Gtk::Dialog>("dialog");

	m_check_buttons_box = get_widget<Gtk::Box>("check-buttons-box");
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::CheckButton *check_button = Gtk::manage(new Gtk::CheckButton(std::to_string(rank)));
		check_button->set_data(rank_id, new int(rank));

		if (init)
		{
			check_button->set_active(true);
		}
		else
		{
			check_button->set_active(m_breakpoint->is_created(rank));
		}
		m_check_buttons_box->pack_start(*check_button);
	}
	get_widget<Gtk::Button>("toggle-all-button")->signal_clicked().connect(sigc::mem_fun(*this, &BreakpointDialog::toggle_all));
	m_dialog->signal_response().connect(sigc::mem_fun(*this, &BreakpointDialog::on_dialog_response));
	m_dialog->add_button("Ok", Gtk::RESPONSE_OK);
	m_dialog->show_all();
}

BreakpointDialog::~BreakpointDialog()
{
	for (Gtk::Widget *check_button_widget : m_check_buttons_box->get_children())
	{
		Gtk::CheckButton *check_button = dynamic_cast<Gtk::CheckButton *>(check_button_widget);
		delete (int *)check_button->get_data(rank_id);
		check_button->remove_data(rank_id);
	}
	delete[] m_button_states;
	delete m_dialog;
}

void BreakpointDialog::on_dialog_response(const int response_id)
{
	if (Gtk::RESPONSE_OK != response_id)
	{
		return;
	}
	for (Gtk::Widget *check_button_widget : m_check_buttons_box->get_children())
	{
		Gtk::CheckButton *check_button = dynamic_cast<Gtk::CheckButton *>(check_button_widget);
		int rank = *(int *)check_button->get_data(rank_id);

		m_button_states[rank] = check_button->get_active();
	}
}

void BreakpointDialog::toggle_all()
{
	bool one_checked = false;
	for (Gtk::Widget *check_button_widget : m_check_buttons_box->get_children())
	{
		Gtk::CheckButton *check_button = dynamic_cast<Gtk::CheckButton *>(check_button_widget);
		if (!check_button)
		{
			continue;
		}
		if (check_button->get_active())
		{
			one_checked = true;
			break;
		}
	}
	bool state = !one_checked;
	for (Gtk::Widget *check_button_widget : m_check_buttons_box->get_children())
	{
		Gtk::CheckButton *check_button = dynamic_cast<Gtk::CheckButton *>(check_button_widget);
		if (!check_button)
		{
			continue;
		}
		check_button->set_active(state);
	}
}