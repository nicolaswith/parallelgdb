#include <string>

#include "window.hpp"

#define NUM_MARKS 4

int colors[][3] = {
	{0xff, 0x00, 0x00},
	{0x00, 0x80, 0x00},
	{0x00, 0x00, 0xff},
	{0xff, 0xd7, 0x00},
	{0xc7, 0x15, 0x85},
	{0x00, 0xff, 0x00},
	{0x00, 0xff, 0xff},
	{0x1e, 0x90, 0xff}};

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

guint8 *create_pixbuf(int width, int height, const int a_process_rank, const int a_num_processes)
{
	guint8 *data = new guint8[4 * width * height];
	guint8 *p = data;

	for (int row = 0; row < height; ++row)
	{
		for (int col = 0; col < width; ++col)
		{
			if ((row - width / 2) * (row - width / 2) + (col - height / 2) * (col - height / 2) < (width / 2) * (width / 2))
			{
				*p++ = guint8(colors[a_process_rank % 8][0]); // r todo: % 8
				*p++ = guint8(colors[a_process_rank % 8][1]); // g
				*p++ = guint8(colors[a_process_rank % 8][2]); // b
				*p++ = guint8(255);							  // a
			}
			else
			{
				*p++ = guint8(0); // r
				*p++ = guint8(0); // g
				*p++ = guint8(0); // b
				*p++ = guint8(0); // a
			}
		}
	}

	return data;
}

void free_pixbuf(const guint8 *data)
{
	delete[] data;
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

	Gtk::Notebook *notebook_files = get_widget<Gtk::Notebook>("files-notebook");
	notebook_files->signal_switch_page().connect(
		sigc::mem_fun(*this, &UIWindow::on_page_switch));

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

void UIWindow::get_mark_pos(const int)
{
	// for (int rank = 0; rank < m_num_processes; ++rank)
	// {
	// 	auto source_view = &m_source_views[rank];
	// 	auto source_buffer = source_view->get_source_buffer();
	// 	auto adjustment = m_scrolled_windows_sw[rank].get_vadjustment();

	// 	for (int i = 0; i < m_num_processes; ++i)
	// 	{
	// 		auto line_iter = source_buffer->get_iter_at_line(m_current_line[i] - 1);
	// 		if (!line_iter || m_source_view_path[rank] != m_source_view_path[i])
	// 		{
	// 			m_draw_areas[rank]->set_y_offset(i, -2 * UIDrawingArea::radius() - 1); // set out of visible area
	// 			continue;
	// 		}

	// 		Gdk::Rectangle rect;
	// 		source_view->get_iter_location(line_iter, rect);

	// 		int draw_pos = rect.get_y() - int(adjustment->get_value());
	// 		m_draw_areas[rank]->set_y_offset(i, draw_pos);
	// 	}
	// 	m_draw_areas[rank]->queue_draw();
	// }

	// rect.get_y();
	// adjustment->get_value();
	// adjustment->get_upper();
	// m_scrolled_windows_sw[a_process_rank].get_height();
}

void UIWindow::on_page_switch(Gtk::Widget *a_page, const int a_page_num)
{
}

void UIWindow::on_scroll_sw(const int a_process_rank)
{
	// get_mark_pos(a_process_rank);
}

void UIWindow::on_marker_update(const Glib::RefPtr<Gtk::TextMark> &, const int a_process_rank)
{
	// get_mark_pos(a_process_rank);
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
	for (int i = 0; i < m_num_processes; ++i)
	{
		Gtk::CheckButton *check_button = dynamic_cast<Gtk::CheckButton *>(check_buttons.at(i));
		if (!check_button->get_active())
			continue;
		asio::write(*a_socket[i], asio::buffer(cmd, cmd.length()));
	}
	entry->set_text("");
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
	for (auto &&check_button_widget : box->get_children())
	{
		Gtk::CheckButton *check_button = dynamic_cast<Gtk::CheckButton *>(check_button_widget);
		if (check_button->get_active())
		{
			one_checked = true;
			break;
		}
	}
	bool state = !one_checked;
	for (auto &&check_button_widget : box->get_children())
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

void UIWindow::update_line_mark(const int a_process_rank)
{
	// Gsv::View *source_view = &m_source_views[a_process_rank];
	// auto source_buffer = source_view->get_source_buffer();
	// Gtk::TextIter line_iter = source_buffer->get_iter_at_line(m_current_line[a_process_rank] - 1);
	// if (!line_iter)
	// {
	// 	std::cerr << "No iter. File: " << m_source_view_path[a_process_rank] << "\n";
	// 	return;
	// }
	// auto where_marker = source_buffer->get_mark(m_where_marks[a_process_rank]);
	// source_buffer->move_mark(where_marker, line_iter);
}

void UIWindow::append_source_file(const int a_process_rank, mi_stop *a_stop_record)
{
	m_current_line[a_process_rank] = a_stop_record->frame->line;
	string fullname = a_stop_record->frame->fullname;
	m_source_view_path[a_process_rank] = fullname;
	Gtk::Notebook *notebook = get_widget<Gtk::Notebook>("files-notebook");
	if (m_opened_files.find(fullname) == m_opened_files.end())
	{
		m_opened_files.insert(fullname);
		string basename = Glib::path_get_basename(fullname);
		Gtk::Label *label = Gtk::manage(new Gtk::Label(basename));
		Gtk::ScrolledWindow *scrolled_window = Gtk::manage(new Gtk::ScrolledWindow());
		Gsv::View *source_view = Gtk::manage(new Gsv::View());
		scrolled_window->add(*source_view);
		Glib::RefPtr<Gsv::Buffer> source_buffer = source_view->get_source_buffer();
		source_buffer->set_language(Gsv::LanguageManager::get_default()->get_language("cpp"));
		source_buffer->set_highlight_matching_brackets(true);
		source_buffer->set_highlight_syntax(true);
		source_view->set_source_buffer(source_buffer);
		source_view->set_show_line_numbers(true);
		source_view->set_editable(false);
		source_view->set_show_line_marks(true);
		Glib::RefPtr<Gsv::MarkAttributes> attributes = Gsv::MarkAttributes::create();
		int pixbuf_width = 32;
		int pixbuf_height = 32;
		const guint8 *const data = create_pixbuf(pixbuf_width, pixbuf_height, 0, m_num_processes);
		Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create_from_data(
			data,
			Gdk::Colorspace::COLORSPACE_RGB,
			true,
			8,
			pixbuf_width,
			pixbuf_height,
			4 * pixbuf_width,
			&free_pixbuf);
		attributes->set_pixbuf(pixbuf);
		source_view->set_mark_attributes(m_where_categories[0], attributes, 0);
		char *content;
		size_t content_length;
		if (g_file_get_contents(a_stop_record->frame->fullname, &content, &content_length, nullptr))
		{
			source_buffer->set_text(content);
		}
		else
		{
			source_buffer->set_text("Could not load file.");
		}
		int page_num = notebook->append_page(*scrolled_window, *label);
		notebook->show_all();
		notebook->set_current_page(page_num);
	}
	else
	{
		notebook->set_current_page(1000); // todo
	}
}

void UIWindow::do_scroll(Gsv::View *a_source_view, const int a_line) const
{
	// Gtk::TextIter iter = a_source_view->get_buffer()->get_iter_at_line(a_line);
	// if (!iter)
	// {
	// 	std::cerr << "No iter. Line: " << a_line << "\n";
	// 	return;
	// }
	// a_source_view->scroll_to(iter, 0.1);
}

void UIWindow::scroll_to_line(Gsv::View *a_source_view, const int a_line) const
{
	// Glib::signal_idle().connect_once(
	// 	sigc::bind(
	// 		sigc::mem_fun(
	// 			this,
	// 			&UIWindow::do_scroll),
	// 		a_source_view,
	// 		a_line - 1));
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

					append_source_file(a_process_rank, stop_record);
					// update_line_mark(a_process_rank);
					// scroll_to_line(&m_source_views[a_process_rank], stop_record->frame->line);
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