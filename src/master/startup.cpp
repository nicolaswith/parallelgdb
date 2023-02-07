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
 * @file startup.cpp
 *
 * @brief Contains the implementation of the StartupDialog class.
 *
 * This file contains the implementation of the StartupDialog class.
 */

#include <fstream>
#include <string>
#include <regex>

#include "startup.hpp"
#include "base64.hpp"

using std::string;

/// Wrapper for the Gtk::get_widget function.
/**
 * This function is a wrapper for the Gtk::get_widget function.
 *
 * @tparam T The widget class.
 *
 * @param[in] widget_name The widget name.
 *
 * @return The pointer to the widget object on success, @c nullptr on error.
 */
template <class T>
T *StartupDialog::get_widget(const string &widget_name)
{
	T *widget;
	m_builder->get_widget<T>(widget_name, widget);
	return widget;
}

/**
 * This is the default constructor for the StartupDialog class. It will
 * parse the startup_dialog.glade file and render the GUI dialog.
 *
 * On accept the entered configuration will be parsed and saved in the member
 * variables.
 */
StartupDialog::StartupDialog()
	: m_is_valid(false),
	  m_processes_per_node(0),
	  m_num_nodes(0),
	  m_mpirun(false),
	  m_srun(false),
	  m_oversubscribe(false),
	  m_slave(nullptr),
	  m_target(nullptr),
	  m_arguments(nullptr),
	  m_ip_address(nullptr),
	  m_ssh(false),
	  m_ssh_address(nullptr),
	  m_ssh_user(nullptr),
	  m_ssh_password(nullptr),
	  m_partition(nullptr),
	  m_custom_launcher(false),
	  m_launcher_cmd("")
{
	// parse the glade file
	m_builder = Gtk::Builder::create_from_file("./ui/startup_dialog.glade");
	m_dialog = get_widget<Gtk::Dialog>("dialog");

	// save pointer to widgets to simplify access
	m_file_chooser_button =
		get_widget<Gtk::FileChooserButton>("file-chooser-button");
	m_entry_processes_per_node =
		get_widget<Gtk::Entry>("processes-per-node-entry");
	m_entry_num_nodes = get_widget<Gtk::Entry>("num-nodes-entry");
	m_radiobutton_mpirun = get_widget<Gtk::RadioButton>("mpirun-radiobutton");
	m_radiobutton_srun = get_widget<Gtk::RadioButton>("srun-radiobutton");
	m_checkbutton_oversubscribe =
		get_widget<Gtk::CheckButton>("oversubscribe-checkbutton");
	m_entry_slave = get_widget<Gtk::Entry>("slave-entry");
	m_entry_target = get_widget<Gtk::Entry>("target-entry");
	m_entry_arguments = get_widget<Gtk::Entry>("arguments-entry");
	m_entry_ip_address = get_widget<Gtk::Entry>("ip-address-entry");
	m_checkbutton_ssh = get_widget<Gtk::CheckButton>("ssh-checkbutton");
	m_entry_ssh_address = get_widget<Gtk::Entry>("ssh-address-entry");
	m_entry_ssh_user = get_widget<Gtk::Entry>("ssh-user-entry");
	m_entry_ssh_password = get_widget<Gtk::Entry>("ssh-password-entry");
	m_entry_partition = get_widget<Gtk::Entry>("partition-entry");
	m_checkbutton_launcher = get_widget<Gtk::CheckButton>("launcher-checkbutton");
	m_entry_launcher = get_widget<Gtk::Entry>("launcher-entry");

	// set button and entry sensitivity for empty configuration
	m_entry_partition->set_sensitive(false);
	set_sensitivity_ssh(false);
	m_entry_launcher->set_sensitive(false);

	// connect signal handlers to buttons
	m_radiobutton_mpirun->signal_toggled().connect(
		sigc::mem_fun(*this, &StartupDialog::on_launcher_toggled));
	m_radiobutton_srun->signal_toggled().connect(
		sigc::mem_fun(*this, &StartupDialog::on_launcher_toggled));
	m_checkbutton_ssh->signal_toggled().connect(
		sigc::bind(sigc::mem_fun(*this, &StartupDialog::on_ssh_button_toggled),
				   m_checkbutton_ssh));
	m_checkbutton_launcher->signal_toggled().connect(sigc::bind(
		sigc::mem_fun(*this, &StartupDialog::on_custom_launcher_toggled),
		m_checkbutton_launcher));

	get_widget<Gtk::Button>("clear-config-button")
		->signal_clicked()
		.connect(sigc::mem_fun(*this, &StartupDialog::clear_all));
	get_widget<Gtk::Button>("export-config-button")
		->signal_clicked()
		.connect(sigc::mem_fun(*this, &StartupDialog::export_config));

	m_file_chooser_button->signal_selection_changed().connect(
		sigc::mem_fun(*this, &StartupDialog::read_config));
	m_dialog->signal_response().connect(
		sigc::mem_fun(*this, &StartupDialog::on_dialog_response));

	m_dialog->show_all();
}

/**
 * This function frees the char arrays and closes the dialog.
 */
StartupDialog::~StartupDialog()
{
	free(m_slave);
	free(m_target);
	free(m_arguments);
	free(m_ip_address);
	free(m_ssh_address);
	free(m_ssh_user);
	free(m_ssh_password);
	free(m_partition);
	delete m_dialog;
}

/**
 * This function resets the dialog to be empty.
 */
void StartupDialog::clear_dialog()
{
	m_entry_processes_per_node->set_text("");
	m_entry_num_nodes->set_text("");
	m_radiobutton_mpirun->set_active(true);
	m_radiobutton_srun->set_active(false);
	m_checkbutton_oversubscribe->set_active(false);
	m_entry_slave->set_text("");
	m_entry_target->set_text("");
	m_entry_arguments->set_text("");
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

/**
 * This function resets the dialog to be empty and clears the file in the
 * file-chooser-button.
 */
void StartupDialog::clear_all()
{
	clear_dialog();
	m_file_chooser_button->unselect_all();
}

/**
 * This function sets a value to an widget in the dialog. For entries the values
 * are strings, for checkbuttons the value is parsed and based on that, the
 * checkbutton is activated or not. If the identifier is unknown nothing
 * is done.
 *
 * @param[in] key The identifier for the widget.
 *
 * @param[in] value The value to be set.
 */
void StartupDialog::set_value(const std::string &key, const std::string &value)
{
	if ("processes_per_node" == key)
		m_entry_processes_per_node->set_text(value);
	if ("num_nodes" == key)
		m_entry_num_nodes->set_text(value);
	if ("slave" == key)
		m_entry_slave->set_text(value);
	if ("target" == key)
		m_entry_target->set_text(value);
	if ("arguments" == key)
		m_entry_arguments->set_text(value);
	if ("ip_address" == key)
		m_entry_ip_address->set_text(value);
	if ("ssh_address" == key)
		m_entry_ssh_address->set_text(value);
	if ("ssh_user" == key)
		m_entry_ssh_user->set_text(value);
	if ("ssh_password" == key)
		m_entry_ssh_password->set_text(Base64::decode(value));
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
	if ("oversubscribe" == key)
	{
		if ("true" == value)
		{
			m_checkbutton_oversubscribe->set_active(true);
		}
		else
		{
			m_checkbutton_oversubscribe->set_active(false);
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

/**
 * This function opens a (configuration) file and reads its entire content.
 * The content is then tokenized and passed to the @ref set_value function.
 */
void StartupDialog::read_config()
{
	// open config file
	FILE *f = fopen(m_file_chooser_button->get_filename().c_str(), "r");
	if (!f)
	{
		return;
	}

	// only clear dialog if successfully opened the config file
	clear_dialog();

	// get file size
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	char *config = new char[size + 8];
	rewind(f);

	// read in file
	fread(config, sizeof(char), size, f);
	config[size] = '\n';
	config[size + 1] = '\0';

	// tokenize config file
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

/**
 * This function prepares the content of the file and saves it in @ref m_config.
 * After that a save dialog is shown, where the user specifies the filename.
 * On accepting this dialog the @ref on_save_dialog_response function is called,
 * where the file is actually written.
 */
void StartupDialog::export_config()
{
	// parse current config
	if (!read_values())
	{
		return;
	}

	// assemble config file content
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

	m_config += "oversubscribe=";
	m_config += m_oversubscribe ? "true" : "false";
	m_config += "\n";

	m_config += "slave=";
	m_config += m_slave ? m_slave : "";
	m_config += "\n";

	m_config += "target=";
	m_config += m_target ? m_target : "";
	m_config += "\n";

	m_config += "arguments=";
	m_config += m_arguments ? m_arguments : "";
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
	m_config += m_ssh_password ? Base64::encode(m_ssh_password) : "";
	m_config += "\n";

	m_config += "partition=";
	m_config += m_partition ? m_partition : "";
	m_config += "\n";

	m_config += "custom_launcher=";
	m_config += m_custom_launcher ? "true" : "false";
	m_config += "\n";

	m_config += "custom_cmd=";
	m_config += m_launcher_cmd;
	m_config += "\n\n";

	// open file-saver dialog
	m_file_chooser_dialog = new Gtk::FileChooserDialog(
		*dynamic_cast<Gtk::Window *>(m_dialog), string("Select Save Location"),
		Gtk::FILE_CHOOSER_ACTION_SAVE);
	m_file_chooser_dialog->add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);
	m_file_chooser_dialog->signal_response().connect(
		sigc::mem_fun(*this, &StartupDialog::on_save_dialog_response));
	m_file_chooser_dialog->run();
	delete m_file_chooser_dialog;
}

/**
 * This function writes the current configuration as a file. The filename is
 * set by the user in the file-saver-dialog.
 *
 * @param response_id The response ID.
 */
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

/**
 * This function parses the current configuration. Empty entries are valid,
 * except the processes_per_node and num_nodes entries, as the are parsed as an
 * integer.
 *
 * @return @c true on success, @c false on error.
 */
bool StartupDialog::read_values()
{
	// get button states
	m_mpirun = m_radiobutton_mpirun->get_active();
	m_srun = m_radiobutton_srun->get_active();
	m_oversubscribe = m_checkbutton_oversubscribe->get_active();
	m_ssh = m_checkbutton_ssh->get_active();
	m_custom_launcher = m_checkbutton_launcher->get_active();

	// clear old configs
	free(m_slave);
	free(m_target);
	free(m_arguments);
	free(m_ip_address);
	free(m_ssh_address);
	free(m_ssh_user);
	free(m_ssh_password);
	free(m_partition);

	// copy new configs
	m_slave = strdup(m_entry_slave->get_text().c_str());
	m_target = strdup(m_entry_target->get_text().c_str());
	m_arguments = strdup(m_entry_arguments->get_text().c_str());
	m_ip_address = strdup(m_entry_ip_address->get_text().c_str());
	m_ssh_address = strdup(m_entry_ssh_address->get_text().c_str());
	m_ssh_user = strdup(m_entry_ssh_user->get_text().c_str());
	m_ssh_password = strdup(m_entry_ssh_password->get_text().c_str());
	m_partition = strdup(m_entry_partition->get_text().c_str());

	// trim custom command
	m_launcher_cmd = m_entry_launcher->get_text();
	m_launcher_cmd = std::regex_replace(m_launcher_cmd, std::regex("^[ \t]+"), "");
	m_launcher_cmd = std::regex_replace(m_launcher_cmd, std::regex("[ \t]+$"), "");
	m_launcher_cmd = std::regex_replace(m_launcher_cmd, std::regex("[ \t]+"), " ");

	// parse intergers
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
		Gtk::MessageDialog dialog(*dynamic_cast<Gtk::Window *>(m_dialog),
								  "Invalid Number: Processes per Node.", false,
								  Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
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
		Gtk::MessageDialog dialog(*dynamic_cast<Gtk::Window *>(m_dialog),
								  "Invalid Number: Number of Nodes.", false,
								  Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
		dialog.run();
		return false;
	}

	return true;
}

/**
 * This function updates the stored configuration ( @a m_config ) with the
 * current configuration. It is called, when the user clicks the "Ok" button
 * or closes the dialog.
 *
 * @param response_id The response ID.
 */
void StartupDialog::on_dialog_response(const int response_id)
{
	if (Gtk::RESPONSE_OK != response_id)
	{
		return;
	}

	m_is_valid = read_values();
}

/**
 * This function sets the sensitivity of the SSH related widgets.
 *
 * @param state The desired sensitivity state. @c true for editable, @c false
 * to disable.
 */
void StartupDialog::set_sensitivity_ssh(const bool state)
{
	m_entry_ssh_address->set_sensitive(state);
	m_entry_ssh_user->set_sensitive(state);
	m_entry_ssh_password->set_sensitive(state);
}

/**
 * This function is the signal handler for the SSH checkbutton. It sets the
 * sensitivity of the related entries base on the button state.
 *
 * @param[in] button The checkbutton which got clicked.
 */
void StartupDialog::on_ssh_button_toggled(Gtk::CheckButton *button)
{
	set_sensitivity_ssh(button->get_active());
}

/**
 * This function is the signal handler for the Custom Launcher checkbutton. It
 * sets the sensitivity of the related entries base on the button state.
 *
 * @param[in] button The checkbutton which got clicked.
 */
void StartupDialog::on_custom_launcher_toggled(Gtk::CheckButton *button)
{
	m_entry_launcher->set_sensitive(button->get_active());
}

/**
 * This function sets the correct sensitivity for the launcher related widgets
 * when a launcher is selected.
 */
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

/**
 * This function assembles the launcher command from the current configuration.
 *
 * @return The launcher command.
 */
string StartupDialog::get_cmd() const
{
	if (m_custom_launcher)
	{
		return m_launcher_cmd;
	}

	string cmd = "";

	if (m_mpirun)
	{
		cmd += "mpirun";
		if (m_oversubscribe)
		{
			cmd += " --oversubscribe";
		}

		cmd += " -np ";
		cmd += std::to_string(num_processes());
	}
	else if (m_srun)
	{
		cmd += "srun";

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
	cmd += m_slave;

	cmd += " -i ";
	cmd += m_ip_address;

	cmd += " ";
	cmd += m_target;

	cmd += " ";
	cmd += m_arguments;

	return cmd;
}