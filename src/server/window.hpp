#include "defs.hpp"

#ifndef _WINDOW_HPP
#define _WINDOW_HPP

#include "mi_gdb.h"

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

class UIWindow : public Gtk::Window
{
	const int m_num_processes;

	char **m_text_view_buffers_gdb;
	char **m_text_view_buffers_trgt;
	string *m_source_view_path;
	size_t *m_buffer_length_gdb;
	size_t *m_buffer_length_trgt;

	Gtk::ScrolledWindow m_scrolled_window_top;
	Gtk::Grid m_grid;
	Gtk::ScrolledWindow *m_scrolled_windows_gdb;
	Gtk::ScrolledWindow *m_scrolled_windows_trgt;
	Gtk::ScrolledWindow *m_scrolled_windows_sw;
	Gtk::TextView *m_text_views_gdb;
	Gtk::TextView *m_text_views_trgt;
	Gsv::View *m_source_views;
	Gtk::Entry *m_entries_gdb;
	Gtk::Entry *m_entries_trgt;
	Gtk::Button *m_buttons_trgt;
	Gtk::Label *m_labels_sw;
	Gtk::Label m_label_all;
	Gtk::Entry m_entry_all_gdb;
	Gtk::Entry m_entry_all_trgt;
	Gtk::Button m_button_all;

	sigc::connection *m_scroll_connections_gdb;
	sigc::connection *m_scroll_connections_trgt;

	std::mutex m_mutex_gui;

	tcp::socket **m_conns_gdb;
	tcp::socket **m_conns_trgt;

	volatile bool *m_conns_open_gdb;

	void do_scroll(Gsv::View *a_source_view, const int a_line) const;
	void scroll_to_line(Gsv::View *a_source_view, const int a_line) const;
	void update_source_file(const int a_process_rank, mi_stop *a_stop_record);
	void print_data_gdb(mi_h *const a_h, const char *const a_data, const int a_process_rank);
	void print_data_trgt(const char *const a_data, const size_t a_data_length, const int a_process_rank);

public:
	UIWindow(const int a_num_processes);
	virtual ~UIWindow();

	void init();
	bool on_delete(GdkEventAny *);
	void scroll_bottom(Gtk::Allocation &a_allocation, Gtk::ScrolledWindow *a_scrolled_window, const bool a_is_gdb, const int a_process_rank);
	void send_sig_int(const int a_process_rank) const;
	void send_sig_int_all() const;
	void send_input_gdb(const int a_process_rank);
	void send_input_trgt(const int a_process_rank);
	void send_input_all_gdb();
	void send_input_all_trgt();
	void print_data(mi_h *const a_h, const char *const a_data, const size_t a_length, const int a_port);

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

#endif /* _WINDOW_HPP */