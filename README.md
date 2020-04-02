Home Alone Monitor

This documentation presents a small electronic gadget called Home Alone. Some folks concerned with the well being 
of elderly relatives living alone could use it to keep track of the movements of their loved ones. 
This gadget detects, counts and 
reports movements in its surroundings. For privacy reasons no camera is used, a simple PIR sensor (Pyroelectric Infrared Sensor) 
detects movement. 
Movements (or the absence of it) are reported to the cloud (Thingspeak). Concerned party (or parties) can inspect 
the data on the Thingspeak website. A public channel is used to show the movement data.
It is based on an idea that I found on YouTube. Ralph Bacon presented his device using an ESP8266 and I heavily borrowed from his ideas.
However, I felt that I wanted to use an ESP32 to make used of the multitasking capabilities of this chip.

I used the Arduino IDE to develop the software. Hardware is based on the Adafruit Feather series of boards.

Check out the project description for all the details.

Documentation and code on GitHub.  

Free to use, modify, and distribute with proper attribution.
Frei für jedermann, vollständige Quellangabe vorausgesetzt.

