#include "breakpoint_dialog.hpp"

BreakpointDialog::BreakpointDialog(const int num_processes, Breakpoint *const breakpoint)
	: m_num_processes(num_processes),
	  m_breakpoint(breakpoint)
{
	Gtk::Box *vbox = Gtk::manage(new Gtk::Box{Gtk::ORIENTATION_VERTICAL, 10});
	Gtk::Label *label = Gtk::manage(new Gtk::Label{"Set Breakpoint for Ranks:"});
	Gtk::Box *hbox = Gtk::manage(new Gtk::Box{Gtk::ORIENTATION_HORIZONTAL, 10});
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::CheckButton *check_button = Gtk::manage(new Gtk::CheckButton(std::to_string(rank)));
		check_button->set_active(true);
		hbox->pack_start(*check_button);
	}

	vbox->pack_start(*label);
	vbox->pack_start(*hbox);
	this->get_content_area()->add(*vbox);
	this->add_button("Ok", RESPONSE_ID_OK);

	this->show_all();
}

BreakpointDialog::~BreakpointDialog()
{
}

void BreakpointDialog::on_dialog_response(const int response_id)
{
	if (RESPONSE_ID_OK != response_id)
	{
		return;
	}
	
	// string valid_for = "Ranks: ";
	// Gtk::Box *box = get_widget<Gtk::Box>("gdb-send-select-wrapper-box");
	// for (Gtk::Widget *check_button_widget : box->get_children())
	// {
	// 	Gtk::CheckButton *check_button = dynamic_cast<Gtk::CheckButton *>(check_button_widget);
	// 	if (check_button->get_active())
	// 	{
	// 		valid_for += check_button->get_label() + ", ";
	// 	}
	// }
}