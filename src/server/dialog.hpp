#ifndef DIALOG_HPP
#define DIALOG_HPP

#include "defs.hpp"

#define RESPONSE_ID_OK 0

class UIDialog : public Gtk::Dialog
{
	bool m_is_valid;
	bool m_srun;
	int m_num_nodes;
	int m_num_tasks;
	int m_num_processes;
	char *m_client;
	char *m_gdb;
	char *m_socat;
	char *m_target;
	char *m_ip_address;
	char *m_ssh_address;
	char *m_ssh_user;
	char *m_ssh_password;
	char *m_partition;

	Gtk::Grid m_grid;

	Gtk::Label m_label_file_chooser_button;
	Gtk::Label m_label_num_tasks;
	Gtk::Label m_label_client;
	Gtk::Label m_label_gdb;
	Gtk::Label m_label_socat;
	Gtk::Label m_label_target;
	Gtk::Label m_label_ip_address;
	Gtk::Label m_label_srun;
	Gtk::Label m_label_num_nodes;
	Gtk::Label m_label_ssh_address;
	Gtk::Label m_label_ssh_user;
	Gtk::Label m_label_ssh_password;
	Gtk::Label m_label_partition;

	Gtk::FileChooserButton m_file_chooser_button;
	Gtk::Entry m_entry_num_nodes;
	Gtk::Entry m_entry_client;
	Gtk::Entry m_entry_gdb;
	Gtk::Entry m_entry_socat;
	Gtk::Entry m_entry_target;
	Gtk::Entry m_entry_ip_address;
	Gtk::CheckButton m_checkbutton_srun;
	Gtk::Entry m_entry_num_tasks;
	Gtk::Entry m_entry_ssh_address;
	Gtk::Entry m_entry_ssh_user;
	Gtk::Entry m_entry_ssh_password;
	Gtk::Entry m_entry_partition;

	void on_dialog_response(const int response_id);
	void set_sensitivity(bool state);
	void on_toggle_button(Gtk::CheckButton *button);
	void clear_dialog();
	void set_value(string key, string value);
	void read_config();

public:
	UIDialog();
	virtual ~UIDialog();

	inline bool is_valid() const
	{
		return m_is_valid;
	}

	inline int num_nodes() const
	{
		return m_num_nodes;
	}

	inline int num_tasks() const
	{
		return m_num_tasks;
	}

	inline int num_processes() const
	{
		return m_srun ? m_num_nodes * m_num_tasks : m_num_tasks;
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

	inline bool srun() const
	{
		return m_srun;
	}
};

#endif /* DIALOG_HPP */