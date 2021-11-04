/*******************************************************************************
  Copyright(c) 2014-2021 Radek Kaczorek  <rkaczorek AT gmail DOT com>

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

class AstroberryFocuser : public INDI::Focuser
{
public:
	AstroberryFocuser();
	virtual ~AstroberryFocuser();
	const char *getDefaultName();
	virtual bool initProperties();
	virtual bool updateProperties();
	virtual void ISGetProperties (const char *dev);
	virtual bool ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n);
	virtual bool ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n);
	virtual bool ISNewText (const char *dev, const char *name, char *texts[], char *names[], int n);
	virtual bool ISSnoopDevice(XMLEle *root);
	static void stepperStandbyHelper(void *context);
	static void updateTemperatureHelper(void *context);
	static void temperatureCompensationHelper(void *context);
protected:
	virtual IPState MoveAbsFocuser(uint32_t ticks) override;
	virtual IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks) override;
	virtual bool ReverseFocuser(bool enabled) override;
	virtual bool SyncFocuser(uint32_t ticks) override;
	virtual bool SetFocuserBacklash(int32_t steps) override;
	virtual bool AbortFocuser() override;
	virtual void TimerHit() override;
	virtual bool saveConfigItems(FILE *fp) override;
private:
	virtual bool Connect();
	virtual bool Disconnect();

	virtual void stepMotor();
	virtual void setResolution(int res);
	virtual int savePosition(int pos);
	virtual bool readDS18B20();
	void getFocuserInfo();
	int stepperStandbyID { -1 };
	void stepperStandby();
	int updateTemperatureID { -1 };
	void updateTemperature();
	int temperatureCompensationID { -1 };
	void temperatureCompensation();

	ISwitch FocusResolutionS[6];
	ISwitchVectorProperty FocusResolutionSP;
	ISwitch MotorBoardS[2];
	ISwitchVectorProperty MotorBoardSP;
	ISwitch TemperatureCompensateS[2];
	ISwitchVectorProperty TemperatureCompensateSP;
	ISwitch StepperStandbyS[2];
	ISwitchVectorProperty StepperStandbySP;
	INumber FocuserInfoN[3];
	INumberVectorProperty FocuserInfoNP;
	INumber BCMpinsN[6];
	INumberVectorProperty BCMpinsNP;
	INumber StepperStandbyTimeN[1];
	INumberVectorProperty StepperStandbyTimeNP;	
	INumber FocusStepDelayN[1];
	INumberVectorProperty FocusStepDelayNP;
	INumber FocuserTravelN[1];
	INumberVectorProperty FocuserTravelNP;
	INumber ScopeParametersN[2];
	INumberVectorProperty ScopeParametersNP;
	INumber FocusTemperatureN[1];
	INumberVectorProperty FocusTemperatureNP;
	INumber TemperatureCoefN[1];
	INumberVectorProperty TemperatureCoefNP;
	IText ActiveTelescopeT[1];
	ITextVectorProperty ActiveTelescopeTP;

	struct gpiod_chip *chip;
	struct gpiod_line *gpio_dir;
	struct gpiod_line *gpio_step;
	struct gpiod_line *gpio_sleep;
	struct gpiod_line *gpio_m1;
	struct gpiod_line *gpio_m2;
	struct gpiod_line *gpio_m3;

	int backlashTicksRemaining;
	int focuserTicksRemaining;
	int stepperDirection = 1;
	
	int resolution = 1;
	float lastTemperature;
};

#endif
