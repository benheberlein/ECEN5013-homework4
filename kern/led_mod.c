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
 * @file led_mod.c
 * @brief Device driver for LEDs on the Beagle Bone Green board
 * 
 * This is a kernel module that implements LED functionality on the Beagle
 * Bone Green board.
 *
 * @author Ben Heberlein
 * @date Nov 14 2017
 * @version 1.0
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>

MODULE_DESCRIPTION("LED char driver on Beagle Bone Green");
MODULE_AUTHOR("Ben Heberlein");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

static ssize_t led_read(struct file *f, char *c, size_t s, loff_t *l);
static ssize_t led_write(struct file *f, const char *c, size_t s, loff_t *l);
static int led_open(struct inode *in, struct file *f);
static int led_release(struct inode *in, struct file *f);

static void led_exit(void);
static int led_init(void);

static int major_number;
static struct class *class = NULL;
static struct device *dev = NULL;
#define LED_DEV "led_driver"
#define LED_CLASS "led_driver"
#define LED0 53
#define LED1 54
#define LED2 55
#define LED3 56

static struct timer_list tm[4];
static int timer_state[4] = {0,0,0,0};     /* Keep track of duty cycle */

__attribute((packed)) struct state_s {
    int on;             /* On or off */
    int duty_cycle;     /* From 0 to 100 */
    int period;         /* In milliseconds */
};
static struct state_s led_state[4] = {
    {1, 50, 1000}, 
    {1, 50, 1000}, 
    {1, 50, 1000}, 
    {1, 50, 1000}
};
#define MSG_SIZE 12*4
static char *test_message = "Hello world.\n";

static struct file_operations file_ops = {
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_release,
    .owner = THIS_MODULE,
};

static void __led_set(int led, bool on) {
    gpio_set_value(led, on);
}

void __timerx(int l) {

    if (led_state[l].on) {
        /* If LED is on */
        if (timer_state[l] == 0) {
            mod_timer(&tm[l], msecs_to_jiffies(led_state[l].period * (100 - led_state[l].duty_cycle) / 100) + jiffies);
            timer_state[l] = 1;
            __led_set(l+LED0, true);
            printk(KERN_INFO "LED driver: Timer fired. LED on.");
        } else {
            mod_timer(&tm[l], msecs_to_jiffies(led_state[l].period * led_state[l].duty_cycle / 100) + jiffies);
            timer_state[l] = 0;
            __led_set(l+LED0, false);
            printk(KERN_INFO "LED driver: Timer fired. LED off.");
        }
    } else {
        /* Just turn LED off */
        __led_set(l+LED0, false);
    }
}

void __timer0(unsigned long data) {
    __timerx(0);
}

void __timer1(unsigned long data) {
    __timerx(1);
}

void __timer2(unsigned long data) {
    __timerx(2);
}

void __timer3(unsigned long data) {
    __timerx(3);
}

static int __init led_init(void) {
    printk(KERN_INFO "LED driver: Initializing");

    /* Register a major number */
    major_number = register_chrdev(0, LED_DEV, &file_ops);
    if (major_number < 0) {
        printk(KERN_WARNING "LED driver: Could not get major number.");
        return -1;
    }

    /* Register device class */
    class = class_create(THIS_MODULE, LED_CLASS);
    if (IS_ERR(class)) {
        unregister_chrdev(major_number, LED_DEV);
        printk(KERN_WARNING "LED driver: Could not register class.");
        return -2;
    }

    /* Register device */
    dev = device_create(class, NULL, MKDEV(major_number, 0), NULL, LED_DEV);
    if (IS_ERR(dev)) {
        class_destroy(class);
        unregister_chrdev(major_number, LED_DEV);
        printk(KERN_WARNING "LED driver: Could not register device.");
        return -3;
    }

    printk(KERN_INFO "LED driver: Successfully initialized driver.");

    /* Init timers */
    setup_timer(&tm[0], __timer0, 0);
    setup_timer(&tm[1], __timer1, 0);
    setup_timer(&tm[2], __timer2, 0);
    setup_timer(&tm[3], __timer3, 0);
    
    mod_timer(&tm[0], msecs_to_jiffies(led_state[0].period * led_state[0].duty_cycle / 100) + jiffies);
    mod_timer(&tm[1], msecs_to_jiffies(led_state[1].period * led_state[1].duty_cycle / 100) + jiffies);
    mod_timer(&tm[2], msecs_to_jiffies(led_state[2].period * led_state[2].duty_cycle / 100) + jiffies);
    mod_timer(&tm[3], msecs_to_jiffies(led_state[3].period * led_state[3].duty_cycle / 100) + jiffies);

    led_state[0].period = 250;
    led_state[1].period = 500;
    led_state[2].period = 1000;
    led_state[3].period = 2000;

    __led_set(LED0, false);
    __led_set(LED1, false);
    __led_set(LED2, false);
    __led_set(LED3, false);

    return 0;
}

static void __exit led_exit(void) {

    /* Unregister device and free class */
    device_destroy(class, MKDEV(major_number, 0));
    class_unregister(class);
    class_destroy(class);
    unregister_chrdev(major_number, LED_DEV);
    printk(KERN_INFO "LED driver: Exiting.");

    /* Uninit timers */
    del_timer(&tm[0]);
    del_timer(&tm[1]);
    del_timer(&tm[2]);
    del_timer(&tm[3]);

    __led_set(LED3, true);
}

static ssize_t led_read(struct file *f, char *c, size_t s, loff_t *l) {
    int err = 0;

    printk(KERN_INFO "LED driver: Called read.");

    err = copy_to_user(c, &led_state, MSG_SIZE);
    if (err == 0) {
        printk(KERN_INFO "LED driver: Read %d characters.", MSG_SIZE);
        return 0;
    } else {
        printk(KERN_WARNING "LED driver: Read failed");
        return -1;
    }

    return 0;
}

static ssize_t led_write(struct file *f, const char *c, size_t s, loff_t *l) {
    int err = 0;

    printk(KERN_INFO "LED driver: Called write.");

    if (s != MSG_SIZE) {
        return -1;
    }

    err = copy_from_user(&led_state, c, s);
    
    if (err == 0) {
        printk(KERN_INFO "LED driver: Wrote %d characters.", MSG_SIZE);
        return s;
    } else {
        printk(KERN_WARNING "LED driver: Write failed.");
        return -2;
    }

    return s;
}

static int led_open(struct inode *in, struct file *f) {
    printk(KERN_INFO "LED driver: Called open.");
    return 0;
}

static int led_release(struct inode *in, struct file *f) {
    printk(KERN_INFO "LED driver: Called release.");
    return 0;
}

module_init(led_init);
module_exit(led_exit);
