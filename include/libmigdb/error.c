/*

	Modified by Nicolas With for ParallelGDB.

*/

/**[txh]********************************************************************

	GDB/MI interface library
	Copyright (c) 2004 by Salvador E. Tropea.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	Module: Error.
	Comment:
	Translates error numbers into messages.

***************************************************************************/

#include "mi_gdb.h"

static const char *error_strs[] = {
	"Ok",
	"Out of memory",
	"Pipe creation",
	"Fork failed",
	"GDB not running",
	"Parser failed",
	"Unknown async response",
	"Unknown result response",
	"Error from gdb",
	"Time out in gdb response",
	"GDB suddenly died",
	"Can't execute X terminal",
	"Failed to create temporal",
	"Can't execute the debugger"};

const char *mi_get_error_str()
{
	if (mi_error < 0 || mi_error > MI_LAST_ERROR)
		return "Unknown";
	return error_strs[mi_error];
}
