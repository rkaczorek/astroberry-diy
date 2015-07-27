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
#include <iostream>
#include <sys/time.h>
#include <ctime>
#include <string>
#include <math.h>
#include <memory>
#include <libgpsmm.h>

#include "rpi_gps.h"


#define POLLMS 250

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
	// init GPS receiver
    //gpsmm gpsHandle("localhost", DEFAULT_GPSD_PORT);
	gpsHandle = new gpsmm("localhost", DEFAULT_GPSD_PORT);

	if (gpsHandle->stream(WATCH_ENABLE|WATCH_JSON) == NULL) {
		IDMessage(getDeviceName(), "Error connecting to GPSD service.");
		return false;
	}
	
	//// reset values
	GPSmodeN[0].value = 0;
	IDSetNumber(&GPSmodeNP, NULL);
	GPStimeT[0].text = NULL;			
	IDSetText(&GPStimeTP, NULL);
	LOCtimeT[0].text = NULL;			
	IDSetText(&LOCtimeTP, NULL);
	GPSlocationN[0].value = 0;
	GPSlocationN[1].value = 0;
	GPSlocationN[2].value = 0;
	IDSetNumber(&GPSlocationNP, NULL);

	// start timer
	timerid = SetTimer ( POLLMS );
	
    IDMessage(getDeviceName(), "Astroberry GPS connected successfully.");
    return true;
}
bool IndiRpigps::Disconnect()
{
	// start timer
	RemoveTimer (timerid);

	// disconnect GPS receiver
	gpsHandle->~gpsmm();

    IDMessage(getDeviceName(), "Astroberry GPS disconnected successfully.");
    return true;
}
void IndiRpigps::TimerHit()
{
	if(isConnected())
	{
		// update local time
		updateLocal();

		// update gps time and position each tick
		if ( counter > 1 )
		{	
			updateGPS();
			counter=0;
		}
		counter++;
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

    IUFillNumber(&GPSmodeN[0],"GPS_FIX","Fix Mode","%0.0f",0,5,1,0 );
    IUFillNumberVector(&GPSmodeNP,GPSmodeN,1,getDeviceName(),"GPS_MODE","GPS Status",MAIN_CONTROL_TAB,IP_RO,60,IPS_OK);

    IUFillText(&GPStimeT[0],"TIME","Time (UTC)","");
    IUFillTextVector(&GPStimeTP,GPStimeT,1,getDeviceName(),"GPS_TIME","GPS Time",MAIN_CONTROL_TAB,IP_RO,60,IPS_OK);

    IUFillText(&LOCtimeT[0],"LOCAL","System Time","");
    IUFillText(&LOCtimeT[1],"OFFSET","UTC Offset","");
    IUFillTextVector(&LOCtimeTP,LOCtimeT,2,getDeviceName(),"LOCAL_TIME","Local Time",MAIN_CONTROL_TAB,IP_RO,60,IPS_OK);

    IUFillNumber(&GPSlocationN[0],"GPSLAT","Lat (dd:mm:ss)","%010.6m",-90,90,0,0.0);
    IUFillNumber(&GPSlocationN[1],"GPSLONG","Lon (dd:mm:ss)","%010.6m",0,360,0,0.0 );
    IUFillNumber(&GPSlocationN[2],"GPSELEV","Elevation (m)","%g",-200,10000,0,0 );
    IUFillNumberVector(&GPSlocationNP,GPSlocationN,3,getDeviceName(),"GPS_COORD","GPS Location",MAIN_CONTROL_TAB,IP_RO,60,IPS_OK);

    IUFillSwitch(&GPSsetS[0], "GPSPOS", "Location", ISS_OFF);
    IUFillSwitch(&GPSsetS[1], "GPSTIME", "Time", ISS_OFF);
    IUFillSwitchVector(&GPSsetSP, GPSsetS, 2, getDeviceName(), "GPS_SYNC", "Set Site", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_OK);

	// local copy of snooped values

    IUFillText(&TimeT[0],"UTC","UTC Time","");
    IUFillText(&TimeT[1],"OFFSET","UTC Offset","");
    IUFillTextVector(&TimeTP,TimeT,2,"EQMod Mount","TIME_UTC","Time",SITE_TAB,IP_RW,60,IPS_OK);

    IUFillNumber(&LocationN[0],"LAT","Lat (dd:mm:ss)","%010.6m",-90,90,0,0.0);
    IUFillNumber(&LocationN[1],"LONG","Lon (dd:mm:ss)","%010.6m",0,360,0,0.0 );
    IUFillNumber(&LocationN[2],"ELEV","Elevation (m)","%g",-200,10000,0,0 );
    IUFillNumberVector(&LocationNP,LocationN,3,"EQMod Mount","GEOGRAPHIC_COORD","Location",SITE_TAB,IP_RW,60,IPS_OK);

	counter = 0;

    return true;
}
bool IndiRpigps::updateProperties()
{
    // Call parent update properties first
    INDI::DefaultDevice::updateProperties();

    if (isConnected())
    {
        defineNumber(&GPSmodeNP);
        defineNumber(&GPSlocationNP);
        defineText(&GPStimeTP);
        defineText(&LOCtimeTP);
		defineSwitch(&GPSsetSP);
        //defineNumber(&LocationNP);
        //defineText(&TimeTP);
        
		IDSnoopDevice("EQMod Mount", "Scope Location");
		IDSnoopDevice("EQMod Mount", "UTC");        
    }
    else
    {
		// We're disconnected
        deleteProperty(GPSmodeNP.name);
        deleteProperty(GPSlocationNP.name);
		deleteProperty(LOCtimeTP.name);
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
bool IndiRpigps::updateLocal()
{
	//LOCtimeTP.s = IPS_BUSY;
	//IDSetText(&LOCtimeTP, NULL);

	// init time vars
	time_t rawtime;
	struct tm * ltm_timeinfo;
	char local_time[32]; // format 2015-01-28T20:28:02
	char utc_offset[1];
	double offset;

	// get local time
	time(&rawtime);
	ltm_timeinfo = localtime (&rawtime);
	strftime(local_time, sizeof(local_time), "%Y-%m-%dT%H:%M:%S", ltm_timeinfo);

	// get utc_offset
	offset = ltm_timeinfo->tm_gmtoff / 3600;

	// adjust offset for DST
	if (!ltm_timeinfo->tm_isdst)
		offset -= 1;
	
	// convert offset to string
	sprintf(utc_offset,"%0.0f", offset);
	
	// update INDI values
	LOCtimeT[0].text = local_time;
	LOCtimeT[1].text = utc_offset;
	//LOCtimeTP.s = IPS_OK;
	IDSetText(&LOCtimeTP, NULL);
	
	return true;
}
bool IndiRpigps::updateGPS()
{
	// init time vars
	time_t rawtime;
	struct tm * utc_timeinfo;
	char utc_time[32]; // format 2015-01-28T20:28:02
	struct gps_data_t* gpsData;
	
//	if ( !gpsHandle->waiting(50000000) )
//		return false;
		
	if ((gpsData = gpsHandle->read()) == NULL)
		return false;

	if ( gpsData->fix.mode < 1 )
		return false;

	// detect 3d fix
	if (GPSmodeN[0].value < gpsData->fix.mode && gpsData->fix.mode == 3)
		IDMessage(getDeviceName(), "Astroberry GPS 3D FIX obtained.");
		
	//// update gps status
	GPSmodeN[0].value = gpsData->fix.mode;
	//GPSmodeNP.s = IPS_OK;
	IDSetNumber(&GPSmodeNP, NULL);

	// update gps time
	if ( GPStimeTP.s == IPS_OK && gpsData->fix.mode >= 1 )
	{
		GPStimeTP.s = IPS_BUSY;

		// get utc_time from gps
		rawtime = gpsData->fix.time;
		utc_timeinfo = gmtime (&rawtime);
		strftime(utc_time, 20, "%Y-%m-%dT%H:%M:%S", utc_timeinfo);

		// update INDI values
		GPStimeT[0].text = utc_time;			
		GPStimeTP.s = IPS_OK;
		IDSetText(&GPStimeTP, NULL);
	}
	
	// update gps location
	if ( GPSlocationNP.s == IPS_OK  && gpsData->fix.mode >= 3 )
	{
		GPSlocationNP.s = IPS_BUSY;
		IDSetNumber(&GPSlocationNP, NULL);
				  
		// update INDI values
		GPSlocationN[0].value = gpsData->fix.latitude;
		GPSlocationN[1].value = gpsData->fix.longitude;
		GPSlocationN[2].value = gpsData->fix.altitude;

		GPSlocationNP.s = IPS_OK;
		IDSetNumber(&GPSlocationNP, NULL);
	}
	
	return true;
}
bool IndiRpigps::updateLocation()
{
	if ( GPSmodeN[0].value < 3 )
	{
		IDMessage(getDeviceName(), "No reliable location data available yet.");
		return false;
	}
		
	LocationNP.s = IPS_BUSY;
	IDSetNumber(&LocationNP, NULL);

	// update INDI values
	LocationN[0].value = GPSlocationN[0].value;
	LocationN[1].value = GPSlocationN[1].value;
	LocationN[2].value = GPSlocationN[2].value;
	LocationNP.s = IPS_OK;
	IDSetNumber(&LocationNP, "Location data received from Astroberry GPS - Lat: %02.3f Lon: %02.3f Elev: %02.3f", GPSlocationN[0].value, GPSlocationN[1].value, GPSlocationN[2].value);
	IDMessage(getDeviceName(), "Location data send to the telescope");

	return true;
}

bool IndiRpigps::updateTime()
{
	if ( GPSmodeN[0].value < 3 )
	{
		IDMessage(getDeviceName(), "No reliable time data available yet.");
		return false;
	}

	// init time vars
	time_t rawtime;
	struct tm * utc_timeinfo;
	char utc_time[32]; // format 2015-01-28T20:28:02
	struct gps_data_t* gpsData;

	if ((gpsData = gpsHandle->read()) == NULL)
		return false;

	if ( gpsData->fix.mode < 3 )
		return false;
	
//	if ( !gpsHandle->waiting(50000000) )
//		return false;
	
	// update gps time
	TimeTP.s = IPS_BUSY;
	IDSetText(&TimeTP, NULL);

	// get utc_time from gps
	rawtime = gpsData->fix.time;
	utc_timeinfo = gmtime (&rawtime);
	strftime(utc_time, 20, "%Y-%m-%dT%H:%M:%S", utc_timeinfo);


	// update INDI values
	TimeT[0].text = utc_time;
	TimeT[1].text = LOCtimeT[1].text;
	//IUSaveText(IUFindText(&TimeTP, "UTC"), utc_time);
	TimeTP.s = IPS_OK;
	IDSetText(&TimeTP, "Time data received from Astroberry GPS - Time: %s Offset: %s", TimeT[0].text, TimeT[1].text);
	IDMessage(getDeviceName(), "Time data send to the telescope");	
	return true;
}
