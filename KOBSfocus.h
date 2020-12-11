/****************************************************
 * Program:     KOBSfocus.h
****************************************************/
#ifndef FOCUSRPI_H
#define FOCUSRPI_H

#include "indifocuser.h"

#define LOW 0x0
#define HIGH 0x1

class FocusRpi : public INDI::Focuser
{
    public:
        FocusRpi();
        virtual ~FocusRpi();
	
	const int SPEED=60;

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
    protected:
		virtual IPState MoveFocuser(FocusDirection dir, int speed, int duration);
        virtual IPState MoveAbsFocuser(int ticks);
        virtual IPState MoveRelFocuser(FocusDirection dir, int ticks);
        virtual bool ReverseFocuser(bool enabled);
		virtual bool AbortFocuser();
        virtual void TimerHit();
    private: 
		INumber FocusBacklashN[1];
		INumberVectorProperty FocusBacklashNP;
		
		INumber FocusResolutionN[1];
		INumberVectorProperty FocusResolutionNP;

		INumber FocusStepDelayN[1];
		INumberVectorProperty FocusStepDelayNP;
		
        ISwitch MotorDirS[2];
        ISwitchVectorProperty MotorDirSP;	

        ISwitch MotorBoardS[2];
        ISwitchVectorProperty MotorBoardSP;	

		INumber MotorStandbyN[1];
		INumberVectorProperty MotorStandbyNP;

        virtual int SetResolution(int speed);
        virtual int regPosition(int pos);
        int timerID { -1 };
};

#endif
