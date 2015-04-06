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
#include <bcm2835.h>

#include "rpi_focus.h"

// We declare an auto pointer to focusRpi.
std::auto_ptr<FocusRpi> focusRpi(0);

// Stepper motor takes 4 miliseconds to move one step = 250 steps per second (real rate = 240,905660377)
// MAX_STEPS calculations for Vixen VMX110
// 1) focusing from min to max takes 7 evolutions):
// 2) 7 evolutions = 12768 steps at full step = 7 * 1824 steps per evolution = 7 * (360deg/15degperstep)*72:1
// 3) 7 evolutions = 204288 steps at 1/16 step = 7 * 29184 steps per evolution
// 4) MAX_STEPS should be around 200000
#define MAX_STEPS 200000 // maximum steps focuser can travel from min=0 to max

#define STEP_DELAY 4 // miliseconds

// indicate GPIOs used - use P1_* pin numbers not gpio numbers (!!!)

//RPi B+
/*
#define DIR RPI_V2_GPIO_P1_07	// GPIO4
#define STEP RPI_V2_GPIO_P1_11	// GPIO17
#define M0 RPI_V2_GPIO_P1_15	// GPIO22
#define M1 RPI_V2_GPIO_P1_13	// GPIO27
#define SLEEP RPI_V2_GPIO_P1_16	// GPIO23
*/

//RPi v2
#define DIR RPI_BPLUS_GPIO_J8_07	// GPIO4
#define STEP RPI_BPLUS_GPIO_J8_11	// GPIO17
#define M0 RPI_BPLUS_GPIO_J8_15		// GPIO22
#define M1 RPI_BPLUS_GPIO_J8_13		// GPIO27
#define SLEEP RPI_BPLUS_GPIO_J8_16	// GPIO23


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
    if (!bcm2835_init())
    {
		IDMessage(getDeviceName(), "Problem initiating Astroberry Focuser.");
		return false;
	}

    // init GPIOs
    std::ofstream exportgpio;
    exportgpio.open("/sys/class/gpio/export");
    exportgpio << DIR << std::endl;
    exportgpio << STEP << std::endl;
    exportgpio << M0 << std::endl;
    exportgpio << M1 << std::endl;
    exportgpio << SLEEP << std::endl;
    exportgpio.close();

    // Set gpios to output mode
    bcm2835_gpio_fsel(DIR, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(STEP, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(SLEEP, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(M0, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(M1, BCM2835_GPIO_FSEL_OUTP);
	
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
	
    // close GPIOs
    std::ofstream unexportgpio;
    unexportgpio.open("/sys/class/gpio/unexport");
    unexportgpio << DIR << std::endl;
    unexportgpio << STEP << std::endl;
    unexportgpio << M0 << std::endl;
    unexportgpio << M1 << std::endl;
    unexportgpio << SLEEP << std::endl;
    unexportgpio.close();
    bcm2835_close();
		
	IDMessage(getDeviceName(), "Astroberry Focuser disconnected successfully.");
    
    return true;
}

bool FocusRpi::initProperties()
{
    INDI::Focuser::initProperties();

    IUFillText(&PortT[0], "PORT", "Port","RPi GPIO");
    IUFillTextVector(&PortTP,PortT,1,getDeviceName(),"DEVICE_PORT","Ports",OPTIONS_TAB,IP_RO,0,IPS_OK);

    IUFillNumber(&FocusAbsPosN[0],"FOCUS_ABSOLUTE_POSITION","Ticks","%0.0f",0,MAX_STEPS,(int)MAX_STEPS/100,0);
    IUFillNumberVector(&FocusAbsPosNP,FocusAbsPosN,1,getDeviceName(),"ABS_FOCUS_POSITION","Position",MAIN_CONTROL_TAB,IP_RW,0,IPS_OK);

    IUFillSwitch(&FocusGotoS[0],"FOCUS_HOME","Min",ISS_OFF);
    IUFillSwitch(&FocusGotoS[1],"FOCUS_CENTER","Center",ISS_OFF);
    IUFillSwitch(&FocusGotoS[2],"FOCUS_END","Max",ISS_OFF);
    IUFillSwitchVector(&FocusGotoSP,FocusGotoS,3,getDeviceName(),"FOCUS_GOTO","Goto",MAIN_CONTROL_TAB,IP_RW,ISR_1OFMANY,0,IPS_OK);

    IUFillSwitch(&FocusMoveS[0],"FOCUS_OUTWARD","Focus Out",ISS_OFF);
    IUFillSwitch(&FocusMoveS[1],"FOCUS_INWARD","Focus In",ISS_OFF);
    IUFillSwitchVector(&FocusMoveSP,FocusMoveS,2,getDeviceName(),"FOCUS_MOVE","Move",MAIN_CONTROL_TAB,IP_RW,ISR_1OFMANY,60,IPS_OK);

    IUFillNumber(&FocusStepN[0],"FOCUS_STEP_SIZE","Ticks","%0.0f",0,(int)MAX_STEPS/10,(int)(MAX_STEPS/1000),(int)(MAX_STEPS/100));
    IUFillNumberVector(&FocusStepNP,FocusStepN,1,getDeviceName(),"FOCUS_STEP","Step size",MAIN_CONTROL_TAB,IP_RW,0,IPS_OK);

	// TO DO
	IUFillNumber(&FocusBacklashN[0], "FOCUS_BACKLASH_VALUE", "Steps", "%0.0f", 0, (int)(MAX_STEPS/100), (int)(MAX_STEPS/1000), 0);
	IUFillNumberVector(&FocusBacklashNP, FocusBacklashN, 1, getDeviceName(), "FOCUS_BACKLASH", "Backlash", MAIN_CONTROL_TAB, IP_RW, 0, IPS_IDLE);

	IUFillNumber(&PresetN[0], "Preset 1", "", "%0.0f", 0, MAX_STEPS, (int)(MAX_STEPS/100), 0);
	IUFillNumber(&PresetN[1], "Preset 2", "", "%0.0f", 0, MAX_STEPS, (int)(MAX_STEPS/100), 0);
	IUFillNumber(&PresetN[2], "Preset 3", "", "%0.0f", 0, MAX_STEPS, (int)(MAX_STEPS/100), 0);
	IUFillNumberVector(&PresetNP, PresetN, 3, getDeviceName(), "Presets", "", "Presets", IP_RW, 0, IPS_IDLE);

    IUFillSwitch(&FocusResetS[0],"FOCUS_RESET","Reset",ISS_OFF);
    IUFillSwitchVector(&FocusResetSP,FocusResetS,1,getDeviceName(),"FOCUS_RESET","Position Reset",OPTIONS_TAB,IP_RW,ISR_1OFMANY,60,IPS_OK);

    IUFillSwitch(&FocusParkingS[0],"FOCUS_PARKON","Enable",ISS_OFF);
    IUFillSwitch(&FocusParkingS[1],"FOCUS_PARKOFF","Disable",ISS_OFF);
    IUFillSwitchVector(&FocusParkingSP,FocusParkingS,2,getDeviceName(),"FOCUS_PARK","Parking Mode",OPTIONS_TAB,IP_RW,ISR_1OFMANY,60,IPS_OK);
	
	// set capabilities
	
	//setFocuserFeatures(bool CanAbsMove, bool CanRelMove, bool CanAbort, bool VariableSpeed);
	//setFocuserFeatures(true, false, false, false);
	
    FocuserCapability cap;
    cap.canAbort=false;
    cap.canAbsMove=true;
    cap.canRelMove=false;
    cap.variableSpeed=false;

    SetFocuserCapability(&cap);

    return true;
}

void FocusRpi::ISGetProperties (const char *dev)
{
    INDI::Focuser::ISGetProperties(dev);

    /* Add debug controls so we may debug driver if necessary */
    //addDebugControl();

    return;
}

bool FocusRpi::updateProperties()
{

    INDI::Focuser::updateProperties();

    if (isConnected())
    {
        deleteProperty(FocusMotionSP.name);
        deleteProperty(PortTP.name);
        
        defineNumber(&FocusAbsPosNP);
		defineNumber(&FocusStepNP);
		defineNumber(&FocusBacklashNP);
		defineSwitch(&FocusMoveSP);
		//defineSwitch(&FocusGotoSP);
		defineSwitch(&FocusParkingSP);
		defineSwitch(&FocusResetSP);

		loadConfig();

    }
    else
    {
        deleteProperty(FocusAbsPosNP.name);
		deleteProperty(FocusBacklashNP.name);
		deleteProperty(FocusStepNP.name);
		deleteProperty(FocusMoveSP.name);
		//deleteProperty(FocusGotoSP.name);
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
        // handle focus step size
        if (!strcmp(name, FocusStepNP.name))
        {
			IUUpdateNumber(&FocusStepNP, values, names, n);
            FocusStepNP.s = IPS_OK;
            IDSetNumber(&FocusStepNP, "Astroberry Focuser step size set to %d", (int) FocusStepN[0].value);
            return true;
        }

        // handle focus absolute position
        if (!strcmp(name, FocusAbsPosNP.name))
        {
			int newPos = (int) values[0];
            if ( MoveAbsFocuser(newPos) == 0 )
            {
               FocusAbsPosNP.s=IPS_OK;
               IUUpdateNumber(&FocusAbsPosNP,values,names,n);
               return true;
            }
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
        // handle focus goto
        if (!strcmp(name, FocusGotoSP.name))
        {
            IUUpdateSwitch(&FocusGotoSP, states, names, n);

            if ( FocusGotoS[0].s == ISS_ON )
                 MoveAbsFocuser(FocusAbsPosN[0].min);
            if ( FocusGotoS[1].s == ISS_ON )
                MoveAbsFocuser(FocusAbsPosN[0].max/2);
            if ( FocusGotoS[2].s == ISS_ON )
                MoveAbsFocuser(FocusAbsPosN[0].max);


            FocusGotoS[0].s = ISS_OFF;
            FocusGotoS[1].s = ISS_OFF;
            FocusGotoS[2].s = ISS_OFF;
            IDSetSwitch(&FocusGotoSP, NULL);
            return true;
        }

        // handle focus in and out
        if (!strcmp(name, FocusMoveSP.name))
        {
            IUUpdateSwitch(&FocusMoveSP, states, names, n);

			//FOCUS_OUTWARD
            if ( FocusMoveS[0].s == ISS_ON )
				MoveAbsFocuser(FocusAbsPosN[0].value - FocusStepN[0].value);

			//FOCUS_INWARD
            if ( FocusMoveS[1].s == ISS_ON )
				MoveAbsFocuser(FocusAbsPosN[0].value + FocusStepN[0].value);

            FocusMoveS[0].s = ISS_OFF;
            FocusMoveS[1].s = ISS_OFF;
            IDSetSwitch(&FocusMoveSP, NULL);
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

        
    }
    return INDI::Focuser::ISNewSwitch(dev,name,states,names,n);
}


bool FocusRpi::ISSnoopDevice (XMLEle *root)
{
    controller->ISSnoopDevice(root);

    return INDI::Focuser::ISSnoopDevice(root);
}

bool FocusRpi::saveConfigItems(FILE *fp)
{
    IUSaveConfigText(fp, &PortTP);
    IUSaveConfigNumber(fp, &PresetNP);
	IUSaveConfigNumber(fp, &FocusStepNP);
    IUSaveConfigNumber(fp, &FocusBacklashNP);
	IUSaveConfigSwitch(fp, &FocusParkingSP);
    
    if ( FocusParkingS[0].s == ISS_ON )
		IUSaveConfigNumber(fp, &FocusAbsPosNP);

    controller->saveConfigItems(fp);

    return true;
}

int FocusRpi::MoveFocuser(FocusDirection dir, int speed, int duration)
{
	INDI_UNUSED(speed);
	
	int steps, targetsteps;
	 
	steps = duration/STEP_DELAY;
	if ( dir == FOCUS_INWARD )
		FocusAbsPosN[0].value += steps;
	else
		FocusAbsPosN[0].value -= steps;
		
	IDSetNumber(&FocusAbsPosNP, "Astroberry Focuser moved to position %d", FocusAbsPosN[0].value);	
	
	return 0;
}
int FocusRpi::MoveAbsFocuser(int targetTicks)
{
    if (targetTicks == FocusAbsPosN[0].value)
    {
        IDMessage(getDeviceName(), "Astroberry Focuser already in the requested position.");
        return 0;
    }

    if (targetTicks < FocusAbsPosN[0].min || targetTicks > FocusAbsPosN[0].max)
    {
        IDMessage(getDeviceName(), "Requested position is out of range.");
        return -1;
    }

    ticks = abs(targetTicks - FocusAbsPosN[0].value);

	// autospeed mode 1 vs 1/16 steps
    if ( ticks < (int)(MAX_STEPS/100) )
    {
		speed = 5;
		ticks = ticks / 1;
    }
    else
    {
		speed = 1;
		ticks = ticks / 16;
    }

    // motor wake up
    bcm2835_gpio_write(SLEEP, HIGH);

	char lastdir = bcm2835_gpio_lev(DIR);

    // set direction
    if (targetTicks > FocusAbsPosN[0].value)
    {
		// INWARD
		bcm2835_gpio_write(DIR, LOW);
	}
    else
	{
		// OUTWARD
		bcm2835_gpio_write(DIR, HIGH);
	}

    // set speed
    SetSpeed(speed);

    // let's go
    IDMessage(getDeviceName() , "Astroberry Focuser is moving to requested position...");

	// if direction changed do backlash compensation
	if ( bcm2835_gpio_lev(DIR) != lastdir && FocusBacklashN[0].value != 0 && FocusAbsPosN[0].value != FocusAbsPosN[0].min && FocusAbsPosN[0].value != FocusAbsPosN[0].max )
	{
		IDMessage(getDeviceName() , "Astroberry Focuser compensating backlash by %0.0f steps...", FocusBacklashN[0].value);
		for ( int b = 0; b < FocusBacklashN[0].value; b++ )
		{
			// step on
			bcm2835_gpio_write(STEP, HIGH);
			// wait
			bcm2835_delay(STEP_DELAY/2);
			// step off
			bcm2835_gpio_write(STEP, LOW);
			// wait 
			bcm2835_delay(STEP_DELAY/2);
		}
	}

    for ( int i = 1; i <= ticks; i++ )
    {
        // step on
        bcm2835_gpio_write(STEP, HIGH);
        // wait
        bcm2835_delay(STEP_DELAY/2);
        // step off
        bcm2835_gpio_write(STEP, LOW);
        // wait 
        bcm2835_delay(STEP_DELAY/2);

	// OUTWARD - count down
        if ( bcm2835_gpio_lev(DIR) == HIGH && speed == 1 )
			FocusAbsPosN[0].value -= 16;
		if ( bcm2835_gpio_lev(DIR) == HIGH && speed == 5 )
			FocusAbsPosN[0].value -= 1;

	// INWARD - count up
        if ( bcm2835_gpio_lev(DIR) == LOW && speed == 1 )
			FocusAbsPosN[0].value += 16;
		if ( bcm2835_gpio_lev(DIR) == LOW && speed == 5 )
			FocusAbsPosN[0].value += 1;

		IDSetNumber(&FocusAbsPosNP, NULL);
    }

    // motor sleep
    bcm2835_gpio_write(SLEEP, LOW);

    // set abspos value
    FocusAbsPosN[0].value = targetTicks;
    IDSetNumber(&FocusAbsPosNP, "Astroberry Focuser moved to position %d", targetTicks);

	// don't return too quickly - EKOS autofocus needs it, so it does not capture image too fast
	sleep(1);

    return 0;
}

bool FocusRpi::SetSpeed(int speed)
{
	/* Stepper motor resolution settings (for PG2528-0502U)
	* 1) 1/1   - M0=0 M1=0
	* 2) 1/2   - M0=1 M1=0
	* 3) 1/4   - M0=X M1=0
	* 4) 1/8   - M0=0 M1=1
	* 5) 1/16  - M0=1 M1=1
	* 6) 1/32  - M0=X M1=1
	*/

    switch(speed)
    {
    case 1:	// 1:1
        bcm2835_gpio_fsel(M0, BCM2835_GPIO_FSEL_OUTP);
		bcm2835_gpio_fsel(M1, BCM2835_GPIO_FSEL_OUTP);
        bcm2835_gpio_write(M0, LOW);
		bcm2835_gpio_write(M1, LOW);
        break;
    case 2:	// 1:2
        bcm2835_gpio_fsel(M0, BCM2835_GPIO_FSEL_OUTP);
		bcm2835_gpio_fsel(M1, BCM2835_GPIO_FSEL_OUTP);
		bcm2835_gpio_write(M0, HIGH);
        bcm2835_gpio_write(M1, LOW);
        break;
    case 3:	// 1:4
        bcm2835_gpio_fsel(M0, BCM2835_GPIO_FSEL_INPT);
		bcm2835_gpio_fsel(M1, BCM2835_GPIO_FSEL_OUTP);
        //bcm2835_gpio_write(M0, LOW); // should be FLOATING
        bcm2835_gpio_write(M1, LOW);
        break;
    case 4:	// 1:8
        bcm2835_gpio_fsel(M0, BCM2835_GPIO_FSEL_OUTP);
		bcm2835_gpio_fsel(M1, BCM2835_GPIO_FSEL_OUTP);
        bcm2835_gpio_write(M0, LOW);
        bcm2835_gpio_write(M1, HIGH);
        break;
    case 5:	// 1:16
        bcm2835_gpio_fsel(M0, BCM2835_GPIO_FSEL_OUTP);
		bcm2835_gpio_fsel(M1, BCM2835_GPIO_FSEL_OUTP);
        bcm2835_gpio_write(M0, HIGH);
        bcm2835_gpio_write(M1, HIGH);
        break;
    case 6:	// 1:32
        bcm2835_gpio_fsel(M0, BCM2835_GPIO_FSEL_INPT);
		bcm2835_gpio_fsel(M1, BCM2835_GPIO_FSEL_OUTP);
        //bcm2835_gpio_write(M0, LOW); // should be FLOATING
        bcm2835_gpio_write(M1, HIGH);
        break;
    default:	// 1:1
        bcm2835_gpio_fsel(M0, BCM2835_GPIO_FSEL_OUTP);
		bcm2835_gpio_fsel(M1, BCM2835_GPIO_FSEL_OUTP);
        bcm2835_gpio_write(M0, LOW);
        bcm2835_gpio_write(M1, LOW);
        break;
    }

	if (speed != 1)
	  IDMessage(getDeviceName() , "Astroberry Focuser set to fine mode.");

	return true;
}
