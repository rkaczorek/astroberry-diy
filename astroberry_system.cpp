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

#include "astroberry_system.h"

#include <gpiod.h>

// We declare an auto pointer to IndiAstroberrySystem
std::unique_ptr<IndiAstroberrySystem> indiAstroberrySystem(new IndiAstroberrySystem());


void ISPoll(void *p);

void ISInit()
{
	static int isInit = 0;

	if (isInit == 1)
		return;
	if(indiAstroberrySystem.get() == 0)
	{
		isInit = 1;
		indiAstroberrySystem.reset(new IndiAstroberrySystem());
	}
}

void ISGetProperties(const char *dev)
{
        ISInit();
        indiAstroberrySystem->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
        ISInit();
        indiAstroberrySystem->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(	const char *dev, const char *name, char *texts[], char *names[], int num)
{
        ISInit();
        indiAstroberrySystem->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
        ISInit();
        indiAstroberrySystem->ISNewNumber(dev, name, values, names, num);
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
	indiAstroberrySystem->ISSnoopDevice(root);
}

IndiAstroberrySystem::IndiAstroberrySystem()
{
	setVersion(VERSION_MAJOR,VERSION_MINOR);
}

IndiAstroberrySystem::~IndiAstroberrySystem()
{
}

bool IndiAstroberrySystem::Connect()
{
	SetTimer(1000);
	IDMessage(getDeviceName(), "Astroberry System connected successfully.");

	// Get basic system info
	FILE* pipe;
	char buffer[128];

	//update Hardware
	//https://www.raspberrypi.org/documentation/hardware/raspberrypi/revision-codes/README.md
	pipe = popen("cat /sys/firmware/devicetree/base/model", "r");
	fgets(buffer, 128, pipe);
	pclose(pipe);
	IUSaveText(&SysInfoT[0], buffer);

	//update CPU temp
	pipe = popen("echo $(($(cat /sys/class/thermal/thermal_zone0/temp)/1000))", "r");
	fgets(buffer, 128, pipe);
	pclose(pipe);
	IUSaveText(&SysInfoT[1], buffer);

	//update uptime
	pipe = popen("uptime|awk -F, '{print $1}'|awk -Fup '{print $2}'|xargs", "r");
	fgets(buffer, 128, pipe);
	pclose(pipe);
	IUSaveText(&SysInfoT[2], buffer);

	//update load
	pipe = popen("uptime|awk -F, '{print $3\" /\"$4\" /\"$5}'|awk -F: '{print $2}'|xargs", "r");
	fgets(buffer, 128, pipe);
	pclose(pipe);
	IUSaveText(&SysInfoT[3], buffer);

	//update Hostname
	pipe = popen("hostname", "r");
	fgets(buffer, 128, pipe);
	pclose(pipe);
	IUSaveText(&SysInfoT[4], buffer);

	//update Local IP
	pipe = popen("hostname -I|awk -F' '  '{print $1}'|xargs", "r");
	fgets(buffer, 128, pipe);
	pclose(pipe);
	IUSaveText(&SysInfoT[5], buffer);

	//update Public IP
	pipe = popen("wget -qO- http://ipecho.net/plain|xargs", "r");
	fgets(buffer, 128, pipe);
	pclose(pipe);
	IUSaveText(&SysInfoT[6], buffer);

	// Update client
	IDSetText(&SysInfoTP, NULL);

	return true;
}
bool IndiAstroberrySystem::Disconnect()
{
	IDMessage(getDeviceName(), "Astroberry System disconnected successfully.");
	return true;
}
void IndiAstroberrySystem::TimerHit()
{
	if(isConnected())
	{
		// update time
		struct tm *local_timeinfo;
		static char ts[32];
		time_t rawtime;
		time(&rawtime);
		local_timeinfo = localtime (&rawtime);
		strftime(ts, 20, "%Y-%m-%dT%H:%M:%S", local_timeinfo);
		IUSaveText(&SysTimeT[0], ts);
		snprintf(ts, sizeof(ts), "%4.2f", (local_timeinfo->tm_gmtoff/3600.0));
		IUSaveText(&SysTimeT[1], ts);
		SysTimeTP.s = IPS_OK;
		IDSetText(&SysTimeTP, NULL);

		if (polling++ > 59)
		{
			FILE* pipe;
			char buffer[128];

			SysInfoTP.s = IPS_BUSY;
			IDSetText(&SysInfoTP, NULL);

			//update CPU temp
			pipe = popen("echo $(($(cat /sys/class/thermal/thermal_zone0/temp)/1000))", "r");
			fgets(buffer, 128, pipe);
			pclose(pipe);
			IUSaveText(&SysInfoT[1], buffer);

			//update uptime
			pipe = popen("uptime|awk -F, '{print $1}'|awk -Fup '{print $2}'|xargs", "r");
			fgets(buffer, 128, pipe);
			pclose(pipe);
			IUSaveText(&SysInfoT[2], buffer);

			//update load
			pipe = popen("uptime|awk -F, '{print $3\" /\"$4\" /\"$5}'|awk -F: '{print $2}'|xargs", "r");
			fgets(buffer, 128, pipe);
			pclose(pipe);
			IUSaveText(&SysInfoT[3], buffer);

			SysInfoTP.s = IPS_OK;
			IDSetText(&SysInfoTP, NULL);

			polling = 0;
		}

		SetTimer(1000);
	}
}

const char * IndiAstroberrySystem::getDefaultName()
{
        return (char *)"Astroberry System";
}

bool IndiAstroberrySystem::initProperties()
{
	// We init parent properties first
	INDI::DefaultDevice::initProperties();

	IUFillText(&SysTimeT[0],"LOCAL_TIME","Local Time",NULL);
	IUFillText(&SysTimeT[1],"UTC_OFFSET","UTC Offset",NULL);
	IUFillTextVector(&SysTimeTP,SysTimeT,2,getDeviceName(),"SYSTEM_TIME","System Time",MAIN_CONTROL_TAB,IP_RO,60,IPS_IDLE);

	IUFillText(&SysInfoT[0],"HARDWARE","Hardware",NULL);
	IUFillText(&SysInfoT[1],"CPU TEMP","CPU Temp (°C)",NULL);
	IUFillText(&SysInfoT[2],"UPTIME","Uptime (hh:mm)",NULL);
	IUFillText(&SysInfoT[3],"LOAD","Load (1 / 5 / 15 min.)",NULL);
	IUFillText(&SysInfoT[4],"HOSTNAME","Hostname",NULL);
	IUFillText(&SysInfoT[5],"LOCAL_IP","Local IP",NULL);
	IUFillText(&SysInfoT[6],"PUBLIC_IP","Public IP",NULL);
	IUFillTextVector(&SysInfoTP,SysInfoT,7,getDeviceName(),"SYSTEM_INFO","System Info",MAIN_CONTROL_TAB,IP_RO,60,IPS_IDLE);

	IUFillSwitch(&SysControlS[0], "SYSCTRL_REBOOT", "Reboot", ISS_OFF);
	IUFillSwitch(&SysControlS[1], "SYSCTRL_SHUTDOWN", "Shutdown", ISS_OFF);
	IUFillSwitchVector(&SysControlSP, SysControlS, 2, getDeviceName(), "SYSCTRL", "System Ctrl", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

	IUFillSwitch(&SysOpConfirmS[0], "SYSOPCONFIRM_CONFIRM", "Yes", ISS_OFF);
	IUFillSwitch(&SysOpConfirmS[1], "SYSOPCONFIRM_CANCEL", "No", ISS_OFF);
	IUFillSwitchVector(&SysOpConfirmSP, SysOpConfirmS, 2, getDeviceName(), "SYSOPCONFIRM", "Continue?", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

	return true;
}

bool IndiAstroberrySystem::updateProperties()
{
	// Call parent update properties first
	INDI::DefaultDevice::updateProperties();

	if (isConnected())
	{
		defineText(&SysTimeTP);
		defineText(&SysInfoTP);
		defineSwitch(&SysControlSP);
	}
	else
	{
		// We're disconnected
		deleteProperty(SysTimeTP.name);
		deleteProperty(SysInfoTP.name);
		deleteProperty(SysControlSP.name);
	}
	return true;
}

void IndiAstroberrySystem::ISGetProperties(const char *dev)
{
	INDI::DefaultDevice::ISGetProperties(dev);
}

bool IndiAstroberrySystem::ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n)
{
	return INDI::DefaultDevice::ISNewNumber(dev,name,values,names,n);
}

bool IndiAstroberrySystem::ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n)
{
	// first we check if it's for our device
	if (!strcmp(dev, getDeviceName()))
	{
		// handle system control
		if (!strcmp(name, SysControlSP.name))
		{
			IUUpdateSwitch(&SysControlSP, states, names, n);

			if ( SysControlS[0].s == ISS_ON )
			{
				DEBUG(INDI::Logger::DBG_SESSION, "Astroberry device is set to REBOOT. Confirm or Cancel operation.");
				SysControlSP.s = IPS_BUSY;
				IDSetSwitch(&SysControlSP, NULL);
				
				// confirm switch
				defineSwitch(&SysOpConfirmSP);

				return true;
			}
			if ( SysControlS[1].s == ISS_ON )
			{
				DEBUG(INDI::Logger::DBG_SESSION, "Astroberry device is set to SHUT DOWN. Confirm or Cancel operation.");
				SysControlSP.s = IPS_BUSY;
				IDSetSwitch(&SysControlSP, NULL);

				// confirm switch
				defineSwitch(&SysOpConfirmSP);

				return true;
			}
		}

		// handle system control confirmation
		if (!strcmp(name, SysOpConfirmSP.name))
		{
			IUUpdateSwitch(&SysOpConfirmSP, states, names, n);

			if ( SysOpConfirmS[0].s == ISS_ON )
			{
				SysOpConfirmSP.s = IPS_IDLE;
				IDSetSwitch(&SysOpConfirmSP, NULL);
				SysOpConfirmS[0].s = ISS_OFF;
				IDSetSwitch(&SysOpConfirmSP, NULL);

				// execute system operation
				if (SysControlS[0].s == ISS_ON)
				{
					DEBUG(INDI::Logger::DBG_SESSION, "System operation confirmed. System is going to REBOOT now");
					FILE* pipe;
					char buffer[512];
					pipe = popen("sudo reboot", "r");
					fgets(buffer, 512, pipe);
					pclose(pipe);
					DEBUGF(INDI::Logger::DBG_SESSION, "System output: %s", buffer);
				}
				if (SysControlS[1].s == ISS_ON)
				{
					DEBUG(INDI::Logger::DBG_SESSION, "System operation confirmed. System is going to SHUT DOWN now");
					FILE* pipe;
					char buffer[512];
					pipe = popen("sudo poweroff", "r");
					fgets(buffer, 512, pipe);
					pclose(pipe);
					DEBUGF(INDI::Logger::DBG_SESSION, "System output: %s", buffer);
				}

				// reset system control buttons
				SysControlSP.s = IPS_IDLE;
				IDSetSwitch(&SysControlSP, NULL);
				SysControlS[0].s = ISS_OFF;
				SysControlS[1].s = ISS_OFF;
				IDSetSwitch(&SysControlSP, NULL);

				deleteProperty(SysOpConfirmSP.name);
				return true;
			}

			if ( SysOpConfirmS[1].s == ISS_ON )
			{
				DEBUG(INDI::Logger::DBG_SESSION, "System operation canceled.");
				SysOpConfirmSP.s = IPS_IDLE;
				IDSetSwitch(&SysOpConfirmSP, NULL);
				SysOpConfirmS[1].s = ISS_OFF;
				IDSetSwitch(&SysOpConfirmSP, NULL);

				// reset system control buttons
				SysControlSP.s = IPS_IDLE;
				IDSetSwitch(&SysControlSP, NULL);
				SysControlS[0].s = ISS_OFF;
				SysControlS[1].s = ISS_OFF;
				IDSetSwitch(&SysControlSP, NULL);

				deleteProperty(SysOpConfirmSP.name);
				return true;
			}
		}
	}
	return INDI::DefaultDevice::ISNewSwitch (dev, name, states, names, n);
}

bool IndiAstroberrySystem::ISNewText (const char *dev, const char *name, char *texts[], char *names[], int n)
{
	return INDI::DefaultDevice::ISNewText (dev, name, texts, names, n);
}

bool IndiAstroberrySystem::ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n)
{
	return INDI::DefaultDevice::ISNewBLOB (dev, name, sizes, blobsizes, blobs, formats, names, n);
}

bool IndiAstroberrySystem::ISSnoopDevice(XMLEle *root)
{
	return INDI::DefaultDevice::ISSnoopDevice(root);
}
