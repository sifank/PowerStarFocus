/***************************************************************
*  Program:      PScontrol.cpp
*  Version:      20201216
*  Author:       Sifan S. Kahale
*  Description:  Power*Star control drivers
****************************************************************/

#include "PScontrol.h"
#include <boost/algorithm/string.hpp>
#include <cmath>
#include <cstring>
#include <memory>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <string>
#include <bits/stdc++.h> 
using namespace std;


static std::unique_ptr<PSCTL> psctl(new PSCTL());

PSCTL::PSCTL() {handle = nullptr;}

/**
const std::map<PS_MOTOR, std::string> PSCTL::MotorMap =
{
    {PS_NOT_MOVING, "Idle"},
    {PS_MOVING_IN, "Moving Inwards"},
    {PS_MOVING_OUT, "Moving Outwards"},
    (PS_BUSY, "Busy"},
    {PS_UNK, "Unknown"}
    {PS_LOCKED, "Locked"}
};
**/

 map<string, uint16_t> devmask{ 
        { "out1", 0x0001 }, 
        { "out2", 0x0002 }, 
        { "out3", 0x0004 }, 
        { "out4", 0x0008 },
        { "var",  0x0040 },
        { "mp",   0x0080 },
        
        { "usb1", 0x0100 },
        { "usb2", 0x0200 },
        { "usb3", 0x0400 },
        { "usb4", 0x0800 },
        { "usb5", 0x1000 },
        { "usb6", 0x2000 },
        { "all",  0xfffe }
        }; 
    map<string, uint8_t>::iterator i;

//******************************************************************
bool PSCTL::Connect()
{
    handle = hid_open(0x4D8, 0xEC42, nullptr);

    if (handle == nullptr)
        return false;
    
    // Close the USB (we only connected to test)
    hid_close(handle);
    hid_exit();
    
    unLockFocusMtr();

    return true;
}

//******************************************************************
bool PSCTL::Disconnect()
{
    lockFocusMtr();
    isConnected = false;
    return true;
}

//******************************************************************
// Get Device Status
//******************************************************************

//******************************************************************
// Reports whether ports or usb are on or off
bool PSCTL::getStatus()
{
    // Port Status
    response = hidCMD(PS_PORT_STATUS, 0x00, 0x00, 3);
        
    statusMap["Out1"].state = (response[1] & 0x01);
    statusMap["USB1"].state = (response[2] & 0x01);
    statusMap["Out2"].state = (response[1] & 0x02);
    statusMap["USB2"].state = (response[2] & 0x02);
    statusMap["Out3"].state = (response[1] & 0x04);
    statusMap["USB3"].state = (response[2] & 0x04);
    statusMap["Out4"].state = (response[1] & 0x08);
    statusMap["USB4"].state = (response[2] & 0x08);
    statusMap["Var"].state = (response[1] & 0x40);
    statusMap["USB5"].state = (response[2] & 0x10);
    statusMap["MP"].state = (response[1] & 0x80);
    statusMap["USB6"].state = (response[2] & 0x20);

    // Dew
    response = hidCMD(PS_DEW_STATUS, 0x00, 0x00, 3);
    statusMap["Dew1"].setting = response[2];
    statusMap["Dew1"].state = (response[2] > 0);
    
    response = hidCMD(PS_DEW_STATUS, 0x01, 0x00, 3);
    statusMap["Dew2"].setting = response[2];
    statusMap["Dew2"].state = (response[2] > 0);

    // Voltages
    response = hidCMD(PS_VOLTS, 0, 0x00, 3);
    float INvolts = (response[2] * 256 + response[1]) * 0.014695;
    statusMap["IN"].levels = INvolts;
    
    response = hidCMD(PS_VOLTS, 1, 0x00, 3);
    float VARvolts = (response[2] * 256 + response[1]) * 0.012813;
    statusMap["Var"].levels = VARvolts;
    
    response = hidCMD(PS_VOLTS, 2, 0x00, 3);
    float INTvolts = (response[2] * 256 + response[1]) * 0.004004;
    statusMap["Int"].levels = INTvolts;
    
    // Port Currents
    response = hidCMD(PS_CURRENT, 0, 0x00, 3);
    statusMap["Out1"].current = (response[2] * 256 + response[1]) * 0.075690;
    response = hidCMD(PS_CURRENT, 1, 0x00, 3);
    statusMap["Out2"].current = (response[2] * 256 + response[1]) * 0.075690;
    response = hidCMD(PS_CURRENT, 2, 0x00, 3);
    statusMap["Out3"].current = (response[2] * 256 + response[1]) * 0.010111;
    response = hidCMD(PS_CURRENT, 3, 0x00, 3);
    statusMap["Out4"].current = (response[2] * 256 + response[1]) * 0.010111;
    
    //Dew
    response = hidCMD(PS_CURRENT, 4, 0x00, 3);
    statusMap["Dew1"].current = ((response[2] * 256 + response[1]) * 0.010111) / 100  * statusMap["Dew1"].setting;
    response = hidCMD(PS_CURRENT, 5, 0x00, 3);
    statusMap["Dew2"].current = ((response[2] * 256 + response[1]) * 0.010111) / 100 * statusMap["Dew2"].setting;
    
    response = hidCMD(PS_CURRENT, 6, 0x00, 3);
    statusMap["Var"].current = (response[2] * 256 + response[1]) * 0.010111;
    response = hidCMD(PS_CURRENT, 7, 0x00, 3);
    statusMap["MP"].current = (response[2] * 256 + response[1]) * 0.010111;

    response = hidCMD(PS_CURRENT, 8, 0x00, 3);
    statusMap["IN"].current = (response[2] * 256 + response[1]) * 0.001780;
    
    // Temperature
    response = hidCMD(PS_GET_WEATHER, PS_TEMP, 0x00, 3);
    float curTemp = ((response[2] * 256 + response[1]) / 256) * 9 / 5.0 + 32; // in F
    statusMap["Temp"].levels = curTemp;

    // Humidity
    response = hidCMD(PS_GET_WEATHER, PS_HUM, 0x00, 3);
    statusMap["Hum"].levels = response[1];

    // autoboot
    response = hidCMD(PS_GET_AUTO, 0x00, 0x00, 3);
    
    statusMap["Out1"].autoboot = (response[1] & 0x01);
    statusMap["Out2"].autoboot = (response[1] & 0x02);
    statusMap["Out3"].autoboot = (response[1] & 0x04);
    statusMap["Out4"].autoboot = (response[1] & 0x08);
    statusMap["Dew1"].autoboot = (response[1] & 0x10);
    statusMap["Dew2"].autoboot = (response[1] & 0x20);
    statusMap["Var"].autoboot = (response[1] & 0x40);
    statusMap["MP"].autoboot = (response[1] & 0x80);
    statusMap["USB1"].autoboot = (response[2] & 0x01);
    statusMap["USB2"].autoboot = (response[2] & 0x02);
    statusMap["USB3"].autoboot = (response[2] & 0x04);
    statusMap["USB4"].autoboot = (response[2] & 0x08);
    statusMap["USB5"].autoboot = (response[2] & 0x10);
    statusMap["USB6"].autoboot = (response[2] & 0x20);
    
    // Variable Out
    response = hidCMD(PS_GET_VAR, 0x00, 0x00, 1);
    statusMap["Var"].levels = response[1] / 10.0;

    // Multiport
    response = hidCMD(PS_GET_MTR_LED, 0x00, 0x00, 3);
    statusMap["MP"].setting = response[1] & 0x03;
    statusMap["LED"].setting = (response[1] % 0xf0) >> 4;
    statusMap["FM"].setting = response[2];
    
    return true;
}

//******************************************************************
void PSCTL::clearFaultStatus()
{    
    // Clear statusMap of L1 faults first
        statusMap["IN"].fault1 = 0;
        statusMap["FM"].fault1 = 0;
        statusMap["Bip"].fault1 = 0;
        statusMap["Int"].fault1 = 0;
        statusMap["Temp"].fault1 = 0;
        statusMap["Var"].fault1 = 0;
        statusMap["Out1"].fault1 = 0;
        statusMap["Out2"].fault1 = 0;
        statusMap["Out3"].fault1 = 0;
        statusMap["Out4"].fault1 = 0;
        statusMap["Dew1"].fault1 = 0;
        statusMap["Dew2"].fault1 = 0;
        statusMap["MP"].fault1 = 0;
        statusMap["Out1"].fault2 = 0;
        statusMap["Out2"].fault2 = 0;
        statusMap["Out3"].fault2 = 0;
        statusMap["Out4"].fault2 = 0;
        statusMap["Dew1"].fault2 = 0;
        statusMap["Dew2"].fault2 = 0;
        statusMap["Var"].fault2 = 0;
        statusMap["MP"].fault2 = 0;
        statusMap["IN"].fault2 = 0;
        statusMap["INT"].fault2 = 0;
}
    
//******************************************************************
uint32_t PSCTL::getFaultStatus(uint16_t mask)
{
    clearFaultStatus();
    
    uint32_t retval = 0;
    response = hidCMD(PS_FAULT2, 0x00, 0x00, 3);
    if (response[1] > 0 || response[2] > 0)
    {
        statusMap["Out1"].fault2 = (response[1] & 0x01);
        statusMap["Out2"].fault2 = (response[1] & 0x02);
        statusMap["Out3"].fault2 = (response[1] & 0x04);
        statusMap["Out4"].fault2 = (response[1] & 0x08);
        statusMap["Dew1"].fault2 = (response[1] & 0x10);
        statusMap["Dew2"].fault2 = (response[1] & 0x20);
        statusMap["Var"].fault2 = (response[1] & 0x40);
        statusMap["MP"].fault2 = (response[1] & 0x80);
       
        // byte 2
        statusMap["IN"].fault2 = (response[2] & 0x01);
        statusMap["IN"].fault2 = (response[2] & 0x02);
        statusMap["IN"].fault2 = (response[2] & 0x04);
        statusMap["IN"].fault2 = (response[2] & 0x08);
        statusMap["INT"].fault2 = (response[2] & 0x10);
        statusMap["INT"].fault2 = (response[2] & 0x20);
        statusMap["INT"].fault2 = (response[2] & 0x40);
        //bit 7 is unused;
        
        retval = (response[2] << 8) + response[1];
    }
    
    // get and report level 1 faults
    response = hidCMD(PS_FAULT1, (mask & 0x00ff), ((mask & 0xff00) >> 8),3);
    if (response[1] > 0 || response[2] > 0)
    {
        // byte1
        statusMap["IN"].fault1 = (response[1] & 0x02);
        statusMap["IN"].fault1 = (response[1] & 0x04);
        statusMap["FM"].fault1 = (response[1] & 0x08);
        statusMap["Bip"].fault1 = (response[1] & 0x10);
        statusMap["Int"].fault1 = (response[1] & 0x20);
        statusMap["Temp"].fault1 = (response[1] & 0x40);
        statusMap["Var"].fault1 = (response[1] & 0x80);
        // byte 2
        statusMap["Out1"].fault1 = (response[2] & 0x01);
        statusMap["Out2"].fault1 = (response[2] & 0x02);
        statusMap["Out3"].fault1 = (response[2] & 0x04);
        statusMap["Out4"].fault1 = (response[2] & 0x08);
        statusMap["Dew1"].fault1 = (response[2] & 0x10);
        statusMap["Dew2"].fault1 = (response[2] & 0x20);
        statusMap["MP"].fault1 = (response[2] & 0x40);
        statusMap["FM"].fault1 = (response[2] & 0x80);
        
        retval = (retval << 16) + (response[2] << 8) + response[1];
    }
    
    return retval;
}

//***************************************************************
void PSCTL::getUserLimitStatus(float usrlimit[12]) 
{
    usrlimit[0] = getUlimit(0) * .014595;
    usrlimit[1] = getUlimit(1) * .014595;
    usrlimit[2] = getUlimit(2) * .0128128;
    usrlimit[3] = getUlimit(3) * .0128128;
    usrlimit[4] = getUlimit(4) / 11.23876;
    usrlimit[5] = getUlimit(5) / 13.21179;
    usrlimit[6] = getUlimit(6) / 13.21179;
    usrlimit[7] = getUlimit(7) / 98.9;
    usrlimit[8] = getUlimit(8) / 98.9;
    usrlimit[9] = getUlimit(9) / 98.9;
    usrlimit[10] = getUlimit(10) / 98.9;
    usrlimit[11] = getUlimit(11) / 98.9;
}

//***************************************************************
void PSCTL::setUserLimitStatus(float usrlimit[12]) 
{
    setUlimit(0, (uint8_t)(usrlimit[0] / .014595 / 4));
    setUlimit(1, (uint8_t)(usrlimit[1] / .014595 / 4));
    setUlimit(2, (uint8_t)(usrlimit[2] / .0128128 / 4));
    setUlimit(3, (uint8_t)(usrlimit[3] / .0128128 / 4));
    setUlimit(4, (uint8_t)(usrlimit[4] * 11.23876));
    setUlimit(5, (uint8_t)(usrlimit[5] * 13.21179));
    setUlimit(6, (uint8_t)(usrlimit[6] * 13.21179));
    setUlimit(7, (uint8_t)(usrlimit[7] * 98.9 / 4));
    setUlimit(8, (uint8_t)(usrlimit[8] * 98.9 / 4));
    setUlimit(9, (uint8_t)(usrlimit[9] * 98.9 / 4));
    setUlimit(10, (uint8_t)(usrlimit[10] * 98.9 / 4));
    setUlimit(11, (uint8_t)(usrlimit[11] * 98.9 / 4));
}

//***************************************************************
PowerStarProfile PSCTL::getProfileStatus() 
{
    PowerStarProfile actProfile;
    strncpy(actProfile.name, "Actual", sizeof(actProfile.name));
    
    response = hidCMD(PS_GET_BACKLASH, 0, 0, 3);
    actProfile.backlash = response[1]; 
    actProfile.prefDir = response[2];
    
    response = hidCMD(PS_GET_MTRCUR, 0, 0, 3);
    actProfile.idleMtrCurrent = response[1];
    actProfile.driveMtrCurrent = response[2];
    
    response = hidCMD(PS_GET_SPERIOD, 0, 0, 2);
    actProfile.stepPeriod = response[1] / 10;
    
    getPosition(&actProfile.curPosition, PS_GET_POS);
    getPosition(&actProfile.maxPosition, PS_GET_MAX);

    response = hidCMD(PS_GET_TMPCO, 0, 0, 3);
    float lbyte = response[1];
    actProfile.tempCoef = response[2] + (lbyte / 256);
    
    response = hidCMD(PS_GET_HYS, 0, 0, 2);
    actProfile.tempHysterisis = response[1] / 10;
    
    response = hidCMD(PS_GET_TCOMP, 0, 0, 2);
    actProfile.tempSensor = response[1];
    
    response = hidCMD(PS_GET_MTRPOL, 0, 0, 2);
    actProfile.reverseMtr = response[1];
    
    actProfile.disablePermFocus = 0;
    
    response = hidCMD(PS_GET_MTRPOL, 0, 0, 3);
    actProfile.motorBraking = 0;    // 0:None 1:Low 2:Normal
    
    response = hidCMD(PS_GET_MTR_LED, 0, 0, 3);
    actProfile.motorType = response[2];
    
    response = hidCMD(PS_GET_MTRLCK, 0, 0, 3);
    actProfile.faultMask = 0;
    strncpy(actProfile.out1, "", sizeof(actProfile.out1));
    strncpy(actProfile.out2, "", sizeof(actProfile.out2));
    strncpy(actProfile.out3, "", sizeof(actProfile.out3));
    strncpy(actProfile.out4, "", sizeof(actProfile.out4));
    strncpy(actProfile.dew1, "", sizeof(actProfile.dew1));
    strncpy(actProfile.dew2, "", sizeof(actProfile.dew2));
    strncpy(actProfile.var, "", sizeof(actProfile.var));
    strncpy(actProfile.mp, "", sizeof(actProfile.mp));
    strncpy(actProfile.usb1, "", sizeof(actProfile.usb1));
    strncpy(actProfile.usb2, "", sizeof(actProfile.usb2));
    strncpy(actProfile.usb3, "", sizeof(actProfile.usb3));
    strncpy(actProfile.usb4, "", sizeof(actProfile.usb4));
    strncpy(actProfile.usb5, "", sizeof(actProfile.usb5));
    strncpy(actProfile.usb6, "", sizeof(actProfile.usb6));
    
    
    return actProfile;
}

//******************************************************************
// Set Devices
//******************************************************************

//******************************************************************
// Turns ports/usb on or off
bool PSCTL::setPowerState(const string &device, const string &action)
{
    uint8_t portCtl;
    uint8_t usbCtl;
    
    response = hidCMD(PS_PORT_STATUS, 0x00, 0x00, 3);
    uint16_t portStatus = response[2] * 256 + response[1];
    
    if (devmask.find(device) == devmask.end())
        return false;
        
    if (action == "yes")
        BITMASK_SET(portStatus, devmask[device]);

    else if (action == "no") 
        BITMASK_CLEAR(portStatus, devmask[device]);

    else
        return false;
        
    BITMASK_CLEAR(portStatus, 0xc030);
    portCtl = portStatus & 0xFF;
    usbCtl = (portStatus & 0xFF00) >> 8;
        
    response = hidCMD(PS_PORT_CTL, portCtl, usbCtl, 3);
        
    if (response[1] == 0xff || response[2] == 0xff)
        return false;

    return true;
        
}

//**************************************************************
bool PSCTL::setDew(uint8_t channel, uint8_t percent)
{
    response = hidCMD(PS_DEW_CTL, channel, percent, 3);
    if (response[2] == 0xff) {
        return false;
    }
    return true;
}

//**************************************************************
bool PSCTL::setUlimit(uint8_t device, uint8_t adcLimit)
{
    response = hidCMD(PS_SET_ULIMIT, device, adcLimit, 3);

    return true;
}

//**************************************************************
uint16_t PSCTL::getUlimit(uint8_t device)
{
    response = hidCMD(PS_GET_ULIMIT, device, 0x00, 3);
    return response[2] * 256 + response[1];
}

//**************************************************************
bool PSCTL::setPWM(uint16_t pwmamt)
{
    uint8_t pwmlow = pwmamt & 0x00ff;
    uint8_t pwmhigh = (pwmamt & 0xff00) / 256;

    response = hidCMD(PS_SET_PWM, pwmlow, pwmhigh, 3);
    if (response[2] == 0xff) {
        return false;
    }
    return true;
}

//******************************************************************
// set the voltage for the variable output port
bool PSCTL::setVar(uint8_t voltage)
{
    response = hidCMD(PS_SET_VAR, voltage, 0x00, 2);
    if (response[1] == 0xff) {
        return false;
    }
    else
        return true;
}

//******************************************************************
// get the pwm duty cycle for MP
uint16_t PSCTL::getPWM()
{
    response = hidCMD(PS_GET_PWM, 0x00, 0x00, 2);
    return response[2] * 256 + response[1];
}

//******************************************************************
// get the dew %
uint8_t PSCTL::getDew(uint8_t device)
{
    // 0 = dew1, 1 = dew2, 2 = MP if set to dew
    response = hidCMD(PS_DEW_STATUS, device, 0x00, 2);
    return response[2];
}

//******************************************************************
// sets autoboot options
bool PSCTL::setAutoBoot(string &device, string &action)
{
    uint8_t portCtl;
    uint8_t usbCtl;
    
    response = hidCMD(PS_GET_AUTO, 0x00, 0x00, 3);
    if (response[2] == 0xff) {
        return false;
    }
    uint16_t portStatus = response[2] * 256 + response[1];

    if (devmask.find(device) != devmask.end()) {
        
        if (action == "on") {
            BITMASK_SET(portStatus, devmask[device]);
            portCtl = portStatus & 0xFF;
            usbCtl = (portStatus & 0xFF00) >> 8;
        }
        else if (action == "off") {
            BITMASK_CLEAR(portStatus, devmask[device]);
            portCtl = portStatus & 0xFF;
            usbCtl = (portStatus & 0xFF00) >> 8;
        }
        
        else {
            return false;
        }
        
        response = hidCMD(PS_SET_AUTO, portCtl, usbCtl, 3);
        if (response[2] == 0xff) {
            return false;
        }
        else {
            return true;
        }
    }
    else {
        return false;
    }
    
    return true;
}

//******************************************************
//MPtype: 0=DC, 1=PWM, 2=Dew
bool PSCTL::setMultiPort(uint8_t MPtype)
{
    response = hidCMD(PS_GET_MTR_LED, 0x00, 0x00, 3);  //get current settings
    
    uint8_t bcmd = ((MPtype & 0x0f) | (response[1] & 0xf0));
    
    response = hidCMD(PS_SET_MTR_LED, bcmd, response[2], 3);
    if (response[1] == 0xff) {
        return false;
    }
    return true;
}
    
//**********************************************************
// brightness: 0=off, 1-5 level
bool PSCTL::setLED(uint8_t brightness)
{
    //get current settings
    response = hidCMD(PS_GET_MTR_LED, 0x00, 0x00, 3);  
    
    uint8_t bcmd = ((brightness << 4) | (response[1] & 0x0f));
    
    response = hidCMD(PS_SET_MTR_LED, bcmd, response[2], 3);
    if (response[1] == 0xff) {
        return false;
    }
    return true;
}

//***************************************************************
bool PSCTL::activateProfile(PowerStarProfile psProfile)
{    
    // Set Motor Type
    // need to read in status first, then set mtr type and put back
    //keep response[1] as that sets Mp and LED modes
    response = hidCMD(PS_GET_MTR_LED, 0x00, 0x00, 3);
    response = hidCMD(PS_SET_MTR_LED, response[1], psProfile.motorType, 3);
    if (response[1] == 0xff)
        return false;
    
    // Set reverse motor
    response = hidCMD(PS_SET_MTRPOL, psProfile.reverseMtr, 0x00, 2);
    if (response[1] == 0xff)
        return false;
    
    // Backlash amount and preferred direction
    hidCMD(PS_SET_BACKLASH, psProfile.backlash, psProfile.prefDir, 3);
    
    // Unlocking the Motor
    response = hidCMD(PS_SET_MTRLCK, 0x5a, psProfile.motorBraking, 3);
    if (response[1] == 0xff)
        return false;
    
    // Set temperature compensation 0=disabled, 1=motor, 2=env
    response = hidCMD(PS_SET_TCOMP, psProfile.tempSensor, 0x00, 2);
    if (response[1] == 0xff)
        return false;
    
    // Temp compensation temperature coefficient
    uint8_t hbyte = (psProfile.tempCoef);
    uint8_t lbyte = (psProfile.tempCoef - hbyte) * 256;
    hidCMD(PS_SET_TMPCO, lbyte, hbyte, 3);
    
    // Temp compensation hysteresis
    response = hidCMD(PS_SET_HYS, (uint8_t)(psProfile.tempHysterisis * 10), 0x00, 2);
    if (response[1] == 0xff)
        return false;

    // Step Period
    response = hidCMD(PS_SET_SPERIOD, (uint8_t)(psProfile.stepPeriod * 10), 0x00, 2);
    if (response[1] == 0xff)
        return false;
        
    // Motor idle and drive current
    response = hidCMD(PS_SET_MTRCUR, psProfile.idleMtrCurrent, psProfile.idleMtrCurrent, 3);
    if (response[1] == 0xff || response[2] == 0xff)
        return false;
    
    if ( ! saveDewPwmFault(psProfile))
        return false;
    
    return true;
}
    
//****************************************************************
bool PSCTL::saveDewPwmFault(PowerStarProfile psProfile)
{
    // save dew, pwm and fault maps to nvm
    response = hidCMD(PS_SET_MTRLCK, 0xaa, (uint8_t)(psProfile.motorBraking * 10), 3);
    if (response[1] == 0xff)
        return false;
    
    return true;
}
    
//****************************************************************
// Maintenance procedures
//****************************************************************

//****************************************************************
// Get Version
uint16_t PSCTL::getVersion(){
    response = hidCMD(PS_VERSION, 0x00, 0x00, 1);
    return (response[2] *256) + response[1];
}

//******************************************************************
// Get Temperature
float PSCTL::getTemperature()
{
    uint8_t* response = hidCMD(PS_GET_WEATHER, PS_TEMP, 0x00, 3);
    float curTemp = ((response[2] * 256 + response[1]) / 256) * 9 / 5.0 + 32; // in F
    return curTemp;
}

//******************************************************************
// Get Humidity
float PSCTL::getHumidity()
{
    uint8_t* response = hidCMD(PS_GET_WEATHER, PS_HUM, 0x00, 3);
    float curhum = response[2] * 256 + response[1];
    return curhum;
}

//******************************************************************
// Clears faults
bool PSCTL::clearFaults()
{
    response = hidCMD(PS_FAULT2, 0x01, 0x00, 2);
    if (response[1] == 0xff )
        return false;
    else
        return true;
}

//******************************************************************
// Restarts PS
bool PSCTL::restart()
{
    response = hidCMD(PS_RESET, 0xa5, 0x5a, 3);
    if (response[1] == 0xff )
        return false;
    else
        return true;
}

//************************************************
uint8_t* PSCTL::hidCMD(PS_COMMANDS hcmd, uint8_t hidArg1, uint8_t hidArg2, int numCmd)
{
    int rc       = 0;
    static uint8_t hRes[3] = {0};
    uint8_t hidcmd[3] = {0};
    hidcmd[0] = hcmd;
    hidcmd[1] = hidArg1;
    hidcmd[2] = hidArg2;
    
    handle = hid_open(0x4D8, 0xEC42, nullptr);
    if (handle == nullptr) {
        hRes[0] = 0xFF;
        hid_close(handle);
        hid_exit();
        return hRes;
    }

    rc = hid_write(handle, hidcmd, numCmd);

    if (rc < 0)
    {
        hRes[0] = 0xff;
        hid_close(handle);
        hid_exit();
        return hRes;
    }

    rc = hid_read_timeout(handle, hRes, 3, PS_TIMEOUT);
    if (rc < 0)
    {
        hRes[0] = 0xff;
        hid_close(handle);
        hid_exit();
        return hRes;
    }
    
    // Close out the USB
    hid_close(handle);
    hid_exit();
    return hRes;
}

//******************************************************************
// Focus functions
//******************************************************************

//******************************************************************
bool PSCTL::MoveAbsFocuser(uint32_t targetTicks)
{
    bool rc = setAbsPosition(targetTicks);

    if (!rc)
        return false;

    targetPosition = targetTicks;
    
    uint8_t* hrc = hidCMD(PS_MTR_CMD, PS_GOTO, 0x00, 2);

    if (hrc < 0)
        return false;

    return true;
}

//******************************************************************
bool PSCTL::setPosition(uint32_t ticks, uint8_t cmdCode)
{    
    uint8_t setTicks1;
    uint8_t setTicks2;

    // 20 bit resolution position. 4 high bits + 16 lower bits
    // Send 4 high bits first
    setTicks1 = (ticks & 0x40000) >> 16;


    response = hidCMD(PS_SET_HBITS, setTicks1, 0x00, 2);
    
    if ( response[1] == 0xff )
    {
        return false;
    }

    // Send lower 16 bit
    setTicks1 = ticks & 0xFF;             // Low Byte
    setTicks2 = (ticks & 0xFF00) >> 8;    // High Byte

    response = hidCMD(PS_SET_POS, setTicks1, setTicks2, 3);

    targetPosition = ticks;

    return true;
}

//******************************************************************
/*
 * input to function: 0x21=abs. 0x23=maximumPosition
 * but read cmd is alwyas 0x21
 * must translate for 2nd byte in cmd
 */
bool PSCTL::getPosition(uint32_t *ticks, uint8_t cmdCode)
{
    uint32_t pos = 0;
    uint8_t posType;

    // 20 bit resolution position. 4 high bits + 16 lower bits
    if (cmdCode == PS_GET_POS)
        posType = PS_ABS; //get abs position
    else
        posType = PS_MAX; //get max position

    response = hidCMD(PS_GET_HBITS, posType, 0x00, 2);

    // Store 4 high bits part of a 20 bit number
    pos = response[1] << 16;

    // Get 16 lower bits
    if (cmdCode == PS_GET_POS)
        posType = PS_ABS; //get abs position
    else
        posType = PS_MAX; //get max position

    response = hidCMD(PS_GET_POS, posType, 0x00, 3);

    // response[1] is lower byte and response[2] is high byte. Combine and add to ticks.
    pos |= response[1] | response[2] << 8;

    *ticks = pos;

    return true;
}

//******************************************************************
bool PSCTL::setAbsPosition(uint32_t ticks)
{
    return setPosition(ticks, PS_SET_POS);
}

//******************************************************************
bool PSCTL::getAbsPosition(uint32_t *ticks)
{    
    return getPosition(ticks, PS_GET_POS);
}

//******************************************************************
bool PSCTL::setMaxPosition(uint32_t ticks)
{
    return setPosition(ticks, PS_SET_MAX);
}

//******************************************************************
bool PSCTL::getMaxPosition(uint32_t *ticks)
{
    return getPosition(ticks, PS_GET_MAX);
}

//******************************************************************
uint8_t PSCTL::getFocusStatus()
{
    response = hidCMD(PS_GET_STATUS, 0x00, 0x00, 1);

    if (response[1] > 5)
        response[1] = 4;

    return response[1];
}

//******************************************************************
bool PSCTL::AbortFocuser()
{    
    uint8_t* hres = hidCMD(PS_MTR_CMD, PS_HALT, 0x00, 2);
    if (hres[1] == 0)
        return true;
    else
        return false;
}

//******************************************************************
bool PSCTL::SyncFocuser(uint32_t ticks)
{
    bool rc = setAbsPosition(ticks);

    if (!rc)
        return false;

    simPosition = ticks;

    uint8_t* hrc = hidCMD(PS_MTR_CMD, PS_CMD_POS, 0x00, 2);

    if (hrc[1] == 0)
        return true;
    else
        return false;
}

//******************************************************************
bool PSCTL::SetFocuserMaxPosition(uint32_t ticks)
{
    bool rc = setMaxPosition(ticks);


    if (!rc)
        return false;
    
    uint8_t* hrc = hidCMD(PS_MTR_CMD, PS_CMD_MAX, 0x00, 2);

    if (hrc[1] == 0)
        return true;
    else
        return false;
}

//******************************************************************
bool PSCTL::lockFocusMtr()
{
    response = hidCMD(PS_SET_MTRLCK, 0xa5, 0x00, 3);
    if (response[1] == 0xff)
        return false;
    
    return true;
}

//******************************************************************
bool PSCTL::unLockFocusMtr()
{
    response = hidCMD(PS_SET_MTRLCK, 0x5a, 0x02, 3);
    if (response[1] == 0xff)
        return false;
    
    return true;
}





