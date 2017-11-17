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
 * @file ipc_client.c
 * @brief Demonstration of socket communication, client process
 * 
 * This process implements a client in IPC architecture using sockets. This 
 * thread will send signals to the host to control the LEDs on the Beagle 
 * Bone Green.
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
#include <arpa/inet.h>

#define SERVER_IP "192.168.0.15"

struct sockaddr_in server_address;

__attribute((packed)) struct state_s {
    int on;
    int duty;
    int period;
};
static struct state_s led_state[4] = {
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
};
#define MSG_SIZE 12*4

char buf[10];

static FILE *f;

static void read_leds(void) {
    int num = fread(&led_state, 1, MSG_SIZE, f);
}

static void write_leds(void) {
    int num = fwrite(&led_state, 1, MSG_SIZE, f);
}

static int read_duty(int led) {
    read_leds();
    printf("LED%d duty cycle is %d\n", led, led_state[led].duty);
    return led_state[led].duty;
}

static int read_period(int led) {
    read_leds();
    printf("LED%d period is %d\n", led, led_state[led].period);
    return led_state[led].period;
}

static int read_on(int led) {
    read_leds();
    printf("LED%d on/off is %d\n", led, led_state[led].on);
    return led_state[led].on;
}

static void write_duty(int led, int duty) {
    read_leds();
    led_state[led].duty = duty;
    write_leds();
    printf("Wrote LED%d duty cycle to %d\n", led, duty);
    read_leds();
}

static void write_period(int led, int per) {
    read_leds();
    led_state[led].period = per;
    write_leds();
    printf("Wrote LED%d period to %d\n", led, per);
    read_leds();
}

static void write_on(int led, int on) {
    read_leds();
    led_state[led].on = on;
    write_leds();
    printf("Wrote LED%d on/off to %d\n", led, on);
    read_leds();
}


static void init(void) {
    f = fopen("/dev/led_driver", "rb+");
    read_leds();
}

static void deinit(void) {
    fclose(f);
}

int main(void) {
    int fd = 0;

    /* Get socket */
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Failed to get socket.\n");
        return -1;
    }

    /* Prepare server address */
    memset(&server_address, '0', sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(4323);

    /* Convert IP */
    if (inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr) <= 0) {
        printf("Failed to convert IP address.\n");
        return -2;
    }

    /* Connect to server */
    if (connect(fd, (struct sockaddr *) &server_address, sizeof(server_address))) {
        printf("Could not connect to server.\n");
        return -3;
    }

    /* Init and set duty to something so we can see it change */
    init();
    read_duty(0);
    read_period(0);
    read_on(0);
    write_duty(0, 10);
    write_period(0, 4000);
    write_on(0, 1);
    read_duty(0);
    read_period(0);
    read_on(0);

    int n = 0, val = 0, led = 0, mode = 0, rw = 0;
    while(1) {
        n = read(fd, buf, 10);
        if (n > 0) {
            rw = buf[0];
			led = buf[1];
			mode = buf[2];
	        val = *((int *)(buf+3));
			
			/* Read and respond */
			if (rw == 0) {
				if (mode == 0) {
					val = read_on(led);
                    memcpy(buf, &val, 4);
                    write(fd, buf, 10);
				} else if (mode == 1) {
					val = read_duty(led);
	                memcpy(buf, &val, 4);
                    write(fd, buf, 10);
				} else {
					val = read_period(led);
		            memcpy(buf, &val, 4);
                    write(fd, buf, 10);
			    }

			/* Write */
			} else {
				if (mode == 0) {
					write_on(led, val);	
				} else if (mode == 1) {
					write_duty(led, val);
                    read_duty(0);
                    read_period(0);
                    read_on(0);
				} else {
					write_period(led, val);
				}
			}
        }
    }

    deinit();

    if (n < 0) {
        printf("Could not read.\n");
        return -4;
    }

    return 0;
}


