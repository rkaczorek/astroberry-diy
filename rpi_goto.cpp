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

#include "rpi_goto.h"

using namespace std;

#define MYSCOPE "EQMod Mount"

/* Our client auto pointer */
auto_ptr<MyClient> myclient(0);

int main(int argc, char *argv[])
{
	if ( argc < 2 )
	{
		IDLog("Usage: indi_goto ra_coord dec_coord \nExample:\nindi_goto 1.234 45.678\n");
		return 1;
	}

	double ra = atof(argv[1]);
	double dec = atof(argv[2]);
	bool silent = true;

	if ( argv[3] == "-verbose" )
		silent = false;

	if (myclient.get() == 0)
		myclient.reset(new MyClient());

	myclient->setServer("localhost", 7624);
	myclient->watchDevice(MYSCOPE);
	myclient->connectServer();

	time_t     now;
	struct tm  tstruct;
    char       buf[80];

	now = time(0);
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tstruct);
    
    if (!silent)
		IDLog("%s: Starting Astroberry Goto...\n", buf);
	
	// Let the client connect to local server
	usleep(3000000);

	
	// Goto
	now = time(0);
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tstruct);

    if (!silent)
		IDLog("%s: Setting new coordinates...\n", buf);
	
	if(!myclient->setCoordinates(ra, dec))
	{
		IDLog("%s: Error: Can not set coordinates...\n", buf);
		return 1;
	}
	
	now = time(0);
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tstruct);

	if (!silent)
		IDLog("%s: Astroberry Goto finished successfully.\n", buf);
	
	return 0;
}

MyClient::MyClient()
{
    telescope = NULL;
}

MyClient::~MyClient()
{

}

void MyClient::newDevice(INDI::BaseDevice *dp)
{
    if (!strcmp(dp->getDeviceName(), MYSCOPE))
    {
        //IDLog("Receiving %s Device...\n", dp->getDeviceName());
		telescope = dp;
		return;
	}
}

void MyClient::newProperty(INDI::Property *property)
{
    if (!strcmp(property->getDeviceName(), MYSCOPE) && !strcmp(property->getName(), "CONNECTION"))
    {
        //connectDevice(MYSCOPE);
		//IDLog("Telescope is connected.\n");
        return;
    }
}

void MyClient::newNumber(INumberVectorProperty *nvp)
{
    // Let's check if we get any new values for EQUATORIAL_EOD_COORD
    if (!strcmp(nvp->name, "EQUATORIAL_EOD_COORD"))
    {
		//IDLog("Receiving new coordinates: RA %f DEC %f\n", nvp->np[0].value, nvp->np[1].value);
		return;
	}
}

void MyClient::newMessage(INDI::BaseDevice *dp, int messageID)
{
	if (!strcmp(dp->getDeviceName(), MYSCOPE))
	{
		//IDLog("%s\n", dp->messageQueue(messageID).c_str());		
        return;
	}
}

void MyClient::newBLOB(IBLOB *bp)
{
	return;
}
bool MyClient::setCoordinates(double ra, double dec)
{
	INumberVectorProperty *coordinates = NULL;
	INumberVectorProperty *current_coordinates = NULL;

    coordinates = telescope->getNumber("EQUATORIAL_EOD_COORD");

	time_t     now;
	struct tm  tstruct;
    char       buf[80];
    
	now = time(0);
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tstruct);
    
    if (coordinates == NULL)
    {
        IDLog("%s: Error: No telescope available\n", buf);
        return false;
    }

	if( ra == coordinates->np[0].value && dec == coordinates->np[1].value )
	{
		IDLog("%s: Telescope already at the position\n", buf);
		return true;
	}

    // Set new coordinates
	now = time(0);
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tstruct);
	
    // IDLog("%s: Telescope moving to new coordinates RA %f DEC %f...\n", buf, ra, dec);
    
    coordinates->np[0].value = ra;
    coordinates->np[1].value = dec;
    
    sendNewNumber(coordinates);
	
	return true;
}
