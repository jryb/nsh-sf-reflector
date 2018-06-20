/*-
 * transport.c - NSH transport API implementation
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

#include "transport.h"
#include "field_extractor.h"
#include "utils.h"

static int sd;
static uint32_t pkts_seen = 0;

/*
 * transport_init
 *
 * Initialize all transport API resources
 */
void transport_init (void) {
    int sockopt;
    
    /*
     * Init read socket
     */
    sd = socket (AF_INET, SOCK_RAW, IPPROTO_GRE);
    if (sd < 0) {
        PRINT_ERR("Could not NSH socket");
        perror("socket() failed to get descriptor");
        exit(EXIT_FAILURE);
    }
    
    /*
     * Set REUSEADDR so we don't have to wait for cleanup if run
     * multiple times in a row
     */
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &sockopt,
                   sizeof(sockopt)) == -1) {
        PRINT_ERR("Could not set sd to REUSEADDR");
        perror("setsockopt() failed to set SO_REUSEADDR");
        exit(EXIT_FAILURE);
    }
    
    PRINT_DEBUG("NSH transport init finished");
}

/*
 * transport_destroy
 *
 * Cleanup and free all socket/transport related resources
 */
void transport_destroy (void) {
    PRINT_DEBUG("Saw %d pkts in NSH transport", pkts_seen);
    close(sd);
}

/*
 * packer_handler
 *
 * Per packet packet handler
 */
static void packet_handler (uint8_t *buffer) {
    uint8_t *payload_start, *nsh_start;
    uint16_t nsh_len, gre_len;
    uint8_t svc_idx;
    PRINT_DEBUG("Got a pkt");
    payload_start = buffer;
    pkts_seen++;
    
    /*
     * Skip past the Outer IP, GRE and NSH headers.
     * TODO: Probably a better way to do this.....
     */
    payload_start += 20; // Outer IP header
    gre_len = GET_GRE_HDR_LENGTH(payload_start);
    payload_start += gre_len; // GRE header
    nsh_start = payload_start;
    nsh_len = GET_NSH_HDR_LENGTH(payload_start);
    payload_start += nsh_len; // NSH header
    
    /*
     * Do interesting stuff
     */
    
    /*
     * Decrement NSH Service Index
     */
    svc_idx = GET_NSH_SERVICE_INDEX(nsh_start);
    SET_NSH_SERVICE_INDEX(nsh_start, svc_idx - 1);
}


/*
 * transport_thread
 *
 * Run NSH transport thread, RX packets, pass them to pkt handler,
 * decrement index, and TX packets back to SFF where they can from
 * when done.
 */
void *transport_thread (void *ctx) {
#define MAX_PKT_SIZE 9000
    u_char buffer[MAX_PKT_SIZE];
    struct sockaddr_in recv_addr;
    socklen_t recv_addr_len;
    size_t rx_bytes, tx_bytes;
    bool *quit = (bool *)ctx;
    
    /*
     * Packet loop
     */
    while (*quit== false) {
        fd_set rfds;
        int retval;
        struct timeval tv = {2, 0}; // Timeout of 2 seconds
        FD_ZERO(&rfds);
        FD_SET(sd, &rfds);
        retval = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);
        if (retval > 0) {
            rx_bytes = recvfrom(sd, buffer, MAX_PKT_SIZE, 0,
                                (struct sockaddr *)&recv_addr,
                                &recv_addr_len);
        
            if (rx_bytes) {
                PRINT_DEBUG("Got a pkt of %zu bytes from %s rxaddr_len %d",
                            rx_bytes, inet_ntoa(recv_addr.sin_addr), recv_addr_len);
                packet_handler(buffer);
            } else {
                perror("recvfrom");
                *quit = true;
                return (0);
                /*
                 * Is this recoverable?  Handle it!
                 */
            }
            
            /*
             * Return packet to sender
             * Note: packet_handler already decremented the service index
             */
            if (recv_addr.sin_addr.s_addr != 0) {
                tx_bytes = sendto(sd, &buffer[20], rx_bytes - 20, 0,
                                  (struct sockaddr *)&recv_addr, recv_addr_len);
                if (tx_bytes <= 0) {
                    perror("sendto:");
                }
            } else {
                PRINT_DEBUG("Got a NULL return addr packet....for some reason. Don't know where to send.");
            }
        } else if (retval < 0) {
            /*
             * Got a timeout.... restart the watch
             */
        }
    }
    return (0);
}
