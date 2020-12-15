 /*******************************************************************************
  Copyright(c) 2019 Jasem Mutlaq. All rights reserved.

 (Originally) Starlight Instruments EFS Focuser

 Modified by Sifan Kahale (2020) for Starlight's PowerHub

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
 
 Modified by:  Sifan Kahale for PowerHub controller 20200718
 Changed:
    HID PID 
    Accept m_Motor of -3
*******************************************************************************/

#include "powerhub.h"

#include <cmath>
#include <cstring>
#include <memory>

#define FOCUS_SETTINGS_TAB "Settings"

static std::unique_ptr<PWRHB> pwrhb(new PWRHB());

const std::map<PWRHB::PH_COMMANDS, std::string> PWRHB::CommandsMap =
{
    {PWRHB::PH_NOOP, "No Operation"},
    {PWRHB::PH_IN, "Moving Inwards"},
    {PWRHB::PH_OUT, "Moving Outwards"},
    {PWRHB::PH_GOTO, "Goto"},
    {PWRHB::PH_CMD_POS, "Set Position"},
    {PWRHB::PH_CMD_MAX, "Set Max Position"},
    {PWRHB::PH_FAST_IN, "Fast In"},
    {PWRHB::PH_FAST_OUT, "Fast Out"},
    {PWRHB::PH_HALT, "Halt"},
};

const std::map<PWRHB::PH_MOTOR, std::string> PWRHB::MotorMap =
{
    {PWRHB::PH_NOT_MOVING, "Idle"},
    {PWRHB::PH_MOVING_IN, "Moving Inwards"},
    {PWRHB::PH_MOVING_OUT, "Moving Outwards"},
    {PWRHB::PH_LOCKED, "Locked"},
    {PH_UNK, "Busy"},
};

void ISGetProperties(const char *dev)
{
    pwrhb->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    pwrhb->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    pwrhb->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    pwrhb->ISNewNumber(dev, name, values, names, n);
}

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

void ISSnoopDevice(XMLEle *root)
{
    pwrhb->ISSnoopDevice(root);
}

PWRHB::PWRHB()
{
    setVersion(0, 6);

    FI::SetCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | FOCUSER_CAN_ABORT | FOCUSER_CAN_SYNC);
    setSupportedConnections(CONNECTION_NONE);
}

bool PWRHB::Connect()
{
    if (isSimulation())
    {
        SetTimer(POLLMS);
        return true;
    }

    handle = hid_open(0x4D8, 0xEC42, nullptr);

    if (handle == nullptr)
    {
        LOG_ERROR("No PowerHub focuser found.");
        return false;
    }
    else
    {
        uint32_t maximumPosition = 0;
        bool rc = getMaxPosition(&maximumPosition);
        if (rc)
        {
            FocusMaxPosN[0].value = maximumPosition;

            FocusAbsPosN[0].max = FocusSyncN[0].max = FocusMaxPosN[0].value;
            FocusAbsPosN[0].step = FocusSyncN[0].step = FocusMaxPosN[0].value / 50.0;
            FocusAbsPosN[0].min = FocusSyncN[0].min = 0;

            FocusRelPosN[0].max  = FocusMaxPosN[0].value / 2;
            FocusRelPosN[0].step = FocusMaxPosN[0].value / 100.0;
            FocusRelPosN[0].min  = 0;
        }

        SetTimer(POLLMS);
        
        uint8_t* hres = hidCMD(PH_VERSION, 0x00, 0x00, 1);
        LOGF_INFO("Version: %i.%i", hres[1], hres[2]);
    
        LOG_INFO("Set MP and Motor Type");
        hidCMD(PH_MTR_TYP, 0x00, 0x00, 3);
    
        LOG_INFO("Unlocking the Motor");
        hidCMD(PH_MTR_LCK, 0x5a, 0x02, 3);
        
        LOG_INFO("Turn off temperature compensation");
        hidCMD(PH_COMP, 0x00, 0x00, 2);
    
        //LOG_INFO("Turning on all port 2, 4 and usb");
        //hidCMD(PH_PORT_CTL, 0xcf, 0x3f, 3);
    
        //LOG_INFO("Turning on the Dew Heaters");
        //hidCMD(PH_DEW_CTL, PH_DEW1, 100, 3);
        //hidCMD(PH_DEW_CTL, PH_DEW2, 100, 3);
    
        LOGF_INFO("Humidity @ opening: %#.1f", getHumidity());
        LOGF_INFO("Temperature @ opening: %#.1f", getTemperature());
    
    }
    
    return (handle != nullptr);
}

bool PWRHB::Disconnect()
{
    if (isSimulation() == false)
    {
        //LOG_INFO("Locking the Motor");
        //hidCMD(PH_MTR_LCK, 0xa5, 0x00, 3);
    
        //LOG_INFO("Turning off all ports (except p1) and usb");
        //hidCMD(PH_PORT_CTL, 0x01, 0x00, 3);
        
        //LOG_INFO("Turning off the Dew Heaters");
        //hidCMD(PH_DEW_CTL, PH_DEW1, 0, 3);
        //hidCMD(PH_DEW_CTL, PH_DEW2, 0, 3);
        
        LOGF_INFO("Humidity @ closing: %#.1f", getHumidity());
        LOGF_INFO("Temperature @ closing: %#.1f", getTemperature());
        
        // Close out the USB
        hid_close(handle);
        hid_exit();
    }

    return true;
}

const char *PWRHB::getDefaultName()
{
    return "PowerHub";
}

bool PWRHB::initProperties()
{
    INDI::Focuser::initProperties();

    addSimulationControl();
    
    addDebugControl();
    
    return true;
}

void PWRHB::TimerHit()
{
    if (!isConnected())
        return;
    
    // check for faults    
    uint8_t* hrc = hidCMD(PH_FAULT1, 0xff, 0x7f, 3);
    if (hrc[1] > 0 || hrc[2] > 0)
        LOGF_ERROR("Level 1 fault: B1 %i  B2 %i", hrc[1], hrc[2]);

    hrc = hidCMD(PH_FAULT2, 0x00, 0x00, 2);
    if (hrc[1] > 0)
        LOGF_ERROR("Level 2 fault: B1 %i  B2 %i", hrc[1], hrc[2]);

    uint32_t currentTicks = 0;

    bool rc = getAbsPosition(&currentTicks);

    if (rc)
        FocusAbsPosN[0].value = currentTicks;

    getStatus();

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
                m_Motor = PH_NOT_MOVING;
            }

            FocusAbsPosN[0].value = simPosition;
        }

        if (m_Motor == PH_NOT_MOVING && targetPosition == FocusAbsPosN[0].value)
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

IPState PWRHB::MoveAbsFocuser(uint32_t targetTicks)
{
    bool rc = setAbsPosition(targetTicks);

    if (!rc)
        return IPS_ALERT;

    targetPosition = targetTicks;
    
    uint8_t* hrc = hidCMD(PH_MTR_CMD, PH_GOTO, 0x00, 2);

    if (hrc < 0)
        return IPS_ALERT;

    FocusAbsPosNP.s = IPS_BUSY;

    return IPS_BUSY;
}

IPState PWRHB::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
    int direction = (dir == FOCUS_INWARD) ? -1 : 1;
    int reversed = (FocusReverseS[INDI_ENABLED].s == ISS_ON) ? -1 : 1;
    int relative = static_cast<int>(ticks);

    int targetAbsPosition = FocusAbsPosN[0].value + (relative * direction * reversed);

    targetAbsPosition = std::min(static_cast<uint32_t>(FocusMaxPosN[0].value),static_cast<uint32_t>(std::max(static_cast<int>(FocusAbsPosN[0].min), targetAbsPosition)));

    return MoveAbsFocuser(targetAbsPosition);
}

bool PWRHB::setPosition(uint32_t ticks, uint8_t cmdCode)
{
    uint8_t setTicks1;
    uint8_t setTicks2;
    uint8_t* response;

    // 20 bit resolution position. 4 high bits + 16 lower bits
    // Send 4 high bits first
    setTicks1 = (ticks & 0x40000) >> 16;

    LOGF_DEBUG("Set High bits command: <%02X %02X> Set %s lower (%ld)", PH_SET_HBITS, setTicks1, cmdCode == 0x20 ? "Absolute" : "Maximum", ticks);

    if (isSimulation())
    {
        response[0] = PH_SET_HBITS;
        response[1] = setTicks1;
    }
    else
        response = hidCMD(PH_SET_HBITS, setTicks1, 0x00, 2);
    
    if ( response[1] == 0xff )
    {
        LOG_ERROR("Error setting focus position");
        return false;
    }

    LOGF_DEBUG("Set High bits result: <%02X %02X>", response[0], response[1]);

    // Send lower 16 bit
    setTicks1 = ticks & 0xFF;             // Low Byte
    setTicks2 = (ticks & 0xFF00) >> 8;    // High Byte

    LOGF_DEBUG("Set Low bit command: <%02X %02X %02X>", PH_SET_POS, setTicks1, setTicks2);

    if (isSimulation())
    {
        response[0] = PH_SET_POS;
        response[1] = setTicks1;
        response[2] = setTicks2;
    }
    else
        response = hidCMD(PH_SET_POS, setTicks1, setTicks2, 3);

    LOGF_DEBUG("Set low bits result <%02X %02X %02X>", response[0], response[1], response[2]);

    targetPosition = ticks;

    return true;
}

/*
 * input to function: 0x21=abs. 0x23=maximumPosition
 * but read cmd is alwyas 0x21
 * must translate for 2nd byte in cmd
 */
bool PWRHB::getPosition(uint32_t *ticks, uint8_t cmdCode)
{
    uint32_t pos = 0;
    uint8_t posType;
    uint8_t* response;

    // 20 bit resolution position. 4 high bits + 16 lower bits
    if (cmdCode == PH_GET_POS)
        posType = PH_ABS; //get abs position
    else
        posType = PH_MAX; //get max position
        
    // Get 4 high bits first
    LOGF_DEBUG("Command %s getPosition (High 4 bits): <%02X %02X>", cmdCode == PH_GET_POS ? "Absolute" : "Maximum", PH_GET_HBITS, posType);

    if (isSimulation())
    {
        response[0] = PH_GET_HBITS;
        response[1] = simPosition >> 16;
    }
    else
        response = hidCMD(PH_GET_HBITS, posType, 0x00, 2);

    LOGF_DEBUG("Data getPosition High bits: <%02X %02X>", response[0], response[1]);

    // Store 4 high bits part of a 20 bit number
    pos = response[1] << 16;

    // Get 16 lower bits
    if (cmdCode == PH_GET_POS)
        posType = PH_ABS; //get abs position
    else
        posType = PH_MAX; //get max position
        
    LOGF_DEBUG("Command Get %s Position (Lower 16 bits): <%02X %02X>", cmdCode == PH_GET_POS ? "Absolute" : "Maximum", PH_GET_POS, posType);

    if (isSimulation())
    {
        //rc          = PH_GET_POS;
        response[0] = PH_GET_POS;
        response[1] = simPosition & 0xFF;
        response[2] = (simPosition & 0xFF00) >> 8;
    }
    else
        response = hidCMD(PH_GET_POS, posType, 0x00, 3);

    LOGF_DEBUG("Data: getPosition Low bits: <%02X %02X %02X>", response[0], response[1], response[2]);

    // response[1] is lower byte and response[2] is high byte. Combine and add to ticks.
    pos |= response[1] | response[2] << 8;

    *ticks = pos;

    LOGF_DEBUG("%s Position: %ld", cmdCode == PH_GET_POS ? "Absolute" : "Maximum", pos);

    return true;
}

bool PWRHB::setAbsPosition(uint32_t ticks)
{
    return setPosition(ticks, PH_GET_POS);
}

bool PWRHB::getAbsPosition(uint32_t *ticks)
{
    return getPosition(ticks, PH_GET_POS);
}

bool PWRHB::setMaxPosition(uint32_t ticks)
{
    return setPosition(ticks, PH_GET_POS);
}

bool PWRHB::getMaxPosition(uint32_t *ticks)
{
    return getPosition(ticks, PH_GET_POS);
}

float PWRHB::getTemperature()
{
    uint8_t* response = hidCMD(PH_GET_WEATHER, PH_TEMP, 0x00, 3);
    float curTemp = ((response[2] * 256 + response[1]) / 256) * 9 / 5.0 + 32; // in F
    return curTemp;
}

float PWRHB::getHumidity()
{
    uint8_t* response = hidCMD(PH_GET_WEATHER, PH_HUM, 0x00, 3);
    float curhum = response[2] * 256 + response[1];
    return curhum;
}

uint8_t* PWRHB::hidCMD(PH_COMMANDS hcmd, uint8_t hidArg1, uint8_t hidArg2, int numCmd)
{
    int rc       = 0;
    static uint8_t hRes[3] = {0};
    uint8_t hidcmd[3] = {0};
    hidcmd[0] = hcmd;
    hidcmd[1] = hidArg1;
    hidcmd[2] = hidArg2;

    LOGF_DEBUG("Sending Command: <%i>", hcmd);

    rc = hid_write(handle, hidcmd, numCmd);

    if (rc < 0)
    {
        LOGF_ERROR("hidCMD: Error writing to device (%s)", hid_error(handle));
        return hRes;
    }

    rc = hid_read_timeout(handle, hRes, 3, PH_TIMEOUT);
    if (rc < 0)
    {
        LOGF_ERROR("hidCMD: Error reading from device (%s)", hid_error(handle));
        return hRes;
    }
    LOGF_DEBUG("Command %i Result: %i %i", hRes[0], hRes[1], hRes[2]);
    return hRes;
}

uint8_t PWRHB::getStatus()
{    
    uint8_t* hrc = {0};

    // need to check for error
    hrc = hidCMD(PH_GET_STATUS, 0x00, 0x00, 1);

    if (isSimulation())
    {
        hrc[0] = PH_GET_STATUS;
        hrc[1] = m_Motor;
    }

    LOGF_DEBUG("getStatus result: <%02X %02X>", hrc[0], hrc[1]);

    m_Motor = static_cast<PH_MOTOR>(hrc[1]);
    if (MotorMap.count(m_Motor) == uint32_t(PH_UNK))
        LOGF_WARN("Warning: Ignoring status (%d)", hrc[1]);
        return m_Motor;
    
    if (MotorMap.count(m_Motor) > 0)
        LOGF_INFO("State: %s", MotorMap.at(m_Motor).c_str());
    else
    {
        LOGF_WARN("Warning: Unknown status (%f)", hrc[1]);
        return m_Motor;
    }

    return m_Motor;
}

bool PWRHB::AbortFocuser()
{
    LOG_INFO("Aborting");
    LOGF_INFO("Current humidity: %#.1f", getHumidity());
    LOGF_INFO("Current temperature: %#.1f", getTemperature());
    
    uint8_t* hres = hidCMD(PH_MTR_CMD, PH_HALT, 0x00, 2);
    if (hres[1] == 0)
        return true;
    else
        return false;
}

bool PWRHB::SyncFocuser(uint32_t ticks)
{
    bool rc = setAbsPosition(ticks);

    if (!rc)
        return false;

    simPosition = ticks;

    uint8_t* hrc = hidCMD(PH_MTR_CMD, PH_CMD_POS, 0x00, 2);

    if (hrc[1] == 0)
        return true;
    else
        return false;
}

bool PWRHB::SetFocuserMaxPosition(uint32_t ticks)
{
    bool rc = setMaxPosition(ticks);


    if (!rc)
        return false;
    
    uint8_t* hrc = hidCMD(PH_MTR_CMD, PH_CMD_MAX, 0x00, 2);

    if (hrc[1] == 0)
        return true;
    else
        return false;
}

