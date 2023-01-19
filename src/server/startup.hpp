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

#ifndef STARTUP_HPP
#define STARTUP_HPP

#include "defs.hpp"

class StartupDialog
{
	bool m_is_valid;

	int m_processes_per_node;
	int m_num_nodes;
	bool m_mpirun;
	bool m_srun;
	bool m_oversubscribe;
	char *m_client;
	char *m_gdb;
	char *m_socat;
	char *m_target;
	char *m_ip_address;
	bool m_ssh;
	char *m_ssh_address;
	char *m_ssh_user;
	char *m_ssh_password;
	char *m_partition;
	bool m_custom_launcher;
	char *m_launcher_cmd;

	Glib::RefPtr<Gtk::Builder> m_builder;
	Gtk::Dialog *m_dialog;

	Gtk::FileChooserDialog *m_file_chooser_dialog;
	string m_config;

	Gtk::FileChooserButton *m_file_chooser_button;
	Gtk::Entry *m_entry_processes_per_node;
	Gtk::Entry *m_entry_num_nodes;
	Gtk::RadioButton *m_radiobutton_mpirun;
	Gtk::RadioButton *m_radiobutton_srun;
	Gtk::CheckButton *m_checkbutton_oversubscribe;
	Gtk::Entry *m_entry_client;
	Gtk::Entry *m_entry_gdb;
	Gtk::Entry *m_entry_socat;
	Gtk::Entry *m_entry_target;
	Gtk::Entry *m_entry_ip_address;
	Gtk::CheckButton *m_checkbutton_ssh;
	Gtk::Entry *m_entry_ssh_address;
	Gtk::Entry *m_entry_ssh_user;
	Gtk::Entry *m_entry_ssh_password;
	Gtk::Entry *m_entry_partition;
	Gtk::CheckButton *m_checkbutton_launcher;
	Gtk::Entry *m_entry_launcher;

	void on_dialog_response(const int response_id);
	void set_sensitivity_ssh(bool state);
	void on_ssh_button_toggled(Gtk::CheckButton *button);
	void on_custom_launcher_toggled(Gtk::CheckButton *button);
	void on_launcher_toggled();
	void clear_dialog();
	void set_value(string key, string value);
	void read_config();
	void export_config();
	bool read_values();
	void on_save_dialog_response(const int response_id);
	void clear_all();

	template <class T>
	T *get_widget(const string &widget_name);

public:
	StartupDialog();
	virtual ~StartupDialog();

	string get_cmd() const;

	inline int run()
	{
		return m_dialog->run();
	}

	inline bool is_valid() const
	{
		return m_is_valid;
	}

	inline int num_processes() const
	{
		return m_processes_per_node * m_num_nodes;
	}

	inline bool ssh() const
	{
		return m_ssh;
	}

	inline const char *ssh_address() const
	{
		return m_ssh_address;
	}

	inline const char *ssh_user() const
	{
		return m_ssh_user;
	}

	inline const char *ssh_password() const
	{
		return m_ssh_password;
	}
};

#endif /* STARTUP_HPP */