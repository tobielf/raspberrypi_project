1. How to build.
Command "make" will take over everything.
Just go to the folder "src" and type: make

2. How to run.
Run: sudo ./spi
It will read and display the digital value from 
channel 0 - 7 on the adc in console and exit.
It rely on the module MCP3208, you need to connect a
MCP3208 ADC to the SPI bus and CE0 on your RaspberryPi.
See more about MCP3208 on:
http://ww1.microchip.com/downloads/en/DeviceDoc/21298c.pdf

3. How to reuse this module.
This project come with the "Doxyfile", which allow 
you generate document using doxygen.
With the help of this document, you will know how 
to use this module. So, you can reuse the code elsewhere.
See more about doxygen on: 
http://www.stack.nl/~dimitri/doxygen/

4. How to contact.
email:Xiangyu.Guo@asu.edu

Have fun.