#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "defs.hpp"
#include "mi_gdb.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtksourceview/gtksource.h>
#include <gtksourceviewmm.h>
#pragma GCC diagnostic pop

enum TargetState
{
	UNKNOWN,
	STOPPED,
	RUNNING,
	EXITED
};

class Breakpoint;
class UIDrawingArea;

class UIWindow
{
	const int m_num_processes;

	int *m_current_line;
	string *m_source_view_path;
	TargetState *m_target_state;

	Glib::RefPtr<Gtk::Builder> m_builder;
	Gtk::Window *m_root_window;
	Glib::RefPtr<Gtk::Application> m_app;

	Gtk::Notebook *m_files_notebook;
	UIDrawingArea *m_drawing_area;

	Gtk::Grid *m_overview_grid;
	Gtk::Separator **m_separators;
	int m_last_row_idx;

	Gtk::TextBuffer **m_text_buffers_gdb;
	Gtk::TextBuffer **m_text_buffers_trgt;
	Gtk::ScrolledWindow **m_scrolled_windows_gdb;
	Gtk::ScrolledWindow **m_scrolled_windows_trgt;

	std::set<string> m_opened_files;
	std::map<string, int> m_path_2_pagenum;
	std::map<int, string> m_pagenum_2_path;
	std::map<string, Gsv::View *> m_path_2_view;
	std::map<string, int> m_path_2_row;

	sigc::connection *m_scroll_connections_gdb;
	sigc::connection *m_scroll_connections_trgt;

	std::mutex m_mutex_gui;

	tcp::socket **m_conns_gdb;
	tcp::socket **m_conns_trgt;

	Breakpoint **m_breakpoints;

	volatile bool *m_started;
	volatile bool m_sent_run;

	volatile bool *m_conns_open_gdb;

	void init_overview();
	void init_grid(Gtk::Grid *grid);
	void init_notebook(Gtk::Notebook *notebook, Gtk::ScrolledWindow **scrolled_windows, Gtk::TextBuffer **text_buffers);
	void do_scroll(const int rank) const;
	void scroll_to_line(const int rank) const;
	void append_overview_row(const string &basename, const string &fullpath);
	void append_source_file(const string &fullpath);
	void print_data_gdb(mi_h *const gdb_handle, const char *const data, const int rank);
	void print_data_trgt(const char *const data, const int rank);
	void update_markers(const int page_num);
	bool update_markers_timeout();
	void scroll_bottom(Gtk::Allocation &, Gtk::ScrolledWindow *const scrolled_window, const bool is_gdb, const int rank);
	void send_input(const string &entry_name, const string &wrapper_name, tcp::socket *const *const socket);
	void send_input_gdb();
	void send_input_trgt();
	void toggle_all(const string &box_name);
	void toggle_all_gdb();
	void toggle_all_trgt();
	void clear_mark(Glib::RefPtr<Gtk::TextMark> &mark, Glib::RefPtr<Gsv::Buffer> &source_buffer, Breakpoint *breakpoint);
	void create_mark(Gtk::TextIter &iter, Glib::RefPtr<Gsv::Buffer> &source_buffer, const string &fullpath);
	void edit_mark(Glib::RefPtr<Gtk::TextMark> &mark, Glib::RefPtr<Gsv::Buffer> &source_buffer);
	void delete_mark(Glib::RefPtr<Gtk::TextMark> &mark, Glib::RefPtr<Gsv::Buffer> &source_buffer);
	void on_line_mark_clicked(Gtk::TextIter &iter, GdkEvent *const event, const string &fullpath);
	void send_data_to_active(tcp::socket *const *const socket, const string &data);
	void interact_with_gdb(const int key_value);
	void on_about_clicked();
	void on_quit_clicked();
	void on_interaction_button_clicked(const int key_value);
	bool on_key_press(GdkEventKey *event);
	void remove_label_overview(const int rank);
	void color_overview();
	void update_overview(const int rank, const string &fullpath, const int line);
	void set_position(const int rank, const string &fullpath, const int line);
	void open_file();
	void close_unused_tabs();

	template <class T>
	T *get_widget(const string &widget_name);

public:
	UIWindow(const int num_processes);
	virtual ~UIWindow();

	bool init(Glib::RefPtr<Gtk::Application> app);
	bool on_delete(GdkEventAny *);
	void print_data(mi_h *const gdb_handle, const char *const data, const int port);
	bool send_data(tcp::socket *const socket, const string &data);

	inline Gtk::Window *root_window() const
	{
		return m_root_window;
	}

	inline int num_processes() const
	{
		return m_num_processes;
	}

	inline TargetState target_state(const int rank) const
	{
		return m_target_state[rank];
	}

	inline tcp::socket *get_conns_gdb(const int rank) const
	{
		return m_conns_gdb[rank];
	}

	inline void set_conns_gdb(const int rank, tcp::socket *const socket)
	{
		m_conns_gdb[rank] = socket;
	}

	inline tcp::socket *get_conns_trgt(const int rank) const
	{
		return m_conns_trgt[rank];
	}

	inline void set_conns_trgt(const int rank, tcp::socket *const socket)
	{
		m_conns_trgt[rank] = socket;
	}

	inline bool get_conns_open_gdb(const int rank) const
	{
		return m_conns_open_gdb[rank];
	}

	inline void set_conns_open_gdb(const int rank, const bool value)
	{
		m_conns_open_gdb[rank] = value;
	}

	inline void set_breakpoint(const int rank, Breakpoint *const breakpoint)
	{
		m_breakpoints[rank] = breakpoint;
	}

	inline static bool src_is_gdb(const int port)
	{
		return (0 == (port & 0x4000));
	}

	inline static int get_rank(const int port)
	{
		return (0x3FFF & port);
	}
};

void set_application(Gtk::Application *app);
void on_quit_clicked();

#endif /* WINDOW_HPP */