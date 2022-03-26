/*******************************************************************************
  Copyright(c) 2014-2022 Radek Kaczorek  <rkaczorek AT gmail DOT com>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

/*
 * TO DO:
 * - Save position in xml instead flat file
 * - Add Thermal expansion ratio selection for various materials
 * - Add temperature compensation auto learning and save temperature compensation curve to xml
 * - Add simulation mode
 */

#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <fstream>
#include <math.h>
#include <memory>
#include "config.h"

#include <gpiod.h>

#include "astroberry_focuser.h"

// We declare an auto pointer to AstroberryFocuser.
std::unique_ptr<AstroberryFocuser> astroberryFocuser(new AstroberryFocuser());

// create millisecond sleep macro
#define msleep(milliseconds) usleep(milliseconds * 1000)

#define MINMAX_MIN_POS 0 // lowest limit for focuser position
#define MINMAX_MAX_POS 100000 // highest limit for focuser position
#define MAX_RESOLUTION 32 // the highest resolution supported is 1/32 step
#define TEMPERATURE_UPDATE_TIMEOUT (60 * 1000) // 60 sec
#define TEMPERATURE_COMPENSATION_TIMEOUT (60 * 1000) // 60 sec

void ISPoll(void *p);


void ISInit()
{
	static int isInit = 0;

	if (isInit == 1)
	return;
	if(astroberryFocuser.get() == 0)
	{
		isInit = 1;
		astroberryFocuser.reset(new AstroberryFocuser());
	}
}

void ISGetProperties(const char *dev)
{
	ISInit();
	astroberryFocuser->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
	ISInit();
	astroberryFocuser->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(	const char *dev, const char *name, char *texts[], char *names[], int num)
{
	ISInit();
	astroberryFocuser->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
	ISInit();
	astroberryFocuser->ISNewNumber(dev, name, values, names, num);
}

void ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n)
{
	INDI_UNUSED(dev);
	INDI_UNUSED(name);
	INDI_UNUSED(sizes);
	INDI_UNUSED(blobsizes);
	INDI_UNUSED(blobs);
	INDI_UNUSED(formats);
	INDI_UNUSED(names);
	INDI_UNUSED(n);
}

void ISSnoopDevice (XMLEle *root)
{
	astroberryFocuser->ISSnoopDevice(root);
}

AstroberryFocuser::AstroberryFocuser()
{
	setVersion(VERSION_MAJOR,VERSION_MINOR);

	FI::SetCapability(
		FOCUSER_CAN_ABS_MOVE	|
		FOCUSER_CAN_REL_MOVE	|
		FOCUSER_CAN_REVERSE 	|
		FOCUSER_HAS_BACKLASH	|
		FOCUSER_CAN_SYNC 	|
		FOCUSER_CAN_ABORT
		// FOCUSER_HAS_VARIABLE_SPEED
		);

	Focuser::setSupportedConnections(CONNECTION_NONE);
}

AstroberryFocuser::~AstroberryFocuser()
{
	deleteProperty(MotorBoardSP.name);
	deleteProperty(BCMpinsNP.name);
}

const char * AstroberryFocuser::getDefaultName()
{
        return (char *)"Astroberry Focuser";
}

bool AstroberryFocuser::Connect()
{
	chip = gpiod_chip_open("/dev/gpiochip0");
	if (!chip)
	{
		DEBUG(INDI::Logger::DBG_ERROR, "Problem initiating Astroberry Focuser.");
		return false;
	}

	// verify BCM Pins are not used by other consumers
	for (unsigned int pin = 0; pin < 6; pin++)
	{
		if (gpiod_line_is_used(gpiod_chip_get_line(chip, BCMpinsN[pin].value)))
		{
			DEBUGF(INDI::Logger::DBG_ERROR, "BCM Pin %0.0f already used", BCMpinsN[pin].value);
			gpiod_chip_close(chip);
			return false;
		}
	}

	// Select gpios
	gpio_dir = gpiod_chip_get_line(chip, BCMpinsN[0].value);
	gpio_step = gpiod_chip_get_line(chip, BCMpinsN[1].value);
	gpio_sleep = gpiod_chip_get_line(chip, BCMpinsN[2].value);
	gpio_m1 = gpiod_chip_get_line(chip, BCMpinsN[3].value);
	gpio_m2 = gpiod_chip_get_line(chip, BCMpinsN[4].value);
	gpio_m3 = gpiod_chip_get_line(chip, BCMpinsN[5].value);

	// Set initial state for gpios
	gpiod_line_request_output(gpio_dir, "dir@astroberry_focuser", 1); // default direction is outward
	gpiod_line_request_output(gpio_step, "step@astroberry_focuser", 0);
	gpiod_line_request_output(gpio_sleep, "sleep@astroberry_focuser", 1); // start stepper in wake up state
	gpiod_line_request_output(gpio_m1, "m1@astroberry_focuser", 0);
	gpiod_line_request_output(gpio_m2, "m2@astroberry_focuser", 0);

	// If A4988 controller, use additional GPIO
	if ( MotorBoardS[1].s == ISS_ON )
		gpiod_line_request_output(gpio_m3, "m3@astroberry_focuser", 0);

	//read last position from file & convert from MAX_RESOLUTION to current resolution
	FocusAbsPosN[0].value = savePosition(-1) != -1 ? (int) savePosition(-1) * resolution / MAX_RESOLUTION : 0;

	// preset resolution
	setResolution(resolution);

	// Lock Motor Board setting
	MotorBoardSP.s=IPS_BUSY;
	IDSetSwitch(&MotorBoardSP, nullptr);

	// Lock BCM Pins setting
	BCMpinsNP.s=IPS_BUSY;
	IDSetNumber(&BCMpinsNP, nullptr);

	// Update focuser parameters
	getFocuserInfo();

	// set motor standby timer
	if ( StepperStandbyS[0].s == ISS_ON)
	{
		stepperStandbyID = IEAddTimer(StepperStandbyTimeN[0].value * 1000, stepperStandbyHelper, this);
		DEBUGF(INDI::Logger::DBG_SESSION, "Focuser going standby in %d seconds", (int) IERemainingTimer(stepperStandbyID) /  1000);
	} else {
		stepperStandbyID = -1;
	}

	DEBUG(INDI::Logger::DBG_SESSION, "Astroberry Focuser connected successfully.");

	return true;
}

bool AstroberryFocuser::Disconnect()
{
	// Stop timers
	IERmTimer(stepperStandbyID);
	IERmTimer(updateTemperatureID);
	IERmTimer(temperatureCompensationID);

	// Set stepper motor asleep
	gpiod_line_set_value(gpio_sleep, 0);

	// Close device
	gpiod_chip_close(chip);

	// Unlock Motor Board setting
	MotorBoardSP.s=IPS_IDLE;
	IDSetSwitch(&MotorBoardSP, nullptr);

	// Unlock BCM Pins setting
	BCMpinsNP.s=IPS_IDLE;
	IDSetNumber(&BCMpinsNP, nullptr);

	DEBUG(INDI::Logger::DBG_SESSION, "Astroberry Focuser disconnected successfully.");

	return true;
}

bool AstroberryFocuser::initProperties()
{
	INDI::Focuser::initProperties();

	// Focuser Resolution
	IUFillSwitch(&FocusResolutionS[0],"FOCUS_RESOLUTION_1","Full Step",ISS_ON);
	IUFillSwitch(&FocusResolutionS[1],"FOCUS_RESOLUTION_2","1/2 Step",ISS_OFF);
	IUFillSwitch(&FocusResolutionS[2],"FOCUS_RESOLUTION_4","1/4 Step",ISS_OFF);
	IUFillSwitch(&FocusResolutionS[3],"FOCUS_RESOLUTION_8","1/8 Step",ISS_OFF);
	IUFillSwitch(&FocusResolutionS[4],"FOCUS_RESOLUTION_16","1/16 Step",ISS_OFF);
	IUFillSwitch(&FocusResolutionS[5],"FOCUS_RESOLUTION_32","1/32 Step",ISS_OFF);
	IUFillSwitchVector(&FocusResolutionSP,FocusResolutionS,6,getDeviceName(),"FOCUS_RESOLUTION","Resolution",MAIN_CONTROL_TAB,IP_RW,ISR_1OFMANY,0,IPS_IDLE);

	// Maximum focuser travel
	IUFillNumber(&FocuserTravelN[0], "FOCUSER_TRAVEL_VALUE", "mm", "%0.0f", 10, 200, 10, 10);
	IUFillNumberVector(&FocuserTravelNP, FocuserTravelN, 1, getDeviceName(), "FOCUSER_TRAVEL", "Max Travel", MAIN_CONTROL_TAB, IP_RW, 0, IPS_IDLE);

	// Focuser Info
	IUFillNumber(&FocuserInfoN[0], "CFZ_STEP_ACT", "Step Size (μm)", "%0.2f", 0, 1000, 1, 0);
	IUFillNumber(&FocuserInfoN[1], "CFZ", "Critical Focus Zone (μm)", "%0.2f", 0, 1000, 1, 0);
	IUFillNumber(&FocuserInfoN[2], "STEPS_PER_CFZ", "Steps / Critical Focus Zone", "%0.0f", 0, 1000, 1, 0);
	IUFillNumberVector(&FocuserInfoNP, FocuserInfoN, 3, getDeviceName(), "FOCUSER_PARAMETERS", "Focuser Info", MAIN_CONTROL_TAB, IP_RO, 0, IPS_IDLE);

	// Focuser Stepper Controller
	IUFillSwitch(&MotorBoardS[0],"DRV8834","DRV8834",ISS_ON);
	IUFillSwitch(&MotorBoardS[1],"A4988","A4988",ISS_OFF);
	IUFillSwitchVector(&MotorBoardSP,MotorBoardS,2,getDeviceName(),"MOTOR_BOARD","Control Board",OPTIONS_TAB,IP_RW,ISR_1OFMANY,0,IPS_IDLE);

	// BCM PINs setting
	IUFillNumber(&BCMpinsN[0], "BCMPIN_DIR", "DIR", "%0.0f", 1, 27, 0, 23); // BCM23 = PIN16
	IUFillNumber(&BCMpinsN[1], "BCMPIN_STEP", "STEP", "%0.0f", 1, 27, 0, 24); // BCM24 = PIN18
	IUFillNumber(&BCMpinsN[2], "BCMPIN_SLEEP", "SLEEP", "%0.0f", 1, 27, 0, 22); // BCM22 = PIN15
	IUFillNumber(&BCMpinsN[3], "BCMPIN_M1", "M1", "%0.0f", 1, 27, 0, 17); // BCM17 = PIN11
	IUFillNumber(&BCMpinsN[4], "BCMPIN_M2", "M2", "%0.0f", 1, 27, 0, 18); // BCM18 = PIN12
	IUFillNumber(&BCMpinsN[5], "BCMPIN_M3", "M3", "%0.0f", 1, 27, 0, 27); // BCM27 = PIN13
	IUFillNumberVector(&BCMpinsNP, BCMpinsN, 6, getDeviceName(), "BCMPINS", "BCM Pins", OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

	// Stepper standby setting
	IUFillSwitch(&StepperStandbyS[0],"STEPPER_STANDBY_ON","Enable",ISS_ON);
	IUFillSwitch(&StepperStandbyS[1],"STEPPER_STANDBY_OFF","Disable",ISS_OFF);
	IUFillSwitchVector(&StepperStandbySP,StepperStandbyS,2,getDeviceName(),"STEPPER_STANDBY","Standby",OPTIONS_TAB,IP_RW,ISR_1OFMANY,0,IPS_IDLE);

	// Stepper standby time setting
	IUFillNumber(&StepperStandbyTimeN[0], "STEPPER_STANDBY_DELAY_VALUE", "seconds", "%0.0f", 0, 600, 10, 60);
	IUFillNumberVector(&StepperStandbyTimeNP, StepperStandbyTimeN, 1, getDeviceName(), "STEPPER_STANDBY_DELAY", "Standby Delay", OPTIONS_TAB, IP_RW, 0, IPS_IDLE);	

	// Step delay setting
	IUFillNumber(&FocusStepDelayN[0], "FOCUS_STEPDELAY_VALUE", "milliseconds", "%0.0f", 1, 10, 1, 1);
	IUFillNumberVector(&FocusStepDelayNP, FocusStepDelayN, 1, getDeviceName(), "FOCUS_STEPDELAY", "Step Delay", OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

	// Active telescope setting
	IUFillText(&ActiveTelescopeT[0], "ACTIVE_TELESCOPE_NAME", "Telescope", "Telescope Simulator");
	IUFillTextVector(&ActiveTelescopeTP, ActiveTelescopeT, 1, getDeviceName(), "ACTIVE_TELESCOPE", "Snoop devices", OPTIONS_TAB,IP_RW, 0, IPS_IDLE);

	// Focuser temperature
	IUFillNumber(&FocusTemperatureN[0], "FOCUS_TEMPERATURE_VALUE", "°C", "%0.2f", -50, 50, 1, 0);
	IUFillNumberVector(&FocusTemperatureNP, FocusTemperatureN, 1, getDeviceName(), "FOCUS_TEMPERATURE", "Temperature", MAIN_CONTROL_TAB, IP_RO, 0, IPS_IDLE);

	// Temperature Coefficient
	IUFillNumber(&TemperatureCoefN[0], "μm/m°C", "", "%.1f", 0, 50, 1, 0);
	IUFillNumberVector(&TemperatureCoefNP, TemperatureCoefN, 1, getDeviceName(), "Temperature Coefficient", "", MAIN_CONTROL_TAB, IP_RW, 0, IPS_IDLE);

	// Compensate for temperature
	IUFillSwitch(&TemperatureCompensateS[0], "Enable", "", ISS_OFF);
	IUFillSwitch(&TemperatureCompensateS[1], "Disable", "", ISS_ON);
	IUFillSwitchVector(&TemperatureCompensateSP, TemperatureCompensateS, 2, getDeviceName(), "Temperature Compensate", "", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

	// Snooping params
	IUFillNumber(&ScopeParametersN[0], "TELESCOPE_APERTURE", "Aperture (mm)", "%g", 10, 5000, 0, 0.0);
	IUFillNumber(&ScopeParametersN[1], "TELESCOPE_FOCAL_LENGTH", "Focal Length (mm)", "%g", 10, 10000, 0, 0.0);
	IUFillNumberVector(&ScopeParametersNP, ScopeParametersN, 2, ActiveTelescopeT[0].text, "TELESCOPE_INFO", "Scope Properties", OPTIONS_TAB, IP_RW, 60, IPS_OK);

	// initial values at resolution 1/1
	FocusMaxPosN[0].min = MINMAX_MIN_POS; // 0
	FocusMaxPosN[0].max = MINMAX_MAX_POS; // 100000
	FocusMaxPosN[0].step = (int) FocusMaxPosN[0].max / 100;
	FocusMaxPosN[0].value = (int) FocusMaxPosN[0].max / 10;

	FocusAbsPosN[0].min = 0;
	FocusAbsPosN[0].max = FocusMaxPosN[0].value; // 10000
	FocusAbsPosN[0].step = (int) FocusAbsPosN[0].max / 100; // 100

	FocusRelPosN[0].min = 0;
	FocusRelPosN[0].max = (int) FocusAbsPosN[0].max / 10; // 1000
	FocusRelPosN[0].step = (int) FocusRelPosN[0].max / 10; // 100
	FocusRelPosN[0].value = (int) FocusRelPosN[0].max / 10; // 100

	FocusSyncN[0].min = 0;
	FocusSyncN[0].max = FocusAbsPosN[0].max; // 10000
	FocusSyncN[0].step = (int) FocusAbsPosN[0].max / 100; // 100

	FocusBacklashN[0].min = 0;
	FocusBacklashN[0].max = (int) FocusAbsPosN[0].max / 100; // 100
	FocusBacklashN[0].step = (int) FocusBacklashN[0].max / 100; // 1

	FocusMotionS[FOCUS_OUTWARD].s = ISS_ON;
	FocusMotionS[FOCUS_INWARD].s = ISS_OFF;

	// Add default properties
	// addAuxControls(); // enable simulation mode
	addDebugControl ();
	addConfigurationControl();
	removeProperty("POLLING_PERIOD", nullptr);

	// Load some custom properties before connecting
	defineSwitch(&MotorBoardSP);
	defineNumber(&BCMpinsNP);

	// Load config values, which cannot be changed after we are connected
	loadConfig(false, "MOTOR_BOARD"); // load stepper motor controller
	loadConfig(false, "BCMPINS"); // load BCM Pins assignment

	return true;
}

void AstroberryFocuser::ISGetProperties (const char *dev)
{
	INDI::Focuser::ISGetProperties(dev);
	return;
}

bool AstroberryFocuser::updateProperties()
{
	INDI::Focuser::updateProperties();

	if (isConnected())
	{
		defineSwitch(&StepperStandbySP);
		defineNumber(&StepperStandbyTimeNP);
		defineText(&ActiveTelescopeTP);
		defineSwitch(&FocusResolutionSP);
		defineNumber(&FocuserTravelNP);
		defineNumber(&FocuserInfoNP);
		defineNumber(&FocusStepDelayNP);

		IDSnoopDevice(ActiveTelescopeT[0].text, "TELESCOPE_INFO");

		if (readDS18B20())
		{
			defineNumber(&FocusTemperatureNP);
			defineNumber(&TemperatureCoefNP);
			defineSwitch(&TemperatureCompensateSP);
			readDS18B20(); // update immediately
			lastTemperature = FocusTemperatureN[0].value; // init last temperature
			IERmTimer(updateTemperatureID);
			updateTemperatureID = IEAddTimer(TEMPERATURE_UPDATE_TIMEOUT, updateTemperatureHelper, this); // set temperature update timer
			IERmTimer(temperatureCompensationID);
			temperatureCompensationID = IEAddTimer(TEMPERATURE_COMPENSATION_TIMEOUT, temperatureCompensationHelper, this); // set temperature compensation timer
		}

	} else {
		deleteProperty(StepperStandbySP.name);
		deleteProperty(StepperStandbyTimeNP.name);
		deleteProperty(ActiveTelescopeTP.name);
		deleteProperty(FocusResolutionSP.name);
		deleteProperty(FocuserTravelNP.name);
		deleteProperty(FocuserInfoNP.name);
		deleteProperty(FocusStepDelayNP.name);
		deleteProperty(FocusTemperatureNP.name);
		deleteProperty(TemperatureCoefNP.name);
		deleteProperty(TemperatureCompensateSP.name);
	}

	return true;
}

bool AstroberryFocuser::ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n)
{
	// first we check if it's for our device
	if(!strcmp(dev,getDeviceName()))
	{
		// handle BCMpins
		if (!strcmp(name, BCMpinsNP.name))
		{
			unsigned int valcount = 6;

			if (isConnected())
			{
				DEBUG(INDI::Logger::DBG_WARNING, "Cannot set BCM Pins while device is connected.");
				return false;
			} else {
				for (unsigned int i = 0; i < valcount; i++)
				{
					// verify a number is a valid BCM Pin
					if ( values[i] < 1 || values[i] > 27 )
					{
						BCMpinsNP.s=IPS_ALERT;
						IDSetNumber(&BCMpinsNP, nullptr);
						DEBUGF(INDI::Logger::DBG_ERROR, "Value %0.0f is not a valid BCM Pin number!", values[i]);
						return false;
					}

					// Verify unique BCM Pin assignement
					for (unsigned j = i + 1; j < valcount; j++)
					{
						if ( values[i] == values[j] )
						{
							BCMpinsNP.s=IPS_ALERT;
							IDSetNumber(&BCMpinsNP, nullptr);
							DEBUG(INDI::Logger::DBG_ERROR, "You cannot assign the same BCM Pin twice!");
							return false;
						}
					}

					// verify BCM Pins are not used by other consumers
					chip = gpiod_chip_open("/dev/gpiochip0");
					if (chip)
					{
						struct gpiod_line *line = gpiod_chip_get_line(chip, values[i]);
						bool line_status = gpiod_line_is_used(line);
						gpiod_chip_close(chip);

						if (line_status)
						{
							BCMpinsNP.s=IPS_ALERT;
							IDSetNumber(&BCMpinsNP, nullptr);
							DEBUGF(INDI::Logger::DBG_ERROR, "BCM Pin %0.0f already used!", values[i]);
							return false;
						}
					} else {
						DEBUG(INDI::Logger::DBG_ERROR, "Problem initiating Astroberry Focuser.");
						return false;
					}
				}

				IUUpdateNumber(&BCMpinsNP,values,names,n);

				BCMpinsNP.s=IPS_OK;
				IDSetNumber(&BCMpinsNP, nullptr);
				DEBUGF(INDI::Logger::DBG_SESSION, "BCM Pins set to DIR: BCM%0.0f, STEP: BCM%0.0f, SLEEP: BCM%0.0f, M1: BCM%0.0f, M2: BCM%0.0f, M3: BCM%0.0f", BCMpinsN[0].value, BCMpinsN[1].value, BCMpinsN[2].value, BCMpinsN[3].value, BCMpinsN[4].value, BCMpinsN[5].value);
				return true;
			}
		}

		// handle stepper standby delay
		if (!strcmp(name, StepperStandbyTimeNP.name))
		{
			IUUpdateNumber(&StepperStandbyTimeNP,values,names,n);
			StepperStandbyTimeNP.s=IPS_OK;
			IDSetNumber(&StepperStandbyTimeNP, nullptr);
			DEBUGF(INDI::Logger::DBG_SESSION, "Focuser standby set to %0.0f  seconds", StepperStandbyTimeN[0].value);
			return true;
		}

		// handle focus maximum position
		if (!strcmp(name, FocusMaxPosNP.name))
		{
			IUUpdateNumber(&FocusMaxPosNP,values,names,n);
			getFocuserInfo();
		}

		// handle focuser travel
		if (!strcmp(name, FocuserTravelNP.name))
		{
			IUUpdateNumber(&FocuserTravelNP,values,names,n);
			FocuserTravelNP.s=IPS_OK;
			IDSetNumber(&FocuserTravelNP, nullptr);
			getFocuserInfo();
			DEBUGF(INDI::Logger::DBG_SESSION, "Maximum focuser travel set to %0.0f mm", FocuserTravelN[0].value);
			return true;
		}

		// handle focus step delay
		if (!strcmp(name, FocusStepDelayNP.name))
		{
			IUUpdateNumber(&FocusStepDelayNP,values,names,n);
			FocusStepDelayNP.s=IPS_BUSY;
			IDSetNumber(&FocusStepDelayNP, nullptr);
			FocusStepDelayNP.s=IPS_OK;
			IDSetNumber(&FocusStepDelayNP, nullptr);
			DEBUGF(INDI::Logger::DBG_SESSION, "Step delay set to %0.0f ms.", FocusStepDelayN[0].value);
			return true;
		}

		// handle temperature coefficient
		if (!strcmp(name, TemperatureCoefNP.name))
		{
			IUUpdateNumber(&TemperatureCoefNP,values,names,n);
			TemperatureCoefNP.s=IPS_OK;
			IDSetNumber(&TemperatureCoefNP, nullptr);
			DEBUGF(INDI::Logger::DBG_SESSION, "Temperature coefficient set to %0.1f μm/m°C", TemperatureCoefN[0].value);
			return true;
		}
	}

	return INDI::Focuser::ISNewNumber(dev,name,values,names,n);
}

bool AstroberryFocuser::ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n)
{
	// first we check if it's for our device
	if (!strcmp(dev, getDeviceName()))
	{
		// handle motor board
		if(!strcmp(name, MotorBoardSP.name))
		{
			int current_switch = IUFindOnSwitchIndex(&MotorBoardSP);

			if (isConnected())
			{
				// reset switch to previous state
				MotorBoardS[current_switch].s = ISS_ON;
				IDSetSwitch(&MotorBoardSP, nullptr);
				DEBUG(INDI::Logger::DBG_WARNING, "Cannot set Control Board while device is connected.");
				return false;
			} else {
				IUUpdateSwitch(&MotorBoardSP, states, names, n);

				if ( MotorBoardS[0].s == ISS_ON)
				{
					DEBUG(INDI::Logger::DBG_SESSION, "Control Board set to DRV8834.");
				}

				if ( MotorBoardS[1].s == ISS_ON)
				{
					DEBUG(INDI::Logger::DBG_SESSION, "Control Board set to A4988.");
				}

				MotorBoardSP.s = IPS_OK;
				IDSetSwitch(&MotorBoardSP, nullptr);
				return true;
			}
		}

		// handle stepper standby
		if(!strcmp(name, StepperStandbySP.name))
		{
			IUUpdateSwitch(&StepperStandbySP, states, names, n);

			if ( StepperStandbyS[0].s == ISS_ON)
			{
				StepperStandbySP.s = IPS_OK;
				DEBUG(INDI::Logger::DBG_SESSION, "Stepper standby enabled.");
			}

			if ( StepperStandbyS[1].s == ISS_ON)
			{
				StepperStandbySP.s = IPS_OK;
				DEBUG(INDI::Logger::DBG_SESSION, "Stepper standby disabled.");
			}

			IDSetSwitch(&StepperStandbySP, nullptr);
			return true;
		}

		// handle focus resolution
		if(!strcmp(name, FocusResolutionSP.name))
		{
			int last_resolution = resolution;
			int current_switch = IUFindOnSwitchIndex(&FocusResolutionSP);

			IUUpdateSwitch(&FocusResolutionSP, states, names, n);

			//Resolution 1/1
			if ( FocusResolutionS[0].s == ISS_ON )
				resolution = 1;

			//Resolution 1/2
			if ( FocusResolutionS[1].s == ISS_ON )
				resolution = 2;

			//Resolution 1/4
			if ( FocusResolutionS[2].s == ISS_ON )
				resolution = 4;

			//Resolution 1/8
			if ( FocusResolutionS[3].s == ISS_ON )
				resolution = 8;

			//Resolution 1/16
			if ( FocusResolutionS[4].s == ISS_ON )
				resolution = 16;

			//Resolution 1/32
			if ( FocusResolutionS[5].s == ISS_ON )
			{
				if ( MotorBoardS[1].s == ISS_ON )
				{
					// reset switch to previous state if resolution is invalid
					FocusResolutionS[current_switch].s = ISS_ON;
					IDSetSwitch(&FocusResolutionSP, nullptr);

					DEBUG(INDI::Logger::DBG_WARNING, "A4988 Control Board does not support this resolution.");
					return false;
				}
				resolution = 32;
			}

			// Adjust position to a step in lower resolution
			int position_adjustment = last_resolution * (FocusAbsPosN[0].value / last_resolution - (int) FocusAbsPosN[0].value / last_resolution);
			if ( resolution < last_resolution && position_adjustment > 0 )
			{
				if ( (float) position_adjustment / last_resolution < 0.5)
				{
					position_adjustment *= -1;
				} else {
					position_adjustment = last_resolution - position_adjustment;
				}
				DEBUGF(INDI::Logger::DBG_SESSION, "Focuser position adjusted by %d steps at 1/%d resolution to sync with 1/%d resolution.", position_adjustment, last_resolution, resolution);
				MoveAbsFocuser(FocusAbsPosN[0].value + position_adjustment);
			}

			setResolution(resolution);

			// update values based on resolution
			FocusMaxPosN[0].max = (int) FocusMaxPosN[0].max * resolution / last_resolution;
			FocusMaxPosN[0].step = (int) FocusMaxPosN[0].step * resolution / last_resolution;
			FocusMaxPosN[0].value = (int) FocusMaxPosN[0].value * resolution / last_resolution;
			IDSetNumber(&FocusMaxPosNP, nullptr);
			IUUpdateMinMax(&FocusMaxPosNP); // This call is not INDI protocol compliant

			FocusAbsPosN[0].max = (int) FocusAbsPosN[0].max * resolution / last_resolution;
			FocusAbsPosN[0].step = (int) FocusAbsPosN[0].step * resolution / last_resolution;
			FocusAbsPosN[0].value = (int) FocusAbsPosN[0].value * resolution / last_resolution;
			IDSetNumber(&FocusAbsPosNP, nullptr);
			IUUpdateMinMax(&FocusAbsPosNP); // This call is not INDI protocol compliant

			FocusRelPosN[0].max = (int) FocusRelPosN[0].max * resolution / last_resolution;
			FocusRelPosN[0].step = (int) FocusRelPosN[0].step * resolution / last_resolution;
			FocusRelPosN[0].value = (int) FocusRelPosN[0].value * resolution / last_resolution;
			IDSetNumber(&FocusRelPosNP, nullptr);
			IUUpdateMinMax(&FocusRelPosNP); // This call is not INDI protocol compliant

			FocusSyncN[0].max = FocusSyncN[0].max * resolution / last_resolution;
			FocusSyncN[0].step = FocusSyncN[0].step * resolution / last_resolution;
			FocusSyncN[0].value = FocusSyncN[0].value * resolution / last_resolution;
			IDSetNumber(&FocusSyncNP, nullptr);
			IUUpdateMinMax(&FocusSyncNP); // This call is not INDI protocol compliant

			FocusBacklashN[0].max = (int) FocusBacklashN[0].max * resolution / last_resolution;
			FocusBacklashN[0].step = (int) FocusBacklashN[0].step * resolution / last_resolution;
			FocusBacklashN[0].value = (int) FocusBacklashN[0].value * resolution / last_resolution;
			IDSetNumber(&FocusBacklashNP, nullptr);
			IUUpdateMinMax(&FocusBacklashNP); // This call is not INDI protocol compliant

			PresetN[0].value = (int) PresetN[0].value * resolution / last_resolution;
			PresetN[1].value = (int) PresetN[1].value * resolution / last_resolution;
			PresetN[2].value = (int) PresetN[2].value * resolution / last_resolution;
			IDSetNumber(&PresetNP, nullptr);

			getFocuserInfo();

			FocusResolutionSP.s = IPS_OK;
			IDSetSwitch(&FocusResolutionSP, nullptr);
			DEBUGF(INDI::Logger::DBG_SESSION, "Focuser resolution set to 1/%d.", resolution);
			return true;
		}

		// handle temperature compensation
		if(!strcmp(name, TemperatureCompensateSP.name))
		{
			IUUpdateSwitch(&TemperatureCompensateSP, states, names, n);

			if ( TemperatureCompensateS[0].s == ISS_ON)
			{
				if (!temperatureCompensationID)
					temperatureCompensationID = IEAddTimer(TEMPERATURE_COMPENSATION_TIMEOUT, temperatureCompensationHelper, this);
				TemperatureCompensateSP.s = IPS_OK;
				DEBUG(INDI::Logger::DBG_SESSION, "Temperature compensation enabled.");
			}

			if ( TemperatureCompensateS[1].s == ISS_ON)
			{
				IERmTimer(temperatureCompensationID);
				TemperatureCompensateSP.s = IPS_IDLE;
				DEBUG(INDI::Logger::DBG_SESSION, "Temperature compensation disabled.");
			}

			IDSetSwitch(&TemperatureCompensateSP, nullptr);
			return true;
		}
	}

	return INDI::Focuser::ISNewSwitch(dev,name,states,names,n);
}

bool AstroberryFocuser::ISNewText (const char *dev, const char *name, char *texts[], char *names[], int n)
{
	// first we check if it's for our device
	if (!strcmp(dev, getDeviceName()))
	{
		// handle active devices
		if (!strcmp(name, ActiveTelescopeTP.name))
		{
			IUUpdateText(&ActiveTelescopeTP,texts,names,n);

			IUFillNumberVector(&ScopeParametersNP, ScopeParametersN, 2, ActiveTelescopeT[0].text, "TELESCOPE_INFO", "Scope Properties", OPTIONS_TAB, IP_RW, 60, IPS_OK);
			IDSnoopDevice(ActiveTelescopeT[0].text, "TELESCOPE_INFO");

			ActiveTelescopeTP.s=IPS_OK;
			IDSetText(&ActiveTelescopeTP, nullptr);
			DEBUGF(INDI::Logger::DBG_SESSION, "Active telescope set to %s.", ActiveTelescopeT[0].text);
			return true;
		}
	}

	return INDI::Focuser::ISNewText(dev,name,texts,names,n);
}

bool AstroberryFocuser::ISSnoopDevice (XMLEle *root)
{
	if (IUSnoopNumber(root, &ScopeParametersNP) == 0)
	{
		getFocuserInfo();
		DEBUGF(INDI::Logger::DBG_DEBUG, "Telescope parameters: %0.0f, %0.0f.", ScopeParametersN[0].value, ScopeParametersN[1].value);
		return true;
	}

	return INDI::Focuser::ISSnoopDevice(root);
}

bool AstroberryFocuser::saveConfigItems(FILE *fp)
{
	IUSaveConfigSwitch(fp, &MotorBoardSP);
	IUSaveConfigNumber(fp, &BCMpinsNP);
	IUSaveConfigSwitch(fp, &StepperStandbySP);
	IUSaveConfigNumber(fp, &StepperStandbyTimeNP);
	IUSaveConfigSwitch(fp, &FocusResolutionSP);
	IUSaveConfigSwitch(fp, &FocusReverseSP);
	IUSaveConfigNumber(fp, &FocusMaxPosNP);
	IUSaveConfigSwitch(fp, &FocusBacklashSP);
	IUSaveConfigNumber(fp, &FocusBacklashNP);
	IUSaveConfigNumber(fp, &FocusStepDelayNP);
	IUSaveConfigNumber(fp, &FocuserTravelNP);
	IUSaveConfigSwitch(fp, &TemperatureCompensateSP);
	IUSaveConfigNumber(fp, &TemperatureCoefNP);
	IUSaveConfigText(fp, &ActiveTelescopeTP);
	IUSaveConfigNumber(fp, &PresetNP);
	return true;
}

void AstroberryFocuser::TimerHit()
{
	if (backlashTicksRemaining == 0 && focuserTicksRemaining == 0)
	{
		//save position to file
		savePosition((int) FocusAbsPosN[0].value * MAX_RESOLUTION / resolution); // always save at MAX_RESOLUTION

		// update abspos value and status
		FocusAbsPosNP.s = IPS_OK;
		IDSetNumber(&FocusAbsPosNP, nullptr);
		FocusRelPosNP.s = IPS_OK;
		IDSetNumber(&FocusRelPosNP, nullptr);
		DEBUGF(INDI::Logger::DBG_SESSION, "Focuser at the position %0.0f.", FocusAbsPosN[0].value);

		// reset last temperature
		lastTemperature = FocusTemperatureN[0].value; // register last temperature

		// set motor standby timer
		if ( StepperStandbyS[0].s == ISS_ON)
		{
			if (stepperStandbyID)
				IERmTimer(stepperStandbyID);
			stepperStandbyID = IEAddTimer(StepperStandbyTimeN[0].value * 1000, stepperStandbyHelper, this);
			DEBUGF(INDI::Logger::DBG_SESSION, "Focuser going standby in %d seconds", (int) IERemainingTimer(stepperStandbyID) /  1000);
		}
		return;
	}

	// handle reverse motion
	if (stepperDirection == 1)
	{
		// outward
		if (FocusReverseS[INDI_ENABLED].s == ISS_ON) {
			gpiod_line_set_value(gpio_dir, 0); // Reverse Motion
		} else {
			gpiod_line_set_value(gpio_dir, 1); // Normal Motion
		}
	} else {
		// inward
		if (FocusReverseS[INDI_ENABLED].s == ISS_ON) {
			gpiod_line_set_value(gpio_dir, 1); // Reverse Motion
		} else {
			gpiod_line_set_value(gpio_dir, 0); // Normal Motion
		}
	}

	bool isBacklash = false;
	if (backlashTicksRemaining > 0)
	{
		isBacklash = true;
	}

	// make a single step
	stepMotor();

	// update absolute position only if processing real steps not backlash
	if (isBacklash)
	{
		backlashTicksRemaining -= 1;
	}  else {
		focuserTicksRemaining -= 1;
		FocusAbsPosN[0].value += 1 * stepperDirection;
		IDSetNumber(&FocusAbsPosNP, nullptr);
	}

	SetTimer(FocusStepDelayN[0].value);
}

bool AstroberryFocuser::ReverseFocuser(bool enabled)
{
	if (enabled)
	{
		DEBUG(INDI::Logger::DBG_SESSION, "Reverse direction enabled.");
	} else {
		DEBUG(INDI::Logger::DBG_SESSION, "Reverse direction disabled.");
	}
	return true;
}

bool AstroberryFocuser::SyncFocuser(uint32_t ticks)
{
	savePosition((int) ticks * MAX_RESOLUTION / resolution); // always save at MAX_RESOLUTION
	DEBUGF(INDI::Logger::DBG_SESSION, "Focuser absolute position sync to %d", ticks);
    return true;
}

bool AstroberryFocuser::SetFocuserBacklash(int32_t steps)
{
	DEBUGF(INDI::Logger::DBG_SESSION, "Backlash compensation set to %d steps.", steps);
	return true;
}

bool AstroberryFocuser::AbortFocuser()
{
	backlashTicksRemaining = 0;
	focuserTicksRemaining = 0;
	DEBUG(INDI::Logger::DBG_SESSION, "Focuser motion aborted.");
	return true;
}

IPState AstroberryFocuser::MoveAbsFocuser(uint32_t targetTicks)
{
	if (backlashTicksRemaining > 0 || focuserTicksRemaining > 0)
	{
		DEBUG(INDI::Logger::DBG_WARNING, "Focuser movement still in progress.");
		return IPS_BUSY;
	}

	if (targetTicks < FocusAbsPosN[0].min || targetTicks > FocusAbsPosN[0].max)
	{
		DEBUG(INDI::Logger::DBG_WARNING, "Requested position is out of range.");
		return IPS_ALERT;
	}

	if (targetTicks == FocusAbsPosN[0].value)
	{
		DEBUG(INDI::Logger::DBG_SESSION, "Already at the requested position.");
		return IPS_OK;
	}

	// set focuser busy
	FocusAbsPosNP.s = IPS_BUSY;
	IDSetNumber(&FocusAbsPosNP, nullptr);
	FocusRelPosNP.s = IPS_BUSY;
	IDSetNumber(&FocusRelPosNP, nullptr);

	// motor wake up
	if ( gpiod_line_get_value(gpio_sleep) == 0 )
	{
		IERmTimer(stepperStandbyID);
		gpiod_line_set_value(gpio_sleep, 1);
		DEBUG(INDI::Logger::DBG_SESSION, "Stepper motor waking up.");
	}

	// set direction
	const char* directionName;
	int newDirection;
	if (targetTicks > FocusAbsPosN[0].value)
	{
		newDirection = 1;
		directionName = "outward";
	} else {
		newDirection = -1;
		directionName = "inward";
	}

	// if direction changed do backlash adjustment
	if (newDirection != stepperDirection && FocusBacklashN[0].value != 0  && FocusBacklashS[INDI_ENABLED].s == ISS_ON)
	{
		DEBUGF(INDI::Logger::DBG_SESSION, "Compensating backlash by %0.0f steps.", FocusBacklashN[0].value);
		backlashTicksRemaining = FocusBacklashN[0].value;
	} else {
		backlashTicksRemaining = 0;
	}

	// update last stepper direction
	stepperDirection = newDirection;

	// process targetTicks
	focuserTicksRemaining = abs(targetTicks - FocusAbsPosN[0].value);
	DEBUGF(INDI::Logger::DBG_SESSION, "Focuser is moving %s to position %d.", directionName, targetTicks);

	SetTimer(FocusStepDelayN[0].value);

	return IPS_BUSY;
}

IPState AstroberryFocuser::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
	uint32_t targetTicks = (uint32_t) FocusAbsPosN[0].value + (ticks * (dir == FOCUS_INWARD ? -1 : 1));
	return MoveAbsFocuser(targetTicks);
}

void AstroberryFocuser::stepMotor()
{
	// step on
	gpiod_line_set_value(gpio_step, 1);
	// wait
	msleep(FocusStepDelayN[0].value);
	// step off
	gpiod_line_set_value(gpio_step, 0);
}

void AstroberryFocuser::setResolution(int res)
{
	// Release lines
	gpiod_line_release(gpio_m1);
	gpiod_line_release(gpio_m2);
	gpiod_line_release(gpio_m3);

	if (MotorBoardS[0].s == ISS_ON) {

		/* Stepper motor resolution settings for ==== DRV8834 =====
		* 1) 1/1   - M1=0 M2=0
		* 2) 1/2   - M1=1 M2=0
		* 3) 1/4   - M1=floating M2=0
		* 4) 1/8   - M1=0 M2=1
		* 5) 1/16  - M1=1 M2=1
		* 6) 1/32  - M1=floating M2=1
		*/

		switch(res)
		{
			case 1:	// 1:1
				gpiod_line_request_output(gpio_m1, "m1@astroberry_focuser", 0);
				gpiod_line_request_output(gpio_m2, "m2@astroberry_focuser", 0);
				break;
			case 2:	// 1:2
				gpiod_line_request_output(gpio_m1, "m1@astroberry_focuser", 1);
				gpiod_line_request_output(gpio_m2, "m2@astroberry_focuser", 0);
				break;
			case 4:	// 1:4
				gpiod_line_request_output_flags(gpio_m1, "m1@astroberry_focuser", GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN, 0);
				gpiod_line_request_output(gpio_m2, "m2@astroberry_focuser", 0);
				break;
			case 8:	// 1:8
				gpiod_line_request_output(gpio_m1, "m1@astroberry_focuser", 0);
				gpiod_line_request_output(gpio_m2, "m2@astroberry_focuser", 1);
				break;
			case 16:	// 1:16
				gpiod_line_request_output(gpio_m1, "m1@astroberry_focuser", 1);
				gpiod_line_request_output(gpio_m2, "m2@astroberry_focuser", 1);
				break;
			case 32:	// 1:32
				gpiod_line_request_output_flags(gpio_m1, "m1@astroberry_focuser", GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN, 0);
				gpiod_line_request_output(gpio_m2, "m2@astroberry_focuser", 1);
				break;
			default:	// 1:1
				gpiod_line_request_output(gpio_m1, "m1@astroberry_focuser", 0);
				gpiod_line_request_output(gpio_m2, "m2@astroberry_focuser", 0);
				break;
		}
	}

	if (MotorBoardS[1].s == ISS_ON) {

		/* Stepper motor resolution settings ===== for A4988 =====
		* 1) 1/1   - M1=0 M2=0 M3=0
		* 2) 1/2   - M1=1 M2=0 M3=0
		* 3) 1/4   - M1=0 M2=1 M3=0
		* 4) 1/8   - M1=1 M2=1 M3=0
		* 5) 1/16  - M1=1 M2=1 M3=1
		*/

		switch(res)
		{
			case 1:	// 1:1
				gpiod_line_request_output(gpio_m1, "m1@astroberry_focuser", 0);
				gpiod_line_request_output(gpio_m2, "m2@astroberry_focuser", 0);
				gpiod_line_request_output(gpio_m3, "m3@astroberry_focuser", 0);
				break;
			case 2:	// 1:2
				gpiod_line_request_output(gpio_m1, "m1@astroberry_focuser", 1);
				gpiod_line_request_output(gpio_m2, "m2@astroberry_focuser", 0);
				gpiod_line_request_output(gpio_m3, "m3@astroberry_focuser", 0);
				break;
			case 4:	// 1:4
				gpiod_line_request_output(gpio_m1, "m1@astroberry_focuser", 0);
				gpiod_line_request_output(gpio_m2, "m2@astroberry_focuser", 1);
				gpiod_line_request_output(gpio_m3, "m3@astroberry_focuser", 0);
				break;
			case 8:	// 1:8
				gpiod_line_request_output(gpio_m1, "m1@astroberry_focuser", 1);
				gpiod_line_request_output(gpio_m2, "m2@astroberry_focuser", 1);
				gpiod_line_request_output(gpio_m3, "m3@astroberry_focuser", 0);
				break;
			case 16:	// 1:16
				gpiod_line_request_output(gpio_m1, "m1@astroberry_focuser", 1);
				gpiod_line_request_output(gpio_m2, "m2@astroberry_focuser", 1);
				gpiod_line_request_output(gpio_m3, "m3@astroberry_focuser", 1);
				break;
			default:	// 1:1
				gpiod_line_request_output(gpio_m1, "m1@astroberry_focuser", 0);
				gpiod_line_request_output(gpio_m2, "m2@astroberry_focuser", 0);
				gpiod_line_request_output(gpio_m3, "m3@astroberry_focuser", 0);
				break;
		}
	}
}

int AstroberryFocuser::savePosition(int pos)
{
	FILE * pFile;
	char posFileName[MAXRBUF];
	char buf [100];

	if (getenv("INDICONFIG"))
	{
		snprintf(posFileName, MAXRBUF, "%s.position", getenv("INDICONFIG"));
	} else {
		snprintf(posFileName, MAXRBUF, "%s/.indi/%s.position", getenv("HOME"), getDeviceName());
	}


	if (pos == -1)
	{
		pFile = fopen (posFileName,"r");
		if (pFile == NULL)
		{
			DEBUGF(INDI::Logger::DBG_ERROR, "Failed to open file %s.", posFileName);
			return -1;
		}

		fgets (buf , 100, pFile);
		pos = atoi (buf);
		DEBUGF(INDI::Logger::DBG_DEBUG, "Reading position %d from %s.", pos, posFileName);
	} else {
		pFile = fopen (posFileName,"w");
		if (pFile == NULL)
		{
			DEBUGF(INDI::Logger::DBG_ERROR, "Failed to open file %s.", posFileName);
			return -1;
		}

		sprintf(buf, "%d", pos);
		fputs (buf, pFile);
		DEBUGF(INDI::Logger::DBG_DEBUG, "Writing position %s to %s.", buf, posFileName);
	}

	fclose (pFile);

	return pos;
}

bool AstroberryFocuser::readDS18B20()
{
	DIR *dir;
	struct dirent *dirent;
	char dev[16];      // Dev ID
	char devPath[128]; // Path to device
	char buf[256]; // Data from device
	char temperatureData[6]; // Temp C * 1000 reported by device
	char path[] = "/sys/bus/w1/devices";
	ssize_t numRead;
	float tempC, tempF;

	dir = opendir (path);

	// search for --the first-- DS18B20 device
	if (dir != NULL)
	{
		while ((dirent = readdir (dir)))
		{
			// DS18B20 device is family code beginning with 28-
			if (dirent->d_type == DT_LNK && strstr(dirent->d_name, "28-") != NULL)
			{
				strcpy(dev, dirent->d_name);
				break;
			}
		}
		(void) closedir (dir);
	} else {
		DEBUG(INDI::Logger::DBG_WARNING, "Temperature sensor disabled. 1-Wire interface is not available.");
		return false;
	}

	// Assemble path to --the first-- DS18B20 device
	sprintf(devPath, "%s/%s/w1_slave", path, dev);

	// Opening the device's file triggers new reading
	int fd = open(devPath, O_RDONLY);
	if(fd == -1)
	{
		DEBUG(INDI::Logger::DBG_WARNING, "Temperature sensor not available.");
		return false;
	}

	// set busy
	FocusTemperatureNP.s=IPS_BUSY;
	IDSetNumber(&FocusTemperatureNP, nullptr);

	// read sensor output
	while((numRead = read(fd, buf, 256)) > 0);
	close(fd);

	// parse temperature value from sensor output
	strncpy(temperatureData, strstr(buf, "t=") + 2, 5);
	DEBUGF(INDI::Logger::DBG_DEBUG, "Temperature sensor raw output: %s", buf);
	DEBUGF(INDI::Logger::DBG_DEBUG, "Temperature string: %s", temperatureData);

	tempC = strtof(temperatureData, NULL) / 1000;
	// tempF = (tempC / 1000) * 9 / 5 + 32;

	// check if temperature is reasonable
	if(abs(tempC) > 100)
	{
		DEBUG(INDI::Logger::DBG_DEBUG, "Temperature reading out of range.");
		return false;
	}

	FocusTemperatureN[0].value = tempC;

	// set OK
	FocusTemperatureNP.s=IPS_OK;
	IDSetNumber(&FocusTemperatureNP, nullptr);
	DEBUGF(INDI::Logger::DBG_DEBUG, "Temperature: %.2f°C", tempC);

	return true;
}

void AstroberryFocuser::getFocuserInfo()
{
	// https://www.innovationsforesight.com/education/how-much-focus-error-is-too-much/
	float travel_mm = (float) FocuserTravelN[0].value;
	float aperture = (float) ScopeParametersN[0].value;
	float focal = (float) ScopeParametersN[1].value;
	float f_ratio;

	// handle no snooping data from telescope
	if ( aperture * focal != 0 )
	{
		f_ratio = focal / aperture;
	} else {
		f_ratio =  0;
		DEBUG(INDI::Logger::DBG_WARNING, "No telescope focal length and/or aperture info available.");
	}

	float cfz = 4.88 * 0.520 * pow(f_ratio, 2); // CFZ = 4.88 · λ · f^2
	float step_size = 1000.0 * travel_mm / FocusMaxPosN[0].value;
	float steps_per_cfz = (int) cfz / step_size;

	// alert is number of steps per critical focus zone is too low
	if ( steps_per_cfz >= 4  )
	{
		FocuserInfoNP.s = IPS_OK;
	}
	else if ( steps_per_cfz > 2 && steps_per_cfz < 4 )
	{
		DEBUG(INDI::Logger::DBG_WARNING, "Resolution set too low for critical focus zone.");
		FocuserInfoNP.s = IPS_BUSY;
	} else {
		DEBUG(INDI::Logger::DBG_WARNING, "Resolution set too low for critical focus zone.");
		FocuserInfoNP.s = IPS_ALERT;
	}

	FocuserInfoN[0].value = step_size;
	FocuserInfoN[1].value = cfz;
	FocuserInfoN[2].value = steps_per_cfz;
	IDSetNumber(&FocuserInfoNP, nullptr);

	DEBUGF(INDI::Logger::DBG_DEBUG, "Focuser Info: %0.2f %0.2f %0.2f.", FocuserInfoN[0].value, FocuserInfoN[1].value, FocuserInfoN[2].value);
}

void AstroberryFocuser::stepperStandbyHelper(void *context)
{
	static_cast<AstroberryFocuser*>(context)->stepperStandby();
}

void AstroberryFocuser::updateTemperatureHelper(void *context)
{
	static_cast<AstroberryFocuser*>(context)->updateTemperature();
}

void AstroberryFocuser::temperatureCompensationHelper(void *context)
{
	static_cast<AstroberryFocuser*>(context)->temperatureCompensation();
}

void AstroberryFocuser::stepperStandby()
{
	if (!isConnected())
		return;

	gpiod_line_set_value(gpio_sleep, 0); // set stepper motor asleep
	DEBUG(INDI::Logger::DBG_SESSION, "Stepper motor going standby.");
}

void AstroberryFocuser::updateTemperature()
{
	if (!isConnected())
		return;

	readDS18B20();
	updateTemperatureID = IEAddTimer(TEMPERATURE_UPDATE_TIMEOUT, updateTemperatureHelper, this);
}

void AstroberryFocuser::temperatureCompensation()
{
	if (!isConnected())
		return;

	if ( TemperatureCompensateS[0].s == ISS_ON && FocusTemperatureN[0].value != lastTemperature )
	{
		float deltaTemperature = FocusTemperatureN[0].value - lastTemperature; // change of temperature from last focuser movement
		float thermalExpansionRatio = TemperatureCoefN[0].value * ScopeParametersN[1].value / 1000; // termal expansion in micrometers per 1 celcius degree
		float thermalExpansion = thermalExpansionRatio * deltaTemperature; // actual thermal expansion

		DEBUGF(INDI::Logger::DBG_DEBUG, "Thermal expansion of %0.1f μm due to temperature change of %0.2f°C", thermalExpansion, deltaTemperature);

		if ( abs(thermalExpansion) > FocuserInfoN[1].value / 2)
		{
			int thermalAdjustment = round((thermalExpansion / FocuserInfoN[0].value) / 2); // adjust focuser by half number of steps to keep it in the center of cfz
			MoveAbsFocuser(FocusAbsPosN[0].value + thermalAdjustment); // adjust focuser position
			lastTemperature = FocusTemperatureN[0].value; // register last temperature
			DEBUGF(INDI::Logger::DBG_SESSION, "Focuser adjusted by %d steps due to temperature change by %0.2f°C", thermalAdjustment, deltaTemperature);
		}
	}

	temperatureCompensationID = IEAddTimer(TEMPERATURE_COMPENSATION_TIMEOUT, temperatureCompensationHelper, this);
}
