
# TEMCO ESP main App CODE

The code can run on any ESP32-WRoom hardware, here is a low cost example of our T3-nano which is a controller with no on board IO. It can work as a handy Bacnet / Modbus router passing both Bacnet and Modbus traffic over the Ethernet and the two RS485 ports, from any port to the other and back. 
[https://temcocontrols.com/shop/t3-nano/](https://temcocontrols.com/shop/t3-nano/)

![images](https://github.com/temcocontrols/T3-programmable-controller-on-ESP32/blob/master/Documents/t3nano.jpg)

This product establishes a TCP connection between Ethernet and the two RS485 subnets. The device can be accessed through the standard modbus and bacnet protocols. Most of the setting up and configuring is done using our T3000 application which also appears in our repos here on Github.
https://github.com/temcocontrols/T3000_Building_Automation_System

Once the device is configured T3000 is not required, you can use your usual modbus and bacnet tools to read & write data through the various communications ports. T3000 can also be used to write programs running on the ESP as well as set up alarms, floorplans, data logging, emailed alarms and more. If you want to use the ESP32 as a router only that is fine as well, it will transparently route messages from one network to the other using 'transparent mode'. 

## How to compile

Please refer to this documentation:
[How to compile.txt](https://github.com/temcocontrols/T3-programmable-controller-on-ESP32/blob/master/Documents/How%20to%20compile.txt)

## How to load the firmware to T3-nano
Please refer to the documentation in the following path.
[Bootloader update guide.pdf](https://github.com/temcocontrols/T3-programmable-controller-on-ESP32/blob/master/Documents/Bootloader%20update%20guide%20for%20T3.pdf)

## Schematic of T3-nano
[T3 Nano schematic](https://github.com/temcocontrols/T3-programmable-controller-on-ESP32/blob/master/Documents/T3_Nano_Schematic.pdf)


