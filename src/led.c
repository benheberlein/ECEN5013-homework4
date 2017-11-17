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
 * @file led.c
 * @brief Userspace application for the LED driver
 * 
 * This application demonstrates usage of the LED device driver for the Beagle
 * Bone Green.
 *
 * @author Ben Heberlein
 * @date Nov 14 2017
 * @version 1.0
 *
 */

#include <stdio.h>

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

#define LED0 0
#define LED1 1
#define LED2 2
#define LED3 3

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
}

static void write_period(int led, int per) {
    read_leds();
    led_state[led].period = per;
    write_leds();
    printf("Wrote LED%d period to %d\n", led, per);
}

static void write_on(int led, int on) {
    read_leds();
    led_state[led].on = on;
    write_leds();
    printf("Wrote LED%d on/off to %d\n", led, on);
}

static void init(void) {
    f = fopen("/dev/led_driver", "rb+");
    read_leds();
}

static void deinit(void) {
    fclose(f);
}

int main() {    

    init();
    read_duty(LED0);
    read_period(LED0);
    read_on(LED0);
    write_duty(LED0, 50);
    write_period(LED0, 4000);
    write_on(LED0, 1);
    read_duty(LED0);
    read_period(LED0);
    read_on(LED0);

    while(1) {}

    deinit();

    return 0;
}
