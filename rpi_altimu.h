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

#ifndef RPIALTIMU_H
#define RPIALTIMU_H

#include <defaultdevice.h>

class IndiRpialtimu : public INDI::DefaultDevice
{
protected:
private:
public:
    IndiRpialtimu();
	virtual ~IndiRpialtimu();

	virtual const char *getDefaultName();

	INumber PositionN[3];
	INumberVectorProperty PositionNP;
	INumber PressureN[1];
	INumberVectorProperty PressureNP;
	INumber AltitudeN[1];
	INumberVectorProperty AltitudeNP;
	INumber TemperatureN[1];
	INumberVectorProperty TemperatureNP;
	INumber AdjustmentN[3];
	INumberVectorProperty AdjustmentNP;

	RTIMU *imu;
	RTPressure *pressure;
	bool GetSensorData();
    int sampleCount;
    int sampleRate;
    uint64_t rateTimer;
    uint64_t displayTimer;
    uint64_t now;
    
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
	virtual bool saveConfigItems(FILE *fp);
};

#endif
