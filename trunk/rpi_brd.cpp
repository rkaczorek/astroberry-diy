/*******************************************************************************
  Copyright(c) 2015 Radek Kaczorek  <rkaczorek AT gmail DOT com>

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
#include <memory>
#include <bcm2835.h>

#include "rpi_brd.h"

// We declare an auto pointer to IndiRpibrd
std::auto_ptr<IndiRpibrd> indiRpibrd(0);

#define POLLMS 1000

// indicate GPIOs used - use P1_* pin numbers not gpio numbers (!!!)
#define IN1 RPI_BPLUS_GPIO_J8_29	// GPIOO5
#define IN2 RPI_BPLUS_GPIO_J8_31	// GPIO06
#define IN3 RPI_BPLUS_GPIO_J8_33	// GPIO13
#define IN4 RPI_BPLUS_GPIO_J8_37	// GPIO26

void ISPoll(void *p);
void ISInit()
{
   static int isInit =0;

   if (isInit == 1)
       return;

    isInit = 1;
    if(indiRpibrd.get() == 0) indiRpibrd.reset(new IndiRpibrd());
    //IEAddTimer(POLLMS, ISPoll, NULL);

}
void ISGetProperties(const char *dev)
{
        ISInit();
        indiRpibrd->ISGetProperties(dev);
}
void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
        ISInit();
        indiRpibrd->ISNewSwitch(dev, name, states, names, num);
}
void ISNewText(	const char *dev, const char *name, char *texts[], char *names[], int num)
{
        ISInit();
        indiRpibrd->ISNewText(dev, name, texts, names, num);
}
void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
        ISInit();
        indiRpibrd->ISNewNumber(dev, name, values, names, num);
}
void ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int num)
{
  INDI_UNUSED(dev);
  INDI_UNUSED(name);
  INDI_UNUSED(sizes);
  INDI_UNUSED(blobsizes);
  INDI_UNUSED(blobs);
  INDI_UNUSED(formats);
  INDI_UNUSED(names);
  INDI_UNUSED(num);
}
void ISSnoopDevice (XMLEle *root)
{
    ISInit();
    indiRpibrd->ISSnoopDevice(root);
}
IndiRpibrd::IndiRpibrd()
{
    if (!bcm2835_init())
    {
		IDLog("Problem initiating Astroberry Board.");
		return;
	}

    // init GPIOs
    std::ofstream exportgpio;
    exportgpio.open("/sys/class/gpio/export");
    exportgpio << IN1 << std::endl;
    exportgpio << IN2 << std::endl;
    exportgpio << IN3 << std::endl;
    exportgpio << IN4 << std::endl;
    exportgpio.close();

    // Set gpios to output mode
    bcm2835_gpio_fsel(IN1, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(IN2, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(IN3, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(IN4, BCM2835_GPIO_FSEL_OUTP);

	bcm2835_gpio_write(IN1, HIGH);
	bcm2835_gpio_write(IN2, HIGH);
	bcm2835_gpio_write(IN3, HIGH);
	bcm2835_gpio_write(IN4, HIGH);
}
IndiRpibrd::~IndiRpibrd()
{
}
bool IndiRpibrd::Connect()
{
/*
	if ( !IndiRpibrd::LoadLines() )
	{
		IDMessage(getDeviceName(), "Astroberry Board connection error.");
		return false;
	}
*/
    IDMessage(getDeviceName(), "Astroberry Board connected successfully.");
    //IDMessage(getDeviceName(), "WARNING!!! Line A is powering your Raspberry Pi. Keep it always ON. Setting it to OFF will immediatelly REBOOT your device. Use with care!!!");
    return true;
}
bool IndiRpibrd::Disconnect()
{
/*
    // close GPIOs
    std::ofstream unexportgpio;
    unexportgpio.open("/sys/class/gpio/unexport");
    unexportgpio << IN1 << std::endl;
    unexportgpio << IN2 << std::endl;
    unexportgpio << IN3 << std::endl;
    unexportgpio << IN4 << std::endl;
    unexportgpio.close();
    bcm2835_close();
*/    	
    IDMessage(getDeviceName(), "Astroberry Board disconnected successfully.");
    return true;
}
void IndiRpibrd::TimerHit()
{
	if(isConnected())
	{
		//SetTimer( POLLMS );
    }
}
const char * IndiRpibrd::getDefaultName()
{
        return (char *)"Astroberry Board";
}
bool IndiRpibrd::initProperties()
{
    // We init parent properties first
    INDI::DefaultDevice::initProperties();

    IUFillSwitch(&Switch1S[0], "SW1ON", "Power ON", ISS_OFF);
    IUFillSwitch(&Switch1S[1], "SW1OFF", "Power OFF", ISS_ON);
    IUFillSwitchVector(&Switch1SP, Switch1S, 2, getDeviceName(), "SWITCH_1", "Line A: 5V", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_OK);

    IUFillSwitch(&Switch2S[0], "SW2ON", "Power ON", ISS_OFF);
    IUFillSwitch(&Switch2S[1], "SW2OFF", "Power OFF", ISS_ON);
    IUFillSwitchVector(&Switch2SP, Switch2S, 2, getDeviceName(), "SWITCH_2", "Line B: 5V", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    IUFillSwitch(&Switch3S[0], "SW3ON", "Power ON", ISS_OFF);
    IUFillSwitch(&Switch3S[1], "SW3OFF", "Power OFF", ISS_ON);
    IUFillSwitchVector(&Switch3SP, Switch3S, 2, getDeviceName(), "SWITCH_3", "Line C: 0-12V", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    IUFillSwitch(&Switch4S[0], "SW4ON", "Power ON", ISS_OFF);
    IUFillSwitch(&Switch4S[1], "SW4OFF", "Power OFF", ISS_ON);
    IUFillSwitchVector(&Switch4SP, Switch4S, 2, getDeviceName(), "SWITCH_4", "Line D: 12V", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    return true;
}
bool IndiRpibrd::updateProperties()
{
    // Call parent update properties first
    INDI::DefaultDevice::updateProperties();

    if (isConnected())
    {
		defineSwitch(&Switch1SP);
		defineSwitch(&Switch2SP);
		defineSwitch(&Switch3SP);
		defineSwitch(&Switch4SP);
	
		loadConfig();
				
		//SetTimer ( POLLMS );		
    }
    else
    {
		// We're disconnected
		deleteProperty(Switch1SP.name);
		deleteProperty(Switch2SP.name);
		deleteProperty(Switch3SP.name);
		deleteProperty(Switch4SP.name);
    }
    return true;
}
void IndiRpibrd::ISGetProperties(const char *dev)
{
    INDI::DefaultDevice::ISGetProperties(dev);

    /* Add debug controls so we may debug driver if necessary */
    //addDebugControl();
}
bool IndiRpibrd::ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n)
{
	return INDI::DefaultDevice::ISNewNumber(dev,name,values,names,n);
}
bool IndiRpibrd::ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n)
{
	// first we check if it's for our device
    if (!strcmp(dev, getDeviceName()))
    {
		// handle switch 1
		if (!strcmp(name, Switch1SP.name))
		{
			IUUpdateSwitch(&Switch1SP, states, names, n);

			if ( Switch1S[0].s == ISS_ON )
			{
				bcm2835_gpio_write(IN1, LOW);
				IDMessage(getDeviceName(), "Astroberry Board Power Line A set to ON");
				Switch1SP.s = IPS_OK;
				Switch1S[1].s = ISS_OFF;
				IDSetSwitch(&Switch1SP, NULL);
				return true;
			}
			if ( Switch1S[1].s == ISS_ON )
			{
				bcm2835_gpio_write(IN1, HIGH);
				IDMessage(getDeviceName(), "Astroberry Board Power Line A set to OFF");
				Switch1SP.s = IPS_IDLE;
				Switch1S[0].s = ISS_OFF;
				IDSetSwitch(&Switch1SP, NULL);
				return true;
			}
		}

		// handle switch 2
		if (!strcmp(name, Switch2SP.name))
		{
			IUUpdateSwitch(&Switch2SP, states, names, n);

			if ( Switch2S[0].s == ISS_ON )
			{
				bcm2835_gpio_write(IN2, LOW);
				IDMessage(getDeviceName(), "Astroberry Board Power Line B set to ON");
				Switch2SP.s = IPS_OK;
				Switch2S[1].s = ISS_OFF;
				IDSetSwitch(&Switch2SP, NULL);
				return true;
			}
			if ( Switch2S[1].s == ISS_ON )
			{
				bcm2835_gpio_write(IN2, HIGH);
				IDMessage(getDeviceName(), "Astroberry Board Power Line B set to OFF");
				Switch2SP.s = IPS_IDLE;
				Switch2S[0].s = ISS_OFF;
				IDSetSwitch(&Switch2SP, NULL);
				return true;
			}
		}

		// handle switch 3
		if (!strcmp(name, Switch3SP.name))
		{
			IUUpdateSwitch(&Switch3SP, states, names, n);

			if ( Switch3S[0].s == ISS_ON )
			{
				bcm2835_gpio_write(IN3, LOW);
				IDMessage(getDeviceName(), "Astroberry Board Power Line C set to ON");
				Switch3SP.s = IPS_OK;
				Switch3S[1].s = ISS_OFF;
				IDSetSwitch(&Switch3SP, NULL);
				return true;
			}
			if ( Switch3S[1].s == ISS_ON )
			{
				bcm2835_gpio_write(IN3, HIGH);
				IDMessage(getDeviceName(), "Astroberry Board Power Line C set to OFF");
				Switch3SP.s = IPS_IDLE;
				Switch3S[0].s = ISS_OFF;
				IDSetSwitch(&Switch3SP, NULL);
				return true;
			}
		}

		// handle switch 4
		if (!strcmp(name, Switch4SP.name))
		{
			IUUpdateSwitch(&Switch4SP, states, names, n);

			if ( Switch4S[0].s == ISS_ON )
			{
				bcm2835_gpio_write(IN4, LOW);
				IDMessage(getDeviceName(), "Astroberry Board Power Line D set to ON");
				Switch4SP.s = IPS_OK;
				Switch4S[1].s = ISS_OFF;
				IDSetSwitch(&Switch4SP, NULL);
				return true;
			}
			if ( Switch4S[1].s == ISS_ON )
			{
				bcm2835_gpio_write(IN4, HIGH);
				IDMessage(getDeviceName(), "Astroberry Board Power Line D set to OFF");
				Switch4SP.s = IPS_IDLE;
				Switch4S[0].s = ISS_OFF;
				IDSetSwitch(&Switch4SP, NULL);
				return true;
			}
		}
	}
	return INDI::DefaultDevice::ISNewSwitch (dev, name, states, names, n);
}
bool IndiRpibrd::ISNewText (const char *dev, const char *name, char *texts[], char *names[], int n)
{
	return INDI::DefaultDevice::ISNewText (dev, name, texts, names, n);
}
bool IndiRpibrd::ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n)
{
	return INDI::DefaultDevice::ISNewBLOB (dev, name, sizes, blobsizes, blobs, formats, names, n);
}
bool IndiRpibrd::ISSnoopDevice(XMLEle *root)
{
    return INDI::DefaultDevice::ISSnoopDevice(root);
}
bool IndiRpibrd::saveConfigItems(FILE *fp)
{
	IUSaveConfigSwitch(fp, &Switch1SP);
	IUSaveConfigSwitch(fp, &Switch2SP);
	IUSaveConfigSwitch(fp, &Switch3SP);
	IUSaveConfigSwitch(fp, &Switch4SP);
        
    //controller->saveConfigItems(fp);

    return true;
}
bool IndiRpibrd::LoadLines()
{
	// load line 1 state - default OFF
	if ( bcm2835_gpio_lev(IN1) == LOW )
	{
		Switch1S[0].s = ISS_ON;
		Switch1S[1].s = ISS_OFF;
		Switch1SP.s = IPS_OK;
		IDSetSwitch(&Switch1SP, NULL);
	}
	else
	{
		Switch1S[0].s = ISS_OFF;
		Switch1S[1].s = ISS_ON;
		Switch1SP.s = IPS_IDLE;
		IDSetSwitch(&Switch1SP, NULL);
	}

	// load line 2 state 
	if ( bcm2835_gpio_lev(IN2) == LOW )
	{
		Switch2S[0].s = ISS_ON;
		Switch2S[1].s = ISS_OFF;
		Switch2SP.s = IPS_OK;
		IDSetSwitch(&Switch2SP, NULL);
	}
	else
	{
		Switch2S[0].s = ISS_OFF;
		Switch2S[1].s = ISS_ON;
		Switch2SP.s = IPS_IDLE;
		IDSetSwitch(&Switch2SP, NULL);
	}

	// load line 3 state 
	if ( bcm2835_gpio_lev(IN3) == LOW )
	{
		Switch3S[0].s = ISS_ON;
		Switch3S[1].s = ISS_OFF;
		Switch3SP.s = IPS_OK;
		IDSetSwitch(&Switch3SP, NULL);
	}
	else
	{
		Switch3S[0].s = ISS_OFF;
		Switch3S[1].s = ISS_ON;
		Switch3SP.s = IPS_IDLE;
		IDSetSwitch(&Switch3SP, NULL);
	}

	// load line 4 state 
	if ( bcm2835_gpio_lev(IN4) == LOW )
	{
		Switch4S[0].s = ISS_ON;
		Switch4S[1].s = ISS_OFF;
		Switch4SP.s = IPS_OK;
		IDSetSwitch(&Switch4SP, NULL);
	}
	else
	{
		Switch4S[0].s = ISS_OFF;
		Switch4S[1].s = ISS_ON;
		Switch4SP.s = IPS_IDLE;
		IDSetSwitch(&Switch4SP, NULL);
	}
	return true;
}
