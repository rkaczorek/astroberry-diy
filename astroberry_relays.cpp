/*******************************************************************************
  Copyright(c) 2015-2020 Radek Kaczorek  <rkaczorek AT gmail DOT com>

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
#include <string.h>
#include "config.h"

#include "astroberry_relays.h"

#include <gpiod.h>

// We declare an auto pointer to IndiAstroberryRelays
std::unique_ptr<IndiAstroberryRelays> indiAstroberryRelays(new IndiAstroberryRelays());

// DO NOT USE pin numbers!
// BCM numbers ONLY
#define RELAY1 5 // PIN29
#define RELAY2 6 // PIN31
#define RELAY3 13 // PIN33
#define RELAY4 19 // PIN35


void ISPoll(void *p);

void ISInit()
{
   static int isInit = 0;

   if (isInit == 1)
       return;
   if(indiAstroberryRelays.get() == 0)
   {
       isInit = 1;
       indiAstroberryRelays.reset(new IndiAstroberryRelays());
   }
}
void ISGetProperties(const char *dev)
{
        ISInit();
        indiAstroberryRelays->ISGetProperties(dev);
}
void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
        ISInit();
        indiAstroberryRelays->ISNewSwitch(dev, name, states, names, num);
}
void ISNewText(	const char *dev, const char *name, char *texts[], char *names[], int num)
{
        ISInit();
        indiAstroberryRelays->ISNewText(dev, name, texts, names, num);
}
void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
        ISInit();
        indiAstroberryRelays->ISNewNumber(dev, name, values, names, num);
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
	indiAstroberryRelays->ISSnoopDevice(root);
}
IndiAstroberryRelays::IndiAstroberryRelays()
{
	setVersion(VERSION_MAJOR,VERSION_MINOR);
}
IndiAstroberryRelays::~IndiAstroberryRelays()
{
}
bool IndiAstroberryRelays::Connect()
{
	chip = gpiod_chip_open("/dev/gpiochip0");
	if (!chip)
        {
                DEBUG(INDI::Logger::DBG_ERROR, "Problem initiating Astroberry Relays.");
                return false;
        }

	// Select gpios
	gpio_relay1 = gpiod_chip_get_line(chip, RELAY1);
	gpio_relay2 = gpiod_chip_get_line(chip, RELAY2);
	gpio_relay3 = gpiod_chip_get_line(chip, RELAY3);
	gpio_relay4 = gpiod_chip_get_line(chip, RELAY4);

	// Set initial gpios direction and default states
	gpiod_line_request_output(gpio_relay1, getDefaultName(), 1);
	gpiod_line_request_output(gpio_relay2, getDefaultName(), 1);
	gpiod_line_request_output(gpio_relay3, getDefaultName(), 1);
	gpiod_line_request_output(gpio_relay4, getDefaultName(), 1);

/*
	if ( !IndiAstroberryRelays::LoadLines() )
	{
		IDMessage(getDeviceName(), "Astroberry Relays connection error.");
		return false;
	}
*/
	IDMessage(getDeviceName(), "Astroberry Relays connected successfully.");
	return true;
}
bool IndiAstroberryRelays::Disconnect()
{
	// Close device
	gpiod_chip_close(chip);

	IDMessage(getDeviceName(), "Astroberry Relays disconnected successfully.");
	return true;
}
const char * IndiAstroberryRelays::getDefaultName()
{
        return (char *)"Astroberry Relays";
}
bool IndiAstroberryRelays::initProperties()
{
    // We init parent properties first
    INDI::DefaultDevice::initProperties();

    IUFillSwitch(&Switch1S[0], "SW1ON", "ON", ISS_OFF);
    IUFillSwitch(&Switch1S[1], "SW1OFF", "OFF", ISS_ON);
    IUFillSwitchVector(&Switch1SP, Switch1S, 2, getDeviceName(), "SWITCH_1", "Relay 1", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    IUFillSwitch(&Switch2S[0], "SW2ON", "ON", ISS_OFF);
    IUFillSwitch(&Switch2S[1], "SW2OFF", "OFF", ISS_ON);
    IUFillSwitchVector(&Switch2SP, Switch2S, 2, getDeviceName(), "SWITCH_2", "Relay 2", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    IUFillSwitch(&Switch3S[0], "SW3ON", "ON", ISS_OFF);
    IUFillSwitch(&Switch3S[1], "SW3OFF", "OFF", ISS_ON);
    IUFillSwitchVector(&Switch3SP, Switch3S, 2, getDeviceName(), "SWITCH_3", "Relay 3", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    IUFillSwitch(&Switch4S[0], "SW4ON", "ON", ISS_OFF);
    IUFillSwitch(&Switch4S[1], "SW4OFF", "OFF", ISS_ON);
    IUFillSwitchVector(&Switch4SP, Switch4S, 2, getDeviceName(), "SWITCH_4", "Relay 4", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    return true;
}
bool IndiAstroberryRelays::updateProperties()
{
    // Call parent update properties first
    INDI::DefaultDevice::updateProperties();

    if (isConnected())
    {
	defineSwitch(&Switch1SP);
	defineSwitch(&Switch2SP);
	defineSwitch(&Switch3SP);
	defineSwitch(&Switch4SP);
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
void IndiAstroberryRelays::ISGetProperties(const char *dev)
{
    INDI::DefaultDevice::ISGetProperties(dev);
}
bool IndiAstroberryRelays::ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n)
{
	return INDI::DefaultDevice::ISNewNumber(dev,name,values,names,n);
}
bool IndiAstroberryRelays::ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n)
{
	// first we check if it's for our device
    if (!strcmp(dev, getDeviceName()))
    {
		// handle relay 1
		if (!strcmp(name, Switch1SP.name))
		{
			IUUpdateSwitch(&Switch1SP, states, names, n);

			if ( Switch1S[0].s == ISS_ON )
			{
				gpiod_line_set_value(gpio_relay1, 0);
				IDMessage(getDeviceName(), "Astroberry Relays #1 set to ON");
				Switch1SP.s = IPS_OK;
				Switch1S[1].s = ISS_OFF;
				IDSetSwitch(&Switch1SP, NULL);
				return true;
			}
			if ( Switch1S[1].s == ISS_ON )
			{
				gpiod_line_set_value(gpio_relay1, 1);
				IDMessage(getDeviceName(), "Astroberry Relays #1 set to OFF");
				Switch1SP.s = IPS_IDLE;
				Switch1S[0].s = ISS_OFF;
				IDSetSwitch(&Switch1SP, NULL);
				return true;
			}
		}

		// handle relay 2
		if (!strcmp(name, Switch2SP.name))
		{
			IUUpdateSwitch(&Switch2SP, states, names, n);

			if ( Switch2S[0].s == ISS_ON )
			{
				gpiod_line_set_value(gpio_relay2, 0);
				IDMessage(getDeviceName(), "Astroberry Relays #2 set to ON");
				Switch2SP.s = IPS_OK;
				Switch2S[1].s = ISS_OFF;
				IDSetSwitch(&Switch2SP, NULL);
				return true;
			}
			if ( Switch2S[1].s == ISS_ON )
			{
				gpiod_line_set_value(gpio_relay2, 1);
				IDMessage(getDeviceName(), "Astroberry Relays #2 set to OFF");
				Switch2SP.s = IPS_IDLE;
				Switch2S[0].s = ISS_OFF;
				IDSetSwitch(&Switch2SP, NULL);
				return true;
			}
		}

		// handle relay 3
		if (!strcmp(name, Switch3SP.name))
		{
			IUUpdateSwitch(&Switch3SP, states, names, n);

			if ( Switch3S[0].s == ISS_ON )
			{
				gpiod_line_set_value(gpio_relay3, 0);
				IDMessage(getDeviceName(), "Astroberry Relays #3 set to ON");
				Switch3SP.s = IPS_OK;
				Switch3S[1].s = ISS_OFF;
				IDSetSwitch(&Switch3SP, NULL);
				return true;
			}
			if ( Switch3S[1].s == ISS_ON )
			{
				gpiod_line_set_value(gpio_relay3, 1);
				IDMessage(getDeviceName(), "Astroberry Relays #3 set to OFF");
				Switch3SP.s = IPS_IDLE;
				Switch3S[0].s = ISS_OFF;
				IDSetSwitch(&Switch3SP, NULL);
				return true;
			}
		}

		// handle relay 4
		if (!strcmp(name, Switch4SP.name))
		{
			IUUpdateSwitch(&Switch4SP, states, names, n);

			if ( Switch4S[0].s == ISS_ON )
			{
				gpiod_line_set_value(gpio_relay4, 0);
				IDMessage(getDeviceName(), "Astroberry Relays #4 set to ON");
				Switch4SP.s = IPS_OK;
				Switch4S[1].s = ISS_OFF;
				IDSetSwitch(&Switch4SP, NULL);
				return true;
			}
			if ( Switch4S[1].s == ISS_ON )
			{
				gpiod_line_set_value(gpio_relay4, 1);
				IDMessage(getDeviceName(), "Astroberry Relays #4 set to OFF");
				Switch4SP.s = IPS_IDLE;
				Switch4S[0].s = ISS_OFF;
				IDSetSwitch(&Switch4SP, NULL);
				return true;
			}
		}
	}
	return INDI::DefaultDevice::ISNewSwitch (dev, name, states, names, n);
}
bool IndiAstroberryRelays::ISNewText (const char *dev, const char *name, char *texts[], char *names[], int n)
{
	return INDI::DefaultDevice::ISNewText (dev, name, texts, names, n);
}
bool IndiAstroberryRelays::ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n)
{
	return INDI::DefaultDevice::ISNewBLOB (dev, name, sizes, blobsizes, blobs, formats, names, n);
}
bool IndiAstroberryRelays::ISSnoopDevice(XMLEle *root)
{
    return INDI::DefaultDevice::ISSnoopDevice(root);
}
bool IndiAstroberryRelays::saveConfigItems(FILE *fp)
{
	IUSaveConfigSwitch(fp, &Switch1SP);
	IUSaveConfigSwitch(fp, &Switch2SP);
	IUSaveConfigSwitch(fp, &Switch3SP);
	IUSaveConfigSwitch(fp, &Switch4SP);

    //controller->saveConfigItems(fp);

    return true;
}
bool IndiAstroberryRelays::LoadLines()
{
	// load line 1 state
	if ( gpiod_line_get_value(gpio_relay1) == 0 )
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
	if ( gpiod_line_get_value(gpio_relay2) == 0 )
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
	if ( gpiod_line_get_value(gpio_relay3) == 0 )
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
	if ( gpiod_line_get_value(gpio_relay4) == 0 )
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
