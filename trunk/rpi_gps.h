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

#include <memory>
#include <libnova/libnova.h>
#include <time.h>
#include <libgpsmm.h>
#include "indigps.h"

class IndiRpigps : public INDI::GPS
{
protected:
private:
	gpsmm* gpsHandle;
	struct gps_data_t* gpsData;
public:
    IndiRpigps();
	virtual ~IndiRpigps();

	INumber GPSmodeN[1];
	INumberVectorProperty GPSmodeNP;

	INumber PolarisHN[1];
	INumberVectorProperty PolarisHNP;

	ISwitch GPSupdateS[1];
	ISwitchVectorProperty GPSupdateSP;

	virtual const char *getDefaultName();

	virtual bool ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n);

	virtual void TimerHit();
	virtual bool Connect();
	virtual bool Disconnect();
	virtual bool initProperties();
	virtual bool updateProperties();
		
	virtual bool updateGPS();
};

#endif
