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
	  m_client(nullptr),
	  m_gdb(nullptr),
	  m_socat(nullptr),
	  m_target(nullptr),
	  m_ip_address(nullptr),
	  m_ssh_address(nullptr),
	  m_ssh_user(nullptr),
	  m_ssh_password(nullptr),
	  m_partition(nullptr)
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

	m_file_chooser_button->signal_selection_changed().connect(sigc::mem_fun(*this, &StartupDialog::read_config));
	m_file_chooser_button->set_filename("/home/nicolas/ma/parallelgdb/configs/config_mpirun");
	// m_file_chooser_button->set_filename("/home/nicolas/ma/parallelgdb/configs/config_mpi");
	// m_file_chooser_button->set_filename("/home/nicolas/ma/parallelgdb/configs/config_ssh");
	// m_file_chooser_button->set_filename("/home/nicolas/ma/parallelgdb/configs/config_ants");

	set_sensitivity(false);

	m_dialog->signal_response().connect(sigc::mem_fun(*this, &StartupDialog::on_dialog_response));
	m_checkbutton_ssh->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &StartupDialog::on_toggle_button), m_checkbutton_ssh));

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
	set_sensitivity(false);
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
	if ("ssh" == key)
	{
		if ("true" == value)
		{
			m_checkbutton_ssh->set_active(true);
			set_sensitivity(true);
		}
		else
		{
			m_checkbutton_ssh->set_active(false);
			set_sensitivity(false);
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

void StartupDialog::on_dialog_response(const int response_id)
{
	if (Gtk::RESPONSE_OK != response_id)
	{
		return;
	}

	try
	{
		m_processes_per_node = std::stoi(m_entry_processes_per_node->get_text());
	}
	catch (const std::exception &e)
	{
		std::cerr << "Invalid value in input: Processes per Node\n";
		return;
	}

	try
	{
		m_num_nodes = std::stoi(m_entry_num_nodes->get_text());
	}
	catch (const std::exception &e)
	{
		std::cerr << "Invalid value in input: Number of Nodes.\n";
		return;
	}

	m_mpirun = m_radiobutton_mpirun->get_active();
	m_srun = m_radiobutton_srun->get_active();

	m_client = strdup(m_entry_client->get_text().c_str());
	m_gdb = strdup(m_entry_gdb->get_text().c_str());
	m_socat = strdup(m_entry_socat->get_text().c_str());
	m_target = strdup(m_entry_target->get_text().c_str());
	m_ip_address = strdup(m_entry_ip_address->get_text().c_str());

	m_ssh = m_checkbutton_ssh->get_active();

	m_ssh_address = strdup(m_entry_ssh_address->get_text().c_str());
	m_ssh_user = strdup(m_entry_ssh_user->get_text().c_str());
	m_ssh_password = strdup(m_entry_ssh_password->get_text().c_str());
	m_partition = strdup(m_entry_partition->get_text().c_str());

	m_is_valid = true;
}

void StartupDialog::set_sensitivity(bool state)
{
	m_entry_ssh_address->set_sensitive(state);
	m_entry_ssh_user->set_sensitive(state);
	m_entry_ssh_password->set_sensitive(state);
	m_entry_partition->set_sensitive(state);
}

void StartupDialog::on_toggle_button(Gtk::CheckButton *button)
{
	set_sensitivity(button->get_active());
}