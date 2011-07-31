#ifndef SHELL_H
#define SHELL_H

/* shell_run() starts the shell main loop. The main loop will run until the
 * user types the "quit" command. */
extern void shell_run();

/* If quiet is set to true, the shell will not print any promt or user message.
 * This is useful, if s script is piped into the shell */
extern void shell_set_quiet(int quiet);

#endif
