/* Copyright (C) 2001-2011, Parrot Foundation. */

/*

=head1 NAME

src/hbdb.c - The Honey Bee Debugger

=head1 DESCRIPTION

This file contains functions and types used by the C<hbdb> debugger.

=head1 COMMAND FUNCTIONS

Each of the following functions serve as the implementation for a particular
command. They are of the form C<hbdb_cmd_*>. For instance, if you are looking
for the code for the C<break> command, it will be the C<hbdb_cmd_break()>
function.

=over 4

=cut

*/

#include <stdio.h>

#include "parrot/parrot.h"
#include "parrot/hbdb.h"
#include "parrot/string_funcs.h"
#include "parrot/sub.h"

/* Size of command-line buffer */
#define HBDB_CMD_BUFFER_LENGTH 128

/* HEADERIZER HFILE: include/parrot/hbdb.h */

typedef void (*cmd_func_t)(ARGMOD(hbdb_t *hbdb), ARGIN(const char * const cmd));

typedef struct cmd      cmd;
typedef struct cmd_list cmd_list;

/* HEADERIZER BEGIN: static */
/* HEADERIZER END: static */

/*
 * Type:
 *     cmd
 *
 * Fields:
 *     c_func - points to the function that executes the command
 *     c_help - the help message associated with the command
 * 
 * Overview:
 *     Contains information about the implementation of a particular command.
 */

struct cmd {
    cmd_func_t         c_func;
    const char * const c_help;
};

/*
 * Type:
 *     cmd_list
 *
 * Fields:
 *     cl_name  - command name
 *     cl_short - command name abberviation
 *     cl_cmd   - "cmd" type for storing implementation details
 *
 * Overview:
 *     Contains general information about a particular command.
 */

struct cmd_list {
    const char * const cl_name;
    const char * const cl_short;
    const cmd  * const cl_cmd;        
};

/* Help message displayed for each command */
const char * const cmd_break_help = "Sets a breakpoint at the specified location.\n\n"
                                    "break LOCATION\n\n"
                                    "If LOCATION is an address, breaks at the exact address.";

const char * const cmd_help_help  = "List of commands:\n\n"
                                    "break\n"
                                    "\nType \"help\" followed by a command name.";

/* Entire list of commands */
const cmd_list commands[] = {
    { "break", 'b', { &hbdb_cmd_break, cmd_break_help } },
    { "help",  'h', { &hbdb_cmd_help,  cmd_help_help  } }
};

/*

=item C<void hbdb_cmd_break(hbdb_t *hbdb, const char * const command)>

Sets a breakpoint at a specific location.

=cut

*/

void
hbdb_cmd_break(ARGMOD(hbdb_t *hbdb), ARGIN(const char * const command))
{
    ASSERT_ARGS(hbdb_cmd_break)
}

/*

=item C<void hbdb_cmd_help(hbdb_t *hbdb, const char * const command)>

If C<command> is non-NULL, displays help message for C<command>. Otherwise, a
general help message is displayed.

=cut

*/

void
hbdb_cmd_help(ARGMOD(hbdb_t *hbdb), ARGIN(const char * const command))
{
    ASSERT_ARGS(hbdb_cmd_help)
}

/*

=back

=head1 GLOBAL FUNCTIONS

The remaining functions define some of the general behavior of the debugger.
They do not follow a particular pattern.

=over 4

=cut

*/

/*

=item C<void hbdb_get_command(PARROT_INTERP)>

Prompts the user to enter a command.

=cut

*/

void
hbdb_get_command(PARROT_INTERP)
{
    ASSERT_ARGS(hbdb_get_command)

    STRING *cmd;

    PMC    *stdinput;
    STRING *readline;
    STRING *prompt;

    /* DEBUG */
    PMC    *stdoutput = Parrot_io_stdhandle(interp, STDOUT_FILENO, NULL);
    STRING *print     = Parrot_str_new_constant(interp, "print");
    STRING *newline   = Parrot_str_new_constant(interp, "\n");
    /* DEBUG */

    /* Create FileHandle PMC for stdin */
    stdinput = Parrot_io_stdhandle(interp, STDIN_FILENO, NULL);

    /* Create string constants */
    readline = Parrot_str_new_constant(interp, "readline_interactive");
    prompt   = Parrot_str_new_constant(interp, "(hbdb) ");

    while (1) {
        Parrot_pcc_invoke_method_from_c_args(interp, stdinput, readline, "S->S", prompt, &cmd);

        Parrot_pcc_invoke_method_from_c_args(interp,
                                             stdoutput,
                                             print,
                                             "S->",
                                             Parrot_str_concat(interp, cmd, newline));
    }
}

/*

=item C<INTVAL hbdb_get_line_number(PARROT_INTERP, PMC *context_pmc)>

Returns the line number for the current context.

=cut

*/

INTVAL
hbdb_get_line_number(PARROT_INTERP, ARGIN(PMC *context_pmc))
{
    ASSERT_ARGS(hbdb_get_line_number)

    INTVAL line_num;
    Parrot_Context * const context = PMC_data_typed(context_pmc, Parrot_Context *);

    /*line_num = Parrot_sub_get_line_from_pc(interp,
                                           Parrot_pcc_get_sub(interp, context_pmc),
                                           context->current_pc);*/

    return line_num;
}

/*

=item C<void hbdb_init(PARROT_INTERP)>

Performs general initialization operations.

=cut

*/

void
hbdb_init(PARROT_INTERP)
{
    ASSERT_ARGS(hbdb_init)

    /* Check that debugger is not already initialized */
    if (!interp->hbdb) {
        hbdb_t        hbdb;
        Parrot_Interp debugger;

        /* Allocate memory for debugger  */
        hbdb = mem_gc_allocate_zeroed_typed(interp, hbdb_t);

        /* Create debugger interpreter */
        debugger = Parrot_new(interp);

        /* Assign global "hbdb_t" structures */
        interp->hbdb   = hbdb;
        debugger->hbdb = hbdb;

        /* Assign debugee and debugger interpreters */
        hbdb->debugee  = interp;
        hbdb->debugger = debugger;

        /* Allocate memory for command-line buffers, NUL terminated c strings */
        hbdb->current_command  = mem_gc_allocate_n_typed(interp, HBDB_CMD_BUFFER_LENGTH + 1, char);
        hbdb->last_command     = mem_gc_allocate_n_typed(interp, HBDB_CMD_BUFFER_LENGTH + 1, char);
        hbdb->file             = mem_gc_allocate_zeroed_typed(interp, hbdb_file_t);
    }

    /* Set HBDB_RUNNING and HBDB_ENTER status flags */
    interp->hbdb->state |= HBDB_RUNNING;
    interp->hbdb->state |= HBDB_ENTER;
}

/*

=back

=head1 SEE ALSO

F<frontend/hbdb/main.c>, F<include/parrot/hbdb.h>

=head1 HISTORY

The initial version of C<hbdb> was written by Kevin Polulak (soh_cah_toa) as
part of Google Summer of Code 2011.

=cut

*/

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4 cinoptions='\:2=2' :
 */

