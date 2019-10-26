# Astroberry DIY
Astroberry DIY provides the INDI drivers for Raspberry Pi devices:
* Astroberry Focuser - stepper motor driver with absolute and relative position capabilities and autofocus with INDI client such as KStars and Ekos
* Astroberry Board - power switch board allowing for remote powering on/off up to 4 devices and monitor system parameters

# Source
https://github.com/rkaczorek/astroberry-diy

# Requirements
* INDI available here http://indilib.org/download.html
* WiringPi (install it from http://wiringpi.com/download-and-install/)
* CMake >= 2.4.7

# Installation
You need to download and install required libraries before compiling Astroberry DIY. See [INDI site](http://indilib.org/download.html) for more details.
In most cases it's enough to run:
```
sudo apt-get install cmake indi-full libindi-dev wiringpi
```
Then you need to compile the drivers:
```
git clone https://github.com/rkaczorek/astroberry-diy.git
cd astroberry-diy
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make
```
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
   - GPIO23 / PIN16 - SLEEP + RST

   Note: Make sure you connect the stepper motor correctly to the controller (B2, B1 and A2, A1 pins on the controller).
         Remember to protect the power line connected to VMOT of the motor controller with 100uF capacitor.

2. Astroberry Board
* Power switch board eg. YwRobot 4 relay
  Four IN pins, each switching ON/OFF a relay:
   - GPIO05 / PIN29 - IN1
   - GPIO06 / PIN31 - IN2
   - GPIO13 / PIN33 - IN3
   - GPIO19 / PIN35 - IN4

   Note: All inputs are set to HIGH by default. Most relay boards require input to be LOW to swich ON a line.
