/*
	This file is part of ParallelGDB.

	Copyright (c) 2023 by Nicolas With

	ParallelGDB is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	ParallelGDB is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with ParallelGDB.  If not, see <https://www.gnu.org/licenses/>.
*/

/**
 * @file window.hpp
 *
 * @brief Header file for the UIWindow class.
 *
 * This is the header file for the UIWindow class.
 */

#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <gtkmm.h>
#include <set>
#include <map>
#include <iosfwd>

#include "asio.hpp"
#include "mi_gdb.h"

class Breakpoint;
class UIDrawingArea;
#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace Gsv
{
	class View;
	class Buffer;
}
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/// The states a process can be in.
enum TargetState
{
	UNKNOWN, /**< The target state has not been detected yet. */
	STOPPED, /**< The target is stopped. */
	RUNNING, /**< The target is running. */
	EXITED	 /**< The target has exited. */
};

/// Wrapper class for the GUI window.
/**
 * This is the wrapper class for the GUI window. It contains the functionality
 * to manipulate the GUI as well as to interact with the socat instances.
 */
class UIWindow
{
	const int m_num_processes;
	int m_max_buttons_per_row;
	int m_follow_rank;

	int *m_current_line;
	std::string *m_current_file;
	TargetState *m_target_state;
	int *m_exit_code;

	mi_h **m_gdb_handle;

	Glib::RefPtr<Gtk::Builder> m_builder;
	Gtk::Window *m_root_window;
	Glib::RefPtr<Gtk::Application> m_app;

	Gtk::Notebook *m_files_notebook;
	UIDrawingArea *m_drawing_area;

	Gtk::Grid *m_overview_grid;
	Gtk::Separator **m_separators;
	int m_first_row_idx;
	int m_last_row_idx;

	Gtk::TextBuffer **m_text_buffers_gdb;
	Gtk::TextBuffer **m_text_buffers_trgt;
	Gtk::ScrolledWindow **m_scrolled_windows_gdb;
	Gtk::ScrolledWindow **m_scrolled_windows_trgt;

	std::set<std::string> m_opened_files;
	std::map<std::string, int> m_path_2_pagenum;
	std::map<int, std::string> m_pagenum_2_path;
	std::map<std::string, Gsv::View *> m_path_2_view;
	std::map<std::string, int> m_path_2_row;
	std::map<int, Breakpoint *> *m_bkptno_2_bkpt;
	std::map<std::string, sigc::connection> m_path_2_connection;

	sigc::connection *m_scroll_connections_gdb;
	sigc::connection *m_scroll_connections_trgt;

	std::mutex m_mutex_gui;

	asio::ip::tcp::socket **m_conns_gdb;
	asio::ip::tcp::socket **m_conns_trgt;

	Breakpoint **m_breakpoints;

	volatile bool *m_started;
	volatile bool m_sent_run;
	volatile bool *m_sent_stop;

	/// Initializes the table-like grid layout in the overview.
	void init_overview();
	/// Attaches checkbuttons in a grid layout.
	void init_grid(Gtk::Grid *grid);
	/// Adds a page in the GDB/target I/O notebook for every process.
	void init_notebook(Gtk::Notebook *notebook,
					   Gtk::ScrolledWindow **scrolled_windows,
					   Gtk::TextBuffer **text_buffers);
	/// Scrolls a scrolled window to a line.
	void do_scroll(const int rank) const;
	/// Appends scrolling a scrolled window to the Gdk idle handler.
	void scroll_to_line(const int rank) const;
	/// Appends a new row for a source file to the Overview.
	void append_overview_row(const std::string &basename,
							 const std::string &fullpath);
	/// Appends a source file page to the source view notebook.
	void append_source_file(const std::string &fullpath, const int rank);
	/// Tokenizes, parses and analyzes the received GDB output.
	void print_data_gdb(const char *const data, const int rank);
	/// Appends text to the target I/O text view.
	void print_data_trgt(const char *const data, const int rank);
	/// Sets the positions of the dots in the drawing area.
	void update_markers(const int page_num);
	/// Checks if the source file notebook has pages and updates the dots if so.
	bool update_markers_timeout();
	/// Scrolls a scrolled window to the bottom.
	void scroll_bottom(Gtk::Allocation &,
					   Gtk::ScrolledWindow *const scrolled_window,
					   const bool is_gdb, const int rank);
	/// Writes the text in an entry to a TCP socket.
	void send_input(const std::string &entry_name,
					const std::string &grid_name,
					asio::ip::tcp::socket *const *const socket);
	/// Sends text to the socat instances connected to GDB.
	void send_input_gdb();
	/// Sends text to the socat instances connected to the target program.
	void send_input_trgt();
	/// Stops all processes.
	void stop_all();
	/// Toggle all checkbuttons in a grid.
	void toggle_all(const std::string &grid_name);
	/// Toggles all checkbuttons in the GDB I/O section grid.
	void toggle_all_gdb();
	/// Toggles all checkbuttons in the target I/O section grid.
	void toggle_all_trgt();
	/// Delete a line mark in a Gsv::View.
	void clear_mark(Glib::RefPtr<Gtk::TextMark> &mark,
					Glib::RefPtr<Gsv::Buffer> &source_buffer,
					Breakpoint *breakpoint);
	/// Creates a line mark in a Gsv::View.
	void create_mark(Gtk::TextIter &iter,
					 Glib::RefPtr<Gsv::Buffer> &source_buffer,
					 const std::string &fullpath);
	/// Displays the BreakpointDialog dialog.
	void edit_mark(Glib::RefPtr<Gtk::TextMark> &mark,
				   Glib::RefPtr<Gsv::Buffer> &source_buffer);
	/// Deletes the breakpoint for all processes and then the mark itself.
	void delete_mark(Glib::RefPtr<Gtk::TextMark> &mark,
					 Glib::RefPtr<Gsv::Buffer> &source_buffer);
	/// Signal handler for the Gsv::View line mark clicked signal.
	void on_line_mark_clicked(Gtk::TextIter &iter, GdkEvent *const event,
							  const std::string &fullpath);
	/// Sends data to all selected processes.
	void send_data_to_active(asio::ip::tcp::socket *const *const socket,
							 const std::string &cmd);
	/// Maps the buttons/hot-keys to the corresponding action.
	void interact_with_gdb(const int key_value);
	/// Displays the about dialog.
	void on_about_clicked();
	/// Terminates the slaves and then quits the application.
	void on_quit_clicked();
	/// Opens a dialog to select the process to follow.
	void on_follow_button_clicked();
	/// Signal handler for all the interaction buttons.
	void on_interaction_button_clicked(const int key_value);
	/// Signal handler for key-press events.
	bool on_key_press(GdkEventKey *event);
	/// Clears all columns of a process in the Overview.
	void clear_labels_overview(const int rank);
	/// Sets the color for the line numbers in the Overview.
	void color_overview();
	/// Updates all labels and their tooltip for a process.
	void update_overview(const int rank, const std::string &fullpath,
						 const int line);
	/// Updates the current line and file for a process.
	void set_position(const int rank, const std::string &fullpath,
					  const int line);
	/// Open a source file in the source view notebook.
	void open_file();
	/// Closes all unused source files.
	void close_unused_tabs();
	/// Updates the running and exited row for a processes.
	void check_overview(const int rank);
	/// Opens a file for a source file which was not found.
	void open_missing(Gtk::TextIter &, GdkEvent *, const std::string &fullpath);
	/// Parses the GDB output for printable output and state changes of the target program.
	void parse_target_state(mi_output *first_output, const int rank);
	/// Searches for a breakpoint record in the GDB output.
	void parse_breakpoint(mi_output *first_output, const int rank);
	/// Searches for a stop record and extracts information from it.
	void parse_stop_record(mi_output *first_output, const int rank);

	/// Wrapper for the Gtk::get_widget function.
	template <class T>
	T *get_widget(const std::string &widget_name);

public:
	/// Default constructor.
	UIWindow(const int num_processes);
	/// Destructor.
	virtual ~UIWindow();

	/// Loads the glade file and connects the signal handlers.
	bool init(Glib::RefPtr<Gtk::Application> app);
	/// Closes the GDB TCP sockets and thus the slaves.
	bool on_delete(GdkEventAny *);
	/// Forwards the received data to the corresponding data handler.
	void print_data(const char *const data, const int port);
	/// Writes data to the TCP socket.
	bool send_data(asio::ip::tcp::socket *const socket,
				   const std::string &data);

	/// Gets the Gtk window object.
	/**
	 * This function gets the Gtk window object.
	 *
	 * @return The Gtk window object.
	 */
	inline Gtk::Window *root_window() const
	{
		return m_root_window;
	}

	/// Gets the total number of processes.
	/**
	 * This function gets the total number of processes.
	 *
	 * @return The total number of processes.
	 */
	inline int num_processes() const
	{
		return m_num_processes;
	}

	/// Gets the state of a process.
	/**
	 * This function gets the state of a process.
	 *
	 * @param rank The process rank.
	 *
	 * @return The state of the process.
	 */
	inline TargetState target_state(const int rank) const
	{
		return m_target_state[rank];
	}

	/// Gets the TCP socket associated to a process. (GDB)
	/**
	 * This function gets the TCP socket associated to a process, which is
	 * connected to the socat instance, handling the communication with GDB.
	 *
	 * @param rank The process rank.
	 *
	 * @return The TCP socket.
	 */
	inline asio::ip::tcp::socket *get_conns_gdb(const int rank) const
	{
		return m_conns_gdb[rank];
	}

	/// Sets the TCP socket associated to a process. (GDB)
	/**
	 * This function sets the TCP socket associated to a process, which is
	 * connected to the socat instance, handling the communication with GDB.
	 *
	 * @param rank The process rank.
	 *
	 * @param[in] socket The TCP socket.
	 */
	inline void set_conns_gdb(const int rank,
							  asio::ip::tcp::socket *const socket)
	{
		m_conns_gdb[rank] = socket;
	}

	/// Gets the TCP socket associated to a process. (target)
	/**
	 * This function gets the TCP socket associated to a process, which is
	 * connected to the socat instance, handling the communication with the
	 * target program.
	 *
	 * @param rank The process rank.
	 *
	 * @return The TCP socket.
	 */
	inline asio::ip::tcp::socket *get_conns_trgt(const int rank) const
	{
		return m_conns_trgt[rank];
	}

	/// Sets the TCP socket associated to a process. (target)
	/**
	 * This function sets the TCP socket associated to a process, which is
	 * connected to the socat instance, handling the communication with the
	 * target program.
	 *
	 * @param rank The process rank.
	 *
	 * @param[in] socket The TCP socket.
	 */
	inline void set_conns_trgt(const int rank,
							   asio::ip::tcp::socket *const socket)
	{
		m_conns_trgt[rank] = socket;
	}

	/// Stores a pointer to a Breakpoint object.
	/**
	 * This function stores a pointer to a Breakpoint object. When this is not
	 * a nullptr, the parser will try to find a related breakpoint-set message
	 * and extract the breakpoint number. See @ref Breakpoint::create_breakpoint.
	 *
	 * @param rank The process rank.
	 *
	 * @param[in] breakpoint A pointer to a Breakpoint object.
	 */
	inline void set_breakpoint(const int rank, Breakpoint *const breakpoint)
	{
		m_breakpoints[rank] = breakpoint;
	}

	/// Checks whether the port is associated with GDB.
	/**
	 * This function checks whether the port is associated with GDB.
	 *
	 * @param port The TCP port.
	 *
	 * @return @c true if the port is associated with GDB, @c false otherwise.
	 */
	inline static bool src_is_gdb(const int port)
	{
		return (0 == (port & 0x4000));
	}

	/// Retrieves the process rank from the TCP port.
	/**
	 * This function retrieves the process rank from the TCP port.
	 *
	 * @param port The TCP port.
	 *
	 * @return The process rank.
	 */
	inline static int get_rank(const int port)
	{
		return (0x3FFF & port);
	}
};

#endif /* WINDOW_HPP */