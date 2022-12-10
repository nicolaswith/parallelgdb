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

	int *m_current_line;
	string *m_source_view_path;

	Glib::RefPtr<Gtk::Builder> m_builder;
	Gtk::Window *m_root_window;

	Gtk::TextBuffer **m_text_buffers_gdb;
	Gtk::TextBuffer **m_text_buffers_trgt;
	Gtk::ScrolledWindow **m_scrolled_windows_gdb;
	Gtk::ScrolledWindow **m_scrolled_windows_trgt;
	UIDrawingArea *m_drawing_area;

	std::set<string> m_opened_files;
	std::map<string, int> m_path_2_pagenum;
	std::map<int, string> m_pagenum_2_path;
	std::map<string, Gsv::View *> m_path_2_view;

	sigc::connection *m_scroll_connections_gdb;
	sigc::connection *m_scroll_connections_trgt;

	std::mutex m_mutex_gui;

	tcp::socket **m_conns_gdb;
	tcp::socket **m_conns_trgt;

	volatile bool *m_conns_open_gdb;

	char **m_where_marks;
	char **m_where_categories;

	void do_scroll(const int a_process_rank) const;
	void scroll_to_line(const int a_process_rank) const;
	void append_source_file(const int a_process_rank, const string &a_fullpath, const int a_line);
	void print_data_gdb(mi_h *const a_h, const char *const a_data, const int a_process_rank);
	void print_data_trgt(const char *const a_data, const int a_process_rank);
	void update_markers(const int a_page_num);
	void on_page_switch(Gtk::Widget *a_page, const int a_page_num);
	void on_scroll_file();
	void scroll_bottom(Gtk::Allocation &a_allocation, Gtk::ScrolledWindow *a_scrolled_window, const bool a_is_gdb, const int a_process_rank);
	void send_input(const string &a_entry_name, const string &a_wrapper_name, tcp::socket **a_socket);
	void send_sig_int();
	void send_input_gdb();
	void send_input_trgt();
	void toggle_all(const string &a_box_name);
	void toggle_all_gdb();
	void toggle_all_trgt();

	template <class T>
	T *get_widget(const string &a_widget_name);

public:
	UIWindow(const int a_num_processes);
	virtual ~UIWindow();

	bool init();
	bool on_delete(GdkEventAny *);
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

	inline static bool src_is_gdb(const int a_port)
	{
		return (0 == (a_port & 0x4000));
	}

	inline static int get_process_rank(const int a_port)
	{
		return (0x3FFF & a_port);
	}
};

#endif /* WINDOW_HPP */