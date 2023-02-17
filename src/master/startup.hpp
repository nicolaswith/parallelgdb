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
	along with ParallelGDB.  If not, see <https://www.gnu.org/licenses/gpl-3.0.txt>.
*/

/**
 * @file startup.hpp
 *
 * @brief Header file for the StartupDialog class.
 *
 * This file is the header file for the StartupDialog class.
 */

#ifndef STARTUP_HPP
#define STARTUP_HPP

#include <gtkmm.h>
#include <iosfwd>

/// Generates the startup dialog.
/**
 * This is a wrapper class for a Gtk::Dialog. It generates the startup dialog,
 * where the user configures ParallelGDB.
 */
class StartupDialog
{
	bool m_is_valid;
	int m_number_of_processes;
	int m_processes_per_node;
	int m_num_nodes;
	bool m_mpirun;
	bool m_srun;
	bool m_ssh;
	bool m_custom_launcher;
	std::string m_slave;
	std::string m_target;
	std::string m_arguments;
	std::string m_ip_address;
	std::string m_ssh_address;
	std::string m_ssh_user;
	std::string m_ssh_password;
	std::string m_launcher_args;
	std::string m_launcher_cmd;

	Glib::RefPtr<Gtk::Builder> m_builder;
	Gtk::Dialog *m_dialog;

	Gtk::FileChooserButton *m_file_chooser_button;
	Gtk::Entry *m_entry_number_of_processes;
	Gtk::Entry *m_entry_processes_per_node;
	Gtk::Entry *m_entry_num_nodes;
	Gtk::RadioButton *m_radiobutton_mpirun;
	Gtk::RadioButton *m_radiobutton_srun;
	Gtk::Entry *m_entry_slave;
	Gtk::Entry *m_entry_target;
	Gtk::Entry *m_entry_arguments;
	Gtk::Entry *m_entry_ip_address;
	Gtk::CheckButton *m_checkbutton_ssh;
	Gtk::Entry *m_entry_ssh_address;
	Gtk::Entry *m_entry_ssh_user;
	Gtk::Entry *m_entry_ssh_password;
	Gtk::Entry *m_entry_launcher_args;
	Gtk::CheckButton *m_checkbutton_launcher;
	Gtk::Entry *m_entry_launcher;

	/// Updates the current configuration.
	void on_dialog_response(const int response_id);
	/// Sets the sensitivity of the SSH related widgets.
	void set_sensitivity_ssh(const bool state);
	/// Signal handler for the SSH checkbutton.
	void on_ssh_button_toggled(Gtk::CheckButton *button);
	/// Signal handler for the Custom Launcher checkbutton.
	void on_custom_launcher_toggled(Gtk::CheckButton *button);
	/// Resets the dialog to be empty.
	void clear_dialog();
	/// Set a value to an widget in the dialog.
	void set_value(const std::string &key, const std::string &value);
	/// Opens and tokenizes a (configuration) file.
	void read_config();
	/// Shows a save file dialog to export the current configuration as a file
	void export_config();
	/// Parses the current configuration.
	bool read_values();
	/// Writes the current configuration as a file.
	void on_save_dialog_response(const int response_id,
								 Gtk::FileChooserDialog *file_chooser_dialog);
	/// Resets the dialog to be empty and clears the file in the file-chooser-button.
	void clear_all();

	template <class T>
	T *get_widget(const std::string &widget_name);

public:
	/// Default constructor.
	StartupDialog();
	/// Destructor.
	virtual ~StartupDialog();

	/// Returns the launcher command.
	std::string get_cmd() const;

	/// Runs the startup dialog.
	/**
	 * This function runs the startup dialog.
	 *
	 * @return The response ID.
	 */
	inline int run()
	{
		return m_dialog->run();
	}

	/// Returns whether the configuration could be successfully parsed.
	/**
	 * This function returns whether the configuration could be
	 * successfully parsed.
	 *
	 * @return @c true on success, @c false otherwise.
	 */
	inline bool is_valid() const
	{
		return m_is_valid;
	}

	/// Returns the total number of processes.
	/**
	 * This function returns the total number of processes.
	 *
	 * @return The total number of processes.
	 */
	inline int num_processes() const
	{
		return m_number_of_processes;
	}

	/// Returns whether SSH should be used.
	/**
	 * This function returns whether SSH should be used.
	 *
	 * @return Whether SSH should be used. @c true for yes. @c false for no.
	 */
	inline bool ssh() const
	{
		return m_ssh;
	}

	/// Returns the SSH address.
	/**
	 * This function returns the SSH address.
	 *
	 * @return The SSH address.
	 */
	inline const char *ssh_address() const
	{
		return m_ssh_address.c_str();
	}

	/// Returns the SSH username.
	/**
	 * This function returns the SSH username.
	 *
	 * @return The SSH username.
	 */
	inline const char *ssh_user() const
	{
		return m_ssh_user.c_str();
	}

	/// Returns the SSH password.
	/**
	 * This function returns the SSH password.
	 *
	 * @return The SSH password.
	 */
	inline const char *ssh_password() const
	{
		return m_ssh_password.c_str();
	}

	/// Returns whether the master should start the slaves.
	/**
	 * This function returns whether the master should start the slaves. When
	 * the custom command is used AND is left blank, the user wants to start
	 * the slaves manually.
	 *
	 * @return Whether the master should start the slaves.
	 */
	inline bool master_starts_slaves() const
	{
		return !(m_custom_launcher && m_launcher_cmd.empty());
	}
};

#endif /* STARTUP_HPP */