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

#ifndef RPIGPS_H
#define RPIGPS_H

#include <defaultdevice.h>
#include <indicom.h>

class IndiRpigps : public INDI::DefaultDevice
{
protected:
private:
	time_t rawtime;
	struct tm * utc_timeinfo;
	struct tm * ltm_timeinfo;
	char utc_time[20]; // format 2015-01-28T20:28:02
	char utc_offset[1];
	double offset;
	loc_t data;
public:
	IText GPStimeT[2];
	ITextVectorProperty GPStimeTP;

	INumber GPSlocationN[3];
	INumberVectorProperty GPSlocationNP;

	ISwitch GPSsetS[2];
	ISwitchVectorProperty GPSsetSP;

	IText TimeT[2];
	ITextVectorProperty TimeTP;
	
	INumber LocationN[3];	
	INumberVectorProperty LocationNP;

    IndiRpigps();
	virtual ~IndiRpigps();

	virtual const char *getDefaultName();

	virtual void TimerHit();
	virtual bool Connect();
	virtual bool Disconnect();
	virtual bool initProperties();
	virtual bool updateProperties();
	virtual void ISGetProperties(const char *dev);
	virtual bool ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n);
	virtual bool ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n);
	virtual bool ISNewText (const char *dev, const char *name, char *texts[], char *names[], int n);
	virtual bool ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n);
	virtual bool ISSnoopDevice(XMLEle *root);

	virtual bool updateGPSLocation();
	virtual bool updateGPSTime();
	virtual bool updateLocation();
	virtual bool updateTime();
	
	// position update couter
	int counter;
};

#endif
