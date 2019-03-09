# Smart Environment Monitoring Device

Environment monitoring device making use of the BeagleBone Green development board and two offboard sensors:
   1) Texas Instruments Temperature Sensor: TMP102
      http://www.ti.com/lit/ds/symlink/tmp102.pdf
   2) Broadcom Light Sensor: APDS-9301
      https://www.broadcom.com/products/optical-sensors/ambient-light-photo-sensors/apds-9301

## Overview:
In this project, we will implement a “smart” environment monitoring device using a BeagleBone Green (BBG) and two offboard sensors: a temperature sensor and a light sensor. The sensors will connect to the BBG using the same I2C bus via grove connectors. The application will periodically monitor the sensors and, in that period, log data to a single file on the system. The application will also be capable of detecting exceptional conditions which occur outside of the monitoring period and log the occurrence to the same log file. Exceptional conditions will also be reported by flashing one of the three LEDs of the BBG.
The application shall consist of three threads, in addition to a main (master) process, which will spawn the three children threads: one thread will interface with the temperature sensor; another, with the light sensor; and, a third to take care of any logging of the system, and sub-systems, states. The main thread will also be responsible for maintaining a watchdog task to check for the health of the three children. 


