# Astroberry DIY
Astroberry DIY provides the INDI drivers for Raspberry Pi devices:
* Astroberry Focuser - stepper motor driver with absolute and relative position capabilities and autofocus with INDI client such as KStars and Ekos
* Astroberry Relays - relays switch board allowing for remote switching up to 4 devices
* Astroberry System - system parameters monitoring

Features:
* Astroberry Focuser
  - Support for virtually any stepper motor, including Moonlite, Robofocus
  - Support for DRV8834 and A4988 stepper controllers
  - Direct stepper motor control without proprietary drivers
  - Customizable GPIO pins
  - Absolute position control
  - Relative position control
  - Forward / Reverse direction configuration
  - Customizable maximum absolute position (steps)
  - Customizable maximum focuser travel (mm)
  - Resolution control from full step to 1/32 microsteps
  - Backlash compensation
  - Speed control
  - Focuser info including: critical focus zone in μm, step size in μm, steps per critical focus zone
  - Automatic temperature compensation based on DS18B20 temperature sensor
* Astroberry Relays
  - Support for virtually any relay controlled from GPIO
  - Up to 4 relays switches
  - Customizable GPIO pins
* Astroberry System
  - Provides system information such as local system time, UTC offset, hardware identification, uptime, system load, hostname, local IP, public IP

# Source
https://github.com/rkaczorek/astroberry-diy

# Requirements
* INDI available here http://indilib.org/download.html
* CMake >= 2.4.7

# Installation
You need to download and install required libraries before compiling Astroberry DIY. See [INDI site](http://indilib.org/download.html) for more details.
In most cases it's enough to run:
```
sudo apt-get install cmake libindi-dev libgpiod-dev
```
Then you can compile the driver:
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
sudo copy indi_astroberry_focuser /usr/bin/
sudo copy indi_astroberry_relays /usr/bin/
sudo copy indi_astroberry_system /usr/bin/
sudo copy indi_astroberry_focuser.xml /usr/share/indi/
sudo copy indi_astroberry_relays.xml /usr/share/indi/
sudo copy indi_astroberry_system.xml /usr/share/indi/

```

# How to use it?
Enable 1-Wire interface using raspi-config or adding 'dtoverlay=w1-gpio' to /boot/configure.txt for temperature compensation support (reboot required). Run Kstars and select Astroberry Focuser (Focuser section) and/or Astroberry Relays (Aux section) and/or Astroberry System (Aux section) in Ekos profile editor. Then start INDI server in Ekos with your profile, containg Astroberry drivers. Alternatively you can start INDI server manually by running:
```
indi_server indi_astroberry_focuser indi_astroberry_relays indi_astroberry_system
```
Start KStars with Ekos, connect to your INDI server and enjoy!

Note that your user account needs proper access right to /dev/gpiochip0 device. By default you can read/write only if you run driver as root or user who is a member of gpio group. Add your user to gpio group by running ```sudo usermod -a -G gpio $USER```

# What hardware is needed for Astroberry DIY drivers?

1. Astroberry Focuser
* A stepper motor
* Stepper motor controller - DRV8834 and A4988 are supported
  Starting from version 2.5 you can set your own BCM Pins on Options Tab!
  Default Motor Controller to Raspberry Pi GPIO wiring from v2.6 (changed!):
   - BCM23 / PIN16 - DIR
   - BCM24 / PIN18 - STEP
   - BCM22 / PIN15 - SLEEP + RST
   - BCM17 / PIN11 - M1/M0
   - BCM18 / PIN12 - M2/M1
   - BCM27 / PIN13 - M3/-

  Default Motor Controller to Raspberry Pi GPIO wiring before v2.6:
   - BCM04 / PIN7 - DIR
   - BCM17 / PIN11 - STEP
   - BCM23 / PIN16 - SLEEP + RST
   - BCM22 / PIN15 - M1/M0
   - BCM27 / PIN13 - M2/M1
   - BCM24 / PIN18 - M3/-

   Note: Make sure you connect the stepper motor correctly to the controller (B2, B1 and A2, A1 pins on the controller).
         Remember to protect the power line connected to VMOT of the motor controller with 100uF capacitor.
* DS18B20 temperature sensor connected to BCM4 / PIN7 for temperature reading and automatic temperature compensation
   Note: You need to use external 4k7 ohm pull-up resistor connected to data pin of DS18B20 sensor

2. Astroberry Relays
* Relay switch board eg. YwRobot 4 relay
  Default pins, each switching ON/OFF a relay (active-low). Starting from version 2.5 you can set your own BCM Pins on Options Tab!
   - BCM05 / PIN29 - IN1
   - BCM06 / PIN31 - IN2
   - BCM13 / PIN33 - IN3
   - BCM19 / PIN35 - IN4

   Note: All inputs are set to HIGH by default. Most relay boards require input to be LOW to swich ON a line.
