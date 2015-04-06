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
#include <sys/time.h>
#include <ctime>
#include <string>
#include <math.h>
#include <memory>
#include <gps.h>

#include "rpi_gps.h"


#define POLLMS 1000

// We declare an auto pointer to IndiRpigps
std::auto_ptr<IndiRpigps> indiRpigps(0);

void ISPoll(void *p);
void ISInit()
{
   static int isInit =0;

   if (isInit == 1)
       return;

    isInit = 1;
    if(indiRpigps.get() == 0) indiRpigps.reset(new IndiRpigps());
    //IEAddTimer(POLLMS, ISPoll, NULL);

}
void ISGetProperties(const char *dev)
{
	ISInit();
	indiRpigps->ISGetProperties(dev);
}
void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
	ISInit();
	indiRpigps->ISNewSwitch(dev, name, states, names, num);
}
void ISNewText(	const char *dev, const char *name, char *texts[], char *names[], int num)
{
	ISInit();
	indiRpigps->ISNewText(dev, name, texts, names, num);
}
void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
	ISInit();
	indiRpigps->ISNewNumber(dev, name, values, names, num);
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
    indiRpigps->ISSnoopDevice(root);
}
IndiRpigps::IndiRpigps()
{
}
IndiRpigps::~IndiRpigps()
{
}
bool IndiRpigps::Connect()
{
    IDMessage(getDeviceName(), "Astroberry GPS connected successfully.");
    return true;
}
bool IndiRpigps::Disconnect()
{    
    IDMessage(getDeviceName(), "Astroberry GPS disconnected successfully.");
    return true;
}
void IndiRpigps::TimerHit()
{
	if(isConnected())
	{
		updateGPSTime();
		counter++;

		// update GPS every 10th time
		if (counter == 10)
		{
			counter = 0;
			updateGPSLocation();
		}
		SetTimer( POLLMS );
    }
}
const char * IndiRpigps::getDefaultName()
{
	return (char *)"Astroberry GPS";
}
bool IndiRpigps::initProperties()
{
    // We init parent properties first
    INDI::DefaultDevice::initProperties();

    IUFillText(&GPStimeT[0],"UTC","UTC Time","");
    IUFillText(&GPStimeT[1],"OFFSET","UTC Offset","");
    IUFillTextVector(&GPStimeTP,GPStimeT,2,getDeviceName(),"GPS_TIME","GPS Time",MAIN_CONTROL_TAB,IP_RO,60,IPS_OK);

    IUFillNumber(&GPSlocationN[0],"GPSLAT","Lat (dd:mm:ss)","%010.6m",-90,90,0,0.0);
    IUFillNumber(&GPSlocationN[1],"GPSLONG","Lon (dd:mm:ss)","%010.6m",0,360,0,0.0 );
    IUFillNumber(&GPSlocationN[2],"GPSELEV","Elevation (m)","%g",-200,10000,0,0 );
    IUFillNumberVector(&GPSlocationNP,GPSlocationN,3,getDeviceName(),"GPS_COORD","GPS Location",MAIN_CONTROL_TAB,IP_RO,60,IPS_OK);

    IUFillSwitch(&GPSsetS[0], "GPSPOS", "Set Location", ISS_OFF);
    IUFillSwitch(&GPSsetS[1], "GPSTIME", "Set Time", ISS_OFF);
    IUFillSwitchVector(&GPSsetSP, GPSsetS, 2, getDeviceName(), "GPS_SYNC", "Telescope", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_OK);

	// local copy of snooped values

    IUFillText(&TimeT[0],"UTC","UTC Time","");
    IUFillText(&TimeT[1],"OFFSET","UTC Offset","");
    IUFillTextVector(&TimeTP,TimeT,2,"EQMod Mount","TIME_UTC","UTC",SITE_TAB,IP_RW,60,IPS_OK);

    IUFillNumber(&LocationN[0],"LAT","Lat (dd:mm:ss)","%010.6m",-90,90,0,0.0);
    IUFillNumber(&LocationN[1],"LONG","Lon (dd:mm:ss)","%010.6m",0,360,0,0.0 );
    IUFillNumber(&LocationN[2],"ELEV","Elevation (m)","%g",-200,10000,0,0 );
    IUFillNumberVector(&LocationNP,LocationN,3,"EQMod Mount","GEOGRAPHIC_COORD","Scope Location",SITE_TAB,IP_RW,60,IPS_OK);

    return true;
}
bool IndiRpigps::updateProperties()
{
    // Call parent update properties first
    INDI::DefaultDevice::updateProperties();

    if (isConnected())
    {
        defineText(&GPStimeTP);
        defineNumber(&GPSlocationNP);
		defineSwitch(&GPSsetSP);		
        //defineNumber(&LocationNP);
        //defineText(&TimeTP);
        
		IDSnoopDevice("EQMod Mount", "Scope Location");
		IDSnoopDevice("EQMod Mount", "UTC");        

		// init GPS update counter
		counter = 0;

		// start timer
		SetTimer ( POLLMS );
		
		// init GPS receiver
		//gps_init();
    }
    else
    {
		// We're disconnected		
        deleteProperty(GPSlocationNP.name);
		deleteProperty(GPStimeTP.name);
		deleteProperty(GPSsetSP.name);
        //deleteProperty(LocationNP.name);
		//deleteProperty(TimeTP.name);
		
		// close GPS receiver
		//gps_off();
    }
    return true;
}
void IndiRpigps::ISGetProperties(const char *dev)
{
    INDI::DefaultDevice::ISGetProperties(dev);

    /* Add debug controls so we may debug driver if necessary */
    // addDebugControl();
}
bool IndiRpigps::ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n)
{
	return INDI::DefaultDevice::ISNewNumber(dev,name,values,names,n);
}
bool IndiRpigps::ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n)
{
	// first we check if it's for our device
    if (!strcmp(dev, getDeviceName()))
    {
		// handle sync with scope
		if(!strcmp(name, GPSsetSP.name))
		{		
			IUUpdateSwitch(&GPSsetSP, states, names, n);

			// Location snooping
			if ( GPSsetS[0].s == ISS_ON )
			{
				updateLocation();
				
				GPSsetS[0].s = ISS_OFF;
				IDSetSwitch(&GPSsetSP, NULL);
			}

			// Time snooping
			if ( GPSsetS[1].s == ISS_ON )
			{
				updateTime();
				
				GPSsetS[1].s = ISS_OFF;
				IDSetSwitch(&GPSsetSP, NULL);
			}
			return true;
		}
	}	
	return INDI::DefaultDevice::ISNewSwitch (dev, name, states, names, n);
}
bool IndiRpigps::ISNewText (const char *dev, const char *name, char *texts[], char *names[], int n)
{
	return INDI::DefaultDevice::ISNewText (dev, name, texts, names, n);
}
bool IndiRpigps::ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n)
{
	return INDI::DefaultDevice::ISNewBLOB (dev, name, sizes, blobsizes, blobs, formats, names, n);
}
bool IndiRpigps::ISSnoopDevice(XMLEle *root)
{
    return INDI::DefaultDevice::ISSnoopDevice(root);
}
bool IndiRpigps::updateGPSLocation()
{
	gps_init();

	GPSlocationNP.s = IPS_BUSY;
	IDSetNumber(&GPSlocationNP, NULL);
	
	gps_location(&data);

	GPSlocationN[0].value = data.latitude;
	GPSlocationN[1].value = data.longitude;
	GPSlocationN[2].value = data.altitude;

	GPSlocationNP.s = IPS_OK;
	IDSetNumber(&GPSlocationNP, NULL);

	gps_off();

	return true;
}
bool IndiRpigps::updateGPSTime()
{
	GPStimeTP.s = IPS_BUSY;
	IDSetText(&GPStimeTP, NULL);

	// init time
	time (&rawtime);
	ltm_timeinfo = localtime (&rawtime);

	// get utc_offset
	offset = ltm_timeinfo->tm_gmtoff / 3600;

	// adjust offset for DST
	if (ltm_timeinfo->tm_isdst)
		offset -= 1;
	
	// convert offset to string
	sprintf(utc_offset,"%0.0f", offset);

	// get utc_time
	time (&rawtime);
	utc_timeinfo = gmtime (&rawtime);
	strftime(utc_time, 20, "%Y-%m-%dT%H:%M:%S", utc_timeinfo);
	
	// update INDI values
	GPStimeT[0].text = utc_time;
	GPStimeT[1].text = utc_offset;
	
	GPStimeTP.s = IPS_OK;
	IDSetText(&GPStimeTP, NULL);

	return true;
}
bool IndiRpigps::updateLocation()
{
	// update INDI values
	LocationN[0].value = GPSlocationN[0].value;
	LocationN[1].value = GPSlocationN[1].value;
	LocationN[2].value = GPSlocationN[2].value;
	IDSetNumber(&LocationNP, "Location data received from Astroberry GPS - Lat: %02.3f Lon: %02.3f Elev: %02.3f", GPSlocationN[0].value, GPSlocationN[1].value, GPSlocationN[2].value);
	IDMessage(getDeviceName(), "Location data send to the telescope - Lat: %02.3f Lon: %02.3f Elev: %02.3f", GPSlocationN[0].value, GPSlocationN[1].value, GPSlocationN[2].value);

	return true;
}

bool IndiRpigps::updateTime()
{
	TimeT[0].text = GPStimeT[0].text;
	TimeT[1].text = GPStimeT[1].text;
	IDSetText(&TimeTP, "Time data received from Astroberry GPS - Time: %s Offset: %s", TimeT[0].text, TimeT[1].text);
	IDMessage(getDeviceName(), "Time data send to the telescope - Time: %s Offset: %s", TimeT[0].text, TimeT[1].text);	
	return true;
}
