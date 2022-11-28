/**[txh]********************************************************************

  GDB/MI interface library
  Copyright (c) 2004-2009 by Salvador E. Tropea.

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

  Module: Connect.
  Comments:
  This module handles the dialog with gdb, including starting and stopping
gdb.@p

GDB Bug workaround for "file -readnow": I tried to workaround a bug using
it but looks like this option also have bugs!!!! so I have to use the
command line option --readnow.
It also have a bug!!!! when the binary is changed and gdb must reload it
this option is ignored. So it looks like we have no solution but 3 gdb bugs
in a row.

***************************************************************************/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "mi_gdb.h"

int mi_error = MI_OK;
char *mi_error_from_gdb = NULL;

mi_h *mi_alloc_h()
{
	mi_h *h = (mi_h *)calloc(1, sizeof(mi_h));
	if (!h)
	{
		mi_error = MI_OUT_OF_MEMORY;
		return NULL;
	}
	h->to_gdb[0] = h->to_gdb[1] = h->from_gdb[0] = h->from_gdb[1] = -1;
	h->pid = -1;
	return h;
}

void mi_free_h(mi_h **handle)
{
	mi_h *h = *handle;
	if (h->to_gdb[0] >= 0)
		close(h->to_gdb[0]);
	if (h->to)
		fclose(h->to);
	else if (h->to_gdb[1] >= 0)
		close(h->to_gdb[1]);
	if (h->from)
		fclose(h->from);
	else if (h->from_gdb[0] >= 0)
		close(h->from_gdb[0]);
	if (h->from_gdb[1] >= 0)
		close(h->from_gdb[1]);
	if (h->line)
		free(h->line);
	mi_free_output(h->po);
	free(h);
	*handle = NULL;
}

int mi_getline(mi_h *h)
{
	char c;

	while (read(h->from_gdb[0], &c, 1) == 1)
	{
		if (h->lread >= h->llen)
		{
			h->llen = h->lread + 128;
			h->line = (char *)realloc(h->line, h->llen);
			if (!h->line)
			{
				h->llen = 0;
				h->lread = 0;
				return -1;
			}
		}
		if (c == '\n')
		{
			int ret = h->lread;
			h->line[ret] = 0;
			h->lread = 0;
			return ret;
		}
		if (c != '\r')
		{
			h->line[h->lread] = c;
			h->lread++;
		}
	}
	return 0;
}

char *get_cstr(mi_output *o)
{
	if (!o->c || o->c->type != t_const)
		return NULL;
	return o->c->v.cstr;
}

int mi_get_response(mi_h *h)
{
	if (h->from_gdb_echo)
		h->from_gdb_echo(h->line, h->from_gdb_echo_data);
	if (strncmp(h->line, "(gdb)", 5) == 0)
	{ /* End of response. */
		return 1;
	}
	else
	{ /* Add to the response. */
		mi_output *o;
		o = mi_parse_gdb_output(h->line);

		if (!o)
			return 0;
		/* Tunneled streams callbacks. */
		// if (o->type == MI_T_OUT_OF_BAND && o->stype == MI_ST_STREAM)
		// {
		// 	char *aux;
		// 	add = 0;
		// 	switch (o->sstype)
		// 	{
		// 	case MI_SST_CONSOLE:
		// 		aux = get_cstr(o);
		// 		if (h->console)
		// 			h->console(aux, h->console_data);
		// 		if (h->catch_console && aux)
		// 		{
		// 			h->catch_console--;
		// 			if (!h->catch_console)
		// 			{
		// 				free(h->catched_console);
		// 				h->catched_console = strdup(aux);
		// 			}
		// 		}
		// 		break;
		// 	case MI_SST_TARGET:
		// 		/* This one seems to be useless. */
		// 		if (h->target)
		// 			h->target(get_cstr(o), h->target_data);
		// 		break;
		// 	case MI_SST_LOG:
		// 		if (h->log)
		// 			h->log(get_cstr(o), h->log_data);
		// 		break;
		// 	}
		// }
		// else if (o->type == MI_T_OUT_OF_BAND && o->stype == MI_ST_ASYNC)
		// {
		// 	if (h->async)
		// 		h->async(o, h->async_data);
		// }
		// else
		if (o->type == MI_T_RESULT_RECORD && o->tclass == MI_CL_ERROR)
		{
			/* Error from gdb, record it. */
			mi_error = MI_FROM_GDB;
			free(mi_error_from_gdb);
			mi_error_from_gdb = NULL;
			if (o->c && strcmp(o->c->var, "msg") == 0 && o->c->type == t_const)
				mi_error_from_gdb = strdup(o->c->v.cstr);
		}
		int is_exit = (o->type == MI_T_RESULT_RECORD && o->tclass == MI_CL_EXIT);
		/* Add to the list of responses. */
		// if (add)
		{
			if (h->last)
				h->last->next = o;
			else
				h->po = o;
			h->last = o;
		}
		// else
		// 	mi_free_output(o);
		/* Exit RR means gdb exited, we won't get a new prompt ;-) */
		if (is_exit)
			return 1;
	}

	return 0;
}

mi_output *mi_retire_response(mi_h *h)
{
	mi_output *ret = h->po;
	h->po = h->last = NULL;
	return ret;
}

mi_output *mi_get_response_blk(mi_h *h)
{
	int r;
	do
	{
		r = mi_get_response(h);
		if (r)
			return mi_retire_response(h);
		else
			usleep(100);
	} while (!r);

	return NULL;
}
