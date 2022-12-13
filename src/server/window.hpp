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

class UIWindow
{
	const int m_num_processes;

	int *m_current_line;
	string *m_source_view_path;

	Glib::RefPtr<Gtk::Builder> m_builder;
	Gtk::Window *m_root_window;
	
	Gtk::Notebook *m_files_notebook;
	UIDrawingArea *m_drawing_area;

	Gtk::TextBuffer **m_text_buffers_gdb;
	Gtk::TextBuffer **m_text_buffers_trgt;
	Gtk::ScrolledWindow **m_scrolled_windows_gdb;
	Gtk::ScrolledWindow **m_scrolled_windows_trgt;

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

	void do_scroll(const int process_rank) const;
	void scroll_to_line(const int process_rank) const;
	void append_source_file(const int process_rank, const string &fullpath, const int line);
	void print_data_gdb(mi_h *const gdb_handle, const char *const data, const int process_rank);
	void print_data_trgt(const char *const data, const int process_rank);
	void update_markers(const int page_num);
	bool update_markers_timeout();
	void scroll_bottom(Gtk::Allocation &, Gtk::ScrolledWindow *scrolled_window, const bool is_gdb, const int process_rank);
	void send_input(const string &entry_name, const string &wrapper_name, tcp::socket **socket);
	void send_sig_int();
	void send_input_gdb();
	void send_input_trgt();
	void toggle_all(const string &box_name);
	void toggle_all_gdb();
	void toggle_all_trgt();

	template <class T>
	T *get_widget(const string &widget_name);

public:
	UIWindow(const int num_processes);
	virtual ~UIWindow();

	bool init();
	bool on_delete(GdkEventAny *);
	void print_data(mi_h *const gdb_handle, const char *const data, const int port);

	inline Gtk::Window *root_window() const
	{
		return m_root_window;
	}

	inline int num_processes() const
	{
		return m_num_processes;
	}

	inline tcp::socket *get_conns_gdb(const int process_rank) const
	{
		return m_conns_gdb[process_rank];
	}

	inline void set_conns_gdb(const int process_rank, tcp::socket *const socket)
	{
		m_conns_gdb[process_rank] = socket;
	}

	inline tcp::socket *get_conns_trgt(const int process_rank) const
	{
		return m_conns_trgt[process_rank];
	}

	inline void set_conns_trgt(const int process_rank, tcp::socket *const socket)
	{
		m_conns_trgt[process_rank] = socket;
	}

	inline bool get_conns_open_gdb(const int process_rank) const
	{
		return m_conns_open_gdb[process_rank];
	}

	inline void set_conns_open_gdb(const int process_rank, const bool value)
	{
		m_conns_open_gdb[process_rank] = value;
	}

	inline static bool src_is_gdb(const int port)
	{
		return (0 == (port & 0x4000));
	}

	inline static int get_process_rank(const int port)
	{
		return (0x3FFF & port);
	}
};

#endif /* WINDOW_HPP */