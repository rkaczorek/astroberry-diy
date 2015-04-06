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
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <memory>
#include <string>

#include "rpi_mosaic.h"


#define POLLMS 3000

// We declare an auto pointer to IndiRpimosaic
std::auto_ptr<IndiRpimosaic> indiRpimosaic(0);

void ISPoll(void *p);
void ISInit()
{
   static int isInit = 0;

   if (isInit == 1)
       return;

    isInit = 1;
    if(indiRpimosaic.get() == 0) indiRpimosaic.reset(new IndiRpimosaic());
    //IEAddTimer(POLLMS, ISPoll, NULL);

}
void ISGetProperties(const char *dev)
{
        ISInit();
        indiRpimosaic->ISGetProperties(dev);
}
void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
        ISInit();
        indiRpimosaic->ISNewSwitch(dev, name, states, names, num);
}
void ISNewText(	const char *dev, const char *name, char *texts[], char *names[], int num)
{
        ISInit();
        indiRpimosaic->ISNewText(dev, name, texts, names, num);
}
void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
        ISInit();
        indiRpimosaic->ISNewNumber(dev, name, values, names, num);
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
    indiRpimosaic->ISSnoopDevice(root);
}
IndiRpimosaic::IndiRpimosaic()
{
}
IndiRpimosaic::~IndiRpimosaic()
{
}
bool IndiRpimosaic::Connect()
{	
    IDMessage(getDeviceName(), "Astroberry Mosaic connected successfully.");        	
    return true;
}
bool IndiRpimosaic::Disconnect()
{
    IDMessage(getDeviceName(), "Astroberry Mosaic disconnected successfully.");
    return true;
}
void IndiRpimosaic::TimerHit()
{
	if(isConnected())
	{		
		SetTimer( POLLMS );
    }
}
const char * IndiRpimosaic::getDefaultName()
{
        return (char *)"Astroberry Mosaic";
}
bool IndiRpimosaic::initProperties()
{
    // We init parent properties first
    INDI::DefaultDevice::initProperties();

    IUFillNumber(&MosaicSizeN[0],"RA","RA Width (arc min)","%0.1f",0,1800,0,0.0);
    IUFillNumber(&MosaicSizeN[1],"DEC","DEC Height (arc min)","%0.1f",0,1800,0,0.0 );
    IUFillNumberVector(&MosaicSizeNP,MosaicSizeN,2,getDeviceName(),"MOSAIC_SIZE","Mosaic Size",MAIN_CONTROL_TAB,IP_RW,60,IPS_OK);

    IUFillNumber(&FrameOverlapN[0],"RA","RA Horizontal overlap (arc min)","%0.1f",0,100,0,0.0);
    IUFillNumber(&FrameOverlapN[1],"DEC","DEC Vertical overlap (arc min)","%0.1f",0,100,0,0.0 );
    IUFillNumberVector(&FrameOverlapNP,FrameOverlapN,2,getDeviceName(),"MOSAIC_OVERLAP","Frame Overlap",MAIN_CONTROL_TAB,IP_RW,60,IPS_OK);

    IUFillNumber(&FrameSizeN[0],"RA","RA Width (arc min)","%0.1f",0,900,0,0.0);
    IUFillNumber(&FrameSizeN[1],"DEC","DEC Height (arc min)","%0.1f",0,900,0,0.0 );
    IUFillNumberVector(&FrameSizeNP,FrameSizeN,2,getDeviceName(),"MOSAIC_CCD_FOV","CCD FOV",MAIN_CONTROL_TAB,IP_RW,60,IPS_OK);

    IUFillNumber(&MosaicN[0],"FRAMES","Number of Frames","%0.0f",0,9999,0,0.0 );
    IUFillNumber(&MosaicN[1],"WIDTH","Width (frames)","%0.0f",0,99,0,0.0 );
    IUFillNumber(&MosaicN[2],"HEIGHT","Height (frames)","%0.0f",0,99,0,0.0 );
    IUFillNumberVector(&MosaicNP,MosaicN,3,getDeviceName(),"MOSAIC_SETUP","Mosaic",MAIN_CONTROL_TAB,IP_RO,60,IPS_OK);

    IUFillNumber(&CenterN[0],"RA","RA (hh:mm:ss)","%010.6m",-90,90,0,0.0);
    IUFillNumber(&CenterN[1],"DEC","DEC (hh:mm:ss)","%010.6m",-90,90,0,0.0 );
    IUFillNumberVector(&CenterNP,CenterN,2,getDeviceName(),"MOSAIC_CENTER_COORD","Center Coord.",MAIN_CONTROL_TAB,IP_RW,60,IPS_OK);

	// local copy of telescope coordinates
    IUFillNumber(&EqN[0],"RA","RA (hh:mm:ss)","%010.6m",0,24,0,0);
    IUFillNumber(&EqN[1],"DEC","DEC (dd:mm:ss)","%010.6m",-90,90,0,0);
    IUFillNumberVector(&EqNP,EqN,2,"EQMod Mount","EQUATORIAL_EOD_COORD","Eq. Coordinates",MAIN_CONTROL_TAB,IP_RW,60,IPS_IDLE);

	IUFillSwitch(&CentralCoordS[0],"SYNC_ON","Get Center Coord.",ISS_OFF);
	IUFillSwitchVector(&CentralCoordSP,CentralCoordS,1,getDeviceName(),"SYNC_COORD","Scope Coord.",MAIN_CONTROL_TAB,IP_RW,ISR_ATMOST1,60,IPS_OK);

	IUFillSwitch(&ProcessMosaicS[0],"START","Start",ISS_OFF);
	IUFillSwitch(&ProcessMosaicS[1],"STOP","Stop",ISS_OFF);
	IUFillSwitchVector(&ProcessMosaicSP,ProcessMosaicS,2,getDeviceName(),"MOSAIC_PROCESSING","Process Mosaic",MAIN_CONTROL_TAB,IP_RW,ISR_ATMOST1,60,IPS_OK);

	// Image Capture
    IUFillNumber(&ExposureN[0],"MOSAIC_EXP","Exposure time (seconds)","%0.2f",0,10600,0,0.0 );
    IUFillNumberVector(&ExposureNP,ExposureN,1,getDeviceName(),"MOSAIC_EXPOSURE","Exposure","CCD",IP_RW,60,IPS_OK);

    IUFillNumber(&ExposuresN[0],"MOSAIC_EXPS","Number of exposures","%0.0f",0,999,0,0.0 );
    IUFillNumberVector(&ExposuresNP,ExposuresN,1,getDeviceName(),"MOSAIC_EXPOSURES","Count","CCD",IP_RW,60,IPS_OK);

    IUFillNumber(&ExpDelayN[0],"MOSAIC_EXP_DELAY","Delay between exposures (seconds)","%0.0f",0,9999,0,0.0 );
    IUFillNumberVector(&ExpDelayNP,ExpDelayN,1,getDeviceName(),"MOSAIC_EXPDELAY","Delay","CCD",IP_RW,60,IPS_OK);

    IUFillNumber(&GotoDelayN[0],"MOSAIC_GOTO_DELAY","Pause after GOTO (seconds)","%0.0f",0,9999,0,0.0 );
    IUFillNumberVector(&GotoDelayNP,GotoDelayN,1,getDeviceName(),"MOSAIC_GOTODELAY","Pause","CCD",IP_RW,60,IPS_OK);
    
    // Mosaic
    // Frame No
    // RA
    // DEC
    // Prev Next
    
    return true;
}
bool IndiRpimosaic::updateProperties()
{
    // Call parent update properties first
    INDI::DefaultDevice::updateProperties();

    if (isConnected())
    {
		defineNumber(&MosaicSizeNP);
		defineNumber(&FrameOverlapNP);
		defineNumber(&FrameSizeNP);
        defineNumber(&MosaicNP);
        defineNumber(&CenterNP);
        defineNumber(&ExposureNP);
        defineNumber(&ExposuresNP);
        defineNumber(&ExpDelayNP);
        defineNumber(&GotoDelayNP);
		defineSwitch(&CentralCoordSP);
		defineSwitch(&ProcessMosaicSP);

		IDSnoopDevice("EQMod Mount", "EQUATORIAL_EOD_COORD");
		IDSnoopDevice("Atik Titan CCD", "CCD_EXPOSURE_VALUE");
		
		loadConfig();
    }
    else
    {
		// We're disconnected
		deleteProperty(FrameSizeNP.name);
		deleteProperty(FrameOverlapNP.name);
		deleteProperty(MosaicSizeNP.name);
		deleteProperty(CenterNP.name);
		deleteProperty(MosaicNP.name);
		deleteProperty(ExposureNP.name);
		deleteProperty(ExposuresNP.name);
		deleteProperty(ExpDelayNP.name);
		deleteProperty(GotoDelayNP.name);
		deleteProperty(CentralCoordSP.name);
		deleteProperty(ProcessMosaicSP.name);
    }
    
    return true;
}

void IndiRpimosaic::ISGetProperties(const char *dev)
{
    INDI::DefaultDevice::ISGetProperties(dev);

    /* Add debug controls so we may debug driver if necessary */
    //addDebugControl();
}
bool IndiRpimosaic::ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n)
{
	// first we check if it's for our device
    if (!strcmp(dev, getDeviceName()))
    {
		if(!strcmp(name, MosaicSizeNP.name))
		{		
			IUUpdateNumber(&MosaicSizeNP, values, names, n);			
			IDSetNumber(&MosaicSizeNP, NULL);
			updateMosaic();
			return true;
		}
		if(!strcmp(name, FrameOverlapNP.name))
		{		
			IUUpdateNumber(&FrameOverlapNP, values, names, n);
			IDSetNumber(&FrameOverlapNP, NULL);
			updateMosaic();
			return true;
		}
		if(!strcmp(name, FrameSizeNP.name))
		{		
			IUUpdateNumber(&FrameSizeNP, values, names, n);
			IDSetNumber(&FrameSizeNP, NULL);
			updateMosaic();
			return true;
		}
		if(!strcmp(name, CenterNP.name))
		{		
			IUUpdateNumber(&CenterNP, values, names, n);
			IDSetNumber(&CenterNP, NULL);
			updateMosaic();
			return true;
		}
		if(!strcmp(name, ExposureNP.name))
		{		
			IUUpdateNumber(&ExposureNP, values, names, n);
			IDSetNumber(&ExposureNP, NULL);
			return true;
		}
		if(!strcmp(name, ExposuresNP.name))
		{		
			IUUpdateNumber(&ExposuresNP, values, names, n);
			IDSetNumber(&ExposuresNP, NULL);
			return true;
		}
		if(!strcmp(name, ExpDelayNP.name))
		{		
			IUUpdateNumber(&ExpDelayNP, values, names, n);
			IDSetNumber(&ExpDelayNP, NULL);
			return true;
		}
		if(!strcmp(name, GotoDelayNP.name))
		{		
			IUUpdateNumber(&GotoDelayNP, values, names, n);
			IDSetNumber(&GotoDelayNP, NULL);
			return true;
		}
		if(!strcmp(name, MosaicNP.name))
		{		
			IUUpdateNumber(&MosaicNP, values, names, n);
			IDSetNumber(&MosaicNP, NULL);
			return true;
		}
	}

	return INDI::DefaultDevice::ISNewNumber(dev,name,values,names,n);
}
bool IndiRpimosaic::ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n)
{
	// first we check if it's for our device
    if (!strcmp(dev, getDeviceName()))
    {
		if(!strcmp(name, CentralCoordSP.name))
		{			
			// set Mosaic central point coordinates
			CenterN[0].value = EqN[0].value;
			CenterN[1].value = EqN[1].value;

			//IDMessage(getDeviceName(), "Mosaic central point updated to telescope coordinates RA: %f DEC: %f", EqN[0].value, EqN[1].value);
			IDSetNumber(&CenterNP, "Mosaic central point set to telescope coordinates RA: %f DEC: %f", EqN[0].value, EqN[1].value);

			// update switch
			IUUpdateSwitch(&CentralCoordSP, states, names, n);
			CentralCoordS[0].s = ISS_OFF;
			CentralCoordSP.s = IPS_OK;
			IDSetSwitch(&CentralCoordSP, NULL);
			
			return true;
		}
		if(!strcmp(name, ProcessMosaicSP.name))
		{
			IUUpdateSwitch(&ProcessMosaicSP, states, names, n);
			
			if ( ProcessMosaicS[0].s == ISS_ON )
			{
				if ( processMosaicCoord() )
				{
					ProcessMosaicSP.s = IPS_OK;
				}
				else
				{
					ProcessMosaicSP.s = IPS_ALERT;
				}
				ProcessMosaicS[0].s = ISS_OFF;
				ProcessMosaicS[1].s = ISS_OFF;
				IDSetSwitch(&ProcessMosaicSP, NULL);
			}
			if ( ProcessMosaicS[1].s == ISS_ON )
			{
				abortProcessing = true;
				ProcessMosaicS[0].s = ISS_OFF;
				ProcessMosaicSP.s = IPS_BUSY;
				IDSetSwitch(&ProcessMosaicSP, NULL);
			}

			return true;
		}
	}	
	
	return INDI::DefaultDevice::ISNewSwitch (dev, name, states, names, n);
}
bool IndiRpimosaic::ISNewText (const char *dev, const char *name, char *texts[], char *names[], int n)
{	
	return INDI::DefaultDevice::ISNewText (dev, name, texts, names, n);
}
bool IndiRpimosaic::ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n)
{
	return INDI::DefaultDevice::ISNewBLOB (dev, name, sizes, blobsizes, blobs, formats, names, n);
}
bool IndiRpimosaic::ISSnoopDevice(XMLEle *root)
{
    //controller->ISSnoopDevice(root);

   if (IUSnoopNumber(root, &EqNP) == 0)
   {
	   //
    }
    
    return INDI::DefaultDevice::ISSnoopDevice(root);
}
bool IndiRpimosaic::saveConfigItems(FILE *fp)
{
	IUSaveConfigNumber(fp, &MosaicSizeNP);
	IUSaveConfigNumber(fp, &FrameOverlapNP);
	IUSaveConfigNumber(fp, &FrameSizeNP);
	//IUSaveConfigNumber(fp, &CenterNP);
	//IUSaveConfigNumber(fp, &MosaicNP);
	IUSaveConfigNumber(fp, &ExposureNP);
	IUSaveConfigNumber(fp, &ExposuresNP);
	IUSaveConfigNumber(fp, &ExpDelayNP);
	IUSaveConfigNumber(fp, &GotoDelayNP);
        
    //controller->saveConfigItems(fp);

    return true;
}
void IndiRpimosaic::updateMosaic()
{
	if ( MosaicSizeN[0].value == 0 || MosaicSizeN[1].value == 0 || FrameSizeN[0].value == 0 || FrameSizeN[1].value == 0)
		return;
		
	// number of frames of given width to cover mosaic assuming given overlap
	int width = ceil((MosaicSizeN[0].value/FrameSizeN[0].value - 1)/(1 - FrameOverlapN[0].value/FrameSizeN[0].value)) + 1;
    
	// number of frames of given width to cover mosaic assuming given overlap
	int height = ceil((MosaicSizeN[1].value/FrameSizeN[1].value - 1)/(1 - FrameOverlapN[1].value/FrameSizeN[1].value)) + 1;

	// total number of frames
	int frames = width * height;

	MosaicN[0].value = frames;
	MosaicN[1].value = width;
	MosaicN[2].value = height;
	IDSetNumber(&MosaicNP, NULL);

	double arcwidth = FrameSizeN[0].value + (width - 1)*(1 - FrameOverlapN[0].value/FrameSizeN[0].value)*FrameSizeN[0].value;
	double archeight = FrameSizeN[1].value + (height - 1)*(1 - FrameOverlapN[1].value/FrameSizeN[1].value)*FrameSizeN[1].value;
	double estimatedTime = ((ExposureN[0].value + ExpDelayN[0].value) * ExposuresN[0].value + 3 + GotoDelayN[0].value) * frames / 60; // 3 s for goto

	MosaicSizeN[0].value = arcwidth;
	MosaicSizeN[1].value = archeight;
	IDSetNumber(&MosaicSizeNP, "Mosaic size set to %dx%d frames, FOV %0.0fx%0.0f arcmin (single frame size: %0.1fx%0.1f, overlap: %0.1fx%0.1f). Estimated time %0.1f minutes.", width, height, arcwidth, archeight, FrameSizeN[0].value, FrameSizeN[1].value, FrameOverlapN[0].value, FrameOverlapN[1].value, estimatedTime);		

}
bool IndiRpimosaic::processMosaicCoord()
{
	// start clock
	time_t begin;
	time(&begin);
	
	int frames = MosaicN[0].value;
	int width = MosaicN[1].value;
	int height = MosaicN[2].value;
	double estimatedTime = ((ExposureN[0].value + ExpDelayN[0].value) * ExposuresN[0].value + 10 + GotoDelayN[0].value) * frames / 60; // 10 s for goto

	IDMessage(getDeviceName(), "Starting mosaic processing. %d frames to be processed (%dx%d) in %0.1f minutes...", frames, width, height, estimatedTime);

	// the FIRST frame is TOP/LEFT
	double framePosRA = CenterN[0].value + 0.5 * (MosaicSizeN[0].value - FrameSizeN[0].value) / (60 * 15); // hours are not degrees
	double framePosDEC = CenterN[1].value + 0.5 * (MosaicSizeN[1].value - FrameSizeN[1].value) / 60;

	// init vars
	int i = 1;
	double pauseAfter;
	
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			if (abortProcessing)
			{
				abortProcessing = false;
				ProcessMosaicS[1].s = ISS_OFF;
				ProcessMosaicSP.s = IPS_OK;
				IDSetSwitch(&ProcessMosaicSP, NULL);				
				return false;
			}

			// populate Mosaic array left to right and top to bottom
			//MosaicLinear[i][0] = framePosRA;
			//MosaicLinear[i][1] = framePosDEC;
						
			// Go to
			IDMessage(getDeviceName(), "Slewing to frame %d/%d (RA %f DEC %f)...", i, frames, framePosRA, framePosDEC);
			Goto(framePosRA, framePosDEC);

			// Pausing after goto
			pauseAfter = GotoDelayN[0].value;
			sleep(pauseAfter);
			//IDMessage(getDeviceName(), "Pausing %0.0f seconds after goto.", pauseAfter);

			// Capture image
			captureImage();
			//if (captureImage())
				//IDMessage(getDeviceName(), "Frame %d.%d: image processing finished successfully.", x, y, framePosRA, framePosDEC);
			
			++i;
			
			// set to next column
			framePosRA -= (FrameSizeN[0].value - FrameOverlapN[0].value) / (60 * 15);
		}
		// after row end set to first column
		framePosRA = CenterN[0].value + 0.5 * (MosaicSizeN[0].value - FrameSizeN[0].value) / (60 * 15);
		
		// set next row
		framePosDEC -= (FrameSizeN[1].value - FrameOverlapN[1].value) / 60;
	}
	
		
	// stop clock
	time_t end;
	time(&end);
	double elapsedTime = difftime(end, begin) / 60;
	
	IDMessage(getDeviceName(), "Mosaic processed successfully. FOV %0.0fx%0.0f in %d frames processed in %0.1f minutes.", MosaicSizeN[0].value, MosaicSizeN[1].value, frames, elapsedTime);

	// return to starting position
	IDMessage(getDeviceName(), "Returning to center position...");
	Goto(CenterN[0].value, CenterN[1].value);

	return true;
}
bool IndiRpimosaic::Goto(double framePosRA, double framePosDEC)
{
	char command [50];
	sprintf (command, "indi_goto %f %f", framePosRA, framePosDEC);
	//sprintf (command, "indi_setprop -n 'EQMod Mount.EQUATORIAL_EOD_COORD.RA=%f;DEC=%f'", framePosRA, framePosDEC);

	//IDMessage(getDeviceName(), "[%s]", command);
	system(command);
	
	// wait while slewing
	double lastRA, lastDEC;

	do
	{
		//IDMessage(getDeviceName(), "Slewing...");
		lastRA = EqN[0].value;
		lastDEC = EqN[1].value;		
		sleep(3);
	} while( lastRA != EqN[0].value && lastDEC != EqN[1].value );

	sleep(3);
	
	IDMessage(getDeviceName(), "Scope at the position.");
	
	// TO DO - handle goto while slewing or tracking
	/*
	while(TrackState == SCOPE_SLEWING)
	{
		IDMessage(getDeviceName(), "Waiting for last slew is finished...");
		sleep(3);
	}
	*/
	/*
	EqN[0].value = framePosRA;
	EqN[1].value = framePosDEC;
	IDSetNumber(&EqNP, "Scope at the position RA %f DEC %f ready for image processing.", EqN[0].value, EqN[1].value);
	*/
	
	return true;
}
bool IndiRpimosaic::captureImage()
{
	int index, count;
	double exp, del;
	char command[50];

	exp = ExposureN[0].value;	
	count = ExposuresN[0].value;
	del = ExpDelayN[0].value;
	
	for( index=1; index <= count; ++index)
	{
		// Capture image
		IDMessage(getDeviceName(), "Capturing image %d/%d", index, count);
		sprintf (command, "indi_shot %f", exp);
		//sprintf (command, "indi_setprop -n 'Atik Titan CCD.CCD_EXPOSURE.CCD_EXPOSURE_VALUE=%f'", exp);
		
		system(command);
		//IDMessage(getDeviceName(), "Image %d captured successfully (%0.1f seconds exposure).", index, exp);
		
		// Wait
		//IDMessage(getDeviceName(), "Pausing %0.1f seconds after image capture...", del);
		sleep(del);
		//IDMessage(getDeviceName(), "Ready for next image capture.", del);
	}
	
	return true;
}
