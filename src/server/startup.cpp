#include "startup.hpp"

StartupDialog::StartupDialog()
	: m_is_valid(false),
	  m_srun(false),
	  m_client(nullptr),
	  m_gdb(nullptr),
	  m_socat(nullptr),
	  m_target(nullptr),
	  m_ip_address(nullptr),
	  m_ssh_address(nullptr),
	  m_ssh_user(nullptr),
	  m_ssh_password(nullptr),
	  m_partition(nullptr),
	  m_label_file_chooser_button("Config File:", Gtk::ALIGN_START),
	  m_label_num_tasks("Number of tasks:", Gtk::ALIGN_START),
	  m_label_client("Client:", Gtk::ALIGN_START),
	  m_label_gdb("GDB:", Gtk::ALIGN_START),
	  m_label_socat("Socat:", Gtk::ALIGN_START),
	  m_label_target("Target:", Gtk::ALIGN_START),
	  m_label_ip_address("IP address:", Gtk::ALIGN_START),
	  m_label_srun("Use srun (ssh):", Gtk::ALIGN_START),
	  m_label_num_nodes("Number of nodes:", Gtk::ALIGN_START),
	  m_label_ssh_address("SSH address:", Gtk::ALIGN_START),
	  m_label_ssh_user("SSH user name:", Gtk::ALIGN_START),
	  m_label_ssh_password("SSH password:", Gtk::ALIGN_START),
	  m_label_partition("Partition:", Gtk::ALIGN_START)
{
	this->get_content_area()->add(m_grid);

	m_grid.set_margin_left(10);
	m_grid.set_margin_top(10);
	m_grid.set_margin_right(10);
	m_grid.set_margin_bottom(10);
	m_grid.set_row_spacing(10);
	m_grid.set_column_spacing(10);

	m_entry_ssh_password.set_visibility(false);

	int row = 0;
	m_grid.attach(m_label_file_chooser_button, 0, row);
	m_grid.attach(m_file_chooser_button, 1, row++);
	m_grid.attach(m_label_num_tasks, 0, row);
	m_grid.attach(m_entry_num_tasks, 1, row++);
	m_grid.attach(m_label_client, 0, row);
	m_grid.attach(m_entry_client, 1, row++);
	m_grid.attach(m_label_gdb, 0, row);
	m_grid.attach(m_entry_gdb, 1, row++);
	m_grid.attach(m_label_socat, 0, row);
	m_grid.attach(m_entry_socat, 1, row++);
	m_grid.attach(m_label_target, 0, row);
	m_grid.attach(m_entry_target, 1, row++);
	m_grid.attach(m_label_ip_address, 0, row);
	m_grid.attach(m_entry_ip_address, 1, row++);
	m_grid.attach(m_label_srun, 0, row);
	m_grid.attach(m_checkbutton_srun, 1, row++);
	m_grid.attach(m_label_num_nodes, 0, row);
	m_grid.attach(m_entry_num_nodes, 1, row++);
	m_grid.attach(m_label_ssh_address, 0, row);
	m_grid.attach(m_entry_ssh_address, 1, row++);
	m_grid.attach(m_label_ssh_user, 0, row);
	m_grid.attach(m_entry_ssh_user, 1, row++);
	m_grid.attach(m_label_ssh_password, 0, row);
	m_grid.attach(m_entry_ssh_password, 1, row++);
	m_grid.attach(m_label_partition, 0, row);
	m_grid.attach(m_entry_partition, 1, row++);

	m_file_chooser_button.set_title("Select Config File");
	m_file_chooser_button.signal_selection_changed().connect(sigc::mem_fun(*this, &StartupDialog::read_config));
	m_file_chooser_button.set_filename("/home/nicolas/ma/parallelgdb/configs/config_mpi");
	// m_file_chooser_button.set_filename("/home/nicolas/ma/parallelgdb/configs/config_ssh");
	// m_file_chooser_button.set_filename("/home/nicolas/ma/parallelgdb/configs/config_ants");

	set_sensitivity(false);
	this->add_button("Ok", RESPONSE_ID_OK);

	this->signal_response().connect(sigc::mem_fun(*this, &StartupDialog::on_dialog_response));
	m_checkbutton_srun.signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &StartupDialog::on_toggle_button), &m_checkbutton_srun));

	this->show_all();
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
}

void StartupDialog::clear_dialog()
{
	m_entry_num_nodes.set_text("");
	m_entry_num_tasks.set_text("");
	m_entry_client.set_text("");
	m_entry_gdb.set_text("");
	m_entry_socat.set_text("");
	m_entry_target.set_text("");
	m_entry_ip_address.set_text("");
	m_entry_ssh_address.set_text("");
	m_entry_ssh_user.set_text("");
	m_entry_ssh_password.set_text("");
	m_entry_partition.set_text("");
	m_checkbutton_srun.set_active(false);
	set_sensitivity(false);
}

void StartupDialog::set_value(string key, string value)
{
	if ("num_nodes" == key)
		m_entry_num_nodes.set_text(value);
	if ("num_tasks" == key)
		m_entry_num_tasks.set_text(value);
	if ("client" == key)
		m_entry_client.set_text(value);
	if ("gdb" == key)
		m_entry_gdb.set_text(value);
	if ("socat" == key)
		m_entry_socat.set_text(value);
	if ("target" == key)
		m_entry_target.set_text(value);
	if ("ip_address" == key)
		m_entry_ip_address.set_text(value);
	if ("ssh_address" == key)
		m_entry_ssh_address.set_text(value);
	if ("ssh_user" == key)
		m_entry_ssh_user.set_text(value);
	if ("ssh_password" == key)
		m_entry_ssh_password.set_text(value);
	if ("partition" == key)
		m_entry_partition.set_text(value);
	if ("srun" == key)
	{
		if ("true" == value)
		{
			m_checkbutton_srun.set_active(true);
			set_sensitivity(true);
		}
		else
		{
			set_sensitivity(false);
		}
	}
}

void StartupDialog::read_config()
{
	FILE *f = fopen(m_file_chooser_button.get_filename().c_str(), "r");
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

void StartupDialog::on_dialog_response(const int response_id)
{
	if (RESPONSE_ID_OK != response_id)
	{
		return;
	}

	m_srun = m_checkbutton_srun.get_active();

	try
	{
		m_num_tasks = std::stoi(m_entry_num_tasks.get_text());
		if (m_srun)
		{
			m_num_nodes = std::stoi(m_entry_num_nodes.get_text());
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << '\n';
		return;
	}

	m_client = strdup(m_entry_client.get_text().c_str());
	m_gdb = strdup(m_entry_gdb.get_text().c_str());
	m_socat = strdup(m_entry_socat.get_text().c_str());
	m_target = strdup(m_entry_target.get_text().c_str());
	m_ip_address = strdup(m_entry_ip_address.get_text().c_str());
	m_ssh_address = strdup(m_entry_ssh_address.get_text().c_str());
	m_ssh_user = strdup(m_entry_ssh_user.get_text().c_str());
	m_ssh_password = strdup(m_entry_ssh_password.get_text().c_str());
	m_partition = strdup(m_entry_partition.get_text().c_str());

	m_is_valid = true;
}

void StartupDialog::set_sensitivity(bool state)
{
	m_entry_num_nodes.set_sensitive(state);
	m_entry_ssh_address.set_sensitive(state);
	m_entry_ssh_user.set_sensitive(state);
	m_entry_ssh_password.set_sensitive(state);
	m_entry_partition.set_sensitive(state);
}

void StartupDialog::on_toggle_button(Gtk::CheckButton *button)
{
	set_sensitivity(button->get_active());
}