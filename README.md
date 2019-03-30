# Astroberry DIY
Astroberry DIY provides the INDI drivers for Raspberry Pi devices:
* Astroberry Focuser - stepper motor driver with absolute and relative position capabilities and autofocus with INDI client such as KStars and Ekos
* Astroberry Board - power switch board allowing for remote powering on/off up to 4 devices and monitor system parameters

# Source
https://github.com/rkaczorek/astroberry-diy

# Requirements
* INDI available here http://indilib.org/download.html
* bcm2835 available here http://www.airspayce.com/mikem/bcm2835/ (no need to install separately, it will be downloaded and compiled during drivers compilation)
* WiringPi (install it from http://wiringpi.com/download-and-install/)
* CMake >= 2.4.7

Note: Driver used BCM2835 by default. If you prefer to use WiringPi as a low level hardware library use -DWITH_WIRINGPI=ON at compilation time (see below)

# Installation
You need to download and install INDI server and libraries before compiling Astroberry DIY. See [INDI site](http://indilib.org/download.html) for more details.
In most cases it's enough to run:
```
sudo apt-get install cmake indi-full libindi-dev
```
If it does not work you probably don't have indi repository configured in your system. In such the case run:
```
sudo apt-add-repository ppa:mutlaqja/ppa
sudo apt-get update
sudo apt-get install cmake indi-full libindi-dev
```

Then you need to compile the drivers:
```
git clone https://github.com/rkaczorek/astroberry-diy.git
cd astroberry-diy
mkdir build && cd build
cmake -DWITH_WIRINGPI=OFF -DCMAKE_INSTALL_PREFIX=/usr ..
make
```
Note: Set -DWITH_WIRINGPI=ON to use WiringPi library as a low level hardware library instead of BCM2835 

You can install the drivers by running:
```
sudo make install
```
OR manually installing files by running:
```
sudo copy indi_rpifocus /usr/bin/
sudo copy indi_rpibrd /usr/bin/
sudo copy indi_rpifocus.xml /usr/share/indi/
sudo copy indi_rpibrd.xml /usr/share/indi/
```

# How to use it?
The easiest way is to run Kstars and select Astroberry Focuser (Focuser section) and/or Astroberry Board (Aux section) in Ekos profile editor.
Then save and start INDI server in Ekos. Alternatively you can start INDI server manually by running:
```
indi_server indi_rpifocus indi_rpibrd
```
Start KStars with Ekos, connect to your INDI server and enjoy!

# What hardware is needed for Astroberry DIY drivers?

1. Astroberry Focuser
* Stepper motor
* Stepper motor controller - DRV8834 and A4988 are supported
  Motor controller to Raspberry Pi wiring:
   - GPIO04 / PIN7 - DIR
   - GPIO17 / PIN11 - STEP
   - GPIO22 / PIN15 - M1 / M0
   - GPIO27 / PIN13 - M2 / M1
   - GPIO24 / PIN18 - M3 / N/A
   - GPIO23 / PIN16 - SLEEP

   Note: Make sure you connect the stepper motor correctly to the controller (B2, B1 and A2, A1 pins on the controller).
         Remember to protect the power line connected to VMOT of the motor controller with 100uF capacitor.

2. Astroberry Board
* Power switch board eg. YwRobot 4 relay
  Four IN pins, each switching ON/OFF a relay:
   - GPIO05 / PIN29 - IN1
   - GPIO06 / PIN31 - IN2
   - GPIO13 / PIN33 - IN3
   - GPIO19 / PIN35 - IN4

   Note: All inputs are set to HIGH by default. YwRobot board requires input to be LOW to swich ON a line.
