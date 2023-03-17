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
 * @file startup.cpp
 *
 * @brief Contains the implementation of the StartupDialog class.
 *
 * This file contains the implementation of the StartupDialog class.
 */

#include <exception>
#include <fstream>
#include <string>
#include <regex>

#include "startup.hpp"
#include "base64.hpp"

using std::string;
using std::size_t;

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
T *StartupDialog::get_widget(const std::string &widget_name)
{
	T *widget;
	m_builder->get_widget<T>(widget_name, widget);
	return widget;
}

/**
 * This is the default constructor for the StartupDialog class. It will
 * parse the startup_dialog.glade file and renders the startup dialog.
 *
 * On accept the entered configuration will be parsed and saved in the member
 * variables.
 */
StartupDialog::StartupDialog()
	: m_is_valid(false),
	  m_launcher_mpirun(false),
	  m_launcher_srun(false),
	  m_launcher_custom(false),
	  m_launcher_args(""),
	  m_number_of_processes(-1),
	  m_processes_per_node(-1),
	  m_num_nodes(-1),
	  m_ip_address(""),
	  m_base_port(-1),
	  m_slave_path(""),
	  m_target_path(""),
	  m_target_args(""),
	  m_ssh(false),
	  m_ssh_address(""),
	  m_ssh_user(""),
	  m_ssh_password("")
{
	// parse the glade file
	m_builder = Gtk::Builder::create_from_resource("/pgdb/ui/startup_dialog.glade");
	m_dialog = get_widget<Gtk::Dialog>("dialog");

	m_dialog->set_wmclass("Parallel GDB", "Parallel GDB");
	string icon_path = "/usr/share/icons/hicolor/scalable/apps/pgdb.svg";
	if (Glib::file_test(icon_path, Glib::FILE_TEST_EXISTS))
	{
		m_dialog->set_icon_from_file(icon_path);
	}

	// save pointer to widgets to simplify access
	m_radiobutton_mpirun = get_widget<Gtk::RadioButton>("mpirun-radiobutton");
	m_radiobutton_srun = get_widget<Gtk::RadioButton>("srun-radiobutton");
	m_radiobutton_custom = get_widget<Gtk::RadioButton>("custom-radiobutton");
	m_entry_launcher_args = get_widget<Gtk::Entry>("launcher-args-entry");
	m_entry_number_of_processes =
		get_widget<Gtk::Entry>("number-of-processes-entry");
	m_entry_processes_per_node =
		get_widget<Gtk::Entry>("processes-per-node-entry");
	m_entry_num_nodes = get_widget<Gtk::Entry>("num-nodes-entry");
	m_entry_ip_address = get_widget<Gtk::Entry>("ip-address-entry");
	m_entry_base_port = get_widget<Gtk::Entry>("base-port-entry");
	m_entry_slave_path = get_widget<Gtk::Entry>("slave-entry");
	m_entry_target_path = get_widget<Gtk::Entry>("target-entry");
	m_entry_target_args = get_widget<Gtk::Entry>("arguments-entry");
	m_checkbutton_ssh = get_widget<Gtk::CheckButton>("ssh-checkbutton");
	m_entry_ssh_address = get_widget<Gtk::Entry>("ssh-address-entry");
	m_entry_ssh_user = get_widget<Gtk::Entry>("ssh-user-entry");
	m_entry_ssh_password = get_widget<Gtk::Entry>("ssh-password-entry");
	m_config_file_chooser =
		get_widget<Gtk::FileChooserButton>("config-file-chooser");
	m_slave_file_chooser =
		get_widget<Gtk::FileChooserButton>("slave-file-chooser");
	m_target_file_chooser =
		get_widget<Gtk::FileChooserButton>("target-file-chooser");

	// set button and entry sensitivity for empty configuration
	set_sensitivity_ssh(false);

	// connect signal handlers to buttons
	m_checkbutton_ssh->signal_toggled().connect(
		sigc::mem_fun(*this, &StartupDialog::on_ssh_button_toggled));
	m_radiobutton_custom->signal_toggled().connect(
		sigc::mem_fun(*this, &StartupDialog::on_custom_launcher_toggled));
	get_widget<Gtk::Button>("clear-config-button")
		->signal_clicked()
		.connect(sigc::mem_fun(*this, &StartupDialog::clear_all));
	get_widget<Gtk::Button>("export-config-button")
		->signal_clicked()
		.connect(sigc::mem_fun(*this, &StartupDialog::export_config));
	m_config_file_chooser->signal_selection_changed().connect(
		sigc::mem_fun(*this, &StartupDialog::read_config));
	m_slave_file_chooser->signal_selection_changed().connect(
		sigc::bind(sigc::mem_fun(*this,
								 &StartupDialog::set_path),
				   m_slave_file_chooser, m_entry_slave_path));
	m_target_file_chooser->signal_selection_changed().connect(
		sigc::bind(sigc::mem_fun(*this,
								 &StartupDialog::set_path),
				   m_target_file_chooser, m_entry_target_path));
	m_dialog->signal_response().connect(
		sigc::mem_fun(*this, &StartupDialog::on_dialog_response));

	// set default value for base port
	m_entry_base_port->set_text("32768");

	m_dialog->show_all();
}

/**
 * This function closes the startup dialog.
 */
StartupDialog::~StartupDialog()
{
	delete m_dialog;
}

/**
 * This function sets the in the @a file_chooser selected file-path in the
 * @a entry.
 *
 * @param file_chooser The file-chooser to get the file-path from.
 *
 * @param entry The entry to write the file-path to.
 */
void StartupDialog::set_path(Gtk::FileChooserButton *file_chooser,
							 Gtk::Entry *entry)
{
	entry->set_text(file_chooser->get_filename());
}

/**
 * This function resets the dialog to be empty.
 */
void StartupDialog::clear_dialog()
{
	m_radiobutton_mpirun->set_active(true);
	m_radiobutton_srun->set_active(false);
	m_radiobutton_custom->set_active(false);
	m_entry_launcher_args->set_text("");
	m_entry_number_of_processes->set_text("");
	m_entry_processes_per_node->set_text("");
	m_entry_num_nodes->set_text("");
	m_entry_ip_address->set_text("");
	m_entry_base_port->set_text("");
	m_entry_slave_path->set_text("");
	m_entry_target_path->set_text("");
	m_entry_target_args->set_text("");
	m_checkbutton_ssh->set_active(false);
	m_entry_ssh_address->set_text("");
	m_entry_ssh_user->set_text("");
	m_entry_ssh_password->set_text("");

	set_sensitivity_ssh(false);
}

/**
 * This function resets the dialog to be empty and clears the file in the
 * file-chooser-button.
 */
void StartupDialog::clear_all()
{
	clear_dialog();
	m_config_file_chooser->unselect_all();
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
	if ("launcher_args" == key)
		m_entry_launcher_args->set_text(value);
	if ("number_of_processes" == key)
		m_entry_number_of_processes->set_text(value);
	if ("processes_per_node" == key)
		m_entry_processes_per_node->set_text(value);
	if ("num_nodes" == key)
		m_entry_num_nodes->set_text(value);
	if ("ip_address" == key)
		m_entry_ip_address->set_text(value);
	if ("base_port" == key)
		m_entry_base_port->set_text(value);
	if ("slave_path" == key)
		m_entry_slave_path->set_text(value);
	if ("target_path" == key)
		m_entry_target_path->set_text(value);
	if ("target_args" == key)
		m_entry_target_args->set_text(value);
	if ("ssh_address" == key)
		m_entry_ssh_address->set_text(value);
	if ("ssh_user" == key)
		m_entry_ssh_user->set_text(value);
	if ("ssh_password" == key)
		m_entry_ssh_password->set_text(Base64::decode(value));
	if ("launcher" == key)
	{
		if ("mpirun" == value)
		{
			m_radiobutton_mpirun->set_active(true);
		}
		else if ("srun" == value)
		{
			m_radiobutton_srun->set_active(true);
		}
		else if ("custom" == value)
		{
			m_radiobutton_custom->set_active(true);
		}
	}
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
}

/**
 * This function opens a (configuration) file. The content is tokenized and
 * passed to the @ref set_value function.
 */
void StartupDialog::read_config()
{
	clear_dialog();
	std::ifstream config_file(m_config_file_chooser->get_filename());
	string line;
	while (std::getline(config_file, line))
	{
		std::istringstream is_line(line);
		string key;
		if (std::getline(is_line, key, '='))
		{
			string value;
			if (std::getline(is_line, value))
			{
				set_value(key, value);
			}
		}
	}
}

/**
 * This function shows a file-chooser-dialog to export the current configuration
 * as a file.
 */
void StartupDialog::export_config()
{
	// parse current config
	read_values(true);

	Gtk::FileChooserDialog file_chooser_dialog(*dynamic_cast<Gtk::Window *>(m_dialog),
											   string("Select Save Location"),
											   Gtk::FILE_CHOOSER_ACTION_SAVE);
	file_chooser_dialog.add_button("Save", Gtk::RESPONSE_OK);
	file_chooser_dialog.signal_response().connect(sigc::bind(
		sigc::mem_fun(*this, &StartupDialog::on_save_dialog_response),
		&file_chooser_dialog));
	file_chooser_dialog.run();
}

/**
 * This function writes the current configuration as a file. The filename is
 * set by the user in the file-chooser-dialog.
 *
 * @param response_id The response ID.
 *
 * @param file_chooser_dialog The file-chooser-dialog.
 */
void StartupDialog::on_save_dialog_response(const int response_id,
											Gtk::FileChooserDialog *file_chooser_dialog)
{
	if (Gtk::RESPONSE_OK != response_id)
	{
		return;
	}

	// assemble config file content
	string config = "";

	config += "launcher=";
	config += m_launcher_mpirun ? "mpirun" : (m_launcher_srun ? "srun" : "custom");
	config += "\n";

	config += "launcher_args=";
	config += m_launcher_args;
	config += "\n";

	config += "number_of_processes=";
	config += m_number_of_processes > 0 ? std::to_string(m_number_of_processes) : "";
	config += "\n";

	config += "processes_per_node=";
	config += m_processes_per_node > 0 ? std::to_string(m_processes_per_node) : "";
	config += "\n";

	config += "num_nodes=";
	config += m_num_nodes > 0 ? std::to_string(m_num_nodes) : "";
	config += "\n";

	config += "ip_address=";
	config += m_ip_address;
	config += "\n";

	config += "base_port=";
	config += m_base_port >= 0 ? std::to_string(m_base_port) : "";
	config += "\n";

	config += "slave_path=";
	config += m_slave_path;
	config += "\n";

	config += "target_path=";
	config += m_target_path;
	config += "\n";

	config += "target_args=";
	config += m_target_args;
	config += "\n";

	config += "ssh=";
	config += m_ssh ? "true" : "false";
	config += "\n";

	config += "ssh_address=";
	config += m_ssh_address;
	config += "\n";

	config += "ssh_user=";
	config += m_ssh_user;
	config += "\n";

	config += "ssh_password=";
	config += Base64::encode(m_ssh_password);
	config += "\n";

	string filename = file_chooser_dialog->get_filename();
	std::ofstream file(filename);
	file << config;
	file.close();
}

/**
 * This function parses the current configuration and saves it to the member
 * variables for later access. If @a exporting is set to @c true no message
 * dialogs will be shown.
 *
 * @param exporting A flag indicating if parsing to start the application
 * ( @c false ) or exporting the configuration to a file ( @c true ).
 *
 * @return @c true on success, @c false on error.
 */
bool StartupDialog::read_values(const bool exporting)
{
	// get button states
	m_launcher_mpirun = m_radiobutton_mpirun->get_active();
	m_launcher_srun = m_radiobutton_srun->get_active();
	m_launcher_custom = m_radiobutton_custom->get_active();
	m_ssh = m_checkbutton_ssh->get_active();

	// copy new configs
	m_launcher_args = m_entry_launcher_args->get_text();
	m_slave_path = m_entry_slave_path->get_text();
	m_ip_address = m_entry_ip_address->get_text();
	m_target_path = m_entry_target_path->get_text();
	m_target_args = m_entry_target_args->get_text();
	m_ssh_address = m_entry_ssh_address->get_text();
	m_ssh_user = m_entry_ssh_user->get_text();
	m_ssh_password = m_entry_ssh_password->get_text();

	// parse intergers
	try
	{
		size_t pos;
		string text = m_entry_number_of_processes->get_text();
		m_number_of_processes = std::stoi(text, &pos, 10);
		if (pos != text.size() || m_number_of_processes <= 0)
		{
			throw std::exception();
		}
	}
	catch (const std::exception &)
	{
		m_number_of_processes = -1;
	}

	try
	{
		size_t pos;
		string text = m_entry_processes_per_node->get_text();
		m_processes_per_node = std::stoi(text, &pos, 10);
		if (pos != text.size() || m_processes_per_node <= 0)
		{
			throw std::exception();
		}
	}
	catch (const std::exception &)
	{
		m_processes_per_node = -1;
	}

	try
	{
		size_t pos;
		string text = m_entry_num_nodes->get_text();
		m_num_nodes = std::stoi(text, &pos, 10);
		if (pos != text.size() || m_num_nodes <= 0)
		{
			throw std::exception();
		}
	}
	catch (const std::exception &)
	{
		m_num_nodes = -1;
	}

	try
	{
		size_t pos;
		string text = m_entry_base_port->get_text();
		m_base_port = std::stoi(text, &pos, 10);
		if (pos != text.size() || m_base_port < 0 || m_base_port > 0xFFFF)
		{
			throw std::exception();
		}
	}
	catch (const std::exception &)
	{
		m_base_port = -1;
	}

	if (exporting)
	{
		return true;
	}

	if (-1 == m_number_of_processes)
	{
		Gtk::MessageDialog dialog(*dynamic_cast<Gtk::Window *>(m_dialog),
								  "Invalid Number of Processes.", false,
								  Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
		dialog.run();
		return false;
	}
	if (-1 == m_base_port)
	{
		Gtk::MessageDialog dialog(*dynamic_cast<Gtk::Window *>(m_dialog),
								  "Invalid Base Port.", false,
								  Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
		dialog.run();
		return false;
	}

	if (!m_launcher_custom)
	{
		if ("" == m_ip_address)
		{
			Gtk::MessageDialog dialog(*dynamic_cast<Gtk::Window *>(m_dialog),
									  "Missing IP address.", false,
									  Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
			dialog.run();
			return false;
		}
		if ("" == m_slave_path)
		{
			Gtk::MessageDialog dialog(*dynamic_cast<Gtk::Window *>(m_dialog),
									  "Missing Slave Path.", false,
									  Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
			dialog.run();
			return false;
		}
		if ("" == m_target_path)
		{
			Gtk::MessageDialog dialog(*dynamic_cast<Gtk::Window *>(m_dialog),
									  "Missing Target Path.", false,
									  Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
			dialog.run();
			return false;
		}
	}

	return true;
}

/**
 * This function updates the current configuration. It is called, when the user
 * clicks the "Ok" button or closes the dialog.
 *
 * @param response_id The response ID.
 */
void StartupDialog::on_dialog_response(const int response_id)
{
	if (Gtk::RESPONSE_OK != response_id)
	{
		return;
	}

	m_is_valid = read_values(false);
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
void StartupDialog::on_ssh_button_toggled()
{
	set_sensitivity_ssh(m_checkbutton_ssh->get_active());
}

/**
 * This function is the signal handler for the Custom Launcher checkbutton. It
 * sets the sensitivity of the related entries base on the button state.
 *
 * @param[in] button The checkbutton which got clicked.
 */
void StartupDialog::on_custom_launcher_toggled()
{
	const bool state = m_radiobutton_custom->get_active();
	m_entry_processes_per_node->set_sensitive(!state);
	m_entry_num_nodes->set_sensitive(!state);
	m_entry_slave_path->set_sensitive(!state);
	m_entry_ip_address->set_sensitive(!state);
	m_entry_target_path->set_sensitive(!state);
	m_entry_target_args->set_sensitive(!state);
	m_slave_file_chooser->set_sensitive(!state);
	m_target_file_chooser->set_sensitive(!state);
}

/**
 * This function assembles the launcher command from the current configuration.
 *
 * @return The launcher command.
 */
string StartupDialog::get_cmd() const
{
	if (m_launcher_custom)
	{
		return m_launcher_args;
	}

	string cmd = "";

	if (m_launcher_mpirun)
	{
		cmd += "mpirun";

		cmd += " -np ";
		cmd += std::to_string(m_number_of_processes);
	}
	else if (m_launcher_srun)
	{
		cmd += "srun";

		cmd += " --ntasks=";
		cmd += std::to_string(m_number_of_processes);

		if (m_num_nodes > 0)
		{
			cmd += " --nodes=";
			cmd += std::to_string(m_num_nodes);
		}

		if (m_processes_per_node > 0)
		{
			cmd += " --ntasks-per-node=";
			cmd += std::to_string(m_processes_per_node);
		}
	}

	cmd += " ";
	cmd += m_launcher_args;

	cmd += " ";
	cmd += m_slave_path;

	cmd += " -i ";
	cmd += m_ip_address;

	cmd += " -p ";
	cmd += std::to_string(m_base_port);

	cmd += " ";
	cmd += m_target_path;

	cmd += " ";
	cmd += m_target_args;

	// Remove all preceding and trailing whitespace
	cmd = std::regex_replace(cmd, std::regex("^[ \t]+"), "");
	cmd = std::regex_replace(cmd, std::regex("[ \t]+$"), "");
	// Reduces multiple whitespace chars to one space char.
	cmd = std::regex_replace(cmd, std::regex("[ \t]+"), " ");

	return cmd;
}

/**
 * This function returns whether the master should start the slaves. When
 * the custom command is used AND is left blank, the user wants to start
 * the slaves manually.
 *
 * @return Whether the master should start the slaves.
 */
bool StartupDialog::master_starts_slaves() const
{
	string cmd = std::regex_replace(m_launcher_args, std::regex("[ \t]*"), "");
	return !(m_launcher_custom && "" == cmd);
}