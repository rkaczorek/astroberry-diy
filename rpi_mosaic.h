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

#ifndef RPIMOSAIC_H
#define RPIMOSAIC_H

#include <defaultdevice.h>
#include <indicom.h>

class IndiRpimosaic : public INDI::DefaultDevice
{
public:
    IndiRpimosaic();
	virtual ~IndiRpimosaic();

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
	virtual bool saveConfigItems(FILE *fp);
protected:
	virtual void updateMosaic();
	virtual bool processMosaicCoord();
	virtual bool Goto(double framePosRA, double framePosDEC);
	virtual bool captureImage();
private:
	INumber MosaicSizeN[2];
	INumberVectorProperty MosaicSizeNP;

	INumber FrameOverlapN[2];
	INumberVectorProperty FrameOverlapNP;

	INumber CenterN[2];
	INumberVectorProperty CenterNP;

	INumber FrameSizeN[2];
	INumberVectorProperty FrameSizeNP;

	INumber MosaicN[3];
	INumberVectorProperty MosaicNP;
	
	ISwitch CentralCoordS[1];
	ISwitchVectorProperty CentralCoordSP;

	ISwitch ProcessMosaicS[2];
	ISwitchVectorProperty ProcessMosaicSP;
	
	INumber ExposureN[1];
	INumberVectorProperty ExposureNP;	
	
	INumber ExposuresN[1];
	INumberVectorProperty ExposuresNP;	
	
	INumber ExpDelayN[1];
	INumberVectorProperty ExpDelayNP;	
	
	INumber GotoDelayN[1];
	INumberVectorProperty GotoDelayNP;	

	INumber EqN[2];
	INumberVectorProperty EqNP;

	bool abortProcessing;
	
	double MosaicLinear[9999][2];
};

#endif
