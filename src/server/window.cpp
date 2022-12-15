#include <string>

#include "window.hpp"
#include "breakpoint.hpp"
#include "breakpoint_dialog.hpp"

const char *const breakpoint_category = "breakpoint-category";
const char *const data_id = "line-number";

UIWindow::UIWindow(const int num_processes)
	: m_num_processes(num_processes)
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
	m_conns_open_gdb = new bool[m_num_processes];
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		m_conns_gdb[rank] = nullptr;
		m_conns_trgt[rank] = nullptr;
		m_conns_open_gdb[rank] = false;
		m_current_line[rank] = 0;
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
	delete[] m_conns_open_gdb;
}

template <class T>
T *UIWindow::get_widget(const string &widget_name)
{
	T *widget;
	m_builder->get_widget<T>(widget_name, widget);
	return widget;
}

bool UIWindow::init()
{
	Gsv::init();

	m_builder = Gtk::Builder::create_from_file("./ui/window.glade");
	m_root_window = get_widget<Gtk::Window>("window");

	// Signal are not compatible with glade and gtkmm ...
	// So connect them the old-fashioned way.
	get_widget<Gtk::Button>("gdb-send-button")->signal_clicked().connect(sigc::mem_fun(*this, &UIWindow::send_input_gdb));
	get_widget<Gtk::Entry>("gdb-send-entry")->signal_activate().connect(sigc::mem_fun(*this, &UIWindow::send_input_gdb));
	get_widget<Gtk::Button>("target-send-button")->signal_clicked().connect(sigc::mem_fun(*this, &UIWindow::send_input_trgt));
	get_widget<Gtk::Entry>("target-send-entry")->signal_activate().connect(sigc::mem_fun(*this, &UIWindow::send_input_trgt));
	get_widget<Gtk::Button>("target-send-sigint-button")->signal_clicked().connect(sigc::mem_fun(*this, &UIWindow::send_sig_int));
	get_widget<Gtk::Button>("gdb-send-select-all-button")->signal_clicked().connect(sigc::mem_fun(*this, &UIWindow::toggle_all_gdb));
	get_widget<Gtk::Button>("target-send-select-all-button")->signal_clicked().connect(sigc::mem_fun(*this, &UIWindow::toggle_all_trgt));

	m_drawing_area = Gtk::manage(new UIDrawingArea(m_num_processes));
	m_drawing_area->set_size_request(UIDrawingArea::spacing() + (2 * UIDrawingArea::radius() + UIDrawingArea::spacing()) * m_num_processes, -1);
	get_widget<Gtk::Box>("files-canvas-box")->pack_start(*m_drawing_area);

	m_files_notebook = get_widget<Gtk::Notebook>("files-notebook");
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &UIWindow::update_markers_timeout), 10);

	Gtk::Notebook *notebook_gdb = get_widget<Gtk::Notebook>("gdb-output-notebook");
	Gtk::Notebook *notebook_trgt = get_widget<Gtk::Notebook>("target-output-notebook");
	Gtk::Box *send_select_gdb = get_widget<Gtk::Box>("gdb-send-select-wrapper-box");
	Gtk::Box *send_select_trgt = get_widget<Gtk::Box>("target-send-select-wrapper-box");
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		// gdb output notebook
		{
			Gtk::Label *tab = Gtk::manage(new Gtk::Label(std::to_string(rank)));
			Gtk::ScrolledWindow *scrolled_window = Gtk::manage(new Gtk::ScrolledWindow());
			m_scrolled_windows_gdb[rank] = scrolled_window;
			Gtk::TextView *text_view = Gtk::manage(new Gtk::TextView());
			text_view->set_editable(false);
			m_text_buffers_gdb[rank] = text_view->get_buffer().get();
			scrolled_window->add(*text_view);
			scrolled_window->set_size_request(-1, 200);
			notebook_gdb->append_page(*scrolled_window, *tab);
		}

		// target output notebook
		{
			Gtk::Label *tab = Gtk::manage(new Gtk::Label(std::to_string(rank)));
			Gtk::ScrolledWindow *scrolled_window = Gtk::manage(new Gtk::ScrolledWindow());
			m_scrolled_windows_trgt[rank] = scrolled_window;
			Gtk::TextView *text_view = Gtk::manage(new Gtk::TextView());
			text_view->set_editable(false);
			m_text_buffers_trgt[rank] = text_view->get_buffer().get();
			scrolled_window->add(*text_view);
			scrolled_window->set_size_request(-1, 200);
			notebook_trgt->append_page(*scrolled_window, *tab);
		}

		// gdb send buttons
		Gtk::CheckButton *check_button_gdb = Gtk::manage(new Gtk::CheckButton(std::to_string(rank)));
		check_button_gdb->set_active(true);
		send_select_gdb->pack_start(*check_button_gdb);

		// target send buttons
		Gtk::CheckButton *check_button_trgt = Gtk::manage(new Gtk::CheckButton(std::to_string(rank)));
		check_button_trgt->set_active(true);
		send_select_trgt->pack_start(*check_button_trgt);
	}

	get_widget<Gtk::Entry>("gdb-send-entry")->set_text("run");

	m_root_window->show_all();
	return true;
}

bool UIWindow::on_delete(GdkEventAny *)
{
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		if (!m_conns_open_gdb[rank])
		{
			continue;
		}
		string cmd = "\4";
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

void UIWindow::scroll_bottom(Gtk::Allocation &, Gtk::ScrolledWindow *const scrolled_window, const bool is_gdb, const int process_rank)
{
	Glib::RefPtr<Gtk::Adjustment> adjustment = scrolled_window->get_vadjustment();
	adjustment->set_value(adjustment->get_upper());
	if (is_gdb)
	{
		m_scroll_connections_gdb[process_rank].disconnect();
	}
	else
	{
		m_scroll_connections_trgt[process_rank].disconnect();
	}
}

void UIWindow::send_sig_int()
{
	string cmd = "\3"; // ^C
	Gtk::Box *box = get_widget<Gtk::Box>("target-send-select-wrapper-box");
	std::vector<Gtk::Widget *> check_buttons = box->get_children();
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::CheckButton *check_button = dynamic_cast<Gtk::CheckButton *>(check_buttons.at(rank));
		if (!check_button->get_active())
			continue;
		asio::write(*m_conns_trgt[rank], asio::buffer(cmd, cmd.length()));
	}
}

void UIWindow::send_input(const string &entry_name, const string &wrapper_name, tcp::socket *const *const socket)
{
	Gtk::Entry *entry = get_widget<Gtk::Entry>(entry_name);
	string cmd = string(entry->get_text()) + string("\n");
	Gtk::Box *box = get_widget<Gtk::Box>(wrapper_name);
	std::vector<Gtk::Widget *> check_buttons = box->get_children();
	bool one_selected = false;
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		Gtk::CheckButton *check_button = dynamic_cast<Gtk::CheckButton *>(check_buttons.at(rank));
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
		Gtk::MessageDialog dialog(*m_root_window, "No Process selected.\nNoting has been sent.", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
		dialog.run();
	}
}

void UIWindow::send_input_gdb()
{
	send_input("gdb-send-entry", "gdb-send-select-wrapper-box", m_conns_gdb);
}

void UIWindow::send_input_trgt()
{
	send_input("target-send-entry", "target-send-select-wrapper-box", m_conns_trgt);
}

void UIWindow::toggle_all(const string &box_name)
{
	Gtk::Box *box = get_widget<Gtk::Box>(box_name);
	bool one_checked = false;
	for (Gtk::Widget *check_button_widget : box->get_children())
	{
		Gtk::CheckButton *check_button = dynamic_cast<Gtk::CheckButton *>(check_button_widget);
		if (check_button->get_active())
		{
			one_checked = true;
			break;
		}
	}
	bool state = !one_checked;
	for (Gtk::Widget *check_button_widget : box->get_children())
	{
		Gtk::CheckButton *check_button = dynamic_cast<Gtk::CheckButton *>(check_button_widget);
		check_button->set_active(state);
	}
}

void UIWindow::toggle_all_gdb()
{
	toggle_all("gdb-send-select-wrapper-box");
}

void UIWindow::toggle_all_trgt()
{
	toggle_all("target-send-select-wrapper-box");
}

void UIWindow::create_mark(Gtk::TextIter &iter, Glib::RefPtr<Gsv::Buffer> &source_buffer, const string &fullpath)
{
	const int line = iter.get_line();
	Breakpoint *breakpoint = new Breakpoint{m_num_processes, line + 1, fullpath, this};
	std::unique_ptr<BreakpointDialog> dialog = std::make_unique<BreakpointDialog>(m_num_processes, breakpoint);
	if (Gtk::RESPONSE_OK != dialog->run())
	{
		return;
	}
	breakpoint->update_breakpoints(dialog->get_button_states());
	if (breakpoint->one_created())
	{
		Glib::RefPtr<Gsv::Mark> new_mark = source_buffer->create_source_mark(std::to_string(line), breakpoint_category, iter);
		new_mark->set_data(data_id, (void *)breakpoint);
	}
	else
	{
		delete breakpoint;
	}
}

void UIWindow::edit_mark(Glib::RefPtr<Gtk::TextMark> &mark, Glib::RefPtr<Gsv::Buffer> &source_buffer)
{
	Breakpoint *breakpoint = (Breakpoint *)mark->get_data(data_id);
	std::unique_ptr<BreakpointDialog> dialog = std::make_unique<BreakpointDialog>(m_num_processes, breakpoint);
	if (Gtk::RESPONSE_OK != dialog->run())
	{
		return;
	}
	breakpoint->update_breakpoints(dialog->get_button_states());
	if (!breakpoint->one_created())
	{
		delete breakpoint;
		mark->remove_data(data_id);
		source_buffer->delete_mark(mark);
	}
}

void UIWindow::delete_mark(Glib::RefPtr<Gtk::TextMark> &mark, Glib::RefPtr<Gsv::Buffer> &source_buffer)
{
	Breakpoint *breakpoint = (Breakpoint *)mark->get_data(data_id);
	if (breakpoint->delete_breakpoints())
	{
		delete breakpoint;
		mark->remove_data(data_id);
		source_buffer->delete_mark(mark);
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

void UIWindow::append_source_file(const int process_rank, const string &fullpath, const int line)
{
	m_source_view_path[process_rank] = fullpath;
	m_current_line[process_rank] = line;
	if (m_opened_files.find(fullpath) == m_opened_files.end())
	{
		m_opened_files.insert(fullpath);
		string basename = Glib::path_get_basename(fullpath);
		Gtk::Label *label = Gtk::manage(new Gtk::Label(basename));
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

void UIWindow::do_scroll(const int process_rank) const
{
	Gsv::View *source_view = m_path_2_view.at(m_source_view_path[process_rank]);
	Gtk::TextIter iter = source_view->get_buffer()->get_iter_at_line(m_current_line[process_rank] - 1);
	if (!iter)
	{
		return;
	}
	source_view->scroll_to(iter, 0.1);
}

void UIWindow::scroll_to_line(const int process_rank) const
{
	Glib::signal_idle().connect_once(
		sigc::bind(
			sigc::mem_fun(
				this,
				&UIWindow::do_scroll),
			process_rank));
}

// todo refactor...
void UIWindow::print_data_gdb(mi_h *const gdb_handle, const char *const data, const int process_rank)
{
	Gtk::TextBuffer *buffer = m_text_buffers_gdb[process_rank];

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
			mi_output *o = mi_retire_response(gdb_handle);
			mi_output *output = o;

			while (NULL != output)
			{
				if (MI_CL_RUNNING == output->tclass)
				{
					m_target_state[process_rank] = RUNNING;
				}
				else if (MI_CL_STOPPED == output->tclass)
				{
					m_target_state[process_rank] = STOPPED;
				}
				if (output->type == MI_T_OUT_OF_BAND && output->stype == MI_ST_STREAM /* && output->sstype == MI_SST_CONSOLE */)
				{
					char *text = get_cstr(output);
					buffer->insert(buffer->end(), text);
				}
				else
				{
					// printf("%d,%d,%d,%d\n", output->type, output->stype, output->sstype, output->tclass);
				}
				output = output->next;
			}

			mi_stop *stop_record;
			stop_record = mi_res_stop(o);
			if (stop_record)
			{
				// printf("Rank: %d stopped, reason: %s\n", process_rank, mi_reason_enum_to_str(stop_record->reason));
				if (stop_record->frame && stop_record->frame->fullname && stop_record->frame->func)
				{
					// printf("\tat %s:%d in function: %s\n", stop_record->frame->fullname, stop_record->frame->line, stop_record->frame->func);
					append_source_file(process_rank, stop_record->frame->fullname, stop_record->frame->line);
					scroll_to_line(process_rank);
				}
				mi_free_stop(stop_record);
			}
			mi_free_output(o);
		}
		token = strtok(NULL, "\n");
	}
}

void UIWindow::print_data_trgt(const char *const data, const int process_rank)
{
	Gtk::TextBuffer *buffer = m_text_buffers_trgt[process_rank];
	buffer->insert(buffer->end(), data);
}

void UIWindow::print_data(mi_h *const gdb_handle, const char *const data, const int port)
{
	const int process_rank = get_process_rank(port);
	const bool is_gdb = src_is_gdb(port);

	m_mutex_gui.lock();

	if (is_gdb)
	{
		print_data_gdb(gdb_handle, data, process_rank);
		Gtk::ScrolledWindow *scrolled_window = m_scrolled_windows_gdb[process_rank];
		if (m_scroll_connections_gdb[process_rank].empty())
		{
			m_scroll_connections_gdb[process_rank] = scrolled_window->signal_size_allocate().connect(
				sigc::bind(
					sigc::mem_fun(
						*this,
						&UIWindow::scroll_bottom),
					scrolled_window,
					is_gdb,
					process_rank));
		}
	}
	else
	{
		print_data_trgt(data, process_rank);
		Gtk::ScrolledWindow *scrolled_window = m_scrolled_windows_trgt[process_rank];
		if (m_scroll_connections_trgt[process_rank].empty())
		{
			m_scroll_connections_trgt[process_rank] = scrolled_window->signal_size_allocate().connect(
				sigc::bind(
					sigc::mem_fun(
						*this,
						&UIWindow::scroll_bottom),
					scrolled_window,
					is_gdb,
					process_rank));
		}
	}

	free((void *)data);

	m_mutex_gui.unlock();
}