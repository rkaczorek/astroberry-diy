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

#include "rpi_gps.h"


#define POLLMS 1000

// We declare an auto pointer to IndiRpigps
std::auto_ptr<IndiRpigps> indiRpigps(0);

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
	INDI_UNUSED(root);
}

IndiRpigps::IndiRpigps()
{
	setVersion(2,0);
}

IndiRpigps::~IndiRpigps()
{
}

bool IndiRpigps::Connect()
{
	// init GPS receiver
	gpsHandle = new gpsmm("localhost", DEFAULT_GPSD_PORT);

	if (gpsHandle->stream(WATCH_ENABLE|WATCH_JSON) == NULL) {
		IDMessage(getDeviceName(), "Error connecting to GPSD service.");
		return false;
	}
	
	//// reset values
	GPSmodeN[0].value = 0;
	IDSetNumber(&GPSmodeNP, NULL);
	PolarisHN[0].value = 0;
	IDSetNumber(&PolarisHNP, NULL);

	// start timer
	SetTimer( POLLMS );
	
    IDMessage(getDeviceName(), "Astroberry GPS connected successfully.");
    return true;
}

bool IndiRpigps::Disconnect()
{
	// disconnect GPS receiver
	gpsHandle->~gpsmm();

    IDMessage(getDeviceName(), "Astroberry GPS disconnected successfully.");
    return true;
}

void IndiRpigps::TimerHit()
{
	if(isConnected())
	{
		// update gps data
		updateGPS();
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
    INDI::GPS::initProperties();

    IUFillNumber(&GPSmodeN[0],"GPS_FIX","Fix Mode","%0.0f",0,5,1,0 );
    IUFillNumberVector(&GPSmodeNP,GPSmodeN,1,getDeviceName(),"GPS_MODE","GPS Status",MAIN_CONTROL_TAB,IP_RO,60,IPS_OK);

    IUFillNumber(&PolarisHN[0],"HOUR_ANGLE","Polaris Hour Angle","%010.6m",0,23,0,0.0);
    IUFillNumberVector(&PolarisHNP,PolarisHN,1,getDeviceName(),"POLARIS_HA","Polaris HA",MAIN_CONTROL_TAB,IP_RO,60,IPS_OK);

    IUFillSwitch(&GPSupdateS[0], "GPS_AUTO", "Enable", ISS_ON);
    IUFillSwitchVector(&GPSupdateSP, GPSupdateS, 1, getDeviceName(), "GPS_UPDATE", "Auto refresh", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_OK);
	

    return true;
}

bool IndiRpigps::updateProperties()
{
    // Call parent update properties first
    INDI::GPS::updateProperties();

    if (isConnected())
    {
		deleteProperty(RefreshSP.name);
        defineNumber(&GPSmodeNP);
        defineNumber(&PolarisHNP);
        defineSwitch(&GPSupdateSP);
        //defineSwitch(&RefreshSP);
    }
    else
    {
		// We're disconnected
        deleteProperty(GPSmodeNP.name);
        deleteProperty(PolarisHNP.name);
        deleteProperty(GPSupdateSP.name);
    }
    return true;
}

bool IndiRpigps::ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n)
{
	// first we check if it's for our device
    if (!strcmp(dev, getDeviceName()))
    {
		// handle sync with scope
		if(!strcmp(name, GPSupdateSP.name))
		{			
			if ( GPSupdateS[0].s == ISS_ON )
			{
				IDMessage(getDeviceName(), "Astroberry GPS refresh disabled.");
				GPSupdateS[0].s = ISS_OFF;
				GPSupdateSP.s = IPS_BUSY;
			} else {
				IDMessage(getDeviceName(), "Astroberry GPS refresh enabled.");
				GPSupdateS[0].s = ISS_ON;
				GPSupdateSP.s = IPS_OK;
			}
			IDSetSwitch(&GPSupdateSP, NULL);
		}
	}
	return INDI::GPS::ISNewSwitch (dev, name, states, names, n);
}


bool IndiRpigps::updateGPS()
{
//	if ( !gpsHandle->waiting(50000000) )
//		return false;

	if ((gpsData = gpsHandle->read()) == NULL || gpsData->fix.mode < 1)
	{
		GPSmodeNP.s = IPS_BUSY;
		IDSetNumber(&GPSmodeNP, NULL);
		return false;
	}

	// detect 3d fix
	if (GPSmodeN[0].value < gpsData->fix.mode && gpsData->fix.mode == 3)
		IDMessage(getDeviceName(), "Astroberry GPS 3D FIX obtained.");

	// detect 3d fix lost
	if (GPSmodeN[0].value > gpsData->fix.mode && gpsData->fix.mode < 3)
		IDMessage(getDeviceName(), "Astroberry GPS 3D FIX lost.");
		
	//// update gps status
	GPSmodeN[0].value = gpsData->fix.mode;
	GPSmodeNP.s = IPS_OK;
	IDSetNumber(&GPSmodeNP, NULL);

	// skip if auto refresh is disabled
	if ( GPSupdateS[0].s == ISS_OFF )
		return false;
	
	// update gps time
	struct tm *utc_timeinfo, *local_timeinfo;
	static char ts[32];
	time_t rawtime;

	if ( gpsData->fix.mode >= 1 )
	{
		TimeTP.s = IPS_BUSY;
		IDSetText(&TimeTP, NULL);

		// get utc_time from gps
		rawtime = gpsData->fix.time;
		utc_timeinfo = gmtime (&rawtime);
		strftime(ts, 20, "%Y-%m-%dT%H:%M:%S", utc_timeinfo);
		IUSaveText(&TimeT[0], ts);

		// get utc offset
		local_timeinfo = localtime (&rawtime);
		snprintf(ts, sizeof(ts), "%4.2f", (local_timeinfo->tm_gmtoff/3600.0));
		IUSaveText(&TimeT[1], ts);
		
		TimeTP.s = IPS_OK;		
		IDSetText(&TimeTP, NULL);
	}
	
	// update gps location
	if ( gpsData->fix.mode >= 3 )
	{
		LocationNP.s = IPS_BUSY;
		IDSetNumber(&LocationNP, NULL);
				  
		// update INDI values
		LocationN[0].value = gpsData->fix.latitude;
		LocationN[1].value = gpsData->fix.longitude;
		LocationN[2].value = gpsData->fix.altitude;

		LocationNP.s = IPS_OK;
		IDSetNumber(&LocationNP, NULL);

		// calculate Polaris HA
		double jd, lst;
		char* siderealtime;
		
		// polaris location - RA 02h 31m 47s DEC 89° 15' 50'' (J2000)
		jd = ln_get_julian_from_sys();
		lst=ln_get_apparent_sidereal_time(jd);

		// Local Hour Angle = Local Sidereal Time - Polaris Right Ascension
		double polarislsrt = lst - 2.529722222 + (gpsData->fix.longitude / 15.0);	

		PolarisHN[0].value = polarislsrt;
		IDSetNumber(&PolarisHNP, NULL);

		// Polaris transit time ----------------- TODO: add text field, set values below
	/*
		struct ln_lnlat_posn observer;
		struct ln_equ_posn polaris;
		struct ln_rst_time rst;
		struct ln_zonedate transit;
		
		jd = ln_get_julian_from_sys();

		// observers location
		observer.lat = gpsData->fix.latitude;
		observer.lng = gpsData->fix.longitude;

		// polaris location - RA 02h 31m 47s DEC 89° 15' 50'' (J2000)
		polaris.ra = 2.529722222;
		polaris.dec = 89.263888889;
		
		// check transit time only		
		ln_get_object_rst (jd, &observer, &polaris, &rst);
		ln_get_local_date(rst.transit, &transit);
		IDMessage(getDeviceName(), "Polaris Transit Time %d:%d:%f", transit.hours, transit.minutes, transit.seconds);
	*/	
	}

    return true;
}
