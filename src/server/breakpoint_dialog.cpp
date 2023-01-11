#include "breakpoint_dialog.hpp"

const char *const rank_id = "rank-id";

template <class T>
T *BreakpointDialog::get_widget(const string &widget_name)
{
	T *widget;
	m_builder->get_widget<T>(widget_name, widget);
	return widget;
}

BreakpointDialog::BreakpointDialog(const int num_processes, const int max_buttons_per_row, Breakpoint *const breakpoint, const bool init)
	: m_num_processes(num_processes),
	  m_max_buttons_per_row(max_buttons_per_row),
	  m_breakpoint(breakpoint),
	  m_button_states(new bool[m_num_processes])
{
	m_builder = Gtk::Builder::create_from_file("./ui/breakpoint_dialog.glade");
	m_dialog = get_widget<Gtk::Dialog>("dialog");

	m_grid = get_widget<Gtk::Grid>("check-buttons-grid");
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::CheckButton *check_button = Gtk::manage(new Gtk::CheckButton(std::to_string(rank)));
		if (init)
		{
			check_button->set_active(true);
		}
		else
		{
			check_button->set_active(m_breakpoint->is_created(rank));
		}
		m_grid->attach(*check_button, rank % m_max_buttons_per_row, rank / m_max_buttons_per_row);
	}
	get_widget<Gtk::Button>("toggle-all-button")->signal_clicked().connect(sigc::mem_fun(*this, &BreakpointDialog::toggle_all));
	m_dialog->signal_response().connect(sigc::mem_fun(*this, &BreakpointDialog::on_dialog_response));
	m_dialog->add_button("Ok", Gtk::RESPONSE_OK);
	m_dialog->show_all();
}

BreakpointDialog::~BreakpointDialog()
{
	delete[] m_button_states;
	delete m_dialog;
}

void BreakpointDialog::on_dialog_response(const int response_id)
{
	if (Gtk::RESPONSE_OK != response_id)
	{
		return;
	}
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::CheckButton *check_button = dynamic_cast<Gtk::CheckButton *>(m_grid->get_child_at(rank % m_max_buttons_per_row, rank / m_max_buttons_per_row));
		m_button_states[rank] = check_button->get_active();
	}
}

void BreakpointDialog::toggle_all()
{
	bool one_checked = false;
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::CheckButton *check_button = dynamic_cast<Gtk::CheckButton *>(m_grid->get_child_at(rank % m_max_buttons_per_row, rank / m_max_buttons_per_row));
		if (check_button->get_active())
		{
			one_checked = true;
			break;
		}
	}
	bool state = !one_checked;
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::CheckButton *check_button = dynamic_cast<Gtk::CheckButton *>(m_grid->get_child_at(rank % m_max_buttons_per_row, rank / m_max_buttons_per_row));
		check_button->set_active(state);
	}
}