#include <string>

#include "util.hpp"
#include "window.hpp"

#define NUM_MARKS 4

UIWindow::UIWindow(const int a_num_processes)
	: m_num_processes(a_num_processes),
	  m_scrolled_window_top(),
	  m_grid(),
	  m_label_all(),
	  m_entry_all_gdb(),
	  m_entry_all_trgt(),
	  m_button_all()
{
	m_text_view_buffers_gdb = new char *[m_num_processes];
	m_text_view_buffers_trgt = new char *[m_num_processes];
	m_current_line = new int[m_num_processes];
	m_source_view_path = new string[m_num_processes];
	m_buffer_length_gdb = new size_t[m_num_processes];
	m_buffer_length_trgt = new size_t[m_num_processes];
	m_scrolled_windows_gdb = new Gtk::ScrolledWindow[m_num_processes];
	m_scrolled_windows_trgt = new Gtk::ScrolledWindow[m_num_processes];
	m_scrolled_windows_sw = new Gtk::ScrolledWindow[m_num_processes];
	m_text_views_gdb = new Gtk::TextView[m_num_processes];
	m_text_views_trgt = new Gtk::TextView[m_num_processes];
	m_draw_areas = new UIDrawingArea *[m_num_processes];
	m_source_views = new Gsv::View[m_num_processes];
	m_entries_gdb = new Gtk::Entry[m_num_processes];
	m_entries_trgt = new Gtk::Entry[m_num_processes];
	m_buttons_trgt = new SIGINTButton[m_num_processes];
	m_labels_sw = new Gtk::Label[m_num_processes];
	m_conns_gdb = new tcp::socket *[m_num_processes];
	m_conns_trgt = new tcp::socket *[m_num_processes];
	m_scroll_connections_gdb = new sigc::connection[m_num_processes];
	m_scroll_connections_trgt = new sigc::connection[m_num_processes];
	m_conns_open_gdb = new bool[m_num_processes];
	m_where_marks = new char *[m_num_processes];
	m_where_categories = new char *[m_num_processes];
	for (int i = 0; i < m_num_processes; ++i)
	{
		m_text_view_buffers_gdb[i] = (char *)malloc(8 * sizeof(char));
		m_text_view_buffers_trgt[i] = (char *)malloc(8 * sizeof(char));
		m_text_view_buffers_gdb[i][0] = '\0';
		m_text_view_buffers_trgt[i][0] = '\0';
		m_buffer_length_gdb[i] = 1;
		m_buffer_length_trgt[i] = 1;
		m_conns_gdb[i] = nullptr;
		m_conns_trgt[i] = nullptr;
		m_conns_open_gdb[i] = false;
		char *mark_name = new char[16];
		char *cat_name = new char[16];
		sprintf(mark_name, "mark-%d", i);
		sprintf(cat_name, "cat-%d", i);
		m_where_marks[i] = strdup(mark_name);
		m_where_categories[i] = strdup(cat_name);
		delete mark_name;
		delete cat_name;
		m_current_line[i] = 0;
		m_draw_areas[i] = new UIDrawingArea{m_num_processes, i};
	}
}

UIWindow::~UIWindow()
{
	for (int i = 0; i < m_num_processes; ++i)
	{
		free(m_text_view_buffers_gdb[i]);
		free(m_text_view_buffers_trgt[i]);
		free(m_where_marks[i]);
		free(m_where_categories[i]);
	}
	delete[] m_text_view_buffers_gdb;
	delete[] m_text_view_buffers_trgt;
	delete[] m_current_line;
	delete[] m_source_view_path;
	delete[] m_buffer_length_gdb;
	delete[] m_buffer_length_trgt;
	delete[] m_scrolled_windows_gdb;
	delete[] m_scrolled_windows_trgt;
	delete[] m_scrolled_windows_sw;
	delete[] m_text_views_gdb;
	delete[] m_text_views_trgt;
	delete[] m_draw_areas;
	delete[] m_source_views;
	delete[] m_entries_gdb;
	delete[] m_entries_trgt;
	delete[] m_buttons_trgt;
	delete[] m_labels_sw;
	delete[] m_conns_gdb;
	delete[] m_conns_trgt;
	delete[] m_scroll_connections_gdb;
	delete[] m_scroll_connections_trgt;
	delete[] m_conns_open_gdb;
}

int colors[][3] = {
	{255, 0, 0},
	{0, 255, 0},
	{0, 0, 255},
	{255, 255, 0}};

unsigned char *create_pixbuf(int width, int height, const int a_process_rank, const int a_num_processes)
{
	unsigned char *data = new unsigned char[4 * width * height];
	unsigned char *p = data;

	for (int row = 0; row < height; ++row)
	{
		for (int col = 0; col < width; ++col)
		{
			if ((row - width / 2) * (row - width / 2) + (col - height / 2) * (col - height / 2) < (width / 2) * (width / 2))
			{
				*p++ = guint8(colors[a_process_rank][0]); // r
				*p++ = guint8(colors[a_process_rank][1]); // g
				*p++ = guint8(colors[a_process_rank][2]); // b
				*p++ = guint8(255);						  // a
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

	this->maximize();
	this->set_title("Parallel GDB");
	this->add(m_scrolled_window_top);

	m_grid.set_row_spacing(10);
	m_grid.set_column_spacing(10);
	m_scrolled_window_top.add(m_grid);

	int width = 600;
	int height = 200;
	for (int i = 0; i < m_num_processes; ++i)
	{
		// gdb
		m_text_views_gdb[i].set_editable(false);
		m_scrolled_windows_gdb[i].set_size_request(width, height);
		m_scrolled_windows_gdb[i].add(m_text_views_gdb[i]);
		m_grid.attach(m_scrolled_windows_gdb[i], 0, (2 * i), 1, 1);
		m_grid.attach(m_entries_gdb[i], 0, (2 * i + 1), 1, 1);
		m_entries_gdb[i].signal_activate().connect(
			[&, i]
			{ send_input_gdb(i); });

		// trgt
		m_text_views_trgt[i].set_editable(false);
		m_scrolled_windows_trgt[i].set_size_request(width, height);
		m_scrolled_windows_trgt[i].add(m_text_views_trgt[i]);
		m_grid.attach(m_scrolled_windows_trgt[i], 1, (2 * i), 2, 1);
		m_grid.attach(m_entries_trgt[i], 1, (2 * i + 1), 1, 1);
		m_grid.attach(m_buttons_trgt[i], 2, (2 * i + 1), 1, 1);
		m_entries_trgt[i].signal_activate().connect(
			[&, i]
			{ send_input_trgt(i); });
		m_buttons_trgt[i].signal_clicked().connect(
			[&, i]
			{ send_sig_int(i); });

		// draw_areas
		m_draw_areas[i]->set_size_request(100, 200);
		m_grid.attach(*m_draw_areas[i], 3, (2 * i), 1, 1);

		// source_views
		Gsv::View *source_view = &m_source_views[i];
		auto source_buffer = source_view->get_source_buffer();
		source_buffer->set_language(Gsv::LanguageManager::get_default()->get_language("cpp"));
		source_buffer->set_highlight_matching_brackets(true);
		source_buffer->set_highlight_syntax(true);
		source_view->set_source_buffer(source_buffer);
		source_view->set_show_line_numbers(true);
		source_view->set_editable(false);
		source_view->set_show_line_marks(true);
		auto attributes = Gsv::MarkAttributes::create();
		int pixbuf_width = 32;
		int pixbuf_height = 32;
		unsigned char *data = create_pixbuf(pixbuf_width, pixbuf_height, i, m_num_processes);
		auto pixbuf = Gdk::Pixbuf::create_from_data(
			data,
			Gdk::Colorspace::COLORSPACE_RGB,
			true,
			8,
			pixbuf_width,
			pixbuf_height,
			4 * pixbuf_width,
			&free_pixbuf);
		attributes->set_pixbuf(pixbuf);
		source_view->set_mark_attributes(m_where_categories[i], attributes, 0);
		m_scrolled_windows_sw[i].set_size_request(width, height);
		m_scrolled_windows_sw[i].add(m_source_views[i]);
		m_grid.attach(m_scrolled_windows_sw[i], 4, (2 * i), 1, 1);
		m_grid.attach(m_labels_sw[i], 4, (2 * i + 1), 1, 1);

		m_scrolled_windows_sw[i].get_vadjustment()->signal_value_changed().connect(
			sigc::bind(
				sigc::mem_fun(
					*this,
					&UIWindow::on_scroll_sw),
				i));
		source_buffer->signal_source_mark_updated().connect(
			sigc::bind(
				sigc::mem_fun(
					*this,
					&UIWindow::on_marker_update),
				i));

		Gtk::TextIter line_iter = source_buffer->get_iter_at_line(0);
		auto where_marker = source_buffer->create_source_mark(m_where_marks[i], m_where_categories[i], line_iter);
	}

	int last_row_idx = 2 * m_num_processes + 2;

	m_label_all.set_text("Send to all:");
	m_grid.attach(m_label_all, 0, last_row_idx, 1, 1);
	last_row_idx += 1;

	m_button_all.set_label("Send SIGINT to all");
	m_grid.attach(m_entry_all_gdb, 0, last_row_idx, 1, 1);
	m_grid.attach(m_entry_all_trgt, 1, last_row_idx, 1, 1);
	m_grid.attach(m_button_all, 2, last_row_idx, 1, 1);

	m_entry_all_gdb.signal_activate().connect(
		[&]
		{ send_input_all_gdb(); });
	m_entry_all_trgt.signal_activate().connect(
		[&]
		{ send_input_all_trgt(); });
	m_button_all.signal_clicked().connect(
		[&]
		{ send_sig_int_all(); });

	this->show_all();

	return true;
}

void UIWindow::get_mark_pos(const int)
{
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		auto source_view = &m_source_views[rank];
		auto source_buffer = source_view->get_source_buffer();
		auto adjustment = m_scrolled_windows_sw[rank].get_vadjustment();

		for (int i = 0; i < m_num_processes; ++i)
		{
			auto line_iter = source_buffer->get_iter_at_line(m_current_line[i] - 1);
			if (!line_iter || m_source_view_path[rank] != m_source_view_path[i])
			{
				m_draw_areas[rank]->set_y_offset(i, -2 * UIDrawingArea::radius() - 1); // set out of visible area
				continue;
			}

			Gdk::Rectangle rect;
			source_view->get_iter_location(line_iter, rect);

			int draw_pos = rect.get_y() - int(adjustment->get_value());
			m_draw_areas[rank]->set_y_offset(i, draw_pos);
		}
		m_draw_areas[rank]->queue_draw();
	}

	// rect.get_y();
	// adjustment->get_value();
	// adjustment->get_upper();
	// m_scrolled_windows_sw[a_process_rank].get_height();
}

void UIWindow::on_scroll_sw(const int a_process_rank)
{
	get_mark_pos(a_process_rank);
}

void UIWindow::on_marker_update(const Glib::RefPtr<Gtk::TextMark> &, const int a_process_rank)
{
	get_mark_pos(a_process_rank);
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

void UIWindow::send_sig_int_all() const
{
	string cmd = "\3"; // ^C
	for (int i = 0; i < m_num_processes; ++i)
	{
		send_sig_int(i);
	}

	// // crash application
	// char *a = strdup("a");
	// free(a);
	// free(a);
}

void UIWindow::send_sig_int(const int a_process_rank) const
{
	string cmd = "\3"; // ^C
	asio::write(*m_conns_trgt[a_process_rank], asio::buffer(cmd, 1));
}

void UIWindow::send_input_all_gdb()
{
	string cmd = string(m_entry_all_gdb.get_text()) + string("\n");
	for (int i = 0; i < m_num_processes; ++i)
	{
		asio::write(*m_conns_gdb[i], asio::buffer(cmd, cmd.length()));
	}
	m_entry_all_gdb.set_text("");
}

void UIWindow::send_input_all_trgt()
{
	string cmd = string(m_entry_all_trgt.get_text()) + string("\n");
	for (int i = 0; i < m_num_processes; ++i)
	{
		asio::write(*m_conns_trgt[i], asio::buffer(cmd, cmd.length()));
	}
	m_entry_all_trgt.set_text("");
}

void UIWindow::send_input_gdb(const int a_process_rank)
{
	string cmd = string(m_entries_gdb[a_process_rank].get_text()) + string("\n");
	asio::write(*m_conns_gdb[a_process_rank], asio::buffer(cmd, cmd.length()));
	m_entries_gdb[a_process_rank].set_text("");
}

void UIWindow::send_input_trgt(const int a_process_rank)
{
	string cmd = string(m_entries_trgt[a_process_rank].get_text()) + string("\n");
	asio::write(*m_conns_trgt[a_process_rank], asio::buffer(cmd, cmd.length()));
	m_entries_trgt[a_process_rank].set_text("");
}

void UIWindow::update_line_mark(const int a_process_rank)
{
	Gsv::View *source_view = &m_source_views[a_process_rank];
	auto source_buffer = source_view->get_source_buffer();
	Gtk::TextIter line_iter = source_buffer->get_iter_at_line(m_current_line[a_process_rank] - 1);
	if (!line_iter)
	{
		std::cerr << "No iter. File: " << m_source_view_path[a_process_rank] << "\n";
		return;
	}
	auto where_marker = source_buffer->get_mark(m_where_marks[a_process_rank]);
	source_buffer->move_mark(where_marker, line_iter);
}

void UIWindow::update_source_file(const int a_process_rank, mi_stop *a_stop_record)
{
	m_current_line[a_process_rank] = a_stop_record->frame->line;
	auto source_view = &m_source_views[a_process_rank];
	auto source_buffer = source_view->get_source_buffer();
	char *content;
	size_t content_length;
	string fullname = string(a_stop_record->frame->fullname);
	if (m_source_view_path[a_process_rank] != fullname)
	{
		m_source_view_path[a_process_rank] = fullname;
		if (g_file_get_contents(a_stop_record->frame->fullname, &content, &content_length, nullptr))
		{
			m_labels_sw[a_process_rank].set_text(a_stop_record->frame->fullname);
			source_buffer->set_text(content);
		}
	}
}

void UIWindow::do_scroll(Gsv::View *a_source_view, const int a_line) const
{
	Gtk::TextIter iter = a_source_view->get_buffer()->get_iter_at_line(a_line);
	if (!iter)
	{
		std::cerr << "No iter. Line: " << a_line << "\n";
		return;
	}
	a_source_view->scroll_to(iter, 0.1);
}

void UIWindow::scroll_to_line(Gsv::View *a_source_view, const int a_line) const
{
	Glib::signal_idle().connect_once(
		sigc::bind(
			sigc::mem_fun(
				this,
				&UIWindow::do_scroll),
			a_source_view,
			a_line - 1));
}

void UIWindow::print_data_gdb(mi_h *const a_gdb_handle, const char *const a_data, const int a_process_rank)
{
	char **buffer = &m_text_view_buffers_gdb[a_process_rank];
	size_t *buffer_length = &m_buffer_length_gdb[a_process_rank];

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
					append_text(buffer, buffer_length, text, strlen(text) + 1);
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

					update_source_file(a_process_rank, stop_record);
					update_line_mark(a_process_rank);
					scroll_to_line(&m_source_views[a_process_rank], stop_record->frame->line);
				}
				mi_free_stop(stop_record);
			}
			mi_free_output(o);
		}
		token = strtok(NULL, "\n");
	}

	Gtk::TextView *text_view = &m_text_views_gdb[a_process_rank];
	auto gtk_buffer = text_view->get_buffer();
	gtk_buffer->set_text(*buffer);
}

void UIWindow::print_data_trgt(const char *const a_data, const size_t a_data_length, const int a_process_rank)
{
	char **buffer = &m_text_view_buffers_trgt[a_process_rank];
	size_t *length = &m_buffer_length_trgt[a_process_rank];
	append_text(buffer, length, a_data, a_data_length);

	Gtk::TextView *text_view = &m_text_views_trgt[a_process_rank];
	auto gtk_buffer = text_view->get_buffer();
	gtk_buffer->set_text(*buffer); // Glib::convert_with_fallback(*buffer, "UTF-8", "ISO-8859-1")
}

void UIWindow::print_data(mi_h *const a_gdb_handle, const char *const a_data, const size_t a_length, const int a_port)
{
	const int process_rank = get_process_rank(a_port);
	const bool is_gdb = src_is_gdb(a_port);

	m_mutex_gui.lock();

	if (is_gdb)
	{
		Gtk::ScrolledWindow *scrolled_window = &m_scrolled_windows_gdb[process_rank];
		print_data_gdb(a_gdb_handle, a_data, process_rank);
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
		Gtk::ScrolledWindow *scrolled_window = &m_scrolled_windows_trgt[process_rank];
		print_data_trgt(a_data, a_length, process_rank);
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