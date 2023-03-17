
# TEMCO ESP main App CODE

The code can run on any ESP32-WRoom hardware, here is a low cost example of out T3-nano which is a controller with no on board IO. It can work as a handy Bacnet / Modbus router passing both Bacnet and Modbus traffic over the Ethernet and the two RS485 ports, from any port to the other and back. 
[https://temcocontrols.com/shop/t3-nano/](https://temcocontrols.com/shop/t3-nano/)

![images](https://github.com/temcocontrols/T3-programmable-controller-on-ESP32/blob/master/Documents/t3nano.jpg)

This application establishes a TCP connection between Ethernet and WIFI, and an RS485 connection. The device can be accessed through the standard modbus protocol, so that the value of the sensor on the device can be obtained. A more intuitive way is to scan and read the device through the T3000 software of TEMCO CONTROLS.

## How to compile

Please refer to this documentation:
[How to compile.txt](https://github.com/temcocontrols/T3-programmable-controller-on-ESP32/blob/master/Documents/How%20to%20compile.txt)

## How to load the firmware to T3-nano
Please refer to the documentation in the following path.
[Bootloader update guide.pdf](https://github.com/temcocontrols/T3-programmable-controller-on-ESP32/blob/master/Documents/Bootloader%20update%20guide%20for%20T3.pdf)

## Schematic of T3-nano
[T3 Nano schematic](https://github.com/temcocontrols/T3-programmable-controller-on-ESP32/blob/master/Documents/T3_Nano_Schematic.pdf)


