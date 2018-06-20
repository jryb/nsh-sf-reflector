/*-
 * nsh-sf-reflector.c - Simple NSH based Service Function
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

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "cli.h"
#include "transport.h"
#include "utils.h"

/*
 * Print debugs for all components
 */
bool g_debug_on = false;

/*
 * init_main
 *
 * Initialize all resources needed
 */
static void init_main (void) {
    transport_init();
}

/*
 * free_main
 *
 * Destroy all resources needed
 */
static void free_main (void) {
    transport_destroy();
}

static void usage (char *prog) {
    printf("Usage: %s\n", prog);
    printf("\n  Options:\n" );
    printf("\t-d -----------------> Turn debug on\n");
    printf("\t-h -----------------> This help message\n");
    printf("\n");
}

int main (int argc, char **argv) {
    pthread_t transport_th;
    pthread_t cli_th;
    int rc;
    int opt;
    bool quit = false;

    /*
     * Get options
     */
    while ((opt = getopt(argc, argv, "hd")) != -1) {
        switch (opt) {
            case 'd':
                g_debug_on = true;
                break;
            case 'h':
            default:
                usage(argv[0]);
                exit(1);
                break;
        }
    }
    
    init_main();
    
    /*
     * Create threads for CLI and NSH transport
     */
    if ((rc = pthread_create(&transport_th, NULL, transport_thread, &quit)) != 0) {
        PRINT_ERR("pthread_create for watcher thread error (%d)\n", rc);
        return (EXIT_FAILURE);
    }
    if ((rc = pthread_create(&cli_th, NULL, cli_thread, &quit)) != 0) {
        PRINT_ERR("pthread_create for CLI thread error (%d)\n", rc);
        return (EXIT_FAILURE);
    }

    /*
     * Wait for threads to finish
     */
    pthread_join(cli_th, 0);
    pthread_join(transport_th, 0);
    
    free_main();
    return(EXIT_SUCCESS);
}
