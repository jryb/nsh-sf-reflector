/*-
 * cli.c - NSH SF Reflector CLI API implementation
 *
 * Copyright (c) 2018, Jeff Rybczynski <jeff.rybczynski@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "cli.h"
#include "utils.h"


#define CLI_MAX_LINE 1024
#define CLI_TOK_BUFSIZE 64
#define CLI_TOK_DELIM " \t\r\n\a"


/*
 * cli_read_line
 *
 * Read in user inputted command
 */
static char *cli_read_line (void) {
    char *line = NULL;
    size_t bufsize = 0;
    ssize_t read_sz;
    read_sz = getline(&line, &bufsize, stdin);
    PRINT_DEBUG("Read in CLI of size %zu", read_sz);
    return (line);
}

/*
 * cli_split_line
 *
 * Tokenize the command line args
 */
static char **cli_split_line (char *line) {
    int bufsize = CLI_TOK_BUFSIZE, pos = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token;
    
    if (!tokens) {
        PRINT_ERR("cli_split_line: tokens malloc error");
        exit(EXIT_FAILURE);
    }
    
    token = strtok(line, CLI_TOK_DELIM);
    while (token != NULL) {
        tokens[pos] = token;
        pos++;
        
        if (pos >= bufsize) {
            bufsize +=CLI_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens) {
                PRINT_ERR("cli_split_line: tokens realloc error");
                exit(EXIT_FAILURE);
            }
        }
        
        token = strtok(NULL, CLI_TOK_DELIM);
    }
    tokens[pos] = NULL;
    return (tokens);
}

/*
 * cli debug
 *
 * Handle debug command
 */
int cli_debug (char **args) {
    if (strcmp(args[1], "on") == 0) {
        g_debug_on = true;
    } else if (strcmp(args[1], "off") == 0) {
        g_debug_on = false;
    } else {
        printf("Couldn't recognize command: %s %s\n", args[0], args[1]);
    }
    return (1);
}

/*
 * cli_exit
 *
 * Handle exit command
 */
int cli_exit (char **args) {
    return (0);
}

/*
 * CLI commands and their coresponding handler functions
 */
char *cli_str[] = {
    "debug",
    "exit"
};

int (*cli_func[]) (char **) = {
    &cli_debug,
    &cli_exit
};

int cli_num_funcs (void) {
    return (sizeof(cli_str)/sizeof(char *));
}

/*
 * cli execute
 *
 * Execute CLI command and pass each command handler the appropriate args
 */
static int cli_execute (char **args) {
    int i;
    
    if (args[0] == NULL) {
        return (1);
    }
    
    for (i = 0; i < cli_num_funcs(); i++) {
        if (strcmp(args[0], cli_str[i]) == 0) {
            return (*cli_func[i])(args);
        }
    }
    
    return (1);
}

/*
 * cli_thread
 *
 * Run CLI thread
 */
void *cli_thread (void *ctx) {
    char *line;
    char **args;
    char *prompt = ">";
    int status;
    bool *quit = (bool *)ctx;
    
    do {
        printf("%s ", prompt);
        line = cli_read_line();
        args = cli_split_line(line);
        status = cli_execute(args);
        
        free(line);
        free(args);
    } while (status && (*quit== false));
    
    *quit = true;
    
    return (0);
}
