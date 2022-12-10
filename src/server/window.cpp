#include <string>

#include "window.hpp"

UIWindow::UIWindow(const int a_num_processes)
	: m_num_processes(a_num_processes)
{
	m_current_line = new int[m_num_processes];
	m_source_view_path = new string[m_num_processes];
	m_conns_gdb = new tcp::socket *[m_num_processes];
	m_conns_trgt = new tcp::socket *[m_num_processes];
	m_text_buffers_gdb = new Gtk::TextBuffer *[m_num_processes];
	m_text_buffers_trgt = new Gtk::TextBuffer *[m_num_processes];
	m_scrolled_windows_gdb = new Gtk::ScrolledWindow *[m_num_processes];
	m_scrolled_windows_trgt = new Gtk::ScrolledWindow *[m_num_processes];
	m_scroll_connections_gdb = new sigc::connection[m_num_processes];
	m_scroll_connections_trgt = new sigc::connection[m_num_processes];
	m_conns_open_gdb = new bool[m_num_processes];
	m_where_marks = new char *[m_num_processes];
	m_where_categories = new char *[m_num_processes];
	for (int i = 0; i < m_num_processes; ++i)
	{
		m_conns_gdb[i] = nullptr;
		m_conns_trgt[i] = nullptr;
		m_conns_open_gdb[i] = false;
		string mark_name = "mark-" + std::to_string(i);
		string cat_name = "cat-" + std::to_string(i);
		m_where_marks[i] = strdup(mark_name.c_str());
		m_where_categories[i] = strdup(cat_name.c_str());
		m_current_line[i] = 0;
	}
}

UIWindow::~UIWindow()
{
	for (int i = 0; i < m_num_processes; ++i)
	{
		free(m_where_marks[i]);
		free(m_where_categories[i]);
	}
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
T *UIWindow::get_widget(const string &a_widget_name)
{
	T *widget;
	m_builder->get_widget<T>(a_widget_name, widget);
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
	get_widget<Gtk::Notebook>("files-notebook")->signal_switch_page().connect(sigc::mem_fun(*this, &UIWindow::on_page_switch));

	m_drawing_area = Gtk::manage(new UIDrawingArea(m_num_processes));
	m_drawing_area->set_size_request(UIDrawingArea::spacing() + (2 * UIDrawingArea::radius() + UIDrawingArea::spacing()) * m_num_processes, -1);
	get_widget<Gtk::Box>("files-canvas-box")->pack_start(*m_drawing_area);

	Gtk::Notebook *notebook_gdb = get_widget<Gtk::Notebook>("gdb-output-notebook");
	Gtk::Notebook *notebook_trgt = get_widget<Gtk::Notebook>("target-output-notebook");
	Gtk::Box *send_select_gdb = get_widget<Gtk::Box>("gdb-send-select-wrapper-box");
	Gtk::Box *send_select_trgt = get_widget<Gtk::Box>("target-send-select-wrapper-box");
	for (int i = 0; i < m_num_processes; ++i)
	{
		// gdb output notebook
		{
			Gtk::Label *tab = Gtk::manage(new Gtk::Label(std::to_string(i)));
			Gtk::ScrolledWindow *scrolled_window = Gtk::manage(new Gtk::ScrolledWindow());
			m_scrolled_windows_gdb[i] = scrolled_window;
			Gtk::TextView *text_view = Gtk::manage(new Gtk::TextView());
			text_view->set_editable(false);
			m_text_buffers_gdb[i] = text_view->get_buffer().get();
			scrolled_window->add(*text_view);
			scrolled_window->set_size_request(-1, 200);
			notebook_gdb->append_page(*scrolled_window, *tab);
		}

		// target output notebook
		{
			Gtk::Label *tab = Gtk::manage(new Gtk::Label(std::to_string(i)));
			Gtk::ScrolledWindow *scrolled_window = Gtk::manage(new Gtk::ScrolledWindow());
			m_scrolled_windows_trgt[i] = scrolled_window;
			Gtk::TextView *text_view = Gtk::manage(new Gtk::TextView());
			text_view->set_editable(false);
			m_text_buffers_trgt[i] = text_view->get_buffer().get();
			scrolled_window->add(*text_view);
			scrolled_window->set_size_request(-1, 200);
			notebook_trgt->append_page(*scrolled_window, *tab);
		}

		// gdb send buttons
		Gtk::CheckButton *check_button_gdb = Gtk::manage(new Gtk::CheckButton(std::to_string(i)));
		check_button_gdb->set_active(true);
		send_select_gdb->pack_start(*check_button_gdb);

		// target send buttons
		Gtk::CheckButton *check_button_trgt = Gtk::manage(new Gtk::CheckButton(std::to_string(i)));
		check_button_trgt->set_active(true);
		send_select_trgt->pack_start(*check_button_trgt);
	}

	m_root_window->show_all();
	return true;
}

bool UIWindow::on_delete(GdkEventAny *)
{
	for (int i = 0; i < m_num_processes; ++i)
	{
		if (!m_conns_open_gdb[i])
		{
			continue;
		}
		char eof = 4;
		string cmd = string(&eof);
		asio::write(*m_conns_gdb[i], asio::buffer(cmd, 1));
	}
	for (int i = 0; i < m_num_processes; ++i)
	{
		while (m_conns_open_gdb[i])
		{
			usleep(100);
		}
	}
	return false;
}

void UIWindow::scroll_bottom(Gtk::Allocation &, Gtk::ScrolledWindow *a_scrolled_window, const bool a_is_gdb, const int a_process_rank)
{
	auto adjustment = a_scrolled_window->get_vadjustment();
	adjustment->set_value(adjustment->get_upper());
	if (a_is_gdb)
	{
		m_scroll_connections_gdb[a_process_rank].disconnect();
	}
	else
	{
		m_scroll_connections_trgt[a_process_rank].disconnect();
	}
}

void UIWindow::send_sig_int()
{
	string cmd = "\3"; // ^C
	Gtk::Box *box = get_widget<Gtk::Box>("target-send-select-wrapper-box");
	std::vector<Gtk::Widget *> check_buttons = box->get_children();
	for (int i = 0; i < m_num_processes; ++i)
	{
		Gtk::CheckButton *check_button = dynamic_cast<Gtk::CheckButton *>(check_buttons.at(i));
		if (!check_button->get_active())
			continue;
		asio::write(*m_conns_trgt[i], asio::buffer(cmd, cmd.length()));
	}
}

void UIWindow::send_input(const string &a_entry_name, const string &a_wrapper_name, tcp::socket **a_socket)
{
	Gtk::Entry *entry = get_widget<Gtk::Entry>(a_entry_name);
	string cmd = string(entry->get_text()) + string("\n");
	Gtk::Box *box = get_widget<Gtk::Box>(a_wrapper_name);
	std::vector<Gtk::Widget *> check_buttons = box->get_children();
	bool one_selected = false;
	for (int i = 0; i < m_num_processes; ++i)
	{
		Gtk::CheckButton *check_button = dynamic_cast<Gtk::CheckButton *>(check_buttons.at(i));
		if (check_button->get_active())
		{
			one_selected = true;
			asio::write(*a_socket[i], asio::buffer(cmd, cmd.length()));
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

void UIWindow::toggle_all(const string &a_box_name)
{
	Gtk::Box *box = get_widget<Gtk::Box>(a_box_name);
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

void UIWindow::append_source_file(const int a_process_rank, const string &a_fullpath, const int a_line)
{
	m_source_view_path[a_process_rank] = a_fullpath;
	m_current_line[a_process_rank] = a_line;
	Gtk::Notebook *notebook = get_widget<Gtk::Notebook>("files-notebook");
	int page_num;
	if (m_opened_files.find(a_fullpath) == m_opened_files.end())
	{
		m_opened_files.insert(a_fullpath);
		string basename = Glib::path_get_basename(a_fullpath);
		Gtk::Label *label = Gtk::manage(new Gtk::Label(basename));
		Gtk::ScrolledWindow *scrolled_window = Gtk::manage(new Gtk::ScrolledWindow());
		Gsv::View *source_view = Gtk::manage(new Gsv::View());
		Glib::RefPtr<Gsv::Buffer> source_buffer = source_view->get_source_buffer();
		source_view->set_source_buffer(source_buffer);
		source_view->set_editable(false);
		scrolled_window->add(*source_view);
		char *content;
		size_t content_length;
		if (g_file_get_contents(a_fullpath.c_str(), &content, &content_length, nullptr))
		{
			scrolled_window->get_vadjustment()->signal_value_changed().connect(sigc::mem_fun(*this, &UIWindow::on_scroll_file));
			source_view->set_show_line_numbers(true);
			source_buffer->set_language(Gsv::LanguageManager::get_default()->get_language("cpp"));
			source_buffer->set_highlight_matching_brackets(true);
			source_buffer->set_highlight_syntax(true);
			source_buffer->set_text(content);
		}
		else
		{
			source_buffer->set_text(string("Could not load file: ") + a_fullpath);
		}
		scrolled_window->show_all();
		page_num = notebook->append_page(*scrolled_window, *label);
		notebook->set_current_page(page_num);
		m_path_2_pagenum[a_fullpath] = page_num;
		m_pagenum_2_path[page_num] = a_fullpath;
		m_path_2_view[a_fullpath] = source_view;
	}
	else
	{
		page_num = m_path_2_pagenum[a_fullpath];
		notebook->set_current_page(page_num);
	}
}

void UIWindow::update_markers(const int a_page_num)
{
	Gtk::Notebook *notebook = get_widget<Gtk::Notebook>("files-notebook");
	Gtk::ScrolledWindow *scrolled_window = dynamic_cast<Gtk::ScrolledWindow *>(notebook->get_nth_page(a_page_num));
	Gsv::View *source_view = dynamic_cast<Gsv::View *>(scrolled_window->get_child());
	string page_path = m_pagenum_2_path[a_page_num];
	int offset = notebook->get_height() - scrolled_window->get_height();

	for (int i = 0; i < m_num_processes; ++i)
	{
		Glib::RefPtr<Gsv::Buffer> source_buffer = source_view->get_source_buffer();
		Glib::RefPtr<Gtk::Adjustment> adjustment = scrolled_window->get_vadjustment();
		Gtk::TextIter line_iter = source_buffer->get_iter_at_line(m_current_line[i] - 1);
		if (!line_iter || page_path != m_source_view_path[i])
		{
			m_drawing_area->set_y_offset(i, -3 * UIDrawingArea::radius()); // set out of visible area
			continue;
		}

		Gdk::Rectangle rect;
		source_view->get_iter_location(line_iter, rect);

		int draw_pos = offset + rect.get_y() - int(adjustment->get_value());
		m_drawing_area->set_y_offset(i, draw_pos);
	}

	m_drawing_area->queue_draw();
}

void UIWindow::on_page_switch(Gtk::Widget *a_page, const int a_page_num)
{
	update_markers(a_page_num);
}

void UIWindow::on_scroll_file()
{
	int page_num = get_widget<Gtk::Notebook>("files-notebook")->get_current_page();
	update_markers(page_num);
}

void UIWindow::do_scroll(const int a_process_rank) const
{
	Gsv::View *source_view = m_path_2_view.at(m_source_view_path[a_process_rank]);
	Gtk::TextIter iter = source_view->get_buffer()->get_iter_at_line(m_current_line[a_process_rank] - 1);
	if (!iter)
	{
		std::cerr << "No iter. Line: " << m_current_line[a_process_rank] - 1 << "\n";
		return;
	}
	source_view->scroll_to(iter, 0.1);
}

void UIWindow::scroll_to_line(const int a_process_rank) const
{
	Glib::signal_idle().connect_once(
		sigc::bind(
			sigc::mem_fun(
				this,
				&UIWindow::do_scroll),
			a_process_rank));
}

// todo refactor...
void UIWindow::print_data_gdb(mi_h *const a_gdb_handle, const char *const a_data, const int a_process_rank)
{
	Gtk::TextBuffer *buffer = m_text_buffers_gdb[a_process_rank];

	char *token = strtok((char *)a_data, "\n");
	while (NULL != token)
	{
		int token_length = strlen(token);
		// token[l]: '\n' replaced with '\0' by strtok
		// token[l-1]: is '\r'
		// replace '\r' with '\0' -> new end for strcpy
		token[token_length - 1] = '\0';
		// '\0' now included in token_length, so no +1 needed
		a_gdb_handle->line = (char *)realloc(a_gdb_handle->line, token_length);
		strcpy(a_gdb_handle->line, token);

		int response = mi_get_response(a_gdb_handle);
		if (0 != response)
		{
			mi_output *o = mi_retire_response(a_gdb_handle);
			mi_output *output = o;

			while (NULL != output)
			{
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
				// printf("Stopped, reason: %s\n", mi_reason_enum_to_str(stop_record->reason));
				if (stop_record->frame && stop_record->frame->fullname && stop_record->frame->func)
				{
					// printf("\tat %s:%d in function: %s\n", stop_record->frame->fullname, stop_record->frame->line, stop_record->frame->func);

					append_source_file(a_process_rank, stop_record->frame->fullname, stop_record->frame->line);
					scroll_to_line(a_process_rank);
					int page_num = get_widget<Gtk::Notebook>("files-notebook")->get_current_page();
					update_markers(page_num);
				}
				mi_free_stop(stop_record);
			}
			mi_free_output(o);
		}
		token = strtok(NULL, "\n");
	}
}

void UIWindow::print_data_trgt(const char *const a_data, const int a_process_rank)
{
	Gtk::TextBuffer *buffer = m_text_buffers_trgt[a_process_rank];
	buffer->insert(buffer->end(), a_data);
}

void UIWindow::print_data(mi_h *const a_gdb_handle, const char *const a_data, const size_t a_length, const int a_port)
{
	const int process_rank = get_process_rank(a_port);
	const bool is_gdb = src_is_gdb(a_port);

	m_mutex_gui.lock();

	if (is_gdb)
	{
		print_data_gdb(a_gdb_handle, a_data, process_rank);
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
		print_data_trgt(a_data, process_rank);
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

	free((void *)a_data);

	m_mutex_gui.unlock();
}