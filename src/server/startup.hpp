#ifndef STARTUP_HPP
#define STARTUP_HPP

#include "defs.hpp"

class StartupDialog
{
	bool m_is_valid;

	int m_num_processes;
	char *m_client;
	char *m_gdb;
	char *m_socat;
	char *m_target;
	char *m_ip_address;
	bool m_ssh;
	int m_num_nodes;
	char *m_ssh_address;
	char *m_ssh_user;
	char *m_ssh_password;
	char *m_partition;

	Glib::RefPtr<Gtk::Builder> m_builder;
	Gtk::Dialog *m_dialog;

	Gtk::FileChooserButton *m_file_chooser_button;
	Gtk::Entry *m_entry_num_processes;
	Gtk::Entry *m_entry_client;
	Gtk::Entry *m_entry_gdb;
	Gtk::Entry *m_entry_socat;
	Gtk::Entry *m_entry_target;
	Gtk::Entry *m_entry_ip_address;
	Gtk::CheckButton *m_checkbutton_ssh;
	Gtk::Entry *m_entry_num_nodes;
	Gtk::Entry *m_entry_ssh_address;
	Gtk::Entry *m_entry_ssh_user;
	Gtk::Entry *m_entry_ssh_password;
	Gtk::Entry *m_entry_partition;

	void on_dialog_response(const int response_id);
	void set_sensitivity(bool state);
	void on_toggle_button(Gtk::CheckButton *button);
	void clear_dialog();
	void set_value(string key, string value);
	void read_config();

	template <class T>
	T *get_widget(const string &widget_name);

public:
	StartupDialog();
	virtual ~StartupDialog();

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
		return m_ssh ? m_num_nodes * m_num_processes : m_num_processes;
	}

	inline int num_processes_per_node() const
	{
		return m_num_processes;
	}

	inline const char *client() const
	{
		return m_client;
	}

	inline const char *gdb() const
	{
		return m_gdb;
	}

	inline const char *socat() const
	{
		return m_socat;
	}

	inline const char *target() const
	{
		return m_target;
	}

	inline const char *ip_address() const
	{
		return m_ip_address;
	}

	inline bool ssh() const
	{
		return m_ssh;
	}

	inline int num_nodes() const
	{
		return m_num_nodes;
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

	inline const char *partition() const
	{
		return m_partition;
	}
};

#endif /* STARTUP_HPP */