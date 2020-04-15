/*
 * API for e-scooter display module.
 * Module has regulator xl7015e1 so should run on 7-80Vdc, tested with 12V and 36V batteries.
 * Logic level is 5V so should use something like BOB-12009 if using 3.3V for control.
 * If required, use non isolated dc-dc -> 5V converter for reference serial voltage.
 * Wires: yellow/green serial, black/red power, blue/white throttle position and brake (not used).
 */
#include "mbed.h"

#define NCHAR 14
// led light
#define OFF 0
#define ON 1
#define BLINK 2
// operation modes
#define BLANK 0
#define CHARGE 1
#define SPEED 2

// serial pins
Serial pc(SERIAL_TX, SERIAL_RX);
Serial disp(D5, D4);


void disp_received()
{
    // forward received messages
    // as raw bytes so need to listen appropriately
    while(disp.readable()) {
        pc.printf("%c", disp.getc());
    }
}

void set_function(char* bytes, int mode)
{
    // 0 screen off, 1 charging, >1 driving
    bytes[4] = mode;
    // cannot turn off screen if led on enabled, can be flashing though
    if (mode == BLANK) bytes[10] = 0xF0;
}

void set_led(char* bytes, int mode)
{
    // set attached LED lamp to off, on, or flashing
    if (mode == OFF) {
        bytes[10] = 0x00;
        bytes[11] = 0x00;
        bytes[12] = 0x00;
    } else if (mode == ON) {
        bytes[10] = 0xF1;
        bytes[11] = 0x80;
        bytes[12] = 0x00;
    } else if (mode == BLINK) {
        bytes[12] = 0x80;
    }
}

void set_battery(char* bytes, int thousandths)
{
    // battery in thousandths stored as 2 byte integer
    bytes[6] = thousandths >> 8;
    bytes[7] = thousandths - (bytes[6] << 8);
}

void set_speed(char* bytes, double value)
{
    // speed in device units stored as integer 2236/speed
    // floating point on device truncates past first decimal place
    // result may be off so add 0.05 to nearest 0.1, also prevents /0

    // very bad method and starts skipping values, more towards higher values
    // cannot display anything between 0 and 1
    // a few more unique sequence:values that could be used:
    // 2:94.1, 3:73.4, 4:47, 5:31.2, 7:63.4, 9:88.4, 10:63.6, 11:43.3

    int sequence = (int)(2236.0/((int)(value/0.1+0.5)*0.1+0.05) + 0.5);
    bytes[8] = sequence >> 8;
    bytes[9] = sequence - (bytes[8] << 8);
}

void update_checksum(char* bytes)
{
    // XOR checksum of all but last bytes stored in last byte
    char cksum = 0;

    for (int i=0; i<NCHAR-1; i++) {
        cksum ^= bytes[i];
    }
    bytes[NCHAR-1] = cksum;
}

void init_message(char* bytes)
{
    bytes[0] = 0x02; // start transmission? seems only 0x02 works.
    bytes[1] = 0x0E; // no effect?
    bytes[2] = 0x01; // no effect?
    bytes[3] = 0x00; // error number (0 no error), blink if charging or always on if driving
    bytes[4] = 0x01; // screen on in conditions: charging (1), driving (2)
    bytes[5] = 0x00; // unknown
    bytes[6] = 0x00; // bat0 first byte perthousands int
    bytes[7] = 0x00; // bat1 second byte perthousands int
    bytes[8] = 0x08; // speed0 first byte int
    bytes[9] = 0xF0; // speed1 second byte int (2236/int = speed, 2nd decimal truncated, sensitive to float error +- 0.1)
    bytes[10] = 0xF0; // 0xF1 enable led on but can't disable screen
    bytes[11] = 0x00; // 0x8- led on (binary pos for 8 on)
    bytes[12] = 0x00; // 0x8- blink led (binary pos for 8 on) overrides led on, byte 10 not needed
    // bytes[13] is the checksum and is calculated as part of the send procedure
}

void send_message(char* bytes)
{
    // send a message, must be sent as bytes
    update_checksum(bytes);
    for (int i=0; i<NCHAR; i++) {
        pc.printf("%c", bytes[i]);
        disp.printf("%c", bytes[i]);
    }
}

int main()
{
    pc.baud(9600);
    disp.baud(9600);
    disp.attach(&disp_received);

    char msg[NCHAR];
    init_message(msg);

    // demo
    while (true) {
        // cycle through speed
        // maximum displayable 97.2
        set_function(msg, SPEED);
        set_led(msg, ON);
        for (int i=0; i<990; i++) {
            set_speed(msg, i * 0.1);
            send_message(msg);
            wait(0.1);
        }

        // disable screen
        set_function(msg, BLANK);
        send_message(msg);
        wait(5);

        // cycle through charge
        // charge perthousands 0 -> 999
        set_function(msg, CHARGE);
        set_led(msg, BLINK);
        for (int i=0; i<1000; i++) {
            set_battery(msg, i);
            send_message(msg);
            wait(0.1);
        }
    }
}
