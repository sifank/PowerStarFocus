 /***************************************************
Program:    PSfocus.cpp
Author:     Sifan Kahale
Version:    20201210
Desc:       INDI interface for PowerStar
Requires:   PScontrol.cpp device interface
****************************************************/
#include "PSfocus.h"

#define FOCUS_SETTINGS_TAB "Settings"

static std::unique_ptr<PWRSTR> pwrhb(new PWRSTR());

//************************************************************
void ISGetProperties(const char *dev)
{
    pwrhb->ISGetProperties(dev);
}

//************************************************************
void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    pwrhb->ISNewSwitch(dev, name, states, names, n);
}

//************************************************************
void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    pwrhb->ISNewText(dev, name, texts, names, n);
}

//************************************************************
void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    pwrhb->ISNewNumber(dev, name, values, names, n);
}

//************************************************************
void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
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

//************************************************************
void ISSnoopDevice(XMLEle *root)
{
    pwrhb->ISSnoopDevice(root);
}

//************************************************************
PWRSTR::PWRSTR()
{
    setVersion(0, 11);
    FI::SetCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | FOCUSER_CAN_ABORT | FOCUSER_CAN_SYNC);
    setSupportedConnections(CONNECTION_NONE);
}

//************************************************************
bool PWRSTR::Connect()
{
    PSCTL psctl;
    if (isSimulation())
    {
        SetTimer(POLLMS);
        return true;
    }

    if ( ! psctl.Connect() )  //this does the unlock as well
    {
        LOG_ERROR("No PowerStar focuser found.");
        return false;
    }
    else
    {        
        // TODO check perm focus to see if we need to do these next two
        uint32_t maximumPosition = 0;
        psctl.getMaxPosition(&maximumPosition);
        psctl.getAbsPosition(&relitivePosition);
        
        FocusMaxPosN[0].value = maximumPosition;

        FocusAbsPosN[0].max = FocusSyncN[0].max = FocusMaxPosN[0].value;
            
        FocusAbsPosN[0].step = FocusSyncN[0].step = FocusMaxPosN[0].value / 50.0;
            
        FocusAbsPosN[0].min = FocusSyncN[0].min = 0;

        FocusRelPosN[0].max  = FocusMaxPosN[0].value / 2;
            
        FocusRelPosN[0].step = FocusMaxPosN[0].value / 100.0;
            
        FocusRelPosN[0].min  = 0;
        
        FocusAbsPosN[0].value = relitivePosition;        
        
        SetTimer(POLLMS);
        
        uint16_t psversion = psctl.getVersion();
        LOGF_INFO("PowerStar Firmware Version: %i.%i", (psversion & 0xFF00) >> 8, psversion & 0xFF);
        
        // read config file and retrieve port names
        FILE* fin = fopen("/etc/powerstar.config", "r");
        int rc = fread(&curProfile, sizeof(PowerStarProfile), 1, fin);     
        if (!rc) {
            LOG_ERROR("Error: could not read profile\n");
            return false;
        }
        fclose(fin);
        
        // TODO test perm foc to see if we need to set cur and max pos

        LOGF_INFO("Humidity @ opening: %#.1f", psctl.getHumidity());
        LOGF_INFO("Temperature @ opening: %#.1f", psctl.getTemperature());
    }

    return true;
}

//************************************************************
bool PWRSTR::Disconnect()
{
    if (isSimulation() == false)
    {        
        LOGF_INFO("Humidity @ closing: %#.1f", psctl.getHumidity());
        LOGF_INFO("Temperature @ closing: %#.1f", psctl.getTemperature());
        
        psctl.Disconnect();
    }

    return true;
}

//************************************************************
const char *PWRSTR::getDefaultName()
{
    return "Power^Star";
}

//************************************************************
bool PWRSTR::initProperties()
{
    INDI::Focuser::initProperties();

    addSimulationControl();
    
    addDebugControl();
    
    return true;
}

//************************************************************
void PWRSTR::TimerHit()
{
    if (!isConnected())
        return;
    
    uint32_t faultstat = psctl.getFaultStatus(curProfile.faultMask);      
        
    if (faultstat) {
        LOGF_ERROR("System Fault: %08x Occurred", faultstat);
        return;
    }

    uint32_t currentTicks = 0;
    
    bool rc = psctl.getAbsPosition(&currentTicks);

    if (rc) {
        FocusAbsPosN[0].value = currentTicks;
    }

    m_Motor = static_cast<PS_MOTOR>(psctl.getFocusStatus());

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

//************************************************************
IPState PWRSTR::MoveAbsFocuser(uint32_t targetTicks)
{
    if ( ! psctl.MoveAbsFocuser(targetTicks))
        return IPS_ALERT;

    targetPosition = targetTicks;
    FocusAbsPosNP.s = IPS_BUSY;

    return IPS_BUSY;
}

//************************************************************
IPState PWRSTR::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
    int direction = (dir == FOCUS_INWARD) ? -1 : 1;
    int reversed = (FocusReverseS[INDI_ENABLED].s == ISS_ON) ? -1 : 1;
    int relative = static_cast<int>(ticks);

    int targetAbsPosition = FocusAbsPosN[0].value + (relative * direction * reversed);

    targetAbsPosition = std::min(static_cast<uint32_t>(FocusMaxPosN[0].value),static_cast<uint32_t>(std::max(static_cast<int>(FocusAbsPosN[0].min), targetAbsPosition)));

    return (IPState)psctl.MoveAbsFocuser(targetAbsPosition);
}

//************************************************************
bool PWRSTR::AbortFocuser()
{
    LOG_INFO("Aborting");
    
    return psctl.AbortFocuser();
}

//************************************************************
bool PWRSTR::SyncFocuser(uint32_t ticks)
{
    if ( ! psctl.setAbsPosition(ticks))
        return false;

    targetPosition = ticks;
    simPosition = ticks;

    return psctl.SyncFocuser(ticks);
}

//************************************************************
bool PWRSTR::SetFocuserMaxPosition(uint32_t ticks)
{
    return psctl.SetFocuserMaxPosition(ticks);
}

