#include "breakpoint.hpp"
#include "window.hpp"

Breakpoint::Breakpoint(const int num_processes, const int line, const string &full_path, const UIWindow *const window)
	: m_num_processes(num_processes),
	  m_line(line),
	  m_full_path(full_path),
	  m_window(window),
	  m_breakpoint_state(new BreakpointState[m_num_processes])
{
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		m_breakpoint_state[rank] = NO_ACTION;
	}
}

Breakpoint::~Breakpoint()
{
	delete[] m_breakpoint_state;
}

bool Breakpoint::create_breakpoint(const int rank)
{
	m_breakpoint_state[rank] = ERROR_CREATING;
	if (m_window->target_state(rank) == TargetState::RUNNING)
	{
		return false;
	}

	// printf("setting breakpoint for rank %d on %s:%d\n", rank, m_full_path.c_str(), m_line);

	m_breakpoint_state[rank] = CREATED;
	return true;
}

bool Breakpoint::delete_breakpoint(const int rank)
{
	m_breakpoint_state[rank] = ERROR_DELETING;
	if (m_window->target_state(rank) == TargetState::RUNNING)
	{
		return false;
	}

	// printf("deleting breakpoint for rank %d on %s:%d\n", rank, m_full_path.c_str(), m_line);

	m_breakpoint_state[rank] = NO_ACTION;
	return true;
}

bool Breakpoint::delete_breakpoints()
{
	bool all_deleted = true;
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		if (m_breakpoint_state[rank] == CREATED)
		{
			all_deleted &= delete_breakpoint(rank);
		}
	}

	if (!all_deleted)
	{
		Gtk::MessageDialog dialog(*m_window->root_window(), string("Could not delete breakpoint for some ranks.\nNot deleted for rank(s): ") + get_list(ERROR_DELETING), false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
		dialog.run();
	}

	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		if (m_breakpoint_state[rank] == ERROR_DELETING)
		{
			m_breakpoint_state[rank] = CREATED;
		}
	}

	return all_deleted;
}

void Breakpoint::update_breakpoints(const bool *const button_states)
{
	bool all_done = true;
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		if (!button_states[rank] && m_breakpoint_state[rank] == BreakpointState::CREATED)
		{
			m_breakpoint_state[rank] = BreakpointState::DELETE;
		}
		if (button_states[rank] && m_breakpoint_state[rank] != BreakpointState::CREATED)
		{
			m_breakpoint_state[rank] = BreakpointState::CREATE;
		}

		if (m_breakpoint_state[rank] == DELETE)
		{
			all_done &= delete_breakpoint(rank);
		}
		if (m_breakpoint_state[rank] == CREATE)
		{
			all_done &= create_breakpoint(rank);
		}
	}

	string creating_desc = "Could not create breakpoints for some ranks. (";
	string deleting_desc = "Could not delete breakpoints for some ranks. (";
	string rank_list_desc = "Active breakpoints for rank(s): ";
	string creating_str = get_list(ERROR_CREATING);
	string deleting_str = get_list(ERROR_DELETING);
	if (!creating_str.empty() || !deleting_str.empty())
	{
		string message = "" + ((!creating_str.empty()) ? creating_desc + creating_str + ")\n" : "") + ((!deleting_str.empty()) ? deleting_desc + deleting_str + ")\n" : "") + rank_list_desc + get_list(CREATED);
		Gtk::MessageDialog info_dialog(
			*m_window->root_window(),
			message,
			false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
		info_dialog.run();
	}
}

string Breakpoint::get_list(const BreakpointState state) const
{
	string str = "";
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		if (m_breakpoint_state[rank] == state)
		{
			if (!str.empty())
			{
				str += ", ";
			}
			str += std::to_string(rank);
		}
	}
	return str;
}

bool Breakpoint::one_created() const
{
	for (int rank = 0; rank < m_num_processes; ++rank)
	{
		if (m_breakpoint_state[rank] == CREATED)
		{
			return true;
		}
	}
	return false;
}