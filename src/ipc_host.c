/******************************************************************************
* Copyright (C) 2017 by Ben Heberlein
*
* Redistribution, modification or use of this software in source or binary
* forms is permitted as long as the files maintain this copyright. This file
* was created for the University of Colorado Boulder course Advanced Practical
* Embedded Software Development. Ben Heberlein and the University of Colorado 
* are not liable for any misuse of this material.
*
*******************************************************************************/
/**
 * @file ipc_host.c
 * @brief Demonstration of socket communication, host process
 * 
 * This process implements a host in IPC architecture using sockets. This 
 * thread will control LEDs on the Beagle Bone Green based on client signals.
 *
 * @author Ben Heberlein
 * @date Nov 14 2017
 * @version 1.0
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static struct sockaddr_in server_address;
char buf[10];

void send_cmd(int conn, char rw, char led, char mode, int val) {
    buf[0] = rw;
    buf[1] = led;
    buf[2] = mode;
    memcpy(buf+3, &val, 4);

    write(conn, buf, 10);
}

int main(void) {

    /* Get socket and initialize */
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server_address, '0', sizeof(server_address));
    memset(buf, '0', sizeof(buf));

    /* Set up connection */
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(4323);

    /* Listen for connection */
    bind(fd, (struct sockaddr *)&server_address, sizeof(server_address));
    listen(fd, 10);

    int conn = 0;
    int val = 0;
    while(1) {
        conn = accept(fd, (struct sockaddr *)NULL, NULL);
        printf("Opened connection.\n");

        /* Write */
        send_cmd(conn, 1, 0, 1, 50);

        /* Read back */
        send_cmd(conn, 0, 0, 1, 0);
        int n = read(conn, buf, 10);
        memcpy(&val, buf, 4);

        printf("Recieved %d duty cycle for LED0.\n", val);

        close(conn);
        printf("Closed connection\n");
        sleep(1);
    }

    return 0;
}
