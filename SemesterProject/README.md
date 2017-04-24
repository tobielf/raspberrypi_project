# Smarthomd
---
## Hardware Connection
**Using WiringPi number**

### LED pins
Pin 7, 0, 2, 3, 25.

### SPI pins
MOSI MISO SCLK 3V3

SPI Module: 

MCP3208 - 8Channel Analog Digital Converter.

### I2C pins
SDA SCL

I2C Module: 

BMP180 - Temperature and Pressure Sensor.

LCD1602 - 2 Line 16 Characters LCD Display.

### L293D pins
Pin 21(LEFT), 22(RIGHT), 23(ENABLE)

### Button pins
Pin 6(UP), 5(DOWN), 26(LEFT), 4(RIGHT)

### Alarm Light
Pin 24

### Mecury Switcher
Pin 27

### DHT11
Pin 28

### Motion Detector
Pin 29

## Software Setting
1. How to build.
Command `make` will take over everything.
Just go to the folder "src" and type: `make`

2. How to run.
Run: `sudo ./bin/smarthomed`
It will start a web server listening on `<yourIP>:80`. And you can interact with the screen display to check value.

3. Send your Siri or Google Assistant request to following URL and it will give you the response.
> "LED ON": GET "http://`<Your IP>`/switch/on?led=`<LED Number>`",
> 
> Response: 200 OK
> 
> "LED OFF": GET "http://`<Your IP>`/switch/off?led=`<LED Number>`",
> 
> Response: 200 OK
> 
> "LED STATUS": GET "http://`<Your IP>`/status?led=`<LED Number>`",
> 
> Response: 200 OK, data: `0` off, `1` on.
> 
> "BMP180": GET "http://`<Your IP>`/temp/status",
> 
> Response: 200 OK, data: `{"temperature": 24.5, "humidity": 0%}`
> 
> "DHT11": GET "http://`<Your IP>`/temp_humi/status"
> 
> Response: 200 OK, data: `{"temperature": 21.5, "humidity": 30%}`

4. How to reuse this module.
This project come with the "Doxyfile", which allow 
you generate document using doxygen.
With the help of this document, you will know how 
to use this module. So, you can reuse the code elsewhere.
See more about doxygen on: 
<http://www.stack.nl/~dimitri/doxygen/>

5. Demo Video
	
	ToDo

6. How to contact.
email:<Xiangyu.Guo@asu.edu>