#include <string>

#include "window.hpp"
#include "breakpoint.hpp"
#include "breakpoint_dialog.hpp"

const char *const breakpoint_category = "breakpoint-category";
const char *const line_number_id = "line-number";
const char *const marks_id = "marks";

int max_buttons_per_row = 8;

UIWindow::UIWindow(const int num_processes)
	: m_num_processes(num_processes),
	  m_sent_run(false)
{
	m_current_line = new int[m_num_processes];
	m_source_view_path = new string[m_num_processes];
	m_target_state = new TargetState[m_num_processes];
	m_conns_gdb = new tcp::socket *[m_num_processes];
	m_conns_trgt = new tcp::socket *[m_num_processes];
	m_text_buffers_gdb = new Gtk::TextBuffer *[m_num_processes];
	m_text_buffers_trgt = new Gtk::TextBuffer *[m_num_processes];
	m_scrolled_windows_gdb = new Gtk::ScrolledWindow *[m_num_processes];
	m_scrolled_windows_trgt = new Gtk::ScrolledWindow *[m_num_processes];
	m_scroll_connections_gdb = new sigc::connection[m_num_processes];
	m_scroll_connections_trgt = new sigc::connection[m_num_processes];
	m_breakpoints = new Breakpoint *[m_num_processes];
	m_started = new bool[m_num_processes];
	m_conns_open_gdb = new bool[m_num_processes];
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		m_target_state[rank] = TargetState::UNKNOWN;
		m_conns_gdb[rank] = nullptr;
		m_conns_trgt[rank] = nullptr;
		m_conns_open_gdb[rank] = false;
		m_current_line[rank] = 0;
		m_breakpoints[rank] = nullptr;
		m_started[rank] = false;
	}
}

UIWindow::~UIWindow()
{
	delete[] m_current_line;
	delete[] m_source_view_path;
	delete[] m_conns_gdb;
	delete[] m_conns_trgt;
	delete[] m_text_buffers_gdb;
	delete[] m_text_buffers_trgt;
	delete[] m_scrolled_windows_gdb;
	delete[] m_scrolled_windows_trgt;
	delete[] m_scroll_connections_gdb;
	delete[] m_scroll_connections_trgt;
	delete[] m_started;
	delete[] m_conns_open_gdb;
}

template <class T>
T *UIWindow::get_widget(const string &widget_name)
{
	T *widget;
	m_builder->get_widget<T>(widget_name, widget);
	return widget;
}

void UIWindow::init_grid(Gtk::Grid *grid)
{
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::CheckButton *check_button = Gtk::manage(new Gtk::CheckButton(std::to_string(rank)));
		check_button->set_active(true);
		grid->attach(*check_button, rank % max_buttons_per_row, rank / max_buttons_per_row);
	}
}

void UIWindow::init_notebook(Gtk::Notebook *notebook, Gtk::ScrolledWindow **scrolled_windows, Gtk::TextBuffer **text_buffers)
{
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::Label *tab = Gtk::manage(new Gtk::Label(std::to_string(rank)));
		Gtk::ScrolledWindow *scrolled_window = Gtk::manage(new Gtk::ScrolledWindow());
		scrolled_windows[rank] = scrolled_window;
		Gtk::TextView *text_view = Gtk::manage(new Gtk::TextView());
		text_view->set_editable(false);
		text_buffers[rank] = text_view->get_buffer().get();
		scrolled_window->add(*text_view);
		scrolled_window->set_size_request(-1, 200);
		notebook->append_page(*scrolled_window, *tab);
	}
}

bool UIWindow::init(Glib::RefPtr<Gtk::Application> app)
{
	Gsv::init();

	if (m_num_processes % 10 == 0)
	{
		max_buttons_per_row = 10;
	}
	if (m_num_processes > 32)
	{
		max_buttons_per_row *= 2;
	}

	m_builder = Gtk::Builder::create_from_file("./ui/window.glade");
	m_root_window = get_widget<Gtk::Window>("window");
	m_app = app;

	// Signal are not compatible with glade and gtkmm ...
	// So connect them the old-fashioned way.
	get_widget<Gtk::Button>("gdb-send-button")->signal_clicked().connect(sigc::mem_fun(*this, &UIWindow::send_input_gdb));
	get_widget<Gtk::Entry>("gdb-send-entry")->signal_activate().connect(sigc::mem_fun(*this, &UIWindow::send_input_gdb));
	get_widget<Gtk::Button>("target-send-button")->signal_clicked().connect(sigc::mem_fun(*this, &UIWindow::send_input_trgt));
	get_widget<Gtk::Entry>("target-send-entry")->signal_activate().connect(sigc::mem_fun(*this, &UIWindow::send_input_trgt));
	get_widget<Gtk::Button>("gdb-send-select-toggle-button")->signal_clicked().connect(sigc::mem_fun(*this, &UIWindow::toggle_all_gdb));
	get_widget<Gtk::Button>("target-send-select-toggle-button")->signal_clicked().connect(sigc::mem_fun(*this, &UIWindow::toggle_all_trgt));
	m_root_window->signal_key_press_event().connect(sigc::mem_fun(*this, &UIWindow::on_key_press), false);
	get_widget<Gtk::Button>("step-over-button")->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &UIWindow::on_interaction_button_clicked), GDK_KEY_F5));
	get_widget<Gtk::Button>("step-in-button")->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &UIWindow::on_interaction_button_clicked), GDK_KEY_F6));
	get_widget<Gtk::Button>("step-out-button")->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &UIWindow::on_interaction_button_clicked), GDK_KEY_F7));
	get_widget<Gtk::Button>("continue-button")->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &UIWindow::on_interaction_button_clicked), GDK_KEY_F8));
	get_widget<Gtk::Button>("stop-button")->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &UIWindow::on_interaction_button_clicked), GDK_KEY_F9));
	get_widget<Gtk::Button>("restart-button")->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &UIWindow::on_interaction_button_clicked), GDK_KEY_F12));
	get_widget<Gtk::Button>("open-file-button")->signal_clicked().connect(sigc::mem_fun(*this, &UIWindow::open_file));
	get_widget<Gtk::Button>("close-unused-button")->signal_clicked().connect(sigc::mem_fun(*this, &UIWindow::close_unused_tabs));
	get_widget<Gtk::MenuItem>("quit-menu-item")->signal_activate().connect(sigc::mem_fun(*this, &UIWindow::on_quit_clicked));

	m_drawing_area = Gtk::manage(new UIDrawingArea(m_num_processes, this));
	m_drawing_area->set_size_request((2 * UIDrawingArea::radius() + UIDrawingArea::spacing()) * m_num_processes, -1);
	get_widget<Gtk::Box>("files-canvas-box")->pack_start(*m_drawing_area);

	m_files_notebook = get_widget<Gtk::Notebook>("files-notebook");
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &UIWindow::update_markers_timeout), 10);

	Gtk::Notebook *notebook_gdb = get_widget<Gtk::Notebook>("gdb-output-notebook");
	Gtk::Grid *grid_gdb = get_widget<Gtk::Grid>("gdb-send-select-grid");
	init_notebook(notebook_gdb, m_scrolled_windows_gdb, m_text_buffers_gdb);
	init_grid(grid_gdb);

	Gtk::Notebook *notebook_trgt = get_widget<Gtk::Notebook>("target-output-notebook");
	Gtk::Grid *grid_trgt = get_widget<Gtk::Grid>("target-send-select-grid");
	init_notebook(notebook_trgt, m_scrolled_windows_trgt, m_text_buffers_trgt);
	init_grid(grid_trgt);

	m_root_window->maximize();
	m_root_window->show_all();
	return true;
}

void UIWindow::on_quit_clicked()
{
	m_app->quit();
}

bool UIWindow::on_delete(GdkEventAny *)
{
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		if (!m_conns_open_gdb[rank])
		{
			continue;
		}
		string cmd = "\4"; // EOF: ^D
		asio::write(*m_conns_gdb[rank], asio::buffer(cmd, cmd.length()));
	}
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		while (m_conns_open_gdb[rank])
		{
			usleep(100);
		}
	}
	return false;
}

void UIWindow::scroll_bottom(Gtk::Allocation &, Gtk::ScrolledWindow *const scrolled_window, const bool is_gdb, const int rank)
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

bool UIWindow::send_data(tcp::socket *const socket, const string &data)
{
	std::size_t bytes_sent = asio::write(*socket, asio::buffer(data, data.length()));
	return bytes_sent == data.length();
}

void UIWindow::send_data_to_active(tcp::socket *const *const socket, const string &cmd)
{
	Gtk::Grid *grid = get_widget<Gtk::Grid>("gdb-send-select-grid");
	bool one_selected = false;
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::CheckButton *check_button = dynamic_cast<Gtk::CheckButton *>(grid->get_child_at(rank % max_buttons_per_row, rank / max_buttons_per_row));
		if (!check_button->get_active())
		{
			continue;
		}
		one_selected = true;
		if (socket == m_conns_trgt || m_target_state[rank] != TargetState::RUNNING)
		{
			send_data(socket[rank], cmd);
		}
	}
	if (!one_selected)
	{
		Gtk::MessageDialog dialog(*m_root_window, "No process selected.", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
		dialog.run();
	}
}

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

void UIWindow::on_interaction_button_clicked(const int key_value)
{
	interact_with_gdb(key_value);
}

bool UIWindow::on_key_press(GdkEventKey *event)
{
	interact_with_gdb(event->keyval);
	return false;
}

void UIWindow::send_input(const string &entry_name, const string &grid_name, tcp::socket *const *const socket)
{
	Gtk::Entry *entry = get_widget<Gtk::Entry>(entry_name);
	string cmd = string(entry->get_text()) + string("\n");
	Gtk::Grid *grid = get_widget<Gtk::Grid>(grid_name);
	bool one_selected = false;
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::CheckButton *check_button = dynamic_cast<Gtk::CheckButton *>(grid->get_child_at(rank % max_buttons_per_row, rank / max_buttons_per_row));
		if (check_button->get_active())
		{
			one_selected = true;
			asio::write(*socket[rank], asio::buffer(cmd, cmd.length()));
		}
	}
	if (one_selected)
	{
		entry->set_text("");
	}
	else
	{
		Gtk::MessageDialog dialog(*m_root_window, "No process selected.\nNoting has been sent.", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
		dialog.run();
	}
}

void UIWindow::send_input_gdb()
{
	send_input("gdb-send-entry", "gdb-send-select-grid", m_conns_gdb);
}

void UIWindow::send_input_trgt()
{
	send_input("target-send-entry", "target-send-select-grid", m_conns_trgt);
}

void UIWindow::toggle_all(const string &grid_name)
{
	Gtk::Grid *grid = get_widget<Gtk::Grid>(grid_name);
	bool one_checked = false;
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::CheckButton *check_button = dynamic_cast<Gtk::CheckButton *>(grid->get_child_at(rank % max_buttons_per_row, rank / max_buttons_per_row));
		if (check_button->get_active())
		{
			one_checked = true;
			break;
		}
	}
	bool state = !one_checked;
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::CheckButton *check_button = dynamic_cast<Gtk::CheckButton *>(grid->get_child_at(rank % max_buttons_per_row, rank / max_buttons_per_row));
		check_button->set_active(state);
	}
}

void UIWindow::toggle_all_gdb()
{
	toggle_all("gdb-send-select-grid");
}

void UIWindow::toggle_all_trgt()
{
	toggle_all("target-send-select-grid");
}

void UIWindow::clear_mark(Glib::RefPtr<Gtk::TextMark> &mark, Glib::RefPtr<Gsv::Buffer> &source_buffer, Breakpoint *breakpoint)
{
	std::set<int> *set = (std::set<int> *)source_buffer->get_data(marks_id);
	set->erase(breakpoint->get_line_number() - 1);
	delete breakpoint;
	mark->remove_data(line_number_id);
	source_buffer->delete_mark(mark);
}

void UIWindow::create_mark(Gtk::TextIter &iter, Glib::RefPtr<Gsv::Buffer> &source_buffer, const string &fullpath)
{
	const int line = iter.get_line();
	Breakpoint *breakpoint = new Breakpoint{m_num_processes, line + 1, fullpath, this};
	std::unique_ptr<BreakpointDialog> dialog = std::make_unique<BreakpointDialog>(m_num_processes, max_buttons_per_row, breakpoint, true);
	if (Gtk::RESPONSE_OK != dialog->run())
	{
		return;
	}
	breakpoint->update_breakpoints(dialog->get_button_states());
	if (breakpoint->one_created())
	{
		Glib::RefPtr<Gsv::Mark> new_mark = source_buffer->create_source_mark(std::to_string(line), breakpoint_category, iter);
		new_mark->set_data(line_number_id, (void *)breakpoint);
		std::set<int> *set = new std::set<int>;
		set->insert(line);
		source_buffer->set_data(marks_id, (void *)set);
	}
	else
	{
		delete breakpoint;
	}
}

void UIWindow::edit_mark(Glib::RefPtr<Gtk::TextMark> &mark, Glib::RefPtr<Gsv::Buffer> &source_buffer)
{
	Breakpoint *breakpoint = (Breakpoint *)mark->get_data(line_number_id);
	std::unique_ptr<BreakpointDialog> dialog = std::make_unique<BreakpointDialog>(m_num_processes, max_buttons_per_row, breakpoint, false);
	if (Gtk::RESPONSE_OK != dialog->run())
	{
		return;
	}
	breakpoint->update_breakpoints(dialog->get_button_states());
	if (!breakpoint->one_created())
	{
		clear_mark(mark, source_buffer, breakpoint);
	}
}

void UIWindow::delete_mark(Glib::RefPtr<Gtk::TextMark> &mark, Glib::RefPtr<Gsv::Buffer> &source_buffer)
{
	Breakpoint *breakpoint = (Breakpoint *)mark->get_data(line_number_id);
	if (breakpoint->delete_breakpoints())
	{
		clear_mark(mark, source_buffer, breakpoint);
	}
}

void UIWindow::on_line_mark_clicked(Gtk::TextIter &iter, GdkEvent *const event, const string &fullpath)
{
	const int line = iter.get_line();
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
	Gtk::ScrolledWindow *scrolled_window = dynamic_cast<Gtk::ScrolledWindow *>(m_files_notebook->get_nth_page(page_num));
	Gsv::View *source_view = dynamic_cast<Gsv::View *>(scrolled_window->get_child());
	Glib::RefPtr<Gsv::Buffer> source_buffer = source_view->get_source_buffer();

	if (event->type == GDK_BUTTON_PRESS && event->button.button == GDK_BUTTON_SECONDARY)
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

void UIWindow::append_source_file(const string &fullpath)
{
	if (m_opened_files.find(fullpath) == m_opened_files.end())
	{
		m_opened_files.insert(fullpath);
		string basename = Glib::path_get_basename(fullpath);
		Gtk::Label *label = Gtk::manage(new Gtk::Label(basename));
		label->set_tooltip_text(fullpath);
		Gtk::ScrolledWindow *scrolled_window = Gtk::manage(new Gtk::ScrolledWindow());
		Gsv::View *source_view = Gtk::manage(new Gsv::View());
		Glib::RefPtr<Gsv::Buffer> source_buffer = source_view->get_source_buffer();
		source_view->set_source_buffer(source_buffer);
		source_view->set_editable(false);
		scrolled_window->add(*source_view);
		char *content;
		size_t content_length;
		if (g_file_get_contents(fullpath.c_str(), &content, &content_length, nullptr))
		{
			source_view->set_show_line_numbers(true);
			source_view->set_show_line_marks(true);
			source_buffer->set_language(Gsv::LanguageManager::get_default()->get_language("cpp"));
			source_buffer->set_highlight_matching_brackets(true);
			source_buffer->set_highlight_syntax(true);
			source_buffer->set_text(content);
			source_view->signal_line_mark_activated().connect(sigc::bind(sigc::mem_fun(*this, &UIWindow::on_line_mark_clicked), fullpath));
			string filename = "./res/breakpoint.png";
			char *path = realpath(filename.c_str(), nullptr);
			if (path == nullptr)
			{
				std::cerr << "Cannot find file: " << filename << "\n";
				return;
			}
			Glib::RefPtr<Gsv::MarkAttributes> attributes = Gsv::MarkAttributes::create();
			attributes->set_icon(Gio::Icon::create(path));
			source_view->set_mark_attributes(breakpoint_category, attributes, 0);
			free(path);
		}
		else
		{
			source_buffer->set_text(string("Could not load file: ") + fullpath);
		}
		scrolled_window->show_all();
		int page_num = m_files_notebook->append_page(*scrolled_window, *label);
		m_files_notebook->set_current_page(page_num);
		m_path_2_pagenum[fullpath] = page_num;
		m_pagenum_2_path[page_num] = fullpath;
		m_path_2_view[fullpath] = source_view;
	}
	else
	{
		m_files_notebook->set_current_page(m_path_2_pagenum[fullpath]);
	}
}

void UIWindow::open_file()
{
	string fullpath = "";
	if (m_opened_files.find(fullpath) == m_opened_files.end())
	{
		std::unique_ptr<Gtk::FileChooserDialog> dialog = std::make_unique<Gtk::FileChooserDialog>(*m_root_window, "Open Source File");
		dialog->add_button("Cancel", Gtk::RESPONSE_CANCEL);
		dialog->add_button("Open", Gtk::RESPONSE_OK);
		int res = dialog->run();
		if (res == Gtk::RESPONSE_OK)
		{
			string fullpath = dialog->get_filename();
			append_source_file(fullpath);
		}
		dialog.reset();
	}
	else
	{
		m_files_notebook->set_current_page(m_path_2_pagenum[fullpath]);
	}
}

void UIWindow::close_unused_tabs()
{
	int num_pages;
	std::set<int> pages_to_close;

	num_pages = m_files_notebook->get_n_pages();
	for (int page_num = 0; page_num < num_pages; ++page_num)
	{
		bool is_used = false;
		string fullpath = m_pagenum_2_path[page_num];
		for (int rank = 0; rank < m_num_processes; ++rank)
		{
			if (m_source_view_path[rank] == fullpath && target_state(rank) != TargetState::EXITED)
			{
				is_used = true;
				break;
			}
		}
		if (!is_used)
		{
			Gtk::Widget *page = m_files_notebook->get_nth_page(page_num);
			Gtk::ScrolledWindow *scrolled_window = dynamic_cast<Gtk::ScrolledWindow *>(page);
			Gsv::View *source_view = dynamic_cast<Gsv::View *>(scrolled_window->get_child());
			Glib::RefPtr<Gsv::Buffer> source_buffer = source_view->get_source_buffer();
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

	// reversed order to keep indexeis valid
	std::set<int>::reverse_iterator rit;
	for (rit = pages_to_close.rbegin(); rit != pages_to_close.rend(); ++rit)
	{
		int page_num = *rit;
		m_files_notebook->remove_page(page_num);
	}

	m_path_2_pagenum.clear();
	m_pagenum_2_path.clear();
	m_path_2_view.clear();
	m_opened_files.clear();

	num_pages = m_files_notebook->get_n_pages();
	for (int page_num = 0; page_num < num_pages; ++page_num)
	{
		Gtk::Widget *page = m_files_notebook->get_nth_page(page_num);
		Gtk::Label *label = dynamic_cast<Gtk::Label *>(m_files_notebook->get_tab_label(*page));
		Gtk::ScrolledWindow *scrolled_window = dynamic_cast<Gtk::ScrolledWindow *>(page);
		Gsv::View *source_view = dynamic_cast<Gsv::View *>(scrolled_window->get_child());
		string fullpath = label->get_tooltip_text();

		m_path_2_pagenum[fullpath] = page_num;
		m_pagenum_2_path[page_num] = fullpath;
		m_path_2_view[fullpath] = source_view;
		m_opened_files.insert(fullpath);
	}
}

void UIWindow::update_markers(const int page_num)
{
	Gtk::ScrolledWindow *scrolled_window = dynamic_cast<Gtk::ScrolledWindow *>(m_files_notebook->get_nth_page(page_num));
	Gsv::View *source_view = dynamic_cast<Gsv::View *>(scrolled_window->get_child());
	Glib::RefPtr<Gsv::Buffer> source_buffer = source_view->get_source_buffer();
	Glib::RefPtr<Gtk::Adjustment> adjustment = scrolled_window->get_vadjustment();
	string page_path = m_pagenum_2_path[page_num];
	int offset = m_files_notebook->get_height() - scrolled_window->get_height();
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::TextIter line_iter = source_buffer->get_iter_at_line(m_current_line[rank] - 1);
		if (!line_iter || page_path != m_source_view_path[rank])
		{
			m_drawing_area->set_y_offset(rank, -3 * UIDrawingArea::radius()); // set out of visible area
			continue;
		}
		Gdk::Rectangle rect;
		source_view->get_iter_location(line_iter, rect);
		int draw_pos = offset + rect.get_y() - int(adjustment->get_value());
		m_drawing_area->set_y_offset(rank, draw_pos);
	}
	m_drawing_area->queue_draw();
}

bool UIWindow::update_markers_timeout()
{
	int page_num = m_files_notebook->get_current_page();
	if (page_num >= 0)
	{
		update_markers(page_num);
	}
	return true;
}

void UIWindow::do_scroll(const int rank) const
{
	Gsv::View *source_view = m_path_2_view.at(m_source_view_path[rank]);
	Gtk::TextIter iter = source_view->get_buffer()->get_iter_at_line(m_current_line[rank] - 1);
	if (!iter)
	{
		return;
	}
	source_view->scroll_to(iter, 0.1);
}

void UIWindow::scroll_to_line(const int rank) const
{
	Glib::signal_idle().connect_once(
		sigc::bind(
			sigc::mem_fun(
				this,
				&UIWindow::do_scroll),
			rank));
}

void UIWindow::set_position(const int rank, const string &fullpath, const int line)
{
	m_source_view_path[rank] = fullpath;
	m_current_line[rank] = line;
}

void UIWindow::print_data_gdb(mi_h *const gdb_handle, const char *const data, const int rank)
{
	Gtk::TextBuffer *buffer = m_text_buffers_gdb[rank];

	char *token = strtok((char *)data, "\n");
	while (NULL != token)
	{
		int token_length = strlen(token);
		// token[token_length]: '\n', replaced with '\0' by strtok
		if (token[token_length - 1] == '\r')
		{
			token[token_length - 1] = '\0';
		}
		free(gdb_handle->line);
		gdb_handle->line = strdup(token);

		int response = mi_get_response(gdb_handle);
		if (0 != response)
		{
			if (!m_started[rank])
			{
				m_started[rank] = true;
			}

			mi_output *first_output = mi_retire_response(gdb_handle);
			mi_output *current_output = first_output;

			while (NULL != current_output)
			{
				if (MI_CL_RUNNING == current_output->tclass)
				{
					m_target_state[rank] = TargetState::RUNNING;
				}
				else if (MI_CL_STOPPED == current_output->tclass)
				{
					m_target_state[rank] = TargetState::STOPPED;
				}
				if (current_output->type == MI_T_OUT_OF_BAND && current_output->stype == MI_ST_STREAM)
				{
					char *text = get_cstr(current_output);
					buffer->insert(buffer->end(), text);
				}
				current_output = current_output->next;
			}

			if (nullptr != m_breakpoints[rank])
			{
				mi_bkpt *breakpoint = mi_res_bkpt(first_output);
				if (breakpoint)
				{
					m_breakpoints[rank]->set_number(breakpoint->number);
					m_breakpoints[rank] = nullptr;
				}
				mi_free_bkpt(breakpoint);
			}

			mi_stop *stop_record = mi_res_stop(first_output);
			if (stop_record)
			{
				if (stop_record->frame && stop_record->frame->fullname && stop_record->frame->func)
				{
					const string fullpath = stop_record->frame->fullname;
					const int line = stop_record->frame->line;
					set_position(rank, fullpath, line);
					append_source_file(fullpath);
					scroll_to_line(rank);
				}
				if (mi_stop_reason::sr_exited_signalled == stop_record->reason ||
					mi_stop_reason::sr_exited == stop_record->reason ||
					mi_stop_reason::sr_exited_normally == stop_record->reason)
				{
					m_target_state[rank] = TargetState::EXITED;
				}
				mi_free_stop(stop_record);
			}

			mi_free_output(first_output);
		}
		token = strtok(NULL, "\n");
	}
}

void UIWindow::print_data_trgt(const char *const data, const int rank)
{
	Gtk::TextBuffer *buffer = m_text_buffers_trgt[rank];
	buffer->insert(buffer->end(), data);
}

void UIWindow::print_data(mi_h *const gdb_handle, const char *const data, const int port)
{
	const int rank = get_rank(port);
	const bool is_gdb = src_is_gdb(port);

	m_mutex_gui.lock();

	if (is_gdb)
	{
		print_data_gdb(gdb_handle, data, rank);
		Gtk::ScrolledWindow *scrolled_window = m_scrolled_windows_gdb[rank];
		if (m_scroll_connections_gdb[rank].empty())
		{
			m_scroll_connections_gdb[rank] = scrolled_window->signal_size_allocate().connect(
				sigc::bind(
					sigc::mem_fun(
						*this,
						&UIWindow::scroll_bottom),
					scrolled_window,
					is_gdb,
					rank));
		}
	}
	else
	{
		print_data_trgt(data, rank);
		Gtk::ScrolledWindow *scrolled_window = m_scrolled_windows_trgt[rank];
		if (m_scroll_connections_trgt[rank].empty())
		{
			m_scroll_connections_trgt[rank] = scrolled_window->signal_size_allocate().connect(
				sigc::bind(
					sigc::mem_fun(
						*this,
						&UIWindow::scroll_bottom),
					scrolled_window,
					is_gdb,
					rank));
		}
	}

	if (!m_sent_run)
	{
		bool all_started = true;
		for (int rank = 0; rank < m_num_processes; ++rank)
		{
			all_started &= m_started[rank];
		}
		if (all_started)
		{
			send_data_to_active(m_conns_gdb, "run\n");
			m_sent_run = true;
		}
	}

	free((void *)data);

	m_mutex_gui.unlock();
}