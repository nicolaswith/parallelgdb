#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "defs.hpp"
#include "mi_gdb.h"
#include "canvas.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtksourceview/gtksource.h>
#include <gtksourceviewmm.h>
#pragma GCC diagnostic pop

class SIGINTButton : public Gtk::Button
{
public:
	SIGINTButton() : Gtk::Button("Send SIGINT") {}
};

class UIWindow
{
	const int m_num_processes;

	char **m_text_view_buffers_gdb;
	char **m_text_view_buffers_trgt;

	int *m_current_line;
	string *m_source_view_path;

	size_t *m_buffer_length_gdb;
	size_t *m_buffer_length_trgt;

	Glib::RefPtr<Gtk::Builder> m_builder;
	Gtk::Window *m_root_window;

	sigc::connection *m_scroll_connections_gdb;
	sigc::connection *m_scroll_connections_trgt;

	std::mutex m_mutex_gui;

	tcp::socket **m_conns_gdb;
	tcp::socket **m_conns_trgt;

	volatile bool *m_conns_open_gdb;

	char **m_where_marks;
	char **m_where_categories;

	void do_scroll(Gsv::View *a_source_view, const int a_line) const;
	void scroll_to_line(Gsv::View *a_source_view, const int a_line) const;
	void update_line_mark(const int a_process_rank);
	void update_source_file(const int a_process_rank, mi_stop *a_stop_record);
	void print_data_gdb(mi_h *const a_h, const char *const a_data, const int a_process_rank);
	void print_data_trgt(const char *const a_data, const size_t a_data_length, const int a_process_rank);
	void on_marker_update(const Glib::RefPtr<Gtk::TextMark> &, const int a_process_rank);
	void get_mark_pos(const int a_process_rank);
	void on_scroll_sw(const int a_process_rank);
	void scroll_bottom(Gtk::Allocation &a_allocation, Gtk::ScrolledWindow *a_scrolled_window, const bool a_is_gdb, const int a_process_rank);

public:
	UIWindow(const int a_num_processes);
	virtual ~UIWindow();

	bool init();
	bool on_delete(GdkEventAny *);
	void send_sig_int(const int a_process_rank) const;
	void send_sig_int_all() const;
	void send_input_gdb(const int a_process_rank);
	void send_input_trgt(const int a_process_rank);
	void send_input_all_gdb();
	void send_input_all_trgt();
	void print_data(mi_h *const a_gdb_handle, const char *const a_data, const size_t a_length, const int a_port);

	inline Gtk::Window *root_window() const
	{
		return m_root_window;
	}

	inline int num_processes() const
	{
		return m_num_processes;
	}

	inline tcp::socket *get_conns_gdb(const int a_process_rank) const
	{
		return m_conns_gdb[a_process_rank];
	}

	inline void set_conns_gdb(const int a_process_rank, tcp::socket *const a_socket)
	{
		m_conns_gdb[a_process_rank] = a_socket;
	}

	inline tcp::socket *get_conns_trgt(const int a_process_rank) const
	{
		return m_conns_trgt[a_process_rank];
	}

	inline void set_conns_trgt(const int a_process_rank, tcp::socket *const a_socket)
	{
		m_conns_trgt[a_process_rank] = a_socket;
	}

	inline bool get_conns_open_gdb(const int a_process_rank) const
	{
		return m_conns_open_gdb[a_process_rank];
	}

	inline void set_conns_open_gdb(const int a_process_rank, const bool a_value)
	{
		m_conns_open_gdb[a_process_rank] = a_value;
	}
};

#endif /* WINDOW_HPP */