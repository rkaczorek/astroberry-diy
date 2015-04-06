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

#ifndef FOCUSRPI_H
#define FOCUSRPI_H

#include <indifocuser.h>

class FocusRpi : public INDI::Focuser
{
    protected:
    private:
        int ticks;
        int speed;
        
		ISwitch FocusGotoS[3];
        ISwitchVectorProperty FocusGotoSP;

        ISwitch FocusMoveS[2];
        ISwitchVectorProperty FocusMoveSP;

        INumber FocusStepN[1];
        INumberVectorProperty FocusStepNP;
        
        ISwitch FocusResetS[1];
        ISwitchVectorProperty FocusResetSP;
        
        ISwitch FocusParkingS[2];
        ISwitchVectorProperty FocusParkingSP;
 
		INumber FocusBacklashN[1];
		INumberVectorProperty FocusBacklashNP; 
    public:
        FocusRpi();
        virtual ~FocusRpi();

        const char *getDefaultName();

        virtual bool Connect();
        virtual bool Disconnect();
        virtual bool initProperties();
        virtual bool updateProperties();        
        void ISGetProperties (const char *dev);
        
        virtual bool ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n);
        virtual bool ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n);
        virtual bool ISSnoopDevice(XMLEle *root);
        virtual bool saveConfigItems(FILE *fp);

		virtual int MoveFocuser(FocusDirection dir, int speed, int duration);
        virtual int MoveAbsFocuser(int ticks);
        virtual bool SetSpeed(int speed);
};

#endif
