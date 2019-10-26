/*******************************************************************************
  Copyright(c) 2014 Radek Kaczorek  <rkaczorek AT gmail DOT com>

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

#include <stdio.h>
#include <unistd.h>
#include <memory>
#include <string.h>
#include <fstream>
#include "config.h"

#include <wiringPi.h>

#include "rpi_focus.h"

// We declare an auto pointer to focusRpi.
std::unique_ptr<FocusRpi> focusRpi(new FocusRpi());

// indicate GPIOs used
#define DIR 7
#define STEP 11
#define M1 15 // M0
#define M2 13 // M1
#define M3 18
#define SLEEP 16

/*
 Maximum resolution switch
 1 - 1/1
 2 - 1/2
 3 - 1/4
 4 - 1/8
 5 - 1/16
 6 - 1/32
 Note that values 3 - 6 do not always work. Set to 2 by default
*/
#define MAX_RESOLUTION 2

void ISPoll(void *p);


void ISInit()
{
   static int isInit = 0;

   if (isInit == 1)
       return;
   if(focusRpi.get() == 0)
   {
       isInit = 1;
       focusRpi.reset(new FocusRpi());
   }
}

void ISGetProperties(const char *dev)
{
        ISInit();
        focusRpi->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
        ISInit();
        focusRpi->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(	const char *dev, const char *name, char *texts[], char *names[], int num)
{
        ISInit();
        focusRpi->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
        ISInit();
        focusRpi->ISNewNumber(dev, name, values, names, num);
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
    ISInit();
    focusRpi->ISSnoopDevice(root);
}

FocusRpi::FocusRpi()
{
	setVersion(VERSION_MAJOR,VERSION_MINOR);
	FI::SetCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | FOCUSER_CAN_REVERSE); // | FOCUSER_CAN_ABORT);
	Focuser::setSupportedConnections(CONNECTION_NONE);
}

FocusRpi::~FocusRpi()
{

}

const char * FocusRpi::getDefaultName()
{
        return (char *)"Astroberry Focuser";
}

bool FocusRpi::Connect()
{
    int ret=!wiringPiSetupPhys();
	if (!ret)
	{
		DEBUG(INDI::Logger::DBG_ERROR, "Problem initiating Astroberry Focuser.");
		return false;
	}

//	DriverInfoT[3].text = (char*)"WiringPi";
//	IDSetText(&DriverInfoTP, nullptr);

	DEBUG(INDI::Logger::DBG_DEBUG, "Astroberry Focuser using WiringPi interface.");

    // Set gpios to output mode
    pinMode(DIR, OUTPUT);
    pinMode(STEP, OUTPUT);
    pinMode(SLEEP, OUTPUT);

	// Starting levels
	digitalWrite(DIR, LOW);
	digitalWrite(STEP, LOW);
	digitalWrite(SLEEP, HIGH);

	//read last position from file & convert from MAX_RESOLUTION to current resolution
	FocusAbsPosN[0].value = regPosition(-1) != -1 ? (int) regPosition(-1) * SetResolution(FocusResolutionN[0].value) / SetResolution(MAX_RESOLUTION) : 0;
	// preset resolution
	SetResolution(FocusResolutionN[0].value);

	// set motor standby timer
	timerID = SetTimer(60 * 1000 * MotorStandbyN[0].value);

    DEBUG(INDI::Logger::DBG_SESSION, "Astroberry Focuser connected successfully.");

    return true;
}

bool FocusRpi::Disconnect()
{
	// Starting levels
	digitalWrite(DIR, LOW);
	digitalWrite(STEP, LOW);
	digitalWrite(SLEEP, LOW); // stepper sleep
	digitalWrite(M1, LOW);
	digitalWrite(M2, LOW);
	digitalWrite(M3, LOW);

	DEBUG(INDI::Logger::DBG_SESSION, "Astroberry Focuser disconnected successfully.");

    return true;
}

bool FocusRpi::initProperties()
{
    INDI::Focuser::initProperties();

	IUFillNumber(&FocusBacklashN[0], "FOCUS_BACKLASH_VALUE", "steps", "%0.0f", 0, 1000, 10, 0);
	IUFillNumberVector(&FocusBacklashNP, FocusBacklashN, 1, getDeviceName(), "FOCUS_BACKLASH", "Backlash", OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

	IUFillNumber(&FocusResolutionN[0], "FOCUS_RESOLUTION_VALUE", "1/n steps", "%0.0f", 1, MAX_RESOLUTION, 1, 1);
	IUFillNumberVector(&FocusResolutionNP, FocusResolutionN, 1, getDeviceName(), "FOCUS_RESOLUTION", "Resolution", OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

	IUFillNumber(&FocusStepDelayN[0], "FOCUS_STEPDELAY_VALUE", "milliseconds", "%0.0f", 1, 10, 1, 1);
	IUFillNumberVector(&FocusStepDelayNP, FocusStepDelayN, 1, getDeviceName(), "FOCUS_STEPDELAY", "Step Delay", OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

	IUFillSwitch(&MotorBoardS[0],"DRV8834","DRV8834",ISS_ON);
	IUFillSwitch(&MotorBoardS[1],"A4988","A4988",ISS_OFF);
	IUFillSwitchVector(&MotorBoardSP,MotorBoardS,2,getDeviceName(),"MOTOR_BOARD","Control Board",OPTIONS_TAB,IP_RW,ISR_1OFMANY,60,IPS_IDLE);

	IUFillNumber(&MotorStandbyN[0], "MOTOR_STANDBY_TIME", "minutes", "%0.0f", 0, 10, 1, 1);
	IUFillNumberVector(&MotorStandbyNP, MotorStandbyN, 1, getDeviceName(), "MOTOR_SLEEP", "Standby", OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

	FocusMaxPosN[0].min = 1000;
	FocusMaxPosN[0].max = 20000;
	FocusMaxPosN[0].step = 1000;
	FocusMaxPosN[0].value = 20000;

	FocusRelPosN[0].min = 0;
	FocusRelPosN[0].max = 1000;
	FocusRelPosN[0].step = 200;
	FocusRelPosN[0].value = 200;

	FocusAbsPosN[0].min = 0;
	FocusAbsPosN[0].max = FocusMaxPosN[0].value;
	FocusAbsPosN[0].step = (int) FocusAbsPosN[0].max / 100;
//	FocusAbsPosN[0].value = regPosition(-1) != -1 ? regPosition(-1) * SetResolution(FocusResolutionN[0].value) : 0; //read last position from file

	FocusMotionS[FOCUS_OUTWARD].s = ISS_ON;
	FocusMotionS[FOCUS_INWARD].s = ISS_OFF;
	IDSetSwitch(&FocusMotionSP, nullptr);

    return true;
}

void FocusRpi::ISGetProperties (const char *dev)
{
    INDI::Focuser::ISGetProperties(dev);

    return;
}

bool FocusRpi::updateProperties()
{

    INDI::Focuser::updateProperties();

    if (isConnected())
    {
		defineSwitch(&FocusMotionSP);
		defineSwitch(&MotorBoardSP);
		defineNumber(&MotorStandbyNP);
		defineNumber(&FocusBacklashNP);
		defineNumber(&FocusStepDelayNP);
		defineNumber(&FocusResolutionNP);
    } else {
		deleteProperty(FocusMotionSP.name);
		deleteProperty(MotorBoardSP.name);
		deleteProperty(MotorStandbyNP.name);
		deleteProperty(FocusBacklashNP.name);
		deleteProperty(FocusStepDelayNP.name);
		deleteProperty(FocusResolutionNP.name);
    }

    return true;
}

bool FocusRpi::ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n)
{
	// first we check if it's for our device
	if(strcmp(dev,getDeviceName())==0)
	{

        // handle focus absolute position
        if (!strcmp(name, FocusAbsPosNP.name))
        {
			int newPos = (int) values[0];
            if ( MoveAbsFocuser(newPos) == IPS_OK )
            {
               IUUpdateNumber(&FocusAbsPosNP,values,names,n);
               FocusAbsPosNP.s=IPS_OK;
               IDSetNumber(&FocusAbsPosNP, nullptr);
            }
            return true;
        }

        // handle focus relative position
        if (!strcmp(name, FocusRelPosNP.name))
        {
			IUUpdateNumber(&FocusRelPosNP,values,names,n);

			//FOCUS_INWARD
			if ( FocusMotionS[0].s == ISS_ON )
				FocusRelPosNP.s = MoveRelFocuser(FOCUS_INWARD, FocusRelPosN[0].value);

			//FOCUS_OUTWARD
			if ( FocusMotionS[1].s == ISS_ON )
				FocusRelPosNP.s = MoveRelFocuser(FOCUS_OUTWARD, FocusRelPosN[0].value);

			IDSetNumber(&FocusRelPosNP, nullptr);
			return true;
        }

        // handle focus backlash
        if (!strcmp(name, FocusBacklashNP.name))
        {
            IUUpdateNumber(&FocusBacklashNP,values,names,n);
            FocusBacklashNP.s=IPS_BUSY;
            IDSetNumber(&FocusBacklashNP, nullptr);
            FocusBacklashNP.s=IPS_OK;
            IDSetNumber(&FocusBacklashNP, nullptr);
            DEBUGF(INDI::Logger::DBG_DEBUG, "Astroberry Focuser backlash set to %0.0f", FocusBacklashN[0].value);
            return true;
        }

        // handle focus resolution
        if (!strcmp(name, FocusResolutionNP.name))
        {
			int last_resolution = SetResolution(FocusResolutionN[0].value);
			IUUpdateNumber(&FocusResolutionNP,values,names,n);
			FocusResolutionNP.s=IPS_BUSY;
			IDSetNumber(&FocusResolutionNP, nullptr);

			// set step size
			int resolution = SetResolution(FocusResolutionN[0].value);

			FocusMaxPosN[0].min = (int) FocusMaxPosN[0].min * resolution / last_resolution;
			FocusMaxPosN[0].max = (int) FocusMaxPosN[0].max * resolution / last_resolution;
			FocusMaxPosN[0].step = (int) FocusMaxPosN[0].step * resolution / last_resolution;
			FocusMaxPosN[0].value = (int) FocusMaxPosN[0].value * resolution / last_resolution;
			IDSetNumber(&FocusMaxPosNP, nullptr);

			FocusRelPosN[0].min = (int) FocusRelPosN[0].min * resolution / last_resolution;
			FocusRelPosN[0].max = (int) FocusRelPosN[0].max * resolution / last_resolution;
			FocusRelPosN[0].step = (int) FocusRelPosN[0].step * resolution / last_resolution;
			FocusRelPosN[0].value = (int) FocusRelPosN[0].value * resolution / last_resolution;
			IDSetNumber(&FocusRelPosNP, nullptr);

			FocusAbsPosN[0].max = (int) FocusAbsPosN[0].max * resolution / last_resolution;
			FocusAbsPosN[0].step = (int) FocusAbsPosN[0].step * resolution / last_resolution;
			FocusAbsPosN[0].value = (int) FocusAbsPosN[0].value * resolution / last_resolution;
			IDSetNumber(&FocusAbsPosNP, nullptr);

			deleteProperty(FocusRelPosNP.name);
			defineNumber(&FocusRelPosNP);

			deleteProperty(FocusAbsPosNP.name);
			defineNumber(&FocusAbsPosNP);

			deleteProperty(FocusMaxPosNP.name);
			defineNumber(&FocusMaxPosNP);

			deleteProperty(FocusReverseSP.name);
			defineSwitch(&FocusReverseSP);

			FocusResolutionNP.s=IPS_OK;
			IDSetNumber(&FocusResolutionNP, nullptr);

			DEBUGF(INDI::Logger::DBG_DEBUG, "Astroberry Focuser resolution changed from 1/%d to 1/%d step", last_resolution, resolution);
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
		   DEBUGF(INDI::Logger::DBG_DEBUG, "Astroberry Focuser step delay set to %0.0f", FocusStepDelayN[0].value);
           return true;
        }

        // handle motor sleep
        if (!strcmp(name, MotorStandbyNP.name))
        {
		   IUUpdateNumber(&MotorStandbyNP,values,names,n);
		   MotorStandbyNP.s=IPS_BUSY;
		   IDSetNumber(&MotorStandbyNP, nullptr);

			// set motor standby timer
			if ( MotorStandbyN[0].value > 0)
			{
				if (timerID)
					RemoveTimer(timerID);
				timerID = SetTimer(60 * 1000 * MotorStandbyN[0].value);
			}

		   MotorStandbyNP.s=IPS_OK;
		   IDSetNumber(&MotorStandbyNP, nullptr);
		   DEBUGF(INDI::Logger::DBG_DEBUG, "Astroberry Focuser standby timeout set to %0.0f", MotorStandbyN[0].value);
           return true;
        }
	}
    return INDI::Focuser::ISNewNumber(dev,name,values,names,n);
}

bool FocusRpi::ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n)
{
	// first we check if it's for our device
    if (!strcmp(dev, getDeviceName()))
    {
        // handle focus presets
        if (!strcmp(name, PresetGotoSP.name))
        {
            IUUpdateSwitch(&PresetGotoSP, states, names, n);
			PresetGotoSP.s = IPS_BUSY;
            IDSetSwitch(&PresetGotoSP, nullptr);

			//Preset 1
			if ( PresetGotoS[0].s == ISS_ON )
				MoveAbsFocuser(PresetN[0].value);

			//Preset 2
			if ( PresetGotoS[1].s == ISS_ON )
				MoveAbsFocuser(PresetN[1].value);

			//Preset 3
			if ( PresetGotoS[2].s == ISS_ON )
				MoveAbsFocuser(PresetN[2].value);

			PresetGotoS[0].s = ISS_OFF;
			PresetGotoS[1].s = ISS_OFF;
			PresetGotoS[2].s = ISS_OFF;
			PresetGotoSP.s = IPS_OK;

            IDSetSwitch(&PresetGotoSP, nullptr);

            return true;
        }

        // handle motor board
        if(!strcmp(name, MotorBoardSP.name))
        {
			IUUpdateSwitch(&MotorBoardSP, states, names, n);
			MotorBoardSP.s = IPS_BUSY;
			IDSetSwitch(&MotorBoardSP, nullptr);


			// set max resolution for motor model
			if ( MotorBoardS[0].s == ISS_ON)
			{
				FocusResolutionN[0].max = MAX_RESOLUTION;
				DEBUG(INDI::Logger::DBG_DEBUG, "Astroberry Focuser control board set to DRV8834");
			}
			if ( MotorBoardS[1].s == ISS_ON)
			{
				if ( FocusResolutionN[0].value > 5 )
					FocusResolutionN[0].value = 5;
				FocusResolutionN[0].max = MAX_RESOLUTION;
				DEBUG(INDI::Logger::DBG_DEBUG, "Astroberry Focuser control board set to A4988");
			}
			IDSetNumber(&FocusResolutionNP, nullptr);
			deleteProperty(FocusResolutionNP.name);
			defineNumber(&FocusResolutionNP);

			MotorBoardSP.s = IPS_OK;
			IDSetSwitch(&MotorBoardSP, nullptr);
			return true;
		}
    }
    return INDI::Focuser::ISNewSwitch(dev,name,states,names,n);
}

bool FocusRpi::ISSnoopDevice (XMLEle *root)
{
    return INDI::Focuser::ISSnoopDevice(root);
}

bool FocusRpi::saveConfigItems(FILE *fp)
{
    IUSaveConfigNumber(fp, &FocusMaxPosNP);
    IUSaveConfigNumber(fp, &PresetNP);
    IUSaveConfigSwitch(fp, &FocusReverseSP);
    IUSaveConfigSwitch(fp, &MotorBoardSP);
    IUSaveConfigNumber(fp, &MotorStandbyNP);
    IUSaveConfigNumber(fp, &FocusBacklashNP);
    IUSaveConfigNumber(fp, &FocusStepDelayNP);
    IUSaveConfigNumber(fp, &FocusResolutionNP);

    return true;
}

void FocusRpi::TimerHit()
{
	if (!isConnected())
		return;

    // motor sleep
	digitalWrite(SLEEP, LOW);
	DEBUG(INDI::Logger::DBG_DEBUG, "Astroberry Focuser going standby");
}

bool FocusRpi::AbortFocuser()
{
	// TODO
	DEBUG(INDI::Logger::DBG_SESSION, "Astroberry Focuser aborting motion");
    return true;
}

IPState FocusRpi::MoveFocuser(FocusDirection dir, int speed, int duration)
{
    int ticks = (int) ( duration / FocusStepDelayN[0].value);
    return 	MoveRelFocuser( dir, ticks);
}

IPState FocusRpi::MoveRelFocuser(FocusDirection dir, int ticks)
{
    int targetTicks = FocusAbsPosN[0].value + (ticks * (dir == FOCUS_INWARD ? -1 : 1));
    return MoveAbsFocuser(targetTicks);
}

IPState FocusRpi::MoveAbsFocuser(int targetTicks)
{
    if (targetTicks < FocusAbsPosN[0].min || targetTicks > FocusAbsPosN[0].max)
    {
        DEBUG(INDI::Logger::DBG_SESSION, "Requested position is out of range");
        return IPS_ALERT;
    }

    if (targetTicks == FocusAbsPosN[0].value)
    {
        DEBUG(INDI::Logger::DBG_SESSION, "Astroberry Focuser already at the requested position");
        return IPS_OK;
    }

    // set focuser busy
    FocusAbsPosNP.s = IPS_BUSY;
    IDSetNumber(&FocusAbsPosNP, nullptr);

    // motor wake up
    if (digitalRead(SLEEP) == LOW)
    {
		digitalWrite(SLEEP, HIGH);
		DEBUG(INDI::Logger::DBG_DEBUG, "Astroberry Focuser motor waking up");
	}

    // check last motion direction for backlash triggering
    char lastdir = digitalRead(DIR);

    // set direction
    const char* direction;
    if (targetTicks > FocusAbsPosN[0].value)
    {
		// OUTWARD
		digitalWrite(DIR, HIGH);
		direction = "OUTWARD";
    } else {
		// INWARD
		digitalWrite(DIR, LOW);
		direction = "INWARD";
    }

	// handle Reverse Motion
	if (FocusReverseS[REVERSED_ENABLED].s == ISS_ON) {
		lastdir = (lastdir == LOW) ? lastdir == HIGH : lastdir == LOW;
		(digitalRead(DIR) == LOW) ? digitalWrite(DIR, HIGH) : digitalWrite(DIR, LOW);
	}

    // if direction changed do backlash adjustment
    if ( digitalRead(DIR) != lastdir && FocusBacklashN[0].value != 0)
    {
		DEBUGF(INDI::Logger::DBG_DEBUG, "Astroberry Focuser BACKLASH compensation by %0.0f steps", FocusBacklashN[0].value);
		for ( int i = 0; i < FocusBacklashN[0].value; i++ )
		{
			// step on
			digitalWrite(STEP, HIGH);
			// wait
			delay(FocusStepDelayN[0].value);
			// step off
			digitalWrite(STEP, LOW);
			// wait
			delay(FocusStepDelayN[0].value);
		}
    }

    // process targetTicks
    int ticks = abs(targetTicks - FocusAbsPosN[0].value);

    DEBUGF(INDI::Logger::DBG_DEBUG, "Astroberry Focuser is moving %s to position %d", direction, targetTicks);

    for ( int i = 0; i < ticks; i++ )
    {
        // step on
        digitalWrite(STEP, HIGH);
        // wait
        delay(FocusStepDelayN[0].value);
        // step off
        digitalWrite(STEP, LOW);
        // wait
        delay(FocusStepDelayN[0].value);

		// INWARD - count down
		if ( direction == "INWARD" )
			FocusAbsPosN[0].value -= 1;

		// OUTWARD - count up
		if ( direction == "OUTWARD" )
			FocusAbsPosN[0].value += 1;

		IDSetNumber(&FocusAbsPosNP, nullptr);
    }

	//save position to file
	regPosition((int) FocusAbsPosN[0].value * SetResolution(MAX_RESOLUTION) / SetResolution(FocusResolutionN[0].value)); // always save at MAX_RESOLUTION

    // update abspos value and status
    DEBUGF(INDI::Logger::DBG_SESSION, "Astroberry Focuser at the position %0.0f", FocusAbsPosN[0].value);

    FocusAbsPosNP.s = IPS_OK;
    IDSetNumber(&FocusAbsPosNP, nullptr);

	// set motor standby timer
	if ( MotorStandbyN[0].value > 0)
	{
		if (timerID)
			RemoveTimer(timerID);
		timerID = SetTimer(60 * 1000 * MotorStandbyN[0].value);
	}

    return IPS_OK;
}

int FocusRpi::SetResolution(int res)
{
	int resolution = 1;

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
				pinMode(M1, OUTPUT);
				pinMode(M2, OUTPUT);
				digitalWrite(M1, LOW);
				digitalWrite(M2, LOW);
				resolution = 1;
				break;
			case 2:	// 1:2
				pinMode(M1, OUTPUT);
				pinMode(M2, OUTPUT);
				digitalWrite(M1, HIGH);
				digitalWrite(M2, LOW);
				resolution = 2;
				break;
			case 3:	// 1:4
				pinMode(M1, INPUT);
				pinMode(M2, OUTPUT);
				pullUpDnControl(M1, PUD_OFF);
				digitalWrite(M2, LOW);
				resolution = 4;
				break;
			case 4:	// 1:8
				pinMode(M1, OUTPUT);
				pinMode(M2, OUTPUT);
				digitalWrite(M1, LOW);
				digitalWrite(M2, HIGH);
				resolution = 8;
				break;
			case 5:	// 1:16
				pinMode(M1, OUTPUT);
				pinMode(M2, OUTPUT);
				digitalWrite(M1, HIGH);
				digitalWrite(M2, HIGH);
				resolution = 16;
				break;
			case 6:	// 1:32
				pinMode(M1, INPUT);
				pinMode(M2, OUTPUT);
				pullUpDnControl(M1, PUD_OFF);
				digitalWrite(M2, HIGH);
				resolution = 32;
				break;
			default:	// 1:1
				pinMode(M1, OUTPUT);
				pinMode(M2, OUTPUT);
				digitalWrite(M1, LOW);
				digitalWrite(M2, LOW);
				resolution = 1;
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

		pinMode(M1, OUTPUT);
		pinMode(M2, OUTPUT);
		pinMode(M2, OUTPUT);

		switch(res)
		{
			case 1:	// 1:1
				digitalWrite(M1, LOW);
				digitalWrite(M2, LOW);
				digitalWrite(M3, LOW);
				resolution = 1;
				break;
			case 2:	// 1:2
				digitalWrite(M1, HIGH);
				digitalWrite(M2, LOW);
				digitalWrite(M3, LOW);
				resolution = 2;
				break;
			case 3:	// 1:4
				digitalWrite(M1, LOW);
				digitalWrite(M2, HIGH);
				digitalWrite(M3, LOW);
				resolution = 4;
				break;
			case 4:	// 1:8
				digitalWrite(M1, HIGH);
				digitalWrite(M2, HIGH);
				digitalWrite(M3, LOW);
				resolution = 8;
				break;
			case 5:	// 1:16
				digitalWrite(M1, HIGH);
				digitalWrite(M2, HIGH);
				digitalWrite(M3, HIGH);
				resolution = 16;
				break;
			default:	// 1:1
				digitalWrite(M1, LOW);
				digitalWrite(M2, LOW);
				digitalWrite(M3, LOW);
				resolution = 1;
				break;
		}
	}

	return resolution;
}

bool FocusRpi::ReverseFocuser(bool enabled)
{
	if (enabled)
	{
		DEBUG(INDI::Logger::DBG_DEBUG, "Astroberry Focuser reverse direction ENABLED");
	} else {
		DEBUG(INDI::Logger::DBG_DEBUG, "Astroberry Focuser reverse direction DISABLED");
	}
    return true;
}

int FocusRpi::regPosition(int pos)
{
	FILE * pFile;
    char posFileName[MAXRBUF];
	char buf [100];

    if (getenv("INDICONFIG"))
        snprintf(posFileName, MAXRBUF, "%s.position", getenv("INDICONFIG"));
    else
        snprintf(posFileName, MAXRBUF, "%s/.indi/%s.position", getenv("HOME"), getDeviceName());


	if (pos == -1)
	{
		pFile = fopen (posFileName,"r");
		if (pFile == NULL)
		{
			DEBUGF(INDI::Logger::DBG_DEBUG, "Astroberry Focuser failed to open file %s", posFileName);
			return -1;
		}

		fgets (buf , 100, pFile);
		pos = atoi (buf);
		DEBUGF(INDI::Logger::DBG_DEBUG, "Astroberry Focuser reading position %d from %s", pos, posFileName);
	} else {
		pFile = fopen (posFileName,"w");
		if (pFile == NULL)
		{
			DEBUGF(INDI::Logger::DBG_DEBUG, "Astroberry Focuser failed to open file %s", posFileName);
			return -1;
		}

		sprintf(buf, "%d", pos);
		fputs (buf, pFile);
		DEBUGF(INDI::Logger::DBG_DEBUG, "Astroberry Focuser writing position %s to %s", buf, posFileName);
	}

	fclose (pFile);

	return pos;
}
