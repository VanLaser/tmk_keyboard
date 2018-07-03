#!/bin/bash

avrdude -v -p atmega32u4 -c avr109 -P /dev/ttyACM0 -U flash:w:ps2_usb_laser.hex:i

