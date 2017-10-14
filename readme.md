NetTempHumSensORly
------------------

Network or serially accessible DHT22 based temperature and humidity sensor.

Hardware
--------

- Board: Arduino Nano
- Processor: ATmega328
- Ethernet Shield (HanRun HR911105A 15/27)

Serial protocoll
----------------
    Requesting sensor data:
      Send     'r'
      Receive  'T<temperature>:H<humidity>'

    Turn on auto mode (keeps periodically sending sensor data)
      Send 'a'

    Turn off auto mode
      Send 's'

Network protocoll
-----------------

The network protocoll is completely http based.

Urls:

    Receive a plain HTML view:
        http://ip-of-device/

    Receive a JSON formatted response:
        http://ip-of-device/j

    Receive a CSV formatted response:
        http://ip-of-device/c

Supporting software
-------------------

**Client/**

nths-client

Python based utility to access sensor data via http or serial and write
it to file.

nths-serial-web-bridge

bash script to either grab data and publish it via netcat or periodically write
to a file.

**Web/**

Contains a small html document which consumes the CSV based data and displays
a neat graph.

Wiring
------

    Arduino direct through Ethernet Shield (HanRun HR911105A 15/27)

    A.5V  <-> DHT22.(+)
    A.GND <-> DHT22.(-)
    A.D3  <-> DHT22.OUT
