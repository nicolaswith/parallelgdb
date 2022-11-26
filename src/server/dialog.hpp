#include "defs.hpp"

#ifndef _DIALOG_HPP
#define _DIALOG_HPP

#define RESPONSE_ID_OK 0

class UIDialog : public Gtk::Dialog
{
public:
	bool m_is_valid;

private:
	int m_num_nodes;
	int m_num_tasks;
	int m_num_processes;
	char *m_client;
	char *m_target;
	char *m_ip_address;
	bool m_srun;
	char *m_ssh_address;
	char *m_ssh_user;
	char *m_ssh_password;
	char *m_partition;

	Gtk::Grid m_grid;
	Gtk::Label m_label_num_nodes;
	Gtk::Entry m_entry_num_nodes;
	Gtk::Label m_label_num_tasks;
	Gtk::Entry m_entry_num_tasks;
	Gtk::Label m_label_client;
	Gtk::Entry m_entry_client;
	Gtk::Label m_label_target;
	Gtk::Entry m_entry_target;
	Gtk::Label m_label_srun;
	Gtk::CheckButton m_checkbutton_srun;
	Gtk::Label m_label_ip_address;
	Gtk::Entry m_entry_ip_address;
	Gtk::Label m_label_ssh_address;
	Gtk::Entry m_entry_ssh_address;
	Gtk::Label m_label_ssh_user;
	Gtk::Entry m_entry_ssh_user;
	Gtk::Label m_label_ssh_password;
	Gtk::Entry m_entry_ssh_password;
	Gtk::Label m_label_partition;
	Gtk::Entry m_entry_partition;

private:
	void on_dialog_response(const int a_response_id);
	void set_sensitivity(bool a_state);
	void on_toggle_button(Gtk::CheckButton *a_button);
	void store_line(string a_key, string a_value);
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

#endif /* _DIALOG_HPP */