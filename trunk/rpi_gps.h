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

#ifndef RPIGPS_H
#define RPIGPS_H

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <ctime>
#include <string>
#include <math.h>
#include <memory>
#include <libgpsmm.h>

#include <defaultdevice.h>

class IndiRpigps : public INDI::DefaultDevice
{
protected:
private:
	int timerid;
	int counter;
	gpsmm* gpsHandle;
	INDI::BaseDevice * telescope;
public:
	INumber GPSmodeN[1];
	INumberVectorProperty GPSmodeNP;

	IText GPStimeT[1];
	ITextVectorProperty GPStimeTP;

	INumber GPSlocationN[3];
	INumberVectorProperty GPSlocationNP;

	ISwitch GPSsetS[2];
	ISwitchVectorProperty GPSsetSP;

	IText LOCtimeT[2];
	ITextVectorProperty LOCtimeTP;

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

	virtual bool updateLocal();
	virtual bool updateGPS();
	virtual bool updateLocation();
	virtual bool updateTime();
};

#endif
