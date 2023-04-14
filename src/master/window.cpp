/*
	This file is part of ParallelGDB.

	Copyright (c) 2023 by Nicolas With

	ParallelGDB is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	ParallelGDB is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with ParallelGDB.  If not, see <https://www.gnu.org/licenses/>.
*/

/**
 * @file window.cpp
 *
 * @brief Contains the implementation of the UIWindow class.
 *
 * This file contains the implementation of the UIWindow class.
 */

#include <exception>
#include <tuple>
#include <iterator>
#include <string>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtksourceview/gtksource.h>
#include <gtksourceviewmm.h>
#pragma GCC diagnostic pop

#include "window.hpp"
#include "breakpoint.hpp"
#include "breakpoint_dialog.hpp"
#include "follow_dialog.hpp"
#include "canvas.hpp"

#include "mi_gdb.h"

using asio::ip::tcp;
using std::string;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
const char *const breakpoint_category = "breakpoint-category";
const char *const line_number_id = "line-number";
const char *const marks_id = "marks";
const char *const open_file_id = "open-file";
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/**
 * This function allocates all necessary arrays and initializes them.
 *
 * @param num_processes The total number of processes.
 *
 * @param base_port The base port.
 */
UIWindow::UIWindow(const int num_processes, const int base_port)
	: m_num_processes(num_processes),
	  m_follow_rank(FOLLOW_ALL),
	  m_base_port(base_port)
{
	// allocate memory and zero-initialize values
	m_current_line = new int[m_num_processes]();
	m_current_file = new string[m_num_processes]();
	m_target_state = new TargetState[m_num_processes]();
	m_gdb_handle = new mi_h *[m_num_processes]();
	m_exit_code = new int[m_num_processes]();
	m_conns_gdb = new tcp::socket *[m_num_processes]();
	m_conns_trgt = new tcp::socket *[m_num_processes]();
	m_separators = new Gtk::Separator *[m_num_processes]();
	m_text_buffers_gdb = new Gtk::TextBuffer *[m_num_processes]();
	m_text_buffers_trgt = new Gtk::TextBuffer *[m_num_processes + 1]();
	m_scrolled_windows_gdb = new Gtk::ScrolledWindow *[m_num_processes]();
	m_scrolled_windows_trgt = new Gtk::ScrolledWindow *[m_num_processes + 1]();
	m_bkptno_2_bkpt = new std::map<int, Breakpoint *>[m_num_processes];
	m_scroll_connections_gdb = new sigc::connection[m_num_processes];
	m_scroll_connections_trgt = new sigc::connection[m_num_processes];
	m_breakpoints = new Breakpoint *[m_num_processes]();
	m_sent_stop = new bool[m_num_processes]();
	// allocate GDB output parsers
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		m_gdb_handle[rank] = mi_alloc_h();
	}
}

/**
 * This function deallocates all arrays owned by this class. If they are pointer
 * arrays (most of them) the pointed-to data is NOT deleted.
 */
UIWindow::~UIWindow()
{
	delete[] m_current_line;
	delete[] m_current_file;
	delete[] m_target_state;
	delete[] m_exit_code;
	delete[] m_conns_gdb;
	delete[] m_conns_trgt;
	delete[] m_separators;
	delete[] m_text_buffers_gdb;
	delete[] m_text_buffers_trgt;
	delete[] m_scrolled_windows_gdb;
	delete[] m_scrolled_windows_trgt;
	delete[] m_bkptno_2_bkpt;
	delete[] m_scroll_connections_gdb;
	delete[] m_scroll_connections_trgt;
	delete[] m_breakpoints;
	delete[] m_sent_stop;
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		mi_free_h(&m_gdb_handle[rank]);
	}
	delete[] m_gdb_handle;
}

/**
 * This function is a wrapper for the Gtk::get_widget function.
 *
 * @tparam T The widget class.
 *
 * @param[in] widget_name The widget name.
 *
 * @return The pointer to the widget object on success, @c nullptr on error.
 */
template <class T>
T *UIWindow::get_widget(const std::string &widget_name)
{
	T *widget;
	m_builder->get_widget<T>(widget_name, widget);
	return widget;
}

/**
 * This function attaches checkbuttons in a grid layout. It is used for the GDB
 * and target I/O section.
 *
 * @param[in] grid The grid to populate.
 */
void UIWindow::init_grid(Gtk::Grid *grid)
{
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::CheckButton *check_button =
			Gtk::manage(new Gtk::CheckButton(std::to_string(rank)));
		check_button->set_active(true);
		grid->attach(*check_button, rank % m_max_buttons_per_row,
					 rank / m_max_buttons_per_row);
	}
}

/**
 * This function adds a page in the GDB/target I/O notebook for every process.
 * It stores pointers to the content of the page in the @p scrolled_windows and
 * @p text_buffers arrays to simplify access.
 *
 * @param[in] notebook The notebook to add pages to.
 *
 * @param[in] scrolled_windows The list of scrolled windows.
 *
 * @param[in] text_buffers The list of text buffers.
 */
void UIWindow::init_notebook(Gtk::Notebook *notebook,
							 Gtk::ScrolledWindow **scrolled_windows,
							 Gtk::TextBuffer **text_buffers)
{
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::Label *tab = Gtk::manage(new Gtk::Label(std::to_string(rank)));
		Gtk::ScrolledWindow *scrolled_window =
			Gtk::manage(new Gtk::ScrolledWindow());
		scrolled_windows[rank] = scrolled_window;
		Gtk::TextView *text_view = Gtk::manage(new Gtk::TextView());
		text_view->set_editable(false);
		text_buffers[rank] = text_view->get_buffer().get();
		scrolled_window->add(*text_view);
		scrolled_window->set_size_request(-1, 200);
		notebook->append_page(*scrolled_window, *tab);
	}
}

/**
 * This function adds the "All" page into the target IO notebook.
 *
 * @param[in] notebook The target IO notebook.
 */
void UIWindow::init_all_page(Gtk::Notebook *notebook)
{
	Gtk::Label *tab = Gtk::manage(new Gtk::Label("All"));
	Gtk::ScrolledWindow *scrolled_window =
		Gtk::manage(new Gtk::ScrolledWindow());
	m_scrolled_windows_trgt[m_num_processes] = scrolled_window;
	Gtk::TextView *text_view = Gtk::manage(new Gtk::TextView());
	text_view->set_editable(false);
	m_text_buffers_trgt[m_num_processes] = text_view->get_buffer().get();
	scrolled_window->add(*text_view);
	scrolled_window->set_size_request(-1, 200);
	notebook->prepend_page(*scrolled_window, *tab);
}

/**
 * This function initializes the table-like grid layout in the overview. It adds
 * a row for process rank, running and exited states. After that columns for
 * every process are added.
 *
 * The index of the first row to use for source files ( @ref m_first_row_idx)
 * and the index of the last row ( @ref m_last_row_idx) are saved for later use.
 */
void UIWindow::init_overview()
{
	m_overview_grid = get_widget<Gtk::Grid>("overview-grid");

	int row = 0;

	Gtk::Label *label;
	Gtk::Separator *separator;

	label = Gtk::manage(new Gtk::Label("Process"));
	label->set_size_request(100, -1);
	m_overview_grid->attach(*label, row++, 0);

	separator = Gtk::manage(new Gtk::Separator);
	separator->set_size_request(-1, 2);
	m_overview_grid->attach(*separator, 0, row++, 2 * m_num_processes + 1, 1);

	label = Gtk::manage(new Gtk::Label("Running"));
	m_overview_grid->attach(*label, 0, row++);

	separator = Gtk::manage(new Gtk::Separator);
	separator->set_size_request(-1, 2);
	m_overview_grid->attach(*separator, 0, row++, 2 * m_num_processes + 1, 1);

	label = Gtk::manage(new Gtk::Label("Exited"));
	m_overview_grid->attach(*label, 0, row++);

	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		separator = Gtk::manage(new Gtk::Separator);
		separator->set_size_request(2, -1);
		m_overview_grid->attach(*separator, 2 * rank + 1, 0, 1, 5);
		m_separators[rank] = separator;

		label = Gtk::manage(new Gtk::Label(std::to_string(rank)));
		label->set_size_request(35, -1);
		m_overview_grid->attach(*label, 2 * rank + 2, 0);

		label = Gtk::manage(new Gtk::Label());
		m_overview_grid->attach(*label, 2 * rank + 2, 2);

		label = Gtk::manage(new Gtk::Label());
		m_overview_grid->attach(*label, 2 * rank + 2, 4);
	}

	m_first_row_idx = row;
	m_last_row_idx = row;
}

/**
 * This function loads the glade file and connects the signal handlers to the
 * correct buttons. Furthermore is creates the drawing area for the source view.
 * It is not described in the glade file because it is a custom widget.
 * See @ref UIDrawingArea.
 *
 * @param app The Gtk application.
 *
 * @return @c true.
 */
bool UIWindow::init(Glib::RefPtr<Gtk::Application> app)
{
	Gsv::init();

	m_max_buttons_per_row = 8;
	if (m_num_processes % 10 == 0)
	{
		m_max_buttons_per_row = 10;
	}
	if (m_num_processes > 32)
	{
		m_max_buttons_per_row *= 2;
	}

	m_builder = Gtk::Builder::create_from_resource("/pgdb/ui/window.glade");
	m_root_window = get_widget<Gtk::Window>("window");
	m_app = app;

	m_root_window->set_wmclass("Parallel GDB", "Parallel GDB");
	string icon_path = "/usr/share/icons/hicolor/scalable/apps/pgdb.svg";
	if (Glib::file_test(icon_path, Glib::FILE_TEST_EXISTS))
	{
		m_root_window->set_icon_from_file(icon_path);
	}

	// Signal are not compatible with glade and gtkmm ...
	// So connect them the old-fashioned way.
	get_widget<Gtk::Button>("gdb-send-button")
		->signal_clicked()
		.connect(sigc::mem_fun(*this, &UIWindow::send_input_gdb));
	get_widget<Gtk::Entry>("gdb-send-entry")
		->signal_activate()
		.connect(sigc::mem_fun(*this, &UIWindow::send_input_gdb));
	get_widget<Gtk::Button>("target-send-button")
		->signal_clicked()
		.connect(sigc::mem_fun(*this, &UIWindow::send_input_trgt));
	get_widget<Gtk::Entry>("target-send-entry")
		->signal_activate()
		.connect(sigc::mem_fun(*this, &UIWindow::send_input_trgt));
	get_widget<Gtk::Button>("gdb-send-select-toggle-button")
		->signal_clicked()
		.connect(sigc::mem_fun(*this, &UIWindow::toggle_all_gdb));
	get_widget<Gtk::Button>("target-send-select-toggle-button")
		->signal_clicked()
		.connect(sigc::mem_fun(*this, &UIWindow::toggle_all_trgt));
	m_root_window->signal_key_press_event().connect(
		sigc::mem_fun(*this, &UIWindow::on_key_press), false);
	m_root_window->signal_delete_event().connect(
		sigc::mem_fun(*this, &UIWindow::on_delete));
	get_widget<Gtk::Button>("follow-process-button")
		->signal_clicked()
		.connect(
			sigc::mem_fun(*this, &UIWindow::on_follow_button_clicked));
	get_widget<Gtk::Button>("step-over-button")
		->signal_clicked()
		.connect(sigc::bind(
			sigc::mem_fun(*this, &UIWindow::on_interaction_button_clicked),
			GDK_KEY_F5));
	get_widget<Gtk::Button>("step-in-button")
		->signal_clicked()
		.connect(sigc::bind(
			sigc::mem_fun(*this, &UIWindow::on_interaction_button_clicked),
			GDK_KEY_F6));
	get_widget<Gtk::Button>("step-out-button")
		->signal_clicked()
		.connect(sigc::bind(
			sigc::mem_fun(*this, &UIWindow::on_interaction_button_clicked),
			GDK_KEY_F7));
	get_widget<Gtk::Button>("continue-button")
		->signal_clicked()
		.connect(sigc::bind(
			sigc::mem_fun(*this, &UIWindow::on_interaction_button_clicked),
			GDK_KEY_F8));
	get_widget<Gtk::Button>("stop-button")
		->signal_clicked()
		.connect(sigc::bind(
			sigc::mem_fun(*this, &UIWindow::on_interaction_button_clicked),
			GDK_KEY_F9));
	get_widget<Gtk::Button>("restart-button")
		->signal_clicked()
		.connect(sigc::bind(
			sigc::mem_fun(*this, &UIWindow::on_interaction_button_clicked),
			GDK_KEY_F12));
	get_widget<Gtk::Button>("open-file-button")
		->signal_clicked()
		.connect(sigc::mem_fun(*this, &UIWindow::open_file));
	get_widget<Gtk::Button>("close-unused-button")
		->signal_clicked()
		.connect(sigc::mem_fun(*this, &UIWindow::close_unused_tabs));
	get_widget<Gtk::MenuItem>("quit-menu-item")
		->signal_activate()
		.connect(sigc::mem_fun(*this, &UIWindow::on_quit_clicked));
	get_widget<Gtk::MenuItem>("about-menu-item")
		->signal_activate()
		.connect(sigc::mem_fun(*this, &UIWindow::on_about_clicked));

	// create the drawing area
	m_drawing_area = Gtk::manage(new UIDrawingArea(m_num_processes));
	m_drawing_area->set_size_request(
		(2 * UIDrawingArea::radius() + UIDrawingArea::spacing()) *
			m_num_processes,
		-1);
	m_drawing_area->set_halign(Gtk::Align::ALIGN_END);
	get_widget<Gtk::ScrolledWindow>("drawing-area-scrolled-window")
		->add(*m_drawing_area);

	m_files_notebook = get_widget<Gtk::Notebook>("files-notebook");
	Glib::signal_timeout().connect(
		sigc::mem_fun(*this, &UIWindow::update_markers_timeout), 10);

	Gtk::Notebook *notebook_gdb =
		get_widget<Gtk::Notebook>("gdb-output-notebook");
	Gtk::Grid *grid_gdb = get_widget<Gtk::Grid>("gdb-send-select-grid");
	init_notebook(notebook_gdb, m_scrolled_windows_gdb, m_text_buffers_gdb);
	init_grid(grid_gdb);

	Gtk::Notebook *notebook_trgt =
		get_widget<Gtk::Notebook>("target-output-notebook");
	Gtk::Grid *grid_trgt = get_widget<Gtk::Grid>("target-send-select-grid");
	init_notebook(notebook_trgt, m_scrolled_windows_trgt, m_text_buffers_trgt);
	init_all_page(notebook_trgt);
	init_grid(grid_trgt);

	init_overview();

	m_root_window->maximize();
	m_root_window->show_all();
	return true;
}

/**
 * This function displays the about dialog.
 */
void UIWindow::on_about_clicked()
{
	Glib::RefPtr<Gtk::Builder> builder =
		Gtk::Builder::create_from_resource("/pgdb/ui/about_dialog.glade");
	Gtk::AboutDialog *dialog = nullptr;
	builder->get_widget<Gtk::AboutDialog>("dialog", dialog);
	dialog->run();
	delete dialog;
}

/**
 * This function terminates the slaves and then quits the application.
 */
void UIWindow::on_quit_clicked()
{
	GdkEventAny event;
	on_delete(&event);
	m_app->quit();
}

/**
 * This function closes the GDB TCP sockets and thus the slaves. It is called
 * before the application quits.
 *
 * @return @c false. The return value is used to indicate whether the event is
 * completely handled.
 */
bool UIWindow::on_delete(GdkEventAny *)
{
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		if (m_conns_gdb[rank])
		{
			m_conns_gdb[rank]->shutdown(asio::ip::tcp::socket::shutdown_both);
		}
		if (m_conns_trgt[rank])
		{
			m_conns_trgt[rank]->shutdown(asio::ip::tcp::socket::shutdown_both);
		}
	}
	return false;
}

/**
 * This function opens a dialog to select the process to follow.
 */
void UIWindow::on_follow_button_clicked()
{
	FollowDialog dialog(m_num_processes, m_max_buttons_per_row, m_follow_rank);
	if (Gtk::RESPONSE_OK != dialog.run())
	{
		return;
	}
	m_follow_rank = dialog.follow_rank();
	string label;
	if (FOLLOW_ALL == m_follow_rank)
	{
		label = "Following All Processes";
	}
	else if (FOLLOW_NONE == m_follow_rank)
	{
		label = "Following No Process";
	}
	else
	{
		label = "Following Process: " + std::to_string(m_follow_rank);
	}
	get_widget<Gtk::Button>("follow-process-button")->set_label(label);
}

/**
 * This function scrolls a scrolled window to the bottom. After scrolling the
 * event listener is deleted, so that the user can freely scroll in the ouput.
 *
 * @param[in] scrolled_window A pointer to the scrolled window.
 *
 * @param is_gdb Whether the scrolled window belongs to the GDB or
 * target section.
 *
 * @param rank The process rank.
 */
void UIWindow::scroll_bottom(Gtk::Allocation &,
							 Gtk::ScrolledWindow *const scrolled_window,
							 const bool is_gdb, const int rank)
{
	Glib::RefPtr<Gtk::Adjustment> adjustment = scrolled_window->get_vadjustment();
	adjustment->set_value(adjustment->get_upper());
	if (is_gdb)
	{
		m_scroll_connections_gdb[rank].disconnect();
	}
	else
	{
		m_scroll_connections_trgt[rank].disconnect();
	}
}

/**
 * This function writes data to the TCP socket.
 *
 * @param[in] socket The TCP socket.
 *
 * @param[in] data The data to send.
 *
 * @return @c true if all data was sent, @c false otherwise.
 */
bool UIWindow::send_data(tcp::socket *const socket, const string &data)
{
	if (nullptr == socket)
	{
		return false;
	}
	std::size_t bytes_sent =
		asio::write(*socket, asio::buffer(data, data.length()));
	return bytes_sent == data.length();
}

/**
 * This function sends data to all processes for which the checkbutton in the
 * GDB I/O section is checked. If no checkbutton is active a error message is
 * displayed.
 *
 * @param[in] socket The TCP socket.
 *
 * @param[in] cmd The command to send.
 *
 * @note
 * The SIGINT signal must be sent to the target directly. But as it is a control
 * command the states of the GDB checkbuttons are examined instead of the target
 * checkbuttons.
 */
void UIWindow::send_data_to_active(tcp::socket *const *const socket,
								   const string &cmd)
{
	Gtk::Grid *grid = get_widget<Gtk::Grid>("gdb-send-select-grid");
	bool one_selected = false;
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::CheckButton *check_button =
			dynamic_cast<Gtk::CheckButton *>(grid->get_child_at(
				rank % m_max_buttons_per_row, rank / m_max_buttons_per_row));
		if (!check_button->get_active())
		{
			continue;
		}
		one_selected = true;
		if (socket == m_conns_trgt ||
			m_target_state[rank] != TargetState::RUNNING)
		{
			send_data(socket[rank], cmd);
		}
	}
	if (!one_selected)
	{
		Gtk::MessageDialog dialog(*m_root_window, "No Process selected.", false,
								  Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
		dialog.run();
	}
}

/**
 * This function maps the buttons/hot-keys to the corresponding action.
 *
 * @param key_value The key value of the pressed key or the value stored in the
 * button which got pressed. Those to are set to be matching in the @ref init
 * function.
 */
void UIWindow::interact_with_gdb(const int key_value)
{
	string cmd;
	tcp::socket **socket;
	switch (key_value)
	{
	case GDK_KEY_F4:
		close_unused_tabs();
		return;
	case GDK_KEY_F5:
		cmd = "next\n";
		socket = m_conns_gdb;
		break;
	case GDK_KEY_F6:
		cmd = "step\n";
		socket = m_conns_gdb;
		break;
	case GDK_KEY_F7:
		cmd = "finish\n";
		socket = m_conns_gdb;
		break;
	case GDK_KEY_F8:
		cmd = "continue\n";
		socket = m_conns_gdb;
		break;
	case GDK_KEY_F9:
		cmd = "\3"; // Stop: ^C
		socket = m_conns_trgt;
		break;
	case GDK_KEY_F12:
		cmd = "run\n";
		socket = m_conns_gdb;
		break;

	default:
		return;
	}

	send_data_to_active(socket, cmd);
}

/**
 * This function is the signal handler for all the interaction buttons.
 *
 * @param key_value The related key value stored in the button which
 * got pressed.
 */
void UIWindow::on_interaction_button_clicked(const int key_value)
{
	interact_with_gdb(key_value);
}

/**
 * This function is the signal handler for key-press events.
 *
 * @param[in] event The key-press event.
 *
 * @return @c false. The return value is used to indicate whether the event is
 * completely handled.
 */
bool UIWindow::on_key_press(GdkEventKey *event)
{
	interact_with_gdb(event->keyval);
	return false;
}

/**
 * This function sends the text written in an entry ( @p entry_name ) to all
 * TCP sockets ( @p socket ), for which the corresponding checkbuttons in the
 * grid ( @p grid_name ) are active. If no checkbutton is active a error
 * message is displayed.
 *
 * @param[in] entry_name The name of the entry to read the text from.
 *
 * @param[in] grid_name The name of the grid where the checkbuttons are stored.
 *
 * @param[in] socket A pointer to the TCP sockets array. (GDB / target)
 */
void UIWindow::send_input(const string &entry_name, const string &grid_name,
						  tcp::socket *const *const socket)
{
	Gtk::Entry *entry = get_widget<Gtk::Entry>(entry_name);
	string cmd = string(entry->get_text()) + string("\n");
	Gtk::Grid *grid = get_widget<Gtk::Grid>(grid_name);
	bool one_selected = false;
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::CheckButton *check_button =
			dynamic_cast<Gtk::CheckButton *>(grid->get_child_at(
				rank % m_max_buttons_per_row, rank / m_max_buttons_per_row));
		if (check_button->get_active())
		{
			one_selected = true;
			send_data(socket[rank], cmd);
		}
	}
	if (one_selected)
	{
		entry->set_text("");
	}
	else
	{
		Gtk::MessageDialog dialog(*m_root_window,
								  "No Process selected.\nNoting has been sent.",
								  false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
		dialog.run();
	}
}

/**
 * This function sends text to the socat instances connected to GDB.
 */
void UIWindow::send_input_gdb()
{
	send_input("gdb-send-entry", "gdb-send-select-grid", m_conns_gdb);
}

/**
 * This function sends text to the socat instances connected to the target
 * program.
 */
void UIWindow::send_input_trgt()
{
	send_input("target-send-entry", "target-send-select-grid", m_conns_trgt);
}

/**
 * This function stops all processes for which the breakpoint is created.
 */
void UIWindow::stop_all(Breakpoint *breakpoint)
{
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		if (TargetState::RUNNING != m_target_state[rank] ||
			m_sent_stop[rank] ||
			!breakpoint->is_created(rank))
		{
			continue;
		}
		if (send_data(m_conns_trgt[rank], "\3"))
		{
			m_sent_stop[rank] = true;
		}
	}
}

/**
 * This function toggles all checkbuttons in a grid.
 *
 * @param[in] grid_name The grid which holds the checkbuttons.
 */
void UIWindow::toggle_all(const string &grid_name)
{
	Gtk::Grid *grid = get_widget<Gtk::Grid>(grid_name);
	bool one_checked = false;
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::CheckButton *check_button =
			dynamic_cast<Gtk::CheckButton *>(grid->get_child_at(
				rank % m_max_buttons_per_row, rank / m_max_buttons_per_row));
		if (check_button->get_active())
		{
			one_checked = true;
			break;
		}
	}
	bool state = !one_checked;
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::CheckButton *check_button =
			dynamic_cast<Gtk::CheckButton *>(grid->get_child_at(
				rank % m_max_buttons_per_row, rank / m_max_buttons_per_row));
		check_button->set_active(state);
	}
}

/**
 * This function toggles all checkbuttons in the GDB I/O section grid.
 */
void UIWindow::toggle_all_gdb()
{
	toggle_all("gdb-send-select-grid");
}

/**
 * This function toggles all checkbuttons in the target I/O section grid.
 */
void UIWindow::toggle_all_trgt()
{
	toggle_all("target-send-select-grid");
}

/**
 * This function deletes a line mark in a Gsv View. The mark stores a pointer to
 * a Breakpoint object. It needs to be ENSURED that the breakpoint has been
 * deleted for all processes before calling this function!
 *
 * @note
 * The @p source_buffer stores a pointer to a std::set, which holds pointers to
 * all marks in this view. This is used by the @ref close_unused_tabs function,
 * as there is no @c get_all_marks function natively in Gtk.
 *
 * @param[in] mark The mark to delete.
 *
 * @param[in] source_buffer The Gsv source buffer holding the mark.
 *
 * @param[in] breakpoint The related breakpoint of this mark.
 */
void UIWindow::clear_mark(Glib::RefPtr<Gtk::TextMark> &mark,
						  Glib::RefPtr<Gsv::Buffer> &source_buffer,
						  Breakpoint *breakpoint)
{
	std::set<int> *set = (std::set<int> *)source_buffer->get_data(marks_id);
	set->erase(breakpoint->get_line_number() - 1);
	delete breakpoint;
	mark->remove_data(line_number_id);
	source_buffer->delete_mark(mark);
}

/**
 * This function creates a line mark in a Gsv::View. The mark stores a pointer
 * to a Breakpoint object.
 *
 * @param[in] iter The text iterator of the desired line.
 *
 * @param[in] source_buffer The Gsv source buffer that will hold the new mark.
 *
 * @param[in] fullpath The full path of the file loaded in the Gsv::View.
 */
void UIWindow::create_mark(Gtk::TextIter &iter,
						   Glib::RefPtr<Gsv::Buffer> &source_buffer,
						   const string &fullpath)
{
	const int line = iter.get_line();
	// create the breakpoint object
	Breakpoint *breakpoint =
		new Breakpoint{m_num_processes, line + 1, fullpath, this};
	// create the dialog to configure the breakpoint
	BreakpointDialog dialog(m_num_processes, m_max_buttons_per_row,
							breakpoint, true);
	if (Gtk::RESPONSE_OK != dialog.run())
	{
		return;
	}
	// set all selected breakpoints
	breakpoint->update_breakpoints(dialog.get_button_states());
	if (breakpoint->one_created())
	{
		// create the line mark
		Glib::RefPtr<Gsv::Mark> new_mark = source_buffer->create_source_mark(
			std::to_string(line), breakpoint_category, iter);
		// store the breakpoint object in the line mark for later access
		new_mark->set_data(line_number_id, (void *)breakpoint);
		std::set<int> *set = (std::set<int> *)source_buffer->get_data(marks_id);
		if (nullptr == set)
		{
			set = new std::set<int>;
			source_buffer->set_data(marks_id, (void *)set);
		}
		set->insert(line);
	}
	else
	{
		delete breakpoint;
	}
}

/**
 * This function displays the BreakpointDialog dialog, so that the user can
 * modify for which processes the breakpoint should be set.
 *
 * @param[in] mark The line mark.
 *
 * @param[in] source_buffer The source buffer which holds the mark.
 */
void UIWindow::edit_mark(Glib::RefPtr<Gtk::TextMark> &mark,
						 Glib::RefPtr<Gsv::Buffer> &source_buffer)
{
	Breakpoint *breakpoint = (Breakpoint *)mark->get_data(line_number_id);
	BreakpointDialog dialog(m_num_processes, m_max_buttons_per_row,
							breakpoint, false);
	if (Gtk::RESPONSE_OK != dialog.run())
	{
		return;
	}
	breakpoint->update_breakpoints(dialog.get_button_states());
	if (!breakpoint->one_created())
	{
		clear_mark(mark, source_buffer, breakpoint);
	}
}

/**
 * This function deletes the breakpoint for all processes and then deletes the
 * mark. If at least one breakpoint could not be deleted the mark will neither
 * be delete.
 *
 * @param[in] mark The line mark.
 *
 * @param[in] source_buffer The source buffer which holds the mark.
 */
void UIWindow::delete_mark(Glib::RefPtr<Gtk::TextMark> &mark,
						   Glib::RefPtr<Gsv::Buffer> &source_buffer)
{
	Breakpoint *breakpoint = (Breakpoint *)mark->get_data(line_number_id);
	if (breakpoint->delete_breakpoints())
	{
		clear_mark(mark, source_buffer, breakpoint);
	}
}

/**
 * This function is the signal handler for the Gsv::View line mark clicked
 * signal. On left click, if there is no line mark yet, a new one will be
 * created. If one exists already it is deleted. On right click, if the mark
 * exists, the modify dialog is displayed. Otherwise nothing happens.
 *
 * @param[in] iter The text iterator of the clicked line.
 *
 * @param[in] event The Gdk event. This holds information about which mouse
 * button was used.
 *
 * @param[in] fullpath The full path of the file loaded in the Gsv View.
 */
void UIWindow::on_line_mark_clicked(Gtk::TextIter &iter, GdkEvent *const event,
									const string &fullpath)
{
	const int line = iter.get_line();
	// check if a line mark exists at this line
	bool line_has_mark = false;
	Glib::RefPtr<Gtk::TextMark> mark;
	for (Glib::RefPtr<Gtk::TextMark> &current_mark : iter.get_marks())
	{
		if (current_mark->get_name() == std::to_string(line))
		{
			line_has_mark = true;
			mark = current_mark;
			break;
		}
	}

	int page_num = m_files_notebook->get_current_page();
	Gtk::ScrolledWindow *scrolled_window = dynamic_cast<Gtk::ScrolledWindow *>(
		m_files_notebook->get_nth_page(page_num));
	Gsv::View *source_view =
		dynamic_cast<Gsv::View *>(scrolled_window->get_child());
	Glib::RefPtr<Gsv::Buffer> source_buffer = source_view->get_source_buffer();

	// choose the appropriate action based on mark state and used mouse button
	if (event->type == GDK_BUTTON_PRESS &&
		event->button.button == GDK_BUTTON_SECONDARY)
	{
		if (line_has_mark)
		{
			edit_mark(mark, source_buffer);
		}
	}
	else if (!line_has_mark)
	{
		create_mark(iter, source_buffer, fullpath);
	}
	else
	{
		delete_mark(mark, source_buffer);
	}
}

/**
 * This function sets the color for the line numbers in the Overview. It uses
 * the colors specified in @ref UIDrawingArea::s_colors to highlight processes
 * that are in the same line. If there are more lines than colors, all excess
 * lines are not colored.
 *
 * The coloring is independent from file to file (row to row).
 */
void UIWindow::color_overview()
{
	for (std::pair<const string, int> &pair : m_path_2_row)
	{
		const string &path = pair.first;
		const int row = pair.second;

		std::map<int, int> line_2_offset;
		int color_offset = 0;
		for (int rank = 0; rank < m_num_processes; ++rank)
		{
			Gtk::Label *label = dynamic_cast<Gtk::Label *>(
				m_overview_grid->get_child_at(2 * rank + 2, row));
			label->unset_color();

			if (m_current_file[rank] != path)
			{
				continue;
			}
			if (line_2_offset.find(m_current_line[rank]) == line_2_offset.end())
			{
				line_2_offset[m_current_line[rank]] = color_offset++;
			}
			if (color_offset <= UIDrawingArea::NUM_COLORS)
			{
				label->override_color(
					UIDrawingArea::s_colors[line_2_offset[m_current_line[rank]]]);
			}
		}
	}
}

/**
 * This function clears all columns of a process in the Overview. This is used
 * when a process goes to the running or exited state. These have separate rows
 * where this information is displayed.
 *
 * @param rank The process rank.
 */
void UIWindow::clear_labels_overview(const int rank)
{
	for (std::pair<const string, int> &pair : m_path_2_row)
	{
		const int row = pair.second;
		Gtk::Label *label = dynamic_cast<Gtk::Label *>(
			m_overview_grid->get_child_at(2 * rank + 2, row));
		if (label)
		{
			label->set_text("");
			label->set_tooltip_text("");
		}
	}
}

/**
 * This function updates the running and exited row for a processes. If the
 * exit code is non-zero (error) it is highlighted in red.
 *
 * It is called after a GDB response has been parsed. See @ref handle_data_gdb.
 */
void UIWindow::check_overview(const int rank)
{
	// clear running label
	Gtk::Label *label = dynamic_cast<Gtk::Label *>(
		m_overview_grid->get_child_at(2 * rank + 2, 2));
	label->set_text("");
	label->set_tooltip_text("");
	// clear exited label
	label = dynamic_cast<Gtk::Label *>(
		m_overview_grid->get_child_at(2 * rank + 2, 4));
	label->set_text("");
	label->set_tooltip_text("");
	if (m_target_state[rank] == TargetState::RUNNING)
	{
		clear_labels_overview(rank);
		Gtk::Label *label = dynamic_cast<Gtk::Label *>(
			m_overview_grid->get_child_at(2 * rank + 2, 2));
		label->set_text("R");
		label->set_tooltip_text("Running");
	}
	else if (m_target_state[rank] == TargetState::EXITED)
	{
		clear_labels_overview(rank);
		Gtk::Label *label = dynamic_cast<Gtk::Label *>(
			m_overview_grid->get_child_at(2 * rank + 2, 4));
		const string exit_code = std::to_string(m_exit_code[rank]);
		label->set_text(exit_code);
		label->set_tooltip_text("Exited with code: " + exit_code);
		// highlight error exit code
		if (0 != m_exit_code[rank])
		{
			label->override_color(UIDrawingArea::s_colors[0]);
		}
		else
		{
			label->unset_color();
		}
	}
}

/**
 * This function updates all labels and their tooltip for a process.
 *
 * It is called after a GDB response has been parsed and a change in location
 * has been detected. See @ref handle_data_gdb.
 *
 * @param rank The process rank.
 *
 * @param[in] fullpath The full path of the file the process is currently
 * stopped in.
 *
 * @param line The line number in the file the process is currently
 * stopped in. This NEEDS to be a one-based index!
 */
void UIWindow::update_overview(const int rank, const string &fullpath,
							   const int line)
{
	for (std::pair<const string, int> &pair : m_path_2_row)
	{
		const string &path = pair.first;
		const int row = pair.second;

		Gtk::Label *label = dynamic_cast<Gtk::Label *>(
			m_overview_grid->get_child_at(2 * rank + 2, row));
		if (!label)
		{
			throw std::invalid_argument("Invalid row.");
		}

		// clear label and tooltip if process is not in this file
		if (fullpath != path)
		{
			label->set_text("");
			label->set_tooltip_text("");
			continue;
		}

		// get source code from source file at line
		string tooltip = "File not found.";
		if (m_path_2_view.find(fullpath) != m_path_2_view.end())
		{
			Gtk::TextIter iter = m_path_2_view[fullpath]
									 ->get_buffer()
									 ->get_iter_at_line(line - 1);
			if (iter)
			{
				Gtk::TextIter end = m_path_2_view[fullpath]
										->get_buffer()
										->get_iter_at_line(line - 1);
				end.forward_to_line_end();
				tooltip = iter.get_text(end);
			}
		}
		label->set_tooltip_text(tooltip);
		label->set_text(std::to_string(line));
	}

	color_overview();
}

/**
 * This function appends a new row for a source file to the Overview.
 *
 * @param[in] basename The basename of the source file.
 *
 * @param[in] fullpath The fullpath of the source file.
 */
void UIWindow::append_overview_row(const string &basename,
								   const string &fullpath)
{
	// append a separator line
	Gtk::Separator *separator = Gtk::manage(new Gtk::Separator);
	separator->set_size_request(-1, 2);
	m_overview_grid->attach(*separator, 0, m_last_row_idx++,
							2 * m_num_processes + 1, 1);
	// append the file basename to the very left
	Gtk::Label *basename_label = Gtk::manage(new Gtk::Label(basename));
	basename_label->set_tooltip_text(fullpath);
	m_overview_grid->attach(*basename_label, 0, m_last_row_idx);
	// append the process column labels and separators
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		m_path_2_row[fullpath] = m_last_row_idx;
		string text = "";
		if (fullpath == m_current_file[rank])
		{
			text = std::to_string(m_current_line[rank]);
		}
		Gtk::Label *line_num = Gtk::manage(new Gtk::Label(text));
		m_overview_grid->attach(*line_num, 2 * rank + 2, m_last_row_idx);
		// prolong the vertical separators
		m_overview_grid->remove(*dynamic_cast<Gtk::Widget *>(m_separators[rank]));
		m_overview_grid->attach(*m_separators[rank], 2 * rank + 1, 0, 1,
								m_last_row_idx + 1);
	}
	m_last_row_idx++;
	m_overview_grid->show_all();
	color_overview();
}

/**
 * This function appends a source file page to the source view notebook.
 *
 * @param[in] fullpath The fullpath of the source file to append.
 *
 * @param rank The process rank the call originates from.
 */
void UIWindow::append_source_file(const string &fullpath, const int rank)
{
	// check that file is not opened already
	if (m_opened_files.find(fullpath) != m_opened_files.end())
	{
		if (rank == m_follow_rank || FOLLOW_ALL == m_follow_rank)
		{
			m_files_notebook->set_current_page(m_path_2_pagenum[fullpath]);
		}
		return;
	}
	m_opened_files.insert(fullpath);
	string basename = Glib::path_get_basename(fullpath);
	Gtk::Label *label = Gtk::manage(new Gtk::Label(basename));
	label->set_tooltip_text(fullpath);
	Gtk::ScrolledWindow *scrolled_window =
		Gtk::manage(new Gtk::ScrolledWindow());
	// prepare the Gsv::View
	Gsv::View *source_view = Gtk::manage(new Gsv::View());
	Glib::RefPtr<Gsv::Buffer> source_buffer = source_view->get_source_buffer();
	source_view->set_source_buffer(source_buffer);
	source_view->set_editable(false);
	source_view->set_show_line_marks(true);
	source_view->set_bottom_margin(30);
	scrolled_window->add(*source_view);
	// prepare breakpoint icon to be used on click events
	Glib::RefPtr<Gsv::MarkAttributes> attributes =
		Gsv::MarkAttributes::create();
	attributes->set_icon(Gio::Icon::create("resource:///pgdb/res/breakpoint.svg"));
	source_view->set_mark_attributes(breakpoint_category, attributes, 0);
	// load the content of the source file
	char *content;
	size_t content_length;
	if (g_file_get_contents(fullpath.c_str(), &content,
							&content_length, nullptr))
	{
		source_buffer->set_language(
			Gsv::LanguageManager::get_default()
				->guess_language(fullpath, Glib::ustring()));
		source_buffer->set_highlight_matching_brackets(true);
		source_buffer->set_highlight_syntax(true);
		source_buffer->set_text(content);
		source_view->set_show_line_numbers(true);
		source_view->signal_line_mark_activated().connect(sigc::bind(
			sigc::mem_fun(*this, &UIWindow::on_line_mark_clicked), fullpath));
	}
	else
	{
		source_buffer->set_text(string("Could not load file: ") +
								fullpath +
								"\n\n<- Click icon to open a file.");
		sigc::connection open_file_connection =
			source_view->signal_line_mark_activated().connect(sigc::bind(
				sigc::mem_fun(*this, &UIWindow::open_missing), fullpath));
		m_path_2_connection[fullpath] = open_file_connection;
		Glib::RefPtr<Gsv::Mark> new_mark =
			source_buffer->create_source_mark(
				open_file_id, breakpoint_category,
				source_buffer->get_iter_at_line(2));
	}
	scrolled_window->show_all();
	int page_num = m_files_notebook->append_page(*scrolled_window, *label);
	if (rank == m_follow_rank || FOLLOW_ALL == m_follow_rank)
	{
		m_files_notebook->set_current_page(page_num);
	}
	m_path_2_pagenum[fullpath] = page_num;
	m_pagenum_2_path[page_num] = fullpath;
	m_path_2_view[fullpath] = source_view;
	append_overview_row(basename, fullpath);
}

/**
 * This function opens a file for a source file which was not found. Therefore
 * a file-chooser dialog is shown, where the user selects the file to open.
 *
 * @param[in] fullpath The source file which was not found.
 */
void UIWindow::open_missing(Gtk::TextIter &, GdkEvent *, const string &fullpath)
{
	Gsv::View *source_view = m_path_2_view[fullpath];
	Glib::RefPtr<Gsv::Buffer> source_buffer = source_view->get_source_buffer();
	string open_path = "";
	// scope for dialog, it will close only on destruction
	{
		Gtk::FileChooserDialog dialog(*m_root_window,
									  string("Open File for: ") + fullpath);
		dialog.add_button("Cancel", Gtk::RESPONSE_CANCEL);
		dialog.add_button("Open", Gtk::RESPONSE_OK);
		if (Gtk::RESPONSE_OK == dialog.run())
		{
			open_path = dialog.get_filename();
		}
		if ("" == open_path)
		{
			return;
		}
	}
	char *content;
	size_t content_length;
	// load the content of the source file
	if (g_file_get_contents(open_path.c_str(), &content,
							&content_length, nullptr))
	{
		source_buffer->delete_mark(source_buffer->get_mark(open_file_id));
		m_path_2_connection[fullpath].disconnect();
		m_path_2_connection.erase(fullpath);
		source_view->set_show_line_numbers(true);
		source_buffer->set_language(
			Gsv::LanguageManager::get_default()
				->guess_language(open_path, Glib::ustring()));
		source_buffer->set_highlight_matching_brackets(true);
		source_buffer->set_highlight_syntax(true);
		source_view->signal_line_mark_activated().connect(sigc::bind(
			sigc::mem_fun(*this, &UIWindow::on_line_mark_clicked), fullpath));
		source_buffer->set_text(content);
		for (int rank = 0; rank < m_num_processes; ++rank)
		{
			update_overview(rank, m_current_file[rank], m_current_line[rank]);
		}
	}
}

/**
 * This function opens a source file in the source view notebook.
 */
void UIWindow::open_file()
{
	Gtk::FileChooserDialog dialog(*m_root_window, "Open Source File");
	dialog.add_button("Cancel", Gtk::RESPONSE_CANCEL);
	dialog.add_button("Open", Gtk::RESPONSE_OK);
	if (Gtk::RESPONSE_OK == dialog.run())
	{
		string fullpath = dialog.get_filename();
		append_source_file(fullpath, m_follow_rank);
	}
}

/**
 * This function closes all unused source files and removes the corresponding
 * rows in the Overview. A source file is used if it contains a process or a
 * breakpoint.
 */
void UIWindow::close_unused_tabs()
{
	int num_pages;
	std::set<int> pages_to_close;

	num_pages = m_files_notebook->get_n_pages();
	for (int page_num = 0; page_num < num_pages; ++page_num)
	{
		bool is_used = false;
		string fullpath = m_pagenum_2_path[page_num];
		// check if the source file contains a process
		for (int rank = 0; rank < m_num_processes; ++rank)
		{
			if (m_current_file[rank] == fullpath)
			{
				is_used = true;
				break;
			}
		}
		if (!is_used)
		{
			// check if the source file contains a breakpoint
			Gtk::Widget *page = m_files_notebook->get_nth_page(page_num);
			Gtk::ScrolledWindow *scrolled_window =
				dynamic_cast<Gtk::ScrolledWindow *>(page);
			Gsv::View *source_view =
				dynamic_cast<Gsv::View *>(scrolled_window->get_child());
			Glib::RefPtr<Gsv::Buffer> source_buffer =
				source_view->get_source_buffer();
			std::set<int> *set = (std::set<int> *)source_buffer->get_data(marks_id);
			if (set)
			{
				if (set->empty())
				{
					delete set;
				}
				else
				{
					is_used = true;
				}
			}
		}
		if (!is_used)
		{
			pages_to_close.insert(page_num);
		}
	}
	// reverse order to keep indexeis valid while deleting
	std::set<int>::reverse_iterator rit;
	for (rit = pages_to_close.rbegin(); rit != pages_to_close.rend(); ++rit)
	{
		const int page_num = *rit;
		m_files_notebook->remove_page(page_num);
		const string fullpath = m_pagenum_2_path[page_num];
		m_overview_grid->remove_row(m_path_2_row[fullpath]);
		m_overview_grid->remove_row(m_path_2_row[fullpath] - 1);
		m_last_row_idx -= 2;
	}
	// clear lookup maps
	m_path_2_pagenum.clear();
	m_path_2_row.clear();
	m_pagenum_2_path.clear();
	m_path_2_view.clear();
	m_opened_files.clear();
	// rebuild lookup maps
	num_pages = m_files_notebook->get_n_pages();
	for (int page_num = 0; page_num < num_pages; ++page_num)
	{
		Gtk::Widget *page = m_files_notebook->get_nth_page(page_num);
		Gtk::Label *label =
			dynamic_cast<Gtk::Label *>(m_files_notebook->get_tab_label(*page));
		Gtk::ScrolledWindow *scrolled_window =
			dynamic_cast<Gtk::ScrolledWindow *>(page);
		Gsv::View *source_view =
			dynamic_cast<Gsv::View *>(scrolled_window->get_child());
		string fullpath = label->get_tooltip_text();

		m_path_2_pagenum[fullpath] = page_num;
		m_path_2_row[fullpath] = m_first_row_idx + (2 * page_num) + 1;
		m_pagenum_2_path[page_num] = fullpath;
		m_path_2_view[fullpath] = source_view;
		m_opened_files.insert(fullpath);
	}
}

/**
 * This function sets the position of the dot in the drawing area for each
 * process. If the process is currently not in this file or running/exited the
 * dots position is set out of the visible area.
 *
 * @param page_num The page number of the currently open notebook page.
 */
void UIWindow::update_markers(const int page_num)
{
	Gtk::ScrolledWindow *scrolled_window = dynamic_cast<Gtk::ScrolledWindow *>(
		m_files_notebook->get_nth_page(page_num));
	Gsv::View *source_view =
		dynamic_cast<Gsv::View *>(scrolled_window->get_child());
	Glib::RefPtr<Gsv::Buffer> source_buffer = source_view->get_source_buffer();
	Glib::RefPtr<Gtk::Adjustment> adjustment = scrolled_window->get_vadjustment();
	string page_path = m_pagenum_2_path[page_num];
	int offset = m_files_notebook->get_height() - scrolled_window->get_height();
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::TextIter line_iter =
			source_buffer->get_iter_at_line(m_current_line[rank] - 1);
		if (!line_iter || page_path != m_current_file[rank] ||
			m_target_state[rank] != TargetState::STOPPED)
		{
			// set out of visible area
			m_drawing_area->set_y_offset(
				rank, -3 * UIDrawingArea::radius());
			continue;
		}
		Gdk::Rectangle rect;
		source_view->get_iter_location(line_iter, rect);
		int draw_pos = offset + rect.get_y() - int(adjustment->get_value());
		m_drawing_area->set_y_offset(rank, draw_pos);
	}
	m_drawing_area->queue_draw();
}

/**
 * This function checks if the source file notebook has pages and if so
 * updates the dots position. It is called periodically by Gtk. See @ref init.
 *
 * @return @c true. The return value is used to indicate whether the event is
 * completely handled.
 */
bool UIWindow::update_markers_timeout()
{
	int page_num = m_files_notebook->get_current_page();
	if (page_num >= 0)
	{
		update_markers(page_num);
	}
	return true;
}

/**
 * This function scrolls a scrolled window to a line.
 *
 * @param rank The process rank.
 */
void UIWindow::do_scroll(const int rank) const
{
	Gsv::View *source_view = m_path_2_view.at(m_current_file[rank]);
	Gtk::TextIter iter =
		source_view->get_buffer()->get_iter_at_line(m_current_line[rank] - 1);
	if (!iter)
	{
		return;
	}
	source_view->scroll_to(iter, 0.1);
}

/**
 * This function appends scrolling a scrolled window to the idle handler. It
 * will execute in the near future. (When Gdk has time for it...)
 *
 * @param rank The process rank.
 */
void UIWindow::scroll_to_line(const int rank) const
{
	if (rank == m_follow_rank || FOLLOW_ALL == m_follow_rank)
	{
		Glib::signal_idle().connect_once(
			sigc::bind(sigc::mem_fun(this, &UIWindow::do_scroll), rank));
	}
}

/**
 * This function updates the current line and file for a process.
 *
 * It is called after a GDB response has been parsed and a change in location
 * has been detected. See @ref handle_data_gdb.
 *
 * @param rank The process rank.
 *
 * @param[in] fullpath The full path of the file the process is currently
 * stopped in.
 *
 * @param line The line number in the file the process is currently
 * stopped in. This NEEDS to be a one-based index!
 */
void UIWindow::set_position(const int rank, const string &fullpath,
							const int line)
{
	m_current_file[rank] = fullpath;
	m_current_line[rank] = line;
	update_overview(rank, fullpath, line);
}

/**
 * This function parses the GDB output for printable output and state changes of
 * the target program.
 *
 * @param[in] first_output A pointer to the first output in the list of outputs
 * sent by GDB.
 *
 * @param rank The process rank the GDB output originates from.
 */
void UIWindow::parse_target_state(mi_output *first_output, const int rank)
{
	Gtk::TextBuffer *buffer = m_text_buffers_gdb[rank];
	mi_output *current_output = first_output;
	while (nullptr != current_output)
	{
		if (MI_CL_RUNNING == current_output->tclass)
		{
			m_target_state[rank] = TargetState::RUNNING;
			m_current_file[rank] = "";
		}
		else if (MI_CL_STOPPED == current_output->tclass)
		{
			m_target_state[rank] = TargetState::STOPPED;
			m_sent_stop[rank] = false;
		}
		if (current_output->type == MI_T_OUT_OF_BAND &&
			current_output->stype == MI_ST_STREAM)
		{
			char *text = get_cstr(current_output);
			if (text)
			{
				buffer->insert(buffer->end(), text);
			}
		}
		current_output = current_output->next;
	}
}

/**
 * This function searches for a breakpoint record in the GDB output. When
 * found, the number (ID) of the breakpoint is saved. It is needed to delete the
 * breakpoint later on.
 *
 * @param[in] first_output A pointer to the first output in the list of outputs
 * sent by GDB.
 *
 * @param rank The process rank the GDB output originates from.
 */
void UIWindow::parse_breakpoint(mi_output *first_output, const int rank)
{
	if (nullptr == m_breakpoints[rank])
	{
		return;
	}
	mi_bkpt *breakpoint = mi_res_bkpt(first_output);
	if (breakpoint)
	{
		m_breakpoints[rank]->set_number(rank, breakpoint->number);
		m_bkptno_2_bkpt[rank][breakpoint->number] = m_breakpoints[rank];
		m_breakpoints[rank] = nullptr;
	}
	mi_free_bkpt(breakpoint);
}

/**
 * This function searches for a stop record and extracts information from it.
 * This includes current file, line and when exited, the exit code.
 *
 * @param[in] first_output A pointer to the first output in the list of outputs
 * sent by GDB.
 *
 * @param rank The process rank the GDB output originates from.
 */
void UIWindow::parse_stop_record(mi_output *first_output, const int rank)
{
	mi_stop *stop_record = mi_res_stop(first_output);
	if (stop_record)
	{
		if (stop_record->have_bkptno && stop_record->bkptno > 1 &&
			m_bkptno_2_bkpt[rank].find(stop_record->bkptno) !=
				m_bkptno_2_bkpt[rank].end() &&
			m_bkptno_2_bkpt[rank][stop_record->bkptno]->get_stop_all())
		{
			stop_all(m_bkptno_2_bkpt[rank][stop_record->bkptno]);
		}
		if (mi_stop_reason::sr_exited_signalled == stop_record->reason ||
			mi_stop_reason::sr_exited == stop_record->reason ||
			mi_stop_reason::sr_exited_normally == stop_record->reason)
		{
			m_target_state[rank] = TargetState::EXITED;
			m_current_file[rank] = "";
			m_exit_code[rank] = stop_record->exit_code;
			clear_labels_overview(rank);
		}
		if (stop_record->frame && stop_record->frame->fullname)
		{
			const string fullpath = stop_record->frame->fullname;
			// this index is one-based!
			const int line = stop_record->frame->line;
			set_position(rank, fullpath, line);
			append_source_file(fullpath, rank);
			scroll_to_line(rank);
		}
	}
	mi_free_stop(stop_record);
}

/**
 * This function tokenizes the received GDB output and feeds it to the output
 * parser. The resulting response objects are then analyzed and based on that
 * states are updated.
 *
 * @param[in] data The received GDB output.
 *
 * @param rank The process rank.
 */
void UIWindow::handle_data_gdb(const char *const data, const int rank)
{
	char *token = strtok((char *)data, "\n");
	while (nullptr != token)
	{
		int token_length = strlen(token);
		// token[token_length]: '\n', replaced with '\0' by strtok
		if (token[token_length - 1] == '\r')
		{
			token[token_length - 1] = '\0';
		}
		m_gdb_handle[rank]->line = token;
		int response = mi_get_response(m_gdb_handle[rank]);
		if (0 != response)
		{
			mi_output *first_output = mi_retire_response(m_gdb_handle[rank]);
			parse_target_state(first_output, rank);
			parse_stop_record(first_output, rank);
			parse_breakpoint(first_output, rank);
			mi_free_output(first_output);
		}
		token = strtok(nullptr, "\n");
	}
	m_gdb_handle[rank]->line = nullptr;
	check_overview(rank);
}

/**
 * This function appends text to the target I/O text view.
 *
 * @param[in] data The text to append.
 *
 * @param rank The process rank.
 */
void UIWindow::handle_data_trgt(const char *const data, const int rank)
{
	Gtk::TextBuffer *buffer = m_text_buffers_trgt[rank];
	buffer->insert(buffer->end(), data);
	buffer = m_text_buffers_trgt[m_num_processes];
	buffer->insert(buffer->end(), "-----" + std::to_string(rank) + "-----\n");
	buffer->insert(buffer->end(), data);
}

/**
 * This function forwards the received data to the corresponding data handler.
 * When the data has been handled it is freed.
 *
 * @param[in] data The received text.
 *
 * @param port The originating TCP port.
 */
void UIWindow::handle_data(const char *const data, const int port)
{
	const int rank = get_rank(port);
	const bool is_gdb = src_is_gdb(port);

	m_mutex_gui.lock();

	if (is_gdb)
	{
		handle_data_gdb(data, rank);
		Gtk::ScrolledWindow *scrolled_window = m_scrolled_windows_gdb[rank];
		if (m_scroll_connections_gdb[rank].empty())
		{
			m_scroll_connections_gdb[rank] =
				scrolled_window->signal_size_allocate().connect(
					sigc::bind(sigc::mem_fun(*this, &UIWindow::scroll_bottom),
							   scrolled_window, is_gdb, rank));
		}
	}
	else
	{
		handle_data_trgt(data, rank);
		Gtk::ScrolledWindow *scrolled_window = m_scrolled_windows_trgt[rank];
		if (m_scroll_connections_trgt[rank].empty())
		{
			m_scroll_connections_trgt[rank] =
				scrolled_window->signal_size_allocate().connect(
					sigc::bind(sigc::mem_fun(*this, &UIWindow::scroll_bottom),
							   scrolled_window, is_gdb, rank));
		}
	}

	free((void *)data);

	m_mutex_gui.unlock();
}

/**
 * Checks how many connections are established and updates the text in the
 * message dialog.
 *
 * @return @c true. The return value is used to indicate whether the event is
 * completely handled.
 */
bool UIWindow::wait_slaves_timeout(Gtk::MessageDialog *dialog)
{
	if (nullptr == dialog)
	{
		return true;
	}
	int num_conns = 0;
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		if (m_conns_gdb[rank])
		{
			num_conns++;
		}
		if (m_conns_trgt[rank])
		{
			num_conns++;
		}
	}
	dialog->set_secondary_text(std::to_string(num_conns) + " connections of " + std::to_string(2 * m_num_processes) + " are established.");
	// close dialog when all connections are established.
	if (num_conns == 2 * m_num_processes)
	{
		delete dialog;
		dialog = nullptr;
	}
	return true;
}

/**
 * Waits for the slaves to connect.
 *
 * @return @c true when all connections are established, @c false, when the user
 * cancels the waiting.
 */
bool UIWindow::wait_slaves()
{
	Gtk::MessageDialog *dialog = new Gtk::MessageDialog(
		*root_window(), "Waiting for Processes.", false,
		Gtk::MESSAGE_INFO, Gtk::BUTTONS_CANCEL);
	sigc::connection timeout_connection = Glib::signal_timeout().connect(
		sigc::bind(
			sigc::mem_fun(*this, &UIWindow::wait_slaves_timeout),
			dialog),
		100);
	int res = dialog->run();
	timeout_connection.disconnect();
	if (Gtk::RESPONSE_NONE == res)
	{
		return true;
	}
	else
	{
		delete dialog;
		dialog = nullptr;
		return false;
	}
}