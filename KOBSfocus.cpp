/*****************************************************
 * Program:     KOBSfocus.cpp
 ******************************************************/

#include <stdio.h>
#include <unistd.h>
#include <memory>
#include <string.h>
#include <fstream>
#include "config.h"

#include "KOBSfocus.h"
#include "PScontrol.h"

static std::unique_ptr<PWRHB> pwrhb(new PWRHB());

#define MAX_RESOLUTION 2

const std::map<PWRHB::PS_MOTOR, std::string> PWRHB::MotorMap =
{
    {PWRHB::PS_NOT_MOVING, "Idle"},
    {PWRHB::PS_MOVING_IN, "Moving Inwards"},
    {PWRHB::PS_MOVING_OUT, "Moving Outwards"},
    {PWRHB::PS_LOCKED, "Locked"},
    {PS_UNK, "Busy"},
};

// Std INDI declarations **************************
void ISPoll(void *p);

void ISInit()
{
   static int isInit = 0;

   if (isInit == 1)
       return;
   if(focusRpi.get() == 0)
   {
       isInit = 1;
       focusRpi.reset(new FocusRpi());
   }
}

void ISGetProperties(const char *dev)
{
        ISInit();
        focusRpi->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
        ISInit();
        focusRpi->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(	const char *dev, const char *name, char *texts[], char *names[], int num)
{
        ISInit();
        focusRpi->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
        ISInit();
        focusRpi->ISNewNumber(dev, name, values, names, num);
}

void ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n)
{
  INDI_UNUSED(dev);
  INDI_UNUSED(name);
  INDI_UNUSED(sizes);
  INDI_UNUSED(blobsizes);
  INDI_UNUSED(blobs);
  INDI_UNUSED(formats);
  INDI_UNUSED(names);
  INDI_UNUSED(n);
}

void ISSnoopDevice (XMLEle *root)
{
    ISInit();
    focusRpi->ISSnoopDevice(root);
}

// Init Stuff (orig) ****************************************
FocusRpi::FocusRpi()
{
	setVersion(VERSION_MAJOR,VERSION_MINOR);
	FI::SetCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | FOCUSER_CAN_REVERSE | FOCUSER_CAN_ABORT);
	Focuser::setSupportedConnections(CONNECTION_NONE);
}

FocusRpi::~FocusRpi()
{
}

// get name (orig) *********************************
const char * FocusRpi::getDefaultName()
{
        return (char *)"KOBS Focuser";
}

// connect (copied) *****************************
bool FocusRpi::Connect()
{
    PSCTL psctl;
    if (isSimulation())
    {
        SetTimer(POLLMS);
        return true;
    }

    if ( ! psctl.Connect() )
    {
        LOG_ERROR("No PowerHub focuser found.");
        return false;
    }
    else
    {
        psctl.getMaxPosition(&maximumPosition);
        //psctl.getAbsPosition)&relitivePosition);
        FocusAbsPosN[0].max = FocusRelPosN[0].max = FocusSyncN[0].max = FocusMaxPosN[0].value = maximumPosition;
        FocusAbsPosN[0].step = FocusRelPosN[0].step = FocusSyncN[0].step = 100;
        FocusAbsPosN[0].min = FocusRelPosN[0].min = FocusSyncN[0].min = 0;
        
        
        
        SetTimer(POLLMS);
        
        uint16_t psversion = psctl.getVersion();
        LOGF_INFO("PowerHub Firmware Version: %i.%i", (psversion & 0xFF00) >> 8, psversion & 0xFF);
        
        // only setting up unipolar for now
        psctl.activateProfile(UNI_12V);

        LOGF_INFO("Humidity @ opening: %#.1f", psctl.getHumidity());
        LOGF_INFO("Temperature @ opening: %#.1f", psctl.getTemperature());
    
    }
    return true;
}

// disconnect (copied) ***************************
bool FocusRpi::Disconnect()
{
    if (isSimulation() == false)
    {        
        LOGF_INFO("Humidity @ closing: %#.1f", psctl.getHumidity());
        LOGF_INFO("Temperature @ closing: %#.1f", psctl.getTemperature());
        
        psctl.Disconnect();
    }
    return true;
}

// init (needs work) *****************************
bool FocusRpi::initProperties()
{
    INDI::Focuser::initProperties();
    addSimulationControl();
    addDebugControl();
    
	IUFillNumber(&FocusBacklashN[0], "FOCUS_BACKLASH_VALUE", "steps", "%0.0f", 0, 1000, 10, 0);
	IUFillNumberVector(&FocusBacklashNP, FocusBacklashN, 1, getDeviceName(), "FOCUS_BACKLASH", "Backlash", OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

	IUFillNumber(&FocusResolutionN[0], "FOCUS_RESOLUTION_VALUE", "1/n steps", "%0.0f", 1, MAX_RESOLUTION, 1, 1);
	IUFillNumberVector(&FocusResolutionNP, FocusResolutionN, 1, getDeviceName(), "FOCUS_RESOLUTION", "Resolution", OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

	IUFillNumber(&FocusStepDelayN[0], "FOCUS_STEPDELAY_VALUE", "milliseconds", "%0.0f", 1, 10, 1, 1);
	IUFillNumberVector(&FocusStepDelayNP, FocusStepDelayN, 1, getDeviceName(), "FOCUS_STEPDELAY", "Step Delay", OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

	IUFillSwitch(&MotorBoardS[0],"MoonLight","MoonLight",ISS_ON);
	IUFillSwitch(&MotorBoardS[1],"Pegasus","Pegasus",ISS_OFF);
	IUFillSwitchVector(&MotorBoardSP,MotorBoardS,2,getDeviceName(),"MOTOR_BOARD","Control Board",OPTIONS_TAB,IP_RW,ISR_1OFMANY,60,IPS_IDLE);

	IUFillNumber(&MotorStandbyN[0], "MOTOR_STANDBY_TIME", "minutes", "%0.0f", 0, 10, 1, 1);
	IUFillNumberVector(&MotorStandbyNP, MotorStandbyN, 1, getDeviceName(), "MOTOR_SLEEP", "Standby", OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

    // TODO this was set in connect - need to consolidate
	FocusMaxPosN[0].min = 1000;
	FocusMaxPosN[0].max = 40000;
	FocusMaxPosN[0].step = 1000;
	FocusMaxPosN[0].value = 40000;

	FocusRelPosN[0].min = 0;
	FocusRelPosN[0].max = 1000;
	FocusRelPosN[0].step = 200;
	FocusRelPosN[0].value = 200;

	FocusAbsPosN[0].min = 0;
	FocusAbsPosN[0].max = FocusMaxPosN[0].value;
	FocusAbsPosN[0].step = (int) FocusAbsPosN[0].max / 100;
	FocusAbsPosN[0].value = 20000;
	// FocusAbsPosN[0].value = regPosition(-1) != -1 ? regPosition(-1) * SetResolution(FocusResolutionN[0].value) : 0; //read last position from file

	FocusMotionS[FOCUS_OUTWARD].s = ISS_ON;
	FocusMotionS[FOCUS_INWARD].s = ISS_OFF;
	IDSetSwitch(&FocusMotionSP, nullptr);

    return true;
}

// get properties (??) **************************
void FocusRpi::ISGetProperties (const char *dev)
{
    INDI::Focuser::ISGetProperties(dev);

    return;
}

// update (??) ******************************
bool FocusRpi::updateProperties()
{

    INDI::Focuser::updateProperties();

    if (isConnected())
    {
		defineSwitch(&FocusMotionSP);
		defineSwitch(&MotorBoardSP);
		defineNumber(&MotorStandbyNP);
		defineNumber(&FocusBacklashNP);
		defineNumber(&FocusStepDelayNP);
		defineNumber(&FocusResolutionNP);
    } else {
		deleteProperty(FocusMotionSP.name);
		deleteProperty(MotorBoardSP.name);
		deleteProperty(MotorStandbyNP.name);
		deleteProperty(FocusBacklashNP.name);
		deleteProperty(FocusStepDelayNP.name);
		deleteProperty(FocusResolutionNP.name);
    }

    return true;
}

// new number (??) ******************************
bool FocusRpi::ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n)
{
	// first we check if it's for our device
	if(strcmp(dev,getDeviceName())==0)
	{

        // handle focus absolute position
        if (!strcmp(name, FocusAbsPosNP.name))
        {
			int newPos = (int) values[0];
            if ( MoveAbsFocuser(newPos) == IPS_OK )
            {
               IUUpdateNumber(&FocusAbsPosNP,values,names,n);
               FocusAbsPosNP.s=IPS_OK;
               IDSetNumber(&FocusAbsPosNP, nullptr);
            }
            return true;
        }

        // handle focus relative position
        if (!strcmp(name, FocusRelPosNP.name))
        {
			IUUpdateNumber(&FocusRelPosNP,values,names,n);

			//FOCUS_INWARD
			if ( FocusMotionS[0].s == ISS_ON )
				FocusRelPosNP.s = MoveRelFocuser(FOCUS_INWARD, FocusRelPosN[0].value);

			//FOCUS_OUTWARD
			if ( FocusMotionS[1].s == ISS_ON )
				FocusRelPosNP.s = MoveRelFocuser(FOCUS_OUTWARD, FocusRelPosN[0].value);

			IDSetNumber(&FocusRelPosNP, nullptr);
			return true;
        }

        // handle focus backlash
        if (!strcmp(name, FocusBacklashNP.name))
        {
            IUUpdateNumber(&FocusBacklashNP,values,names,n);
            FocusBacklashNP.s=IPS_BUSY;
            IDSetNumber(&FocusBacklashNP, nullptr);
            FocusBacklashNP.s=IPS_OK;
            IDSetNumber(&FocusBacklashNP, nullptr);
            DEBUGF(INDI::Logger::DBG_DEBUG, "KOBS Focuser backlash set to %0.0f", FocusBacklashN[0].value);
            return true;
        }

        // handle focus resolution
        if (!strcmp(name, FocusResolutionNP.name))
        {
			int last_resolution = SetResolution(FocusResolutionN[0].value);
			IUUpdateNumber(&FocusResolutionNP,values,names,n);
			FocusResolutionNP.s=IPS_BUSY;
			IDSetNumber(&FocusResolutionNP, nullptr);

			// set step size
			int resolution = SetResolution(FocusResolutionN[0].value);

			FocusMaxPosN[0].min = (int) FocusMaxPosN[0].min * resolution / last_resolution;
			FocusMaxPosN[0].max = (int) FocusMaxPosN[0].max * resolution / last_resolution;
			FocusMaxPosN[0].step = (int) FocusMaxPosN[0].step * resolution / last_resolution;
			FocusMaxPosN[0].value = (int) FocusMaxPosN[0].value * resolution / last_resolution;
			IDSetNumber(&FocusMaxPosNP, nullptr);

			FocusRelPosN[0].min = (int) FocusRelPosN[0].min * resolution / last_resolution;
			FocusRelPosN[0].max = (int) FocusRelPosN[0].max * resolution / last_resolution;
			FocusRelPosN[0].step = (int) FocusRelPosN[0].step * resolution / last_resolution;
			FocusRelPosN[0].value = (int) FocusRelPosN[0].value * resolution / last_resolution;
			IDSetNumber(&FocusRelPosNP, nullptr);

			FocusAbsPosN[0].max = (int) FocusAbsPosN[0].max * resolution / last_resolution;
			FocusAbsPosN[0].step = (int) FocusAbsPosN[0].step * resolution / last_resolution;
			FocusAbsPosN[0].value = (int) FocusAbsPosN[0].value * resolution / last_resolution;
			IDSetNumber(&FocusAbsPosNP, nullptr);

			deleteProperty(FocusRelPosNP.name);
			defineNumber(&FocusRelPosNP);

			deleteProperty(FocusAbsPosNP.name);
			defineNumber(&FocusAbsPosNP);

			deleteProperty(FocusMaxPosNP.name);
			defineNumber(&FocusMaxPosNP);

			deleteProperty(FocusReverseSP.name);
			defineSwitch(&FocusReverseSP);

			FocusResolutionNP.s=IPS_OK;
			IDSetNumber(&FocusResolutionNP, nullptr);

			DEBUGF(INDI::Logger::DBG_DEBUG, "KOBS Focuser resolution changed from 1/%d to 1/%d step", last_resolution, resolution);
			return true;
        }

        // handle focus step delay
        if (!strcmp(name, FocusStepDelayNP.name))
        {
		   IUUpdateNumber(&FocusStepDelayNP,values,names,n);
		   FocusStepDelayNP.s=IPS_BUSY;
		   IDSetNumber(&FocusStepDelayNP, nullptr);
		   FocusStepDelayNP.s=IPS_OK;
		   IDSetNumber(&FocusStepDelayNP, nullptr);
		   DEBUGF(INDI::Logger::DBG_DEBUG, "KOBS Focuser step delay set to %0.0f", FocusStepDelayN[0].value);
           return true;
        }

        // handle motor sleep
        if (!strcmp(name, MotorStandbyNP.name))
        {
		   IUUpdateNumber(&MotorStandbyNP,values,names,n);
		   MotorStandbyNP.s=IPS_BUSY;
		   IDSetNumber(&MotorStandbyNP, nullptr);

			// set motor standby timer
			if ( MotorStandbyN[0].value > 0)
			{
				if (timerID)
					RemoveTimer(timerID);
				timerID = SetTimer(60 * 1000 * MotorStandbyN[0].value);
			}

		   MotorStandbyNP.s=IPS_OK;
		   IDSetNumber(&MotorStandbyNP, nullptr);
		   DEBUGF(INDI::Logger::DBG_DEBUG, "KOBS Focuser standby timeout set to %0.0f", MotorStandbyN[0].value);
           return true;
        }
	}
    return INDI::Focuser::ISNewNumber(dev,name,values,names,n);
}

// new switch (??) ********************************
bool FocusRpi::ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n)
{
	// first we check if it's for our device
    if (!strcmp(dev, getDeviceName()))
    {
        // handle focus presets
        if (!strcmp(name, PresetGotoSP.name))
        {
            IUUpdateSwitch(&PresetGotoSP, states, names, n);
			PresetGotoSP.s = IPS_BUSY;
            IDSetSwitch(&PresetGotoSP, nullptr);

			//Preset 1
			if ( PresetGotoS[0].s == ISS_ON )
				MoveAbsFocuser(PresetN[0].value);

			//Preset 2
			if ( PresetGotoS[1].s == ISS_ON )
				MoveAbsFocuser(PresetN[1].value);

			//Preset 3
			if ( PresetGotoS[2].s == ISS_ON )
				MoveAbsFocuser(PresetN[2].value);

			PresetGotoS[0].s = ISS_OFF;
			PresetGotoS[1].s = ISS_OFF;
			PresetGotoS[2].s = ISS_OFF;
			PresetGotoSP.s = IPS_OK;

            IDSetSwitch(&PresetGotoSP, nullptr);

            return true;
        }

        // handle motor board
        if(!strcmp(name, MotorBoardSP.name))
        {
			IUUpdateSwitch(&MotorBoardSP, states, names, n);
			MotorBoardSP.s = IPS_BUSY;
			IDSetSwitch(&MotorBoardSP, nullptr);


			// set max resolution for motor model
			if ( MotorBoardS[0].s == ISS_ON)
			{
				FocusResolutionN[0].max = MAX_RESOLUTION;
				DEBUG(INDI::Logger::DBG_DEBUG, "KOBS Focuser control board set to MoonLight");
			}
			if ( MotorBoardS[1].s == ISS_ON)
			{
				if ( FocusResolutionN[0].value > 5 )
					FocusResolutionN[0].value = 5;
				FocusResolutionN[0].max = MAX_RESOLUTION;
				DEBUG(INDI::Logger::DBG_DEBUG, "KOBS Focuser control board set to Pegasus");
			}
			IDSetNumber(&FocusResolutionNP, nullptr);
			deleteProperty(FocusResolutionNP.name);
			defineNumber(&FocusResolutionNP);

			MotorBoardSP.s = IPS_OK;
			IDSetSwitch(&MotorBoardSP, nullptr);
			return true;
		}
    }
    return INDI::Focuser::ISNewSwitch(dev,name,states,names,n);
}

// snoop dev (orig) ****************************************
bool FocusRpi::ISSnoopDevice (XMLEle *root)
{
    return INDI::Focuser::ISSnoopDevice(root);
}

bool FocusRpi::saveConfigItems(FILE *fp)
{
    IUSaveConfigNumber(fp, &FocusMaxPosNP);
    IUSaveConfigNumber(fp, &PresetNP);
    IUSaveConfigSwitch(fp, &FocusReverseSP);
    IUSaveConfigSwitch(fp, &MotorBoardSP);
    IUSaveConfigNumber(fp, &MotorStandbyNP);
    IUSaveConfigNumber(fp, &FocusBacklashNP);
    IUSaveConfigNumber(fp, &FocusStepDelayNP);
    IUSaveConfigNumber(fp, &FocusResolutionNP);

    return true;
}

// Timer (copied) ********************************************
void FocusRpi::TimerHit()
{
if (!isConnected())
        return;
    
    uint32_t faultstat = psctl.getFaultStatus();
    if (faultstat) {
        LOGF_ERROR("System Fault <%06x>", faultstat);
        return;
    }

    uint32_t currentTicks = 0;
    
    bool rc = psctl.getAbsPosition(&currentTicks);

    if (rc)
        FocusAbsPosN[0].value = currentTicks;

    psctl.getFocusStatus();

    if (FocusAbsPosNP.s == IPS_BUSY || FocusRelPosNP.s == IPS_BUSY)
    {
        if (isSimulation())
        {
            if (FocusAbsPosN[0].value < targetPosition)
                simPosition += 500;
            else
                simPosition -= 500;

            if (std::abs(simPosition - static_cast<int32_t>(targetPosition)) < 500)
            {
                FocusAbsPosN[0].value = targetPosition;
                simPosition = FocusAbsPosN[0].value;
                m_Motor = PS_NOT_MOVING;
            }

            FocusAbsPosN[0].value = simPosition;
        }

        if (m_Motor == PS_NOT_MOVING && targetPosition == FocusAbsPosN[0].value)
        {
            if (FocusRelPosNP.s == IPS_BUSY)
            {
                FocusRelPosNP.s = IPS_OK;
                IDSetNumber(&FocusRelPosNP, nullptr);
            }

            FocusAbsPosNP.s = IPS_OK;
            LOGF_INFO("Focuser now at %d", targetPosition);
            LOG_DEBUG("Focuser reached target position.");
        }
    }

    IDSetNumber(&FocusAbsPosNP, nullptr);

    SetTimer(POLLMS);
}

// Abort (copied) *****************************************
bool FocusRpi::AbortFocuser()
{
     LOG_INFO("Aborting");
    //LOGF_INFO("Current humidity: %#.1f", psctl.getHumidity());
    //LOGF_INFO("Current temperature: %#.1f", psctl.getTemperature());
    
    return psctl.AbortFocuser();
}

// Move (orig)  ********************************************
IPState FocusRpi::MoveFocuser(FocusDirection dir, int speed, int duration)
{
    int ticks = (int) ( duration / FocusStepDelayN[0].value);
    return 	MoveRelFocuser( dir, ticks);
}

// MoveRel (copied) ************************************************
IPState FocusRpi::MoveRelFocuser(FocusDirection dir, int ticks)
{
    int direction = (dir == FOCUS_INWARD) ? -1 : 1;
    int reversed = (FocusReverseS[INDI_ENABLED].s == ISS_ON) ? -1 : 1;
    int relative = static_cast<int>(ticks);

    int targetAbsPosition = FocusAbsPosN[0].value + (relative * direction * reversed);

    targetAbsPosition = std::min(static_cast<uint32_t>(FocusMaxPosN[0].value),static_cast<uint32_t>(std::max(static_cast<int>(FocusAbsPosN[0].min), targetAbsPosition)));

    return (IPState)psctl.MoveAbsFocuser(targetAbsPosition);
}

// MoveABS (copied) ***********************************************
IPState FocusRpi::MoveAbsFocuser(int targetTicks)
{
    if ( ! psctl.MoveAbsFocuser(targetTicks))
        return IPS_ALERT;

    FocusAbsPosNP.s = IPS_BUSY;

    return IPS_BUSY;
}


// set resolution (??) *************************************
int FocusRpi::SetResolution(int res)
{
	// TODO should not need this section
}

// reverse (orig) ***************************************
bool FocusRpi::ReverseFocuser(bool enabled)
{
	if (enabled)
	{
		DEBUG(INDI::Logger::DBG_DEBUG, "KOBS Focuser reverse direction ENABLED");
	} else {
		DEBUG(INDI::Logger::DBG_DEBUG, "KOBS Focuser reverse direction DISABLED");
	}
    return true;
}

// request Position (orig) ******************************
int FocusRpi::regPosition(int pos)
{
	// TODO

	return pos;
}

bool PWRHB::SyncFocuser(uint32_t ticks)
{
    if ( ! psctl.setAbsPosition(ticks))
        return false;

    simPosition = ticks;

    return psctl.SyncFocuser(ticks);
}

bool PWRHB::SetFocuserMaxPosition(uint32_t ticks)
{
    return psctl.SetFocuserMaxPosition(ticks);
}
