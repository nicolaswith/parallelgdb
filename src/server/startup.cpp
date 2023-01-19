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

#include <fstream>

#include "startup.hpp"

template <class T>
T *StartupDialog::get_widget(const string &widget_name)
{
	T *widget;
	m_builder->get_widget<T>(widget_name, widget);
	return widget;
}

StartupDialog::StartupDialog()
	: m_is_valid(false),
	  m_processes_per_node(0),
	  m_num_nodes(0),
	  m_mpirun(false),
	  m_srun(false),
	  m_client(nullptr),
	  m_gdb(nullptr),
	  m_socat(nullptr),
	  m_target(nullptr),
	  m_ip_address(nullptr),
	  m_ssh(false),
	  m_ssh_address(nullptr),
	  m_ssh_user(nullptr),
	  m_ssh_password(nullptr),
	  m_partition(nullptr),
	  m_custom_launcher(false),
	  m_launcher_cmd(nullptr)
{
	m_builder = Gtk::Builder::create_from_file("./ui/startup_dialog.glade");
	m_dialog = get_widget<Gtk::Dialog>("dialog");

	m_file_chooser_button = get_widget<Gtk::FileChooserButton>("file-chooser-button");
	m_entry_processes_per_node = get_widget<Gtk::Entry>("processes-per-node-entry");
	m_entry_num_nodes = get_widget<Gtk::Entry>("num-nodes-entry");
	m_radiobutton_mpirun = get_widget<Gtk::RadioButton>("mpirun-radiobutton");
	m_radiobutton_srun = get_widget<Gtk::RadioButton>("srun-radiobutton");
	m_entry_client = get_widget<Gtk::Entry>("client-entry");
	m_entry_gdb = get_widget<Gtk::Entry>("gdb-entry");
	m_entry_socat = get_widget<Gtk::Entry>("socat-entry");
	m_entry_target = get_widget<Gtk::Entry>("target-entry");
	m_entry_ip_address = get_widget<Gtk::Entry>("ip-address-entry");
	m_checkbutton_ssh = get_widget<Gtk::CheckButton>("ssh-checkbutton");
	m_entry_ssh_address = get_widget<Gtk::Entry>("ssh-address-entry");
	m_entry_ssh_user = get_widget<Gtk::Entry>("ssh-user-entry");
	m_entry_ssh_password = get_widget<Gtk::Entry>("ssh-password-entry");
	m_entry_partition = get_widget<Gtk::Entry>("partition-entry");
	m_checkbutton_launcher = get_widget<Gtk::CheckButton>("launcher-checkbutton");
	m_entry_launcher = get_widget<Gtk::Entry>("launcher-entry");

	m_entry_partition->set_sensitive(false);
	set_sensitivity_ssh(false);
	m_entry_launcher->set_sensitive(false);

	m_radiobutton_mpirun->signal_toggled().connect(sigc::mem_fun(*this, &StartupDialog::on_launcher_toggled));
	m_radiobutton_srun->signal_toggled().connect(sigc::mem_fun(*this, &StartupDialog::on_launcher_toggled));
	m_checkbutton_ssh->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &StartupDialog::on_ssh_button_toggled), m_checkbutton_ssh));
	m_checkbutton_launcher->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &StartupDialog::on_custom_launcher_toggled), m_checkbutton_launcher));

	get_widget<Gtk::Button>("clear-config-button")->signal_clicked().connect(sigc::mem_fun(*this, &StartupDialog::clear_all));
	get_widget<Gtk::Button>("export-config-button")->signal_clicked().connect(sigc::mem_fun(*this, &StartupDialog::export_config));

	m_file_chooser_button->signal_selection_changed().connect(sigc::mem_fun(*this, &StartupDialog::read_config));
	m_dialog->signal_response().connect(sigc::mem_fun(*this, &StartupDialog::on_dialog_response));

	m_dialog->show_all();
}

StartupDialog::~StartupDialog()
{
	free(m_client);
	free(m_gdb);
	free(m_socat);
	free(m_target);
	free(m_ip_address);
	free(m_ssh_address);
	free(m_ssh_user);
	free(m_ssh_password);
	free(m_partition);
	delete m_dialog;
}

void StartupDialog::clear_dialog()
{
	m_entry_processes_per_node->set_text("");
	m_entry_num_nodes->set_text("");
	m_radiobutton_mpirun->set_active(true);
	m_radiobutton_srun->set_active(false);
	m_entry_client->set_text("");
	m_entry_gdb->set_text("");
	m_entry_socat->set_text("");
	m_entry_target->set_text("");
	m_entry_ip_address->set_text("");
	m_checkbutton_ssh->set_active(false);
	m_entry_ssh_address->set_text("");
	m_entry_ssh_user->set_text("");
	m_entry_ssh_password->set_text("");
	m_entry_partition->set_text("");
	m_checkbutton_launcher->set_active(false);
	m_entry_launcher->set_text("");

	m_entry_partition->set_sensitive(false);
	set_sensitivity_ssh(false);
	m_entry_launcher->set_sensitive(false);
}

void StartupDialog::clear_all()
{
	clear_dialog();
	m_file_chooser_button->unselect_all();
}

void StartupDialog::set_value(string key, string value)
{
	if ("processes_per_node" == key)
		m_entry_processes_per_node->set_text(value);
	if ("num_nodes" == key)
		m_entry_num_nodes->set_text(value);
	if ("client" == key)
		m_entry_client->set_text(value);
	if ("gdb" == key)
		m_entry_gdb->set_text(value);
	if ("socat" == key)
		m_entry_socat->set_text(value);
	if ("target" == key)
		m_entry_target->set_text(value);
	if ("ip_address" == key)
		m_entry_ip_address->set_text(value);
	if ("ssh_address" == key)
		m_entry_ssh_address->set_text(value);
	if ("ssh_user" == key)
		m_entry_ssh_user->set_text(value);
	if ("ssh_password" == key)
		m_entry_ssh_password->set_text(value);
	if ("partition" == key)
		m_entry_partition->set_text(value);
	if ("custom_cmd" == key)
		m_entry_launcher->set_text(value);
	if ("ssh" == key)
	{
		if ("true" == value)
		{
			m_checkbutton_ssh->set_active(true);
			set_sensitivity_ssh(true);
		}
		else
		{
			m_checkbutton_ssh->set_active(false);
			set_sensitivity_ssh(false);
		}
	}
	if ("launcher" == key)
	{
		if ("mpirun" == value)
		{
			m_radiobutton_mpirun->set_active(true);
			m_radiobutton_srun->set_active(false);
		}
		else if ("srun" == value)
		{
			m_radiobutton_mpirun->set_active(false);
			m_radiobutton_srun->set_active(true);
		}
	}
	if ("custom_launcher" == key)
	{
		if ("true" == value)
		{
			m_checkbutton_launcher->set_active(true);
			m_entry_launcher->set_sensitive(true);
		}
		else
		{
			m_checkbutton_launcher->set_active(false);
			m_entry_launcher->set_sensitive(true);
		}
	}
}

void StartupDialog::read_config()
{
	FILE *f = fopen(m_file_chooser_button->get_filename().c_str(), "r");
	if (!f)
	{
		return;
	}
	clear_dialog();
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	char *config = new char[size + 8];
	rewind(f);
	fread(config, sizeof(char), size, f);
	config[size] = '\n';
	config[size + 1] = '\0';
	std::istringstream is_file(config);
	std::string line;
	while (std::getline(is_file, line))
	{
		std::istringstream is_line(line);
		std::string key;
		if (std::getline(is_line, key, '='))
		{
			std::string value;
			if (std::getline(is_line, value))
			{
				set_value(key, value);
			}
		}
	}
	delete[] config;
}

void StartupDialog::export_config()
{
	if (!read_values())
	{
		return;
	}

	m_config = "";

	m_config += "processes_per_node=";
	m_config += std::to_string(m_processes_per_node);
	m_config += "\n";

	m_config += "num_nodes=";
	m_config += std::to_string(m_num_nodes);
	m_config += "\n";

	m_config += "launcher=";
	m_config += m_mpirun ? "mpirun" : "srun";
	m_config += "\n";

	m_config += "client=";
	m_config += m_client ? m_client : "";
	m_config += "\n";

	m_config += "gdb=";
	m_config += m_gdb ? m_gdb : "";
	m_config += "\n";

	m_config += "socat=";
	m_config += m_socat ? m_socat : "";
	m_config += "\n";

	m_config += "target=";
	m_config += m_target ? m_target : "";
	m_config += "\n";

	m_config += "ip_address=";
	m_config += m_ip_address ? m_ip_address : "";
	m_config += "\n";

	m_config += "ssh=";
	m_config += m_ssh ? "true" : "false";
	m_config += "\n";

	m_config += "ssh_address=";
	m_config += m_ssh_address ? m_ssh_address : "";
	m_config += "\n";

	m_config += "ssh_user=";
	m_config += m_ssh_user ? m_ssh_user : "";
	m_config += "\n";

	m_config += "ssh_password=";
	m_config += m_ssh_password ? m_ssh_password : "";
	m_config += "\n";

	m_config += "partition=";
	m_config += m_partition ? m_partition : "";
	m_config += "\n";

	m_config += "custom_launcher=";
	m_config += m_custom_launcher ? "true" : "false";
	m_config += "\n";

	m_config += "custom_cmd=";
	m_config += m_launcher_cmd ? m_launcher_cmd : "";
	m_config += "\n\n";

	m_file_chooser_dialog = new Gtk::FileChooserDialog(*dynamic_cast<Gtk::Window *>(m_dialog), string("Select Save Location"), Gtk::FILE_CHOOSER_ACTION_SAVE);
	m_file_chooser_dialog->add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);
	m_file_chooser_dialog->signal_response().connect(sigc::mem_fun(*this, &StartupDialog::on_save_dialog_response));
	m_file_chooser_dialog->run();
	delete m_file_chooser_dialog;
}

void StartupDialog::on_save_dialog_response(const int response_id)
{
	if (Gtk::RESPONSE_OK != response_id)
	{
		return;
	}

	string filename = m_file_chooser_dialog->get_filename();

	std::ofstream file(filename);
	file << m_config;
	file.close();
}

bool StartupDialog::read_values()
{
	m_mpirun = m_radiobutton_mpirun->get_active();
	m_srun = m_radiobutton_srun->get_active();
	m_ssh = m_checkbutton_ssh->get_active();
	m_custom_launcher = m_checkbutton_launcher->get_active();

	free(m_client);
	free(m_gdb);
	free(m_socat);
	free(m_target);
	free(m_ip_address);
	free(m_ssh_address);
	free(m_ssh_user);
	free(m_ssh_password);
	free(m_partition);
	free(m_launcher_cmd);

	m_client = strdup(m_entry_client->get_text().c_str());
	m_gdb = strdup(m_entry_gdb->get_text().c_str());
	m_socat = strdup(m_entry_socat->get_text().c_str());
	m_target = strdup(m_entry_target->get_text().c_str());
	m_ip_address = strdup(m_entry_ip_address->get_text().c_str());
	m_ssh_address = strdup(m_entry_ssh_address->get_text().c_str());
	m_ssh_user = strdup(m_entry_ssh_user->get_text().c_str());
	m_ssh_password = strdup(m_entry_ssh_password->get_text().c_str());
	m_partition = strdup(m_entry_partition->get_text().c_str());
	m_launcher_cmd = strdup(m_entry_launcher->get_text().c_str());

	try
	{
		std::size_t pos;
		string text = m_entry_processes_per_node->get_text();
		m_processes_per_node = std::stoi(text, &pos, 10);
		if (pos != text.size())
		{
			throw std::exception();
		}
	}
	catch (const std::exception &e)
	{
		Gtk::MessageDialog dialog(*dynamic_cast<Gtk::Window *>(m_dialog), "Invalid Number: Processes per Node.", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
		dialog.run();
		return false;
	}

	try
	{
		std::size_t pos;
		string text = m_entry_num_nodes->get_text();
		m_num_nodes = std::stoi(text, &pos, 10);
		if (pos != text.size())
		{
			throw std::exception();
		}
	}
	catch (const std::exception &e)
	{
		Gtk::MessageDialog dialog(*dynamic_cast<Gtk::Window *>(m_dialog), "Invalid Number: Number of Nodes.", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
		dialog.run();
		return false;
	}

	return true;
}

void StartupDialog::on_dialog_response(const int response_id)
{
	if (Gtk::RESPONSE_OK != response_id)
	{
		return;
	}

	m_is_valid = read_values();
}

void StartupDialog::set_sensitivity_ssh(bool state)
{
	m_entry_ssh_address->set_sensitive(state);
	m_entry_ssh_user->set_sensitive(state);
	m_entry_ssh_password->set_sensitive(state);
}

void StartupDialog::on_ssh_button_toggled(Gtk::CheckButton *button)
{
	set_sensitivity_ssh(button->get_active());
}

void StartupDialog::on_custom_launcher_toggled(Gtk::CheckButton *button)
{
	m_entry_launcher->set_sensitive(button->get_active());
}

void StartupDialog::on_launcher_toggled()
{
	if (m_radiobutton_mpirun->get_active())
	{
		m_entry_partition->set_sensitive(false);
	}
	else if (m_radiobutton_srun->get_active())
	{
		m_entry_partition->set_sensitive(true);
	}
}

string StartupDialog::get_cmd() const
{
	if (m_custom_launcher)
	{
		return m_entry_launcher->get_text();
	}

	string cmd = "";

	if (m_mpirun)
	{
		cmd += "/usr/bin/mpirun";
		cmd += " --oversubscribe";

		cmd += " -np ";
		cmd += std::to_string(num_processes());
	}
	else if (m_srun)
	{
		cmd += "/usr/bin/srun";

		cmd += " --mpi=";
		cmd += "pmi2";

		cmd += " --nodes=";
		cmd += std::to_string(m_num_nodes);

		cmd += " --ntasks-per-node=";
		cmd += std::to_string(m_processes_per_node);

		if (m_partition && string(m_partition) != string(""))
		{
			cmd += " --partition=";
			cmd += m_partition;
		}
	}

	cmd += " ";
	cmd += m_client;

	cmd += " -s ";
	cmd += m_socat;

	cmd += " -g ";
	cmd += m_gdb;

	cmd += " -i ";
	cmd += m_ip_address;

	cmd += " ";
	cmd += m_target;

	return cmd;
}