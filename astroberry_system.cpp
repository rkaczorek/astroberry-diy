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

		SysInfoTP.s = IPS_BUSY;
		IDSetText(&SysInfoTP, NULL);

		FILE* pipe;
		char buffer[128];

		//update uptime
		pipe = popen("uptime|awk -F, '{print $1}'|awk -Fup '{print $2}'|xargs", "r");
		fgets(buffer, 128, pipe);
		pclose(pipe);
		IUSaveText(&SysInfoT[1], buffer);

		//update load
		pipe = popen("uptime|awk -F, '{print $3\" /\"$4\" /\"$5}'|awk -F: '{print $2}'|xargs", "r");
		fgets(buffer, 128, pipe);
		pclose(pipe);
		IUSaveText(&SysInfoT[2], buffer);

		SysInfoTP.s = IPS_OK;
		IDSetText(&SysInfoTP, NULL);

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
    IUFillText(&SysInfoT[1],"UPTIME","Uptime (hh:mm)",NULL);
    IUFillText(&SysInfoT[2],"LOAD","Load (1 / 5 / 15 min.)",NULL);
    IUFillText(&SysInfoT[3],"HOSTNAME","Hostname",NULL);
    IUFillText(&SysInfoT[4],"LOCAL_IP","Local IP",NULL);
    IUFillText(&SysInfoT[5],"PUBLIC_IP","Public IP",NULL);
    IUFillTextVector(&SysInfoTP,SysInfoT,6,getDeviceName(),"SYSTEM_INFO","System Info",MAIN_CONTROL_TAB,IP_RO,60,IPS_IDLE);

	// Get basic system info
	FILE* pipe;
	char buffer[128];

	//update Hardware
	//https://www.raspberrypi.org/documentation/hardware/raspberrypi/revision-codes/README.md
	pipe = popen("cat /sys/firmware/devicetree/base/model", "r");
	fgets(buffer, 128, pipe);
	pclose(pipe);
	IUSaveText(&SysInfoT[0], buffer);
	//IUSaveText(&SysInfoT[0], getHardwareRev());

	//update Hostname
	pipe = popen("hostname", "r");
	fgets(buffer, 128, pipe);
	pclose(pipe);
	IUSaveText(&SysInfoT[3], buffer);

	//update Local IP
	pipe = popen("hostname -I|awk -F' '  '{print $1}'|xargs", "r");
	fgets(buffer, 128, pipe);
	pclose(pipe);
	IUSaveText(&SysInfoT[4], buffer);

	//update Public IP
	pipe = popen("wget -qO- http://ipecho.net/plain|xargs", "r");
	fgets(buffer, 128, pipe);
	pclose(pipe);
	IUSaveText(&SysInfoT[5], buffer);

	// Update client
	IDSetText(&SysInfoTP, NULL);

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
    }
    else
    {
	// We're disconnected
	deleteProperty(SysTimeTP.name);
	deleteProperty(SysInfoTP.name);
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
/*
const char * IndiAstroberrySystem::getHardwareRev()
{
	FILE* pipe;
	char buffer[128];

	pipe = popen("cat /proc/cpuinfo|grep Revision|awk -F: '{print $2}'|xargs", "r");
	fgets(buffer, 128, pipe);
	pclose(pipe);

	int hex = strtol(buffer,NULL, 16);

	switch(hex)
	{
		case 0x0007:
		case 0x0008:
		case 0x0009:
			return (char *) "Raspberry Pi Model A";
			break;
		case 0x0012:
		case 0x0015:
			return (char *) "Raspberry Pi Model A+";
			break;
		case 0x0002:
		case 0x0003:
			return (char *) "Raspberry Pi Model B Rev 1";
			break;
		case 0x0004:
		case 0x0005:
		case 0x0006:
		case 0x000d:
		case 0x000e:
		case 0x000f:
			return (char *) "Raspberry Pi Model B Rev 2";
			break;
		case 0x0010:
		case 0x0013:
		case 0x900032:
			return (char *) "Raspberry Pi Model B+";
			break;
		case 0x0011:
		case 0x0014:
			return (char *) "Raspberry Pi Compute Module";
			break;
		case 0xa01041:
		case 0xa21041:
			return (char *) "Raspberry Pi 2 Model B v1.1";
			break;
		case 0xa22042:
			return (char *) "Raspberry Pi 2 Model B v1.2";
			break;
		case 0x900092:
			return (char *) "Raspberry Pi Zero v1.2";
			break;
		case 0x900093:
			return (char *) "Raspberry Pi Zero v1.3";
			break;
		case 0x9000c1:
			return (char *) "Raspberry Pi Zero W";
			break;
		case 0xa02082:
		case 0xa22082:
			return (char *) "Raspberry Pi 3 Model B";
			break;
		case 0xa020d3:
			return (char *) "Raspberry Pi 3 Model B+";
			break;
		case 0xa03111:
		case 0xb03111:
		case 0xc03111:
			return (char *) "Raspberry Pi 4";
			break;
		default:
			return (char *) "unknown";
			break;
	}
}
*/
