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

#include <string>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <memory>

#include "rpi_shot.h"

using namespace std;

#define MYCCD "Atik Titan CCD"

/* Our client auto pointer */
auto_ptr<MyClient> myclient(0);

int main(int argc, char *argv[])
{
	if ( argc < 1 )
	{
		IDLog("Usage: indi_shot exposure (seconds) \nExample:\nindi_shot 3\n");
		return 1;
	}

	double exposure = atof(argv[1]);
	bool silent = true;

	if ( argv[2] == "-verbose" )
		silent = false;

	if (myclient.get() == 0)
		myclient.reset(new MyClient());

	myclient->setServer("localhost", 7624);
	myclient->watchDevice(MYCCD);
	myclient->connectServer();

	time_t     now;
	struct tm  tstruct;
    char       buf[80];

	now = time(0);
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tstruct);
    
    if (!silent)
		IDLog("%s: Starting Astroberry Shot...\n", buf);
	
	// Let the client connect to local server
	usleep(3000000);

	
	// Capture
	now = time(0);
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tstruct);

    if (!silent)
		IDLog("%s: Starting exposure...\n", buf);
	
	if(!myclient->setExposure(exposure))
	{
		IDLog("%s: Error: Can not start exposure...\n", buf);
		return 1;
	}
	
	usleep(exposure * 1000000);
	
	now = time(0);
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tstruct);

	if (!silent)
		IDLog("%s: Astroberry Shot finished successfully.\n", buf);
	
	return 0;
}

MyClient::MyClient()
{
    ccd = NULL;
}

MyClient::~MyClient()
{

}

void MyClient::newDevice(INDI::BaseDevice *dp)
{
    if (!strcmp(dp->getDeviceName(), MYCCD))
    {
        //IDLog("Receiving %s Device...\n", dp->getDeviceName());
		ccd = dp;
		return;
	}
}

void MyClient::newProperty(INDI::Property *property)
{
    if (!strcmp(property->getDeviceName(), MYCCD) && !strcmp(property->getName(), "CONNECTION"))
    {
        //connectDevice(MYCCD);
		//IDLog("CCD is connected.\n");
        return;
    }
}

void MyClient::newNumber(INumberVectorProperty *nvp)
{
    // Let's check if we get any new values for CCD_EXPOSURE_VALUE
    if (!strcmp(nvp->name, "CCD_EXPOSURE"))
    {
		//IDLog("Receiving new exposure value: %f seconds\n", nvp->np[0].value);
		return;
	}
}

void MyClient::newMessage(INDI::BaseDevice *dp, int messageID)
{
	if (!strcmp(dp->getDeviceName(), MYCCD))
	{
		//IDLog("%s\n", dp->messageQueue(messageID).c_str());
        return;
	}
}

void MyClient::newBLOB(IBLOB *bp)
{
	return;
}
bool MyClient::setExposure(double exp)
{
	INumberVectorProperty *exposure = NULL;

    exposure = ccd->getNumber("CCD_EXPOSURE");

	time_t     now;
	struct tm  tstruct;
    char       buf[80];
    
	now = time(0);
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tstruct);
    
    if (exposure == NULL)
    {
        IDLog("%s: Error: No CCD available\n", buf);
        return false;
    }


    // Set new exposure
	now = time(0);
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tstruct);
	
    // IDLog("%s: Setting exposure to %f s...\n", buf, exp);
    
    exposure->np[0].value = exp;
    
    sendNewNumber(exposure);
	
	return true;
}
