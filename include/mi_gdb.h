/*

	Modified by Nicolas With for ParallelGDB.

*/

/**[txh]********************************************************************

	GDB/MI interface library
	Copyright (c) 2004-2016 by Salvador E. Tropea.

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

	Comments:
	Main header for libmigdb.

***************************************************************************/

#ifndef MI_GDB_H
#define MI_GDB_H

#include <unistd.h>

#define MI_OK                      0
#define MI_OUT_OF_MEMORY           1
#define MI_PIPE_CREATE             2
#define MI_FORK                    3
#define MI_DEBUGGER_RUN            4
#define MI_PARSER                  5
#define MI_UNKNOWN_ASYNC           6
#define MI_UNKNOWN_RESULT          7
#define MI_FROM_GDB                8
#define MI_GDB_TIME_OUT            9
#define MI_GDB_DIED               10
#define MI_MISSING_XTERM          11
#define MI_CREATE_TEMPORAL        12
#define MI_MISSING_GDB            13
#define MI_LAST_ERROR             13

enum mi_val_type
{
	t_const,
	t_tuple,
	t_list
};

/* Types and subtypes. */
/* Type. */
#define MI_T_OUT_OF_BAND   0
#define MI_T_RESULT_RECORD 1
/* Out of band subtypes. */
#define MI_ST_ASYNC        0
#define MI_ST_STREAM       1
/* Async sub-subtypes. */
#define MI_SST_EXEC        0
#define MI_SST_STATUS      1
#define MI_SST_NOTIFY      2
/* Stream sub-subtypes. */
#define MI_SST_CONSOLE     3
#define MI_SST_TARGET      4
#define MI_SST_LOG         5
/* Classes. */
/* Async classes. */
#define MI_CL_UNKNOWN      0
#define MI_CL_STOPPED      1
#define MI_CL_DOWNLOAD     2
/* Result classes. */
#define MI_CL_DONE         2
#define MI_CL_RUNNING      3
#define MI_CL_CONNECTED    4
#define MI_CL_ERROR        5
#define MI_CL_EXIT         6

#define MI_VERSION_STR "0.8.13"
#define MI_VERSION_MAJOR  0
#define MI_VERSION_MIDDLE 8
#define MI_VERSION_MINOR  13

struct mi_results_struct
{
	char *var; /* Result name or NULL if just a value. */
	enum mi_val_type type;
	union
	{
		char *cstr;
		struct mi_results_struct *rs;
	} v;
	struct mi_results_struct *next;
};
typedef struct mi_results_struct mi_results;

struct mi_output_struct
{
	/* Type of output. */
	char type;
	char stype;
	char sstype;
	char tclass;
	/* Content. */
	mi_results *c;
	/* Always modeled as a list. */
	struct mi_output_struct *next;
};
typedef struct mi_output_struct mi_output;

/* Values of this structure shouldn't be manipulated by the user. */
struct mi_h_struct
{
	/* The line we are reading. */
	char *line;
	/* Parsed output. */
	mi_output *po, *last;
};
typedef struct mi_h_struct mi_h;

enum mi_bkp_type
{
	t_unknown = 0,
	t_breakpoint = 1,
	t_hw = 2
};
enum mi_bkp_disp
{
	d_unknown = 0,
	d_keep = 1,
	d_del = 2
};
enum mi_bkp_mode
{
	m_file_line = 0,
	m_function = 1,
	m_file_function = 2,
	m_address = 3
};

struct mi_bkpt_struct
{
	int number;
	enum mi_bkp_type type;
	enum mi_bkp_disp disp; /* keep or del if temporal */
	char enabled;
	void *addr;
	char *func;
	char *file;
	int line;
	int ignore;
	int times;

	/* For the user: */
	char *cond;
	char *file_abs;
	int thread;
	enum mi_bkp_mode mode;
	struct mi_bkpt_struct *next;
};
typedef struct mi_bkpt_struct mi_bkpt;

enum mi_wp_mode
{
	wm_unknown = 0,
	wm_write = 1,
	wm_read = 2,
	wm_rw = 3
};

struct mi_wp_struct
{
	int number;
	char *exp;
	enum mi_wp_mode mode;

	/* For the user: */
	struct mi_wp_struct *next;
	char enabled;
};
typedef struct mi_wp_struct mi_wp;

struct mi_frames_struct
{
	int level;	/* The frame number, 0 being the topmost frame, i.e. the innermost
				   function. */
	void *addr; /* The `$pc' value for that frame. */
	char *func; /* Function name. */
	char *file; /* File name of the source file where the function lives. */
	char *fullname;
	char *from;
	int line; /* Line number corresponding to the `$pc'. */
	/* When arguments are available: */
	mi_results *args;
	int thread_id;
	/* When more than one is provided: */
	struct mi_frames_struct *next;
};
typedef struct mi_frames_struct mi_frames;

struct mi_aux_term_struct
{
	pid_t pid;
	char *tty;
};
typedef struct mi_aux_term_struct mi_aux_term;

struct mi_pty_struct
{
	char *slave;
	int master;
};
typedef struct mi_pty_struct mi_pty;

enum mi_gvar_fmt
{
	fm_natural = 0,
	fm_binary = 1,
	fm_decimal = 2,
	fm_hexadecimal = 3,
	fm_octal = 4,
	/* Only for registers format: */
	fm_raw = 5
};
enum mi_gvar_lang
{
	lg_unknown = 0,
	lg_c,
	lg_cpp,
	lg_java
};

#define MI_ATTR_DONT_KNOW   0
#define MI_ATTR_NONEDITABLE 1
#define MI_ATTR_EDITABLE    2

struct mi_gvar_struct
{
	char *name;
	int numchild;
	char *type;
	enum mi_gvar_fmt format;
	enum mi_gvar_lang lang;
	char *exp;
	int attr;

	/* MI v2 fills it, not yet implemented here. */
	/* Use gmi_var_evaluate_expression. */
	char *value;

	/* Pointer to the parent. NULL if none. */
	struct mi_gvar_struct *parent;
	/* List containing the children.
	   Filled by gmi_var_list_children.
	   NULL if numchild==0 or not yet filled. */
	struct mi_gvar_struct *child;
	/* Next var in the list. */
	struct mi_gvar_struct *next;

	/* For the user: */
	char opened;  /* We will show its children. 1 when we fill "child" */
	char changed; /* Needs to be updated. 0 when created. */
	int vischild; /* How many items visible. numchild when we fill "child" */
	int depth;	  /* How deep is this var. */
	char ispointer;
};
typedef struct mi_gvar_struct mi_gvar;

struct mi_gvar_chg_struct
{
	char *name;
	int in_scope;		  /* if true the other fields apply. */
	char *new_type;		  /* NULL if type_changed==false */
	int new_num_children; /* only when new_type!=NULL */

	struct mi_gvar_chg_struct *next;
};
typedef struct mi_gvar_chg_struct mi_gvar_chg;

/* A list of assembler instructions. */
struct mi_asm_insn_struct
{
	void *addr;
	char *func;
	unsigned offset;
	char *inst;

	struct mi_asm_insn_struct *next;
};
typedef struct mi_asm_insn_struct mi_asm_insn;

/* A list of source lines containing assembler instructions. */
struct mi_asm_insns_struct
{
	char *file;
	int line;
	mi_asm_insn *ins;

	struct mi_asm_insns_struct *next;
};
typedef struct mi_asm_insns_struct mi_asm_insns;

/* Changed register. */
struct mi_chg_reg_struct
{
	int reg;
	char *val;
	char *name;
	char updated;

	struct mi_chg_reg_struct *next;
};
typedef struct mi_chg_reg_struct mi_chg_reg;

/*
 Examining gdb sources and looking at docs I can see the following "stop"
reasons:

Breakpoints:
a) breakpoint-hit (bkptno) + frame
Also: without reason for temporal breakpoints.

Watchpoints:
b) watchpoint-trigger (wpt={number,exp};value={old,new}) + frame
c) read-watchpoint-trigger (hw-rwpt{number,exp};value={value}) + frame
d) access-watchpoint-trigger (hw-awpt{number,exp};value={[old,]new}) + frame
e) watchpoint-scope (wpnum) + frame

Movement:
f) function-finished ([gdb-result-var,return-value]) + frame
g) location-reached + frame
h) end-stepping-range + frame

Exit:
i) exited-signalled (signal-name,signal-meaning)
j) exited (exit-code)
k) exited-normally

Signal:
l) signal-received (signal-name,signal-meaning) + frame

Plus: thread-id
*/
enum mi_stop_reason
{
	sr_unknown = 0,
	sr_bkpt_hit,
	sr_wp_trigger, sr_read_wp_trigger, sr_access_wp_trigger, sr_wp_scope,
	sr_function_finished, sr_location_reached, sr_end_stepping_range,
	sr_exited_signalled, sr_exited, sr_exited_normally,
	sr_signal_received
};

struct mi_stop_struct
{
	enum mi_stop_reason reason; /* If more than one reason just the last. */
	/* Flags indicating if non-pointer fields are filled. */
	char have_thread_id;
	char have_bkptno;
	char have_exit_code;
	char have_wpno;
	/* Where stopped. Doesn't exist for sr_exited*. */
	int thread_id;
	mi_frames *frame;
	/* sr_bkpt_hit */
	int bkptno;
	/* sr_*wp_* no scope */
	mi_wp *wp;
	char *wp_old;
	char *wp_val;
	/* sr_wp_scope */
	int wpno;
	/* sr_function_finished. Not for void func. */
	char *gdb_result_var;
	char *return_value;
	/* sr_exited_signalled, sr_signal_received */
	char *signal_name;
	char *signal_meaning;
	/* sr_exited */
	int exit_code;
};
typedef struct mi_stop_struct mi_stop;

/* Variable containing the last error. */
extern int mi_error;
extern char *mi_error_from_gdb;
const char *mi_get_error_str();

/* Parse gdb output. */
mi_output *mi_parse_gdb_output(const char *str);
/* Wait until gdb sends a response. */
mi_output *mi_get_response_blk(mi_h *h);
/* Check if gdb sent a complete response. Use with mi_retire_response. */
int mi_get_response(mi_h *h);
/* Get the last response. Use with mi_get_response. */
mi_output *mi_retire_response(mi_h *h);
/* Look for a result record in gdb output. */
mi_output *mi_get_rrecord(mi_output *r);
/* Look if the output contains an async stop.
   If that's the case return the reason for the stop.
   If the output contains an error the description is returned in reason. */
int mi_get_async_stop_reason(mi_output *r, char **reason);
mi_stop *mi_get_stopped(mi_results *r);
mi_frames *mi_get_async_frame(mi_output *r);
/* Wait until gdb sends a response.
   Then check if the response is of the desired type. */
int mi_res_simple_exit(mi_h *h);
int mi_res_simple_done(mi_h *h);
int mi_res_simple_running(mi_h *h);
int mi_res_simple_connected(mi_h *h);
/* It additionally extracts an specified variable. */
mi_results *mi_res_done_var(mi_h *h, const char *var);
/* Extract a frames list from the response. */
mi_frames *mi_res_frames_array(mi_h *h, const char *var);
mi_frames *mi_res_frames_list(mi_h *h);
mi_frames *mi_parse_frame(mi_results *c);
mi_frames *mi_res_frame(mi_h *h);
/* Extract a list of thread IDs from response. */
int mi_res_thread_ids(mi_h *h, int **list);
int mi_get_thread_ids(mi_output *res, int **list);
/* A variable response. */
mi_gvar *mi_res_gvar(mi_h *h, mi_gvar *cur, const char *expression);
enum mi_gvar_fmt mi_format_str_to_enum(const char *format);
const char *mi_format_enum_to_str(enum mi_gvar_fmt format);
char mi_format_enum_to_char(enum mi_gvar_fmt format);
enum mi_gvar_lang mi_lang_str_to_enum(const char *lang);
const char *mi_lang_enum_to_str(enum mi_gvar_lang lang);
int mi_res_changelist(mi_h *h, mi_gvar_chg **changed);
int mi_res_children(mi_h *h, mi_gvar *v);
// mi_bkpt *mi_res_bkpt(mi_h *h); // original func
mi_bkpt *mi_res_bkpt(mi_output *output);
mi_wp *mi_res_wp(mi_h *h);
char *mi_res_value(mi_h *h);
// mi_stop *mi_res_stop(mi_h *h); // original func
mi_stop *mi_res_stop(mi_output *o);
enum mi_stop_reason mi_reason_str_to_enum(const char *s);
const char *mi_reason_enum_to_str(enum mi_stop_reason r);
int mi_get_read_memory(mi_h *h, unsigned char *dest, unsigned ws,
					   int *na, unsigned long *addr);
mi_asm_insns *mi_get_asm_insns(mi_h *h);
mi_chg_reg *mi_get_list_registers(mi_h *h, int *how_many);
int mi_get_list_registers_l(mi_h *h, mi_chg_reg *l);
mi_chg_reg *mi_get_list_changed_regs(mi_h *h);
int mi_get_reg_values(mi_h *h, mi_chg_reg *l);
mi_chg_reg *mi_get_reg_values_l(mi_h *h, int *how_many);

/* Allocation functions: */
void *mi_calloc(size_t count, size_t sz);
void *mi_calloc1(size_t sz);
char *mi_malloc(size_t sz);
mi_h *mi_alloc_h();
mi_results *mi_alloc_results(void);
mi_output *mi_alloc_output(void);
mi_frames *mi_alloc_frames(void);
mi_gvar *mi_alloc_gvar(void);
mi_gvar_chg *mi_alloc_gvar_chg(void);
mi_bkpt *mi_alloc_bkpt(void);
mi_wp *mi_alloc_wp(void);
mi_stop *mi_alloc_stop(void);
mi_asm_insns *mi_alloc_asm_insns(void);
mi_asm_insn *mi_alloc_asm_insn(void);
mi_chg_reg *mi_alloc_chg_reg(void);
void mi_free_h(mi_h **handle);
void mi_free_output(mi_output *r);
void mi_free_output_but(mi_output *r, mi_output *no, mi_results *no_r);
void mi_free_frames(mi_frames *f);
void mi_free_bkpt(mi_bkpt *b);
void mi_free_aux_term(mi_aux_term *t);
void mi_free_results(mi_results *r);
void mi_free_results_but(mi_results *r, mi_results *no);
void mi_free_gvar(mi_gvar *v);
void mi_free_gvar_chg(mi_gvar_chg *p);
void mi_free_wp(mi_wp *wp);
void mi_free_stop(mi_stop *s);
void mi_free_asm_insns(mi_asm_insns *i);
void mi_free_asm_insn(mi_asm_insn *i);
void mi_free_charp_list(char **l);
void mi_free_chg_reg(mi_chg_reg *r);

char *get_cstr(mi_output *o);

#endif /* MI_GDB_H */