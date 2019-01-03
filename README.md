# Astroberry DIY
Astroberry DIY provides the INDI drivers for Raspberry Pi devices:
* Astroberry Focuser - stepper motor driver with absolute and relative position capabilities and autofocus with INDI client such as KStars and Ekos
* Astroberry Board - power switch board allowing for remote powering on/off up to 4 devices and monitor system parameters

# Source
https://github.com/rkaczorek/astroberry-diy

# Requirements
* INDI available here http://indilib.org/download.html
* bcm2835 available here http://www.airspayce.com/mikem/bcm2835/ (no need to install separately, it will be downloaded and compiled during drivers compilation)
* CMake >= 2.4.7

# Installation
You need to download and install INDI server and libraries before compiling Astroberry DIY. See [INDI site](http://indilib.org/download.html) for more details.
In most cases it's enough to run:
```
sudo apt-add-repository ppa:mutlaqja/ppa
sudo apt-get update
sudo apt-get install indi-full
```

Then you can compile the drivers:
```
git clone https://github.com/rkaczorek/astroberry-diy.git
cd astroberry-diy
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make
sudo make install
```

# How to use it?
Start INDI server with the drivers:
```
indi_server indi_rpifocus indi_rpibrd
```
Start KStars with Ekos, connect to your INDI server and enjoy!

# What hardware is needed for Astroberry DIY drivers?

1. Astroberry Focuser
* Stepper motor eg. 28HS32-0674(200 steps/revolution 3,8V / 0,67A / 0,06Nm)
* Stepper motor controller eg. DRV8834 (10,8V / 2A)
  Wiring Raspberry Pi to the motor controller:
   - GPIO04 - DIR
   - GPIO17 - STEP
   - GPIO22 - M0
   - GPIO27 - M1
   - GPIO23 - SLEEP
     Note: Make sure you connect the stepper motor correctly to the controller (B2, B1 and A2, A1 pins on the controller).
           Remember to protect the 5V power line connected to VMOT pin on the motor controller with 100uF capacitor.

2. Astroberry Board
* Power switch board eg. YwRobot 4 relay
  Four IN pins, each switching ON/OFF a relay:
   - GPIO05 - IN1
   - GPIO06 - IN2
   - GPIO13 - IN3
   - GPIO26 - IN4
     Note: All inputs are set to HIGH by default. YwRobot board requires input to be LOW to swich ON a line.

# Changelog

v1.1.0
* indi_rpigps -	removed because already stable version is distributed with core INDI library
* indi_rpialtimu  - removed because separate project is maintained for this driver (https://github.com/rkaczorek/astroberry-altimu)
* indi_rpifocus -	stepper resolution, speed control and configurable max absolute position added

v1.0.5
* all -	Sync to latest libindi changes. Focuser controller mapping removed
* other -	Adding bcm2835-1.52 lib and setting statical linking

v1.0.4
* indi_rpifocus	-	Sync to latest libindi changes. Focuser controller mapping added
* other -	FindINDI/cmake updated to the latest version

v1.0.3
* indi_rpigps	-	Sync to latest INDI::GPS changes
* indi_rpibrd	-	Added: 1) system shutdown and restart buttons, 2) System info (board, uptime, load, ip) 3) System time info
* indi_rpifocus	-	Code improvements

v1.0.2
* indi_rpigps - Rewrite fo the driver to sync with libindi changes (GPS::INDI)
* indi_rpigs  - Adding Polaris Hour Angle for easy polar alignment
* indi_rpibrd - Shutdown button added for secure system power off

v1.0.1
* indi_rpigps - FIX for gps reading freeze and full integration with gpsd service (required)
* indi_rpifocus - FIX for lack of focuser status handling e.g. taking images during move
* indi_rpifocus - FIX for inconsistences in focus direction
* indi_rpifocus - disabling dualspeed mode. single speed mode only used (native)
* indi_rpialtimu  - change of values format from 0.3f to 0.1f

v1.0.0
* all - Initial release of astroberry drivers
