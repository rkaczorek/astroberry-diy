
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
#include <wiringPi.h>
#include <string.h>
#include "rpi_focus.h"

// We declare an auto pointer to focusRpi.
std::unique_ptr<FocusRpi> focusRpi(new FocusRpi());

// Stepper motor takes 4 miliseconds to move one step = 250 steps per second (real rate = 240,905660377)
// 1) focusing from min to max takes 7 evolutions
// 2) PG2528-0502U step motor makes 7 * (360deg/15degperstep)*72:1 = 1728 steps per evolution
// 3) MAX_STEPS for 7 evolutions should be 12096

#define MAX_STEPS 10000 // maximum steps focuser can travel from min=0 to max

#define STEP_DELAY 4 // miliseconds

// indicate GPIOs used - use P1_* pin numbers not gpio numbers (!!!)

//RPi 3B

#define DIR 29
#define STEP 31
#define SLEEP 33
#define M1 37
#define M2 35
#define M3 38

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
	setVersion(2,1);
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
    wiringPiSetupPhys();
    int ret=!wiringPiSetupPhys();
    if (!ret)
    {
		IDMessage(getDeviceName(), "Problem initiating Astroberry Focuser.");
		return false;
	}

    // init GPIOs
    
    pinMode(DIR, OUTPUT);
    pinMode(STEP, OUTPUT);
    pinMode(SLEEP, OUTPUT);
    pinMode(M1, OUTPUT);
    pinMode(M2, OUTPUT);
    pinMode(M3, OUTPUT);
    IDMessage(getDeviceName(), "Astroberry Focuser connected successfully.");
    return true;
}

bool FocusRpi::Disconnect()
{
	// park focuser
	if ( FocusParkingS[0].s == ISS_ON )
	{
		IDMessage(getDeviceName(), "Astroberry Focuser is parking...");
		MoveAbsFocuser(FocusAbsPosN[0].min);
	}
    
	IDMessage(getDeviceName(), "Astroberry Focuser disconnected successfully.");
        return true;
}

bool FocusRpi::initProperties()
{
    INDI::Focuser::initProperties();

    IUFillNumber(&FocusAbsPosN[0],"FOCUS_ABSOLUTE_POSITION","Ticks","%0.0f",0,MAX_STEPS,(int)MAX_STEPS/100,0);
    IUFillNumberVector(&FocusAbsPosNP,FocusAbsPosN,1,getDeviceName(),"ABS_FOCUS_POSITION","Position",MAIN_CONTROL_TAB,IP_RW,0,IPS_OK);

	IUFillNumber(&PresetN[0], "Preset 1", "", "%0.0f", 0, MAX_STEPS, (int)(MAX_STEPS/100), 0);
	IUFillNumber(&PresetN[1], "Preset 2", "", "%0.0f", 0, MAX_STEPS, (int)(MAX_STEPS/100), 0);
	IUFillNumber(&PresetN[2], "Preset 3", "", "%0.0f", 0, MAX_STEPS, (int)(MAX_STEPS/100), 0);
	IUFillNumberVector(&PresetNP, PresetN, 3, getDeviceName(), "Presets", "Presets", "Presets", IP_RW, 0, IPS_IDLE);

	IUFillSwitch(&PresetGotoS[0], "Preset 1", "Preset 1", ISS_OFF);
	IUFillSwitch(&PresetGotoS[1], "Preset 2", "Preset 2", ISS_OFF);
	IUFillSwitch(&PresetGotoS[2], "Preset 3", "Preset 3", ISS_OFF);
	IUFillSwitchVector(&PresetGotoSP, PresetGotoS, 3, getDeviceName(), "Presets Goto", "Goto", MAIN_CONTROL_TAB,IP_RW,ISR_1OFMANY,60,IPS_OK);

	IUFillNumber(&FocusBacklashN[0], "FOCUS_BACKLASH_VALUE", "Steps", "%0.0f", 0, (int)(MAX_STEPS/100), (int)(MAX_STEPS/1000), 0);
	IUFillNumberVector(&FocusBacklashNP, FocusBacklashN, 1, getDeviceName(), "FOCUS_BACKLASH", "Backlash", OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

	IUFillSwitch(&FocusResetS[0],"FOCUS_RESET","Reset",ISS_OFF);
	IUFillSwitchVector(&FocusResetSP,FocusResetS,1,getDeviceName(),"FOCUS_RESET","Position Reset",OPTIONS_TAB,IP_RW,ISR_1OFMANY,60,IPS_OK);

	IUFillSwitch(&FocusParkingS[0],"FOCUS_PARKON","Enable",ISS_OFF);
	IUFillSwitch(&FocusParkingS[1],"FOCUS_PARKOFF","Disable",ISS_OFF);
	IUFillSwitchVector(&FocusParkingSP,FocusParkingS,2,getDeviceName(),"FOCUS_PARK","Parking Mode",OPTIONS_TAB,IP_RW,ISR_1OFMANY,60,IPS_OK);

	// set capabilities
        // SetFocuserCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE);

    return true;
}

void FocusRpi::ISGetProperties (const char *dev)
{
    INDI::Focuser::ISGetProperties(dev);

    /* Add debug controls so we may debug driver if necessary */
    addDebugControl();

    return;
}

bool FocusRpi::updateProperties()
{

    INDI::Focuser::updateProperties();

    if (isConnected())
    {
		deleteProperty(FocusSpeedNP.name);
        defineNumber(&FocusAbsPosNP);
        defineSwitch(&FocusMotionSP);
		defineNumber(&FocusBacklashNP);
		defineSwitch(&FocusParkingSP);
		defineSwitch(&FocusResetSP);
    }
    else
    {
        deleteProperty(FocusAbsPosNP.name);
        deleteProperty(FocusMotionSP.name);
		deleteProperty(FocusBacklashNP.name);
		deleteProperty(FocusParkingSP.name);
		deleteProperty(FocusResetSP.name);
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
               IDSetNumber(&FocusAbsPosNP, NULL);
            }
            return true;
        }        

        // handle focus relative position
        if (!strcmp(name, FocusRelPosNP.name))
        {
			IUUpdateNumber(&FocusRelPosNP,values,names,n);
			
			//FOCUS_INWARD
            if ( FocusMotionS[0].s == ISS_ON )
				MoveRelFocuser(FOCUS_INWARD, FocusRelPosN[0].value);

			//FOCUS_OUTWARD
            if ( FocusMotionS[1].s == ISS_ON )
				MoveRelFocuser(FOCUS_OUTWARD, FocusRelPosN[0].value);

			FocusRelPosNP.s=IPS_OK;
			IDSetNumber(&FocusRelPosNP, NULL);
			return true;
        }

        // handle focus timer
        if (!strcmp(name, FocusTimerNP.name))
        {
			IUUpdateNumber(&FocusTimerNP,values,names,n);

			//FOCUS_INWARD
            if ( FocusMotionS[0].s == ISS_ON )
				MoveFocuser(FOCUS_INWARD, 0, FocusTimerN[0].value);

			//FOCUS_OUTWARD
            if ( FocusMotionS[1].s == ISS_ON )
				MoveFocuser(FOCUS_OUTWARD, 0, FocusTimerN[0].value);

			FocusTimerNP.s=IPS_OK;
			IDSetNumber(&FocusTimerNP, NULL);
			return true;
        }

        // handle focus backlash
        if (!strcmp(name, FocusBacklashNP.name))
        {
            IUUpdateNumber(&FocusBacklashNP,values,names,n);
            FocusBacklashNP.s=IPS_OK;
            IDSetNumber(&FocusBacklashNP, "Astroberry Focuser backlash set to %d", (int) FocusBacklashN[0].value);
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
/*		
        // handle focus motion in and out
        if (!strcmp(name, FocusMotionSP.name))
        {
            IUUpdateSwitch(&FocusMotionSP, states, names, n);

			//FOCUS_INWARD
            if ( FocusMotionS[0].s == ISS_ON )
				MoveRelFocuser(FOCUS_INWARD, FocusRelPosN[0].value);

			//FOCUS_OUTWARD
            if ( FocusMotionS[1].s == ISS_ON )
				MoveRelFocuser(FOCUS_OUTWARD, FocusRelPosN[0].value);

            //FocusMotionS[0].s = ISS_OFF;
            //FocusMotionS[1].s = ISS_OFF;

			FocusMotionSP.s = IPS_OK;
            IDSetSwitch(&FocusMotionSP, NULL);
            return true;
        }
*/
        // handle focus presets
        if (!strcmp(name, PresetGotoSP.name))
        {
            IUUpdateSwitch(&PresetGotoSP, states, names, n);

			//Preset 1
            if ( PresetGotoS[0].s == ISS_ON )
				MoveAbsFocuser(PresetN[0].value);

			//Preset 2
            if ( PresetGotoS[1].s == ISS_ON )
				MoveAbsFocuser(PresetN[1].value);

			//Preset 2
            if ( PresetGotoS[2].s == ISS_ON )
				MoveAbsFocuser(PresetN[2].value);

			PresetGotoS[0].s = ISS_OFF;
			PresetGotoS[1].s = ISS_OFF;
			PresetGotoS[2].s = ISS_OFF;
			PresetGotoSP.s = IPS_OK;
            IDSetSwitch(&PresetGotoSP, NULL);
            return true;
        }
                
        // handle focus reset
        if(!strcmp(name, FocusResetSP.name))
        {
			IUUpdateSwitch(&FocusResetSP, states, names, n);

            if ( FocusResetS[0].s == ISS_ON && FocusAbsPosN[0].value == FocusAbsPosN[0].min  )
            {
				FocusAbsPosN[0].value = (int)MAX_STEPS/100;
				IDSetNumber(&FocusAbsPosNP, NULL);
				MoveAbsFocuser(0);
			}
            FocusResetS[0].s = ISS_OFF;
            IDSetSwitch(&FocusResetSP, NULL);
			return true;
		}

        // handle parking mode
        if(!strcmp(name, FocusParkingSP.name))
        {
			IUUpdateSwitch(&FocusParkingSP, states, names, n);
			IDSetSwitch(&FocusParkingSP, NULL);
			return true;
		}

        // handle focus abort - TODO
        if (!strcmp(name, AbortSP.name))
        {
            IUUpdateSwitch(&AbortSP, states, names, n);
            AbortS[0].s = ISS_OFF;
			AbortSP.s = IPS_OK;
            IDSetSwitch(&AbortSP, NULL);
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
    IUSaveConfigNumber(fp, &FocusRelPosNP);
    IUSaveConfigNumber(fp, &PresetNP);
    IUSaveConfigNumber(fp, &FocusBacklashNP);
	IUSaveConfigSwitch(fp, &FocusParkingSP);

    if ( FocusParkingS[0].s == ISS_ON )
		IUSaveConfigNumber(fp, &FocusAbsPosNP);

    return true;
}

IPState FocusRpi::MoveFocuser(FocusDirection dir, int speed, int duration)
{
	int ticks = (int) ( duration / STEP_DELAY);
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
        IDMessage(getDeviceName(), "Requested position is out of range.");
        return IPS_ALERT;
    }
    	
    if (targetTicks == FocusAbsPosN[0].value)
    {
        // IDMessage(getDeviceName(), "Astroberry Focuser already in the requested position.");
        return IPS_OK;
    }

	// set focuser busy
	FocusAbsPosNP.s = IPS_BUSY;
	IDSetNumber(&FocusAbsPosNP, NULL);

    // motor wake up
        digitalWrite(SLEEP, HIGH);
	// set 1/8 step size
	SetSpeed(2);

	// check last motion direction for backlash triggering
        char lastdir = digitalRead(DIR);
    // set direction
    const char* direction;    
    if (targetTicks > FocusAbsPosN[0].value)
    {
		// OUTWARD
		digitalWrite(DIR, LOW);
                direction = " outward ";
	}
    else
	{
		// INWARD
		digitalWrite(DIR, HIGH);
		direction = " inward ";
	}

    IDMessage(getDeviceName() , "Astroberry Focuser is moving %s", direction);

	// if direction changed do backlash adjustment
	if ( digitalRead(DIR) != lastdir && FocusAbsPosN[0].value != 0 && FocusBacklashN[0].value != 0 )
	{
		IDMessage(getDeviceName() , "Astroberry Focuser backlash compensation by %0.0f steps...", FocusBacklashN[0].value);	
		for ( int i = 0; i < FocusBacklashN[0].value; i++ )
		{
			// step on
			digitalWrite(STEP, HIGH);
			// wait
			delay (STEP_DELAY/8);
			// step off
			digitalWrite(STEP, LOW); 
			// wait 
			delay (STEP_DELAY/8);
		}
	}

	// process targetTicks
    int ticks = abs(targetTicks - FocusAbsPosN[0].value);

    for ( int i = 0; i < ticks; i++ )
    {
        // step on
	digitalWrite(STEP, HIGH);
        // wait
	delay(STEP_DELAY/2);
        // step off
	digitalWrite(STEP, LOW);
        // wait 
	delay(STEP_DELAY/2);

		// INWARD - count down
		if  ( digitalRead(DIR) == HIGH )
			FocusAbsPosN[0].value -= 1;

		// OUTWARD - count up
		if ( digitalRead(DIR) == LOW )
			FocusAbsPosN[0].value += 1;

		IDSetNumber(&FocusAbsPosNP, NULL);
    }

    // motor sleep
    digitalWrite(SLEEP, LOW);

    // update abspos value and status
    IDSetNumber(&FocusAbsPosNP, "Astroberry Focuser moved to position %0.0f", FocusAbsPosN[0].value);
	FocusAbsPosNP.s = IPS_OK;
	IDSetNumber(&FocusAbsPosNP, NULL);
	
    return IPS_OK;
}


 bool FocusRpi::SetSpeed(int speed)

{
	/* Stepper motor resolution settings (for A4988 POLOLU)
	* 1) 1/1   - M1=0 M2=0 M3=0
	* 2) 1/2   - M1=1 M2=0 M3=0
	* 3) 1/4   - M1=0 M2=1 M3=0
	* 4) 1/8   - M1=1 M2=1 M3=0
	* 5) 1/16  - M1=1 M2=1 M3=1 
	*/

    switch(speed)
    {
    case 1:	// 1:1
        digitalWrite(M1, LOW);
	digitalWrite(M2, LOW);
	digitalWrite(M3, LOW);
        break;
    case 2:	// 1:2
	digitalWrite(M1, HIGH);
        digitalWrite(M2, LOW);
	digitalWrite(M3, LOW);
        break;
    case 3:	// 1:4
        digitalWrite(M1, LOW);
        digitalWrite(M2, HIGH);
	digitalWrite(M3, LOW);
        break;
    case 4:	// 1:8
        digitalWrite(M1, HIGH);
        digitalWrite(M2, HIGH);
        digitalWrite(M3, LOW);
        break;
    case 5:	// 1:16 
        digitalWrite(M1, HIGH);
        digitalWrite(M2, HIGH);
        digitalWrite(M3, HIGH);
        break;
    default:	// 1:8 
        digitalWrite(M1, HIGH);
        digitalWrite(M2, HIGH);
        digitalWrite(M3, LOW);
        break;
    }
	return true;
}

