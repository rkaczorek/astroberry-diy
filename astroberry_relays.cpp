/*******************************************************************************
  Copyright(c) 2015-2021 Radek Kaczorek  <rkaczorek AT gmail DOT com>

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
	// Delete BCM Pins options
	deleteProperty(BCMpinsNP.name);
}
bool IndiAstroberryRelays::Connect()
{
	// Init GPIO
	chip = gpiod_chip_open("/dev/gpiochip0");
	if (!chip)
	{
		DEBUG(INDI::Logger::DBG_SESSION, "Problem initiating Astroberry Relays.");
		return false;
	}

	// verify BCM Pins are not used by other consumers
	for (unsigned int pin = 0; pin < 4; pin++)
	{
		if (gpiod_line_is_used(gpiod_chip_get_line(chip, BCMpinsN[pin].value)))
		{
			DEBUGF(INDI::Logger::DBG_ERROR, "BCM Pin %0.0f already used", BCMpinsN[pin].value);
			gpiod_chip_close(chip);
			return false;
		}
	}

	// Select gpios
	gpio_relay1 = gpiod_chip_get_line(chip, BCMpinsN[0].value);
	gpio_relay2 = gpiod_chip_get_line(chip, BCMpinsN[1].value);
	gpio_relay3 = gpiod_chip_get_line(chip, BCMpinsN[2].value);
	gpio_relay4 = gpiod_chip_get_line(chip, BCMpinsN[3].value);

	// Set initial gpios direction and default states
	gpiod_line_request_output(gpio_relay1, "1@astroberry_relays", !activeState);
	gpiod_line_request_output(gpio_relay2, "2@astroberry_relays", !activeState);
	gpiod_line_request_output(gpio_relay3, "3@astroberry_relays", !activeState);
	gpiod_line_request_output(gpio_relay4, "4@astroberry_relays", !activeState);

	// Lock BCM Pins setting
	BCMpinsNP.s=IPS_BUSY;
	IDSetNumber(&BCMpinsNP, nullptr);

	DEBUG(INDI::Logger::DBG_SESSION, "Astroberry Relays connected successfully.");
	return true;
}
bool IndiAstroberryRelays::Disconnect()
{
	// Close GPIO
	gpiod_chip_close(chip);

	// Unlock BCM Pins setting
	BCMpinsNP.s=IPS_IDLE;
	IDSetNumber(&BCMpinsNP, nullptr);

	DEBUG(INDI::Logger::DBG_SESSION, "Astroberry Relays disconnected successfully.");
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

	IUFillNumber(&BCMpinsN[0], "BCMPIN01", "Relay 1", "%0.0f", 1, 27, 0, 5); // BCM5 = PIN29
	IUFillNumber(&BCMpinsN[1], "BCMPIN02", "Relay 2", "%0.0f", 1, 27, 0, 6); // BCM6 = PIN31
	IUFillNumber(&BCMpinsN[2], "BCMPIN03", "Relay 3", "%0.0f", 1, 27, 0, 13); // BCM13 = PIN33
	IUFillNumber(&BCMpinsN[3], "BCMPIN04", "Relay 4", "%0.0f", 1, 27, 0, 19); // BCM19 = PIN35
	IUFillNumberVector(&BCMpinsNP, BCMpinsN, 4, getDeviceName(), "BCMPINS", "BCM Pins", OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

	IUFillSwitch(&ActiveStateS[0], "ACTIVELO", "Low", ISS_ON);
	IUFillSwitch(&ActiveStateS[1], "ACTIVEHI", "High", ISS_OFF);
	IUFillSwitchVector(&ActiveStateSP, ActiveStateS, 2, getDeviceName(), "ACTIVESTATE", "Active State", OPTIONS_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);


	// Load BCM Pins
	defineNumber(&BCMpinsNP);
	defineSwitch(&ActiveStateSP);
	loadConfig();

	return true;
}
bool IndiAstroberryRelays::updateProperties()
{
	// Call parent update properties first
	INDI::DefaultDevice::updateProperties();

	if (isConnected())
	{
		// We're connected
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
	// first we check if it's for our device
	if(strcmp(dev,getDeviceName())==0)
	{
	        // handle BCMpins
	        if (!strcmp(name, BCMpinsNP.name))
	        {
			unsigned int valcount = 4;

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
						DEBUG(INDI::Logger::DBG_ERROR, "Problem initiating Astroberry Relays.");
						return false;
					}
				}

				IUUpdateNumber(&BCMpinsNP,values,names,n);

				BCMpinsNP.s=IPS_OK;
				IDSetNumber(&BCMpinsNP, nullptr);
				DEBUGF(INDI::Logger::DBG_SESSION, "Astroberry Relays BCM Pins set to Relay1: BCM%0.0f, Relay2: BCM%0.0f, Relay3: BCM%0.0f, Relay4: BCM%0.0f", BCMpinsN[0].value, BCMpinsN[1].value, BCMpinsN[2].value, BCMpinsN[3].value);
				return true;
			}
        	}
	}

	return INDI::DefaultDevice::ISNewNumber(dev,name,values,names,n);
}
bool IndiAstroberryRelays::ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n)
{
	// first we check if it's for our device
	if (!strcmp(dev, getDeviceName()))
	{
		// handle active state
		if (!strcmp(name, ActiveStateSP.name))
		{
			IUUpdateSwitch(&ActiveStateSP, states, names, n);

			if ( ActiveStateS[0].s == ISS_ON )
			{
				activeState = 0;
				DEBUG(INDI::Logger::DBG_SESSION, "Astroberry Relays active state set to LOW");
				ActiveStateSP.s = IPS_OK;
				ActiveStateS[1].s = ISS_OFF;
				IDSetSwitch(&ActiveStateSP, NULL);
				return true;
			}
			if ( ActiveStateS[1].s == ISS_ON )
			{
				activeState = 1;
				DEBUG(INDI::Logger::DBG_SESSION, "Astroberry Relays active state set to HIGH");
				ActiveStateSP.s = IPS_IDLE;
				ActiveStateS[0].s = ISS_OFF;
				IDSetSwitch(&ActiveStateSP, NULL);
				return true;
			}
		}

		// handle relay 1
		if (!strcmp(name, Switch1SP.name))
		{
			IUUpdateSwitch(&Switch1SP, states, names, n);

			if ( Switch1S[0].s == ISS_ON )
			{
				gpiod_line_set_value(gpio_relay1, activeState);
				DEBUG(INDI::Logger::DBG_SESSION, "Astroberry Relays #1 set to ON");
				Switch1SP.s = IPS_OK;
				Switch1S[1].s = ISS_OFF;
				IDSetSwitch(&Switch1SP, NULL);
				return true;
			}
			if ( Switch1S[1].s == ISS_ON )
			{
				gpiod_line_set_value(gpio_relay1, !activeState);
				DEBUG(INDI::Logger::DBG_SESSION, "Astroberry Relays #1 set to OFF");
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
				gpiod_line_set_value(gpio_relay2, activeState);
				DEBUG(INDI::Logger::DBG_SESSION, "Astroberry Relays #2 set to ON");
				Switch2SP.s = IPS_OK;
				Switch2S[1].s = ISS_OFF;
				IDSetSwitch(&Switch2SP, NULL);
				return true;
			}
			if ( Switch2S[1].s == ISS_ON )
			{
				gpiod_line_set_value(gpio_relay2, !activeState);
				DEBUG(INDI::Logger::DBG_SESSION, "Astroberry Relays #2 set to OFF");
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
				gpiod_line_set_value(gpio_relay3, activeState);
				DEBUG(INDI::Logger::DBG_SESSION, "Astroberry Relays #3 set to ON");
				Switch3SP.s = IPS_OK;
				Switch3S[1].s = ISS_OFF;
				IDSetSwitch(&Switch3SP, NULL);
				return true;
			}
			if ( Switch3S[1].s == ISS_ON )
			{
				gpiod_line_set_value(gpio_relay3, !activeState);
				DEBUG(INDI::Logger::DBG_SESSION, "Astroberry Relays #3 set to OFF");
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
				gpiod_line_set_value(gpio_relay4, activeState);
				DEBUG(INDI::Logger::DBG_SESSION, "Astroberry Relays #4 set to ON");
				Switch4SP.s = IPS_OK;
				Switch4S[1].s = ISS_OFF;
				IDSetSwitch(&Switch4SP, NULL);
				return true;
			}
			if ( Switch4S[1].s == ISS_ON )
			{
				gpiod_line_set_value(gpio_relay4, !activeState);
				DEBUG(INDI::Logger::DBG_SESSION, "Astroberry Relays #4 set to OFF");
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
	IUSaveConfigNumber(fp, &BCMpinsNP);
	IUSaveConfigSwitch(fp, &ActiveStateSP);
	IUSaveConfigSwitch(fp, &Switch1SP);
	IUSaveConfigSwitch(fp, &Switch2SP);
	IUSaveConfigSwitch(fp, &Switch3SP);
	IUSaveConfigSwitch(fp, &Switch4SP);

	return true;
}
