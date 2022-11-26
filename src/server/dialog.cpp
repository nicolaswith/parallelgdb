#include "dialog.hpp"

UIDialog::UIDialog()
	: m_is_valid(false),
	  m_client(nullptr),
	  m_target(nullptr),
	  m_ip_address(nullptr),
	  m_ssh_address(nullptr),
	  m_ssh_user(nullptr),
	  m_ssh_password(nullptr),
	  m_partition(nullptr),
	  m_label_num_nodes("Number of nodes:", Gtk::ALIGN_START),
	  m_label_num_tasks("Number of tasks:", Gtk::ALIGN_START),
	  m_label_client("Client:", Gtk::ALIGN_START),
	  m_label_target("Target:", Gtk::ALIGN_START),
	  m_label_srun("Use srun (ssh):", Gtk::ALIGN_START),
	  m_label_ip_address("Host IP address:", Gtk::ALIGN_START),
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
	m_grid.attach(m_label_num_tasks, 0, row);
	m_grid.attach(m_entry_num_tasks, 1, row++);
	m_grid.attach(m_label_client, 0, row);
	m_grid.attach(m_entry_client, 1, row++);
	m_grid.attach(m_label_target, 0, row);
	m_grid.attach(m_entry_target, 1, row++);
	m_grid.attach(m_label_srun, 0, row);
	m_grid.attach(m_checkbutton_srun, 1, row++);
	m_grid.attach(m_label_num_nodes, 0, row);
	m_grid.attach(m_entry_num_nodes, 1, row++);
	m_grid.attach(m_label_ip_address, 0, row);
	m_grid.attach(m_entry_ip_address, 1, row++);
	m_grid.attach(m_label_ssh_address, 0, row);
	m_grid.attach(m_entry_ssh_address, 1, row++);
	m_grid.attach(m_label_ssh_user, 0, row);
	m_grid.attach(m_entry_ssh_user, 1, row++);
	m_grid.attach(m_label_ssh_password, 0, row);
	m_grid.attach(m_entry_ssh_password, 1, row++);
	m_grid.attach(m_label_partition, 0, row);
	m_grid.attach(m_entry_partition, 1, row++);

	set_sensitivity(false);
	read_config();
	this->add_button("Ok", RESPONSE_ID_OK);

	this->signal_response().connect(sigc::mem_fun(*this, &UIDialog::on_dialog_response));
	m_checkbutton_srun.signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &UIDialog::on_toggle_button), &m_checkbutton_srun));

	this->show_all();
}

UIDialog::~UIDialog()
{
	if (m_client)
		free(m_client);
	if (m_target)
		free(m_target);
	if (m_ip_address)
		free(m_ip_address);
	if (m_ssh_address)
		free(m_ssh_address);
	if (m_ssh_user)
		free(m_ssh_user);
	if (m_ssh_password)
		free(m_ssh_password);
	if (m_partition)
		free(m_partition);
}

void UIDialog::store_line(string a_key, string a_value)
{
	if ("num_nodes" == a_key)
		m_entry_num_nodes.set_text(a_value);
	if ("num_tasks" == a_key)
		m_entry_num_tasks.set_text(a_value);
	if ("client" == a_key)
		m_entry_client.set_text(a_value);
	if ("target" == a_key)
		m_entry_target.set_text(a_value);
	if ("ip_address" == a_key)
		m_entry_ip_address.set_text(a_value);
	if ("ssh_address" == a_key)
		m_entry_ssh_address.set_text(a_value);
	if ("ssh_user" == a_key)
		m_entry_ssh_user.set_text(a_value);
	if ("ssh_password" == a_key)
		m_entry_ssh_password.set_text(a_value);
	if ("partition" == a_key)
		m_entry_partition.set_text(a_value);
	if ("srun" == a_key)
	{
		if ("true" == a_value)
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

void UIDialog::read_config()
{
	// file picker ?
	// FILE *f = fopen("./configs/config_ssh", "r");
	FILE *f = fopen("./configs/config_mpi", "r");
	if (!f)
	{
		return;
	}
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
				store_line(key, value);
			}
		}
	}
	delete[] config;
}

void UIDialog::on_dialog_response(const int a_response_id)
{
	if (RESPONSE_ID_OK != a_response_id)
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
	m_target = strdup(m_entry_target.get_text().c_str());
	m_ip_address = strdup(m_entry_ip_address.get_text().c_str());
	m_ssh_address = strdup(m_entry_ssh_address.get_text().c_str());
	m_ssh_user = strdup(m_entry_ssh_user.get_text().c_str());
	m_ssh_password = strdup(m_entry_ssh_password.get_text().c_str());
	m_partition = strdup(m_entry_partition.get_text().c_str());

	m_is_valid = true;
}

void UIDialog::set_sensitivity(bool a_state)
{
	m_entry_num_nodes.set_sensitive(a_state);
	m_entry_ip_address.set_sensitive(a_state);
	m_entry_ssh_address.set_sensitive(a_state);
	m_entry_ssh_user.set_sensitive(a_state);
	m_entry_ssh_password.set_sensitive(a_state);
	m_entry_partition.set_sensitive(a_state);
}

void UIDialog::on_toggle_button(Gtk::CheckButton *a_button)
{
	set_sensitivity(a_button->get_active());
}