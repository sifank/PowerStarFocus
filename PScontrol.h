/********************************************************
*  Program:      PScontrol.h
*  Version:      20201208
*  Author:       Sifan S. Kahale
*  Description:  Power*Star control .h file
*********************************************************/

#pragma once

#include "hidapi.h"
#include <map>
#include <vector>
using namespace std;


#define BITMASK_SET(x,y) ((x) |= (y))
#define BITMASK_CLEAR(x,y) ((x) &= (~(y)))
#define BITMASK_FLIP(x,y) ((x) ^= (y))
#define BITMASK_CHECK_ALL(x,y) (((x) & (y)) == (y))   // warning: evaluates y twice
#define BITMASK_CHECK_ANY(x,y) ((x) & (y)) 

typedef struct {
            char     name[11];
            uint16_t backlash;
            uint8_t  prefDir;          //0:in 1:out
            uint8_t  idleMtrCurrent;
            uint8_t  driveMtrCurrent;
            float    stepPeriod;
            uint32_t maxPosition;
            uint32_t curPosition;
            float    tempCoef;
            float    tempHysterisis;
            uint8_t  tempSensor;       // 0:Disabled 1:Motor 2:Env
            bool     reverseMtr;
            bool     disablePermFocus;
            uint8_t  motorBraking;    // 0:None 1:Low 2:Idle
            uint8_t  motorType;        // 0: Unipolar 1:BiPolar
            uint16_t faultMask;
            char     out1[11];
            char     out2[11];
            char     out3[11];
            char     out4[11];
            char     dew1[11];
            char     dew2[11];
            char     var[11];
            char     mp[11];
            char     usb1[11];
            char     usb2[11];
            char     usb3[11];
            char     usb4[11];
            char     usb5[11];
            char     usb6[11];
} PowerStarProfile;

class PSCTL
{
    public:

        // Power*Star commands
        typedef enum { PS_NOOP,
                   PS_IN = 1,
                   PS_OUT = 2,
                   PS_GOTO = 3,
                   PS_CMD_POS = 4,
                   PS_CMD_MAX = 5,
                   PS_MTR_CMD = 0x10,
                   PS_FAST_IN  = 0x11,
                   PS_GET_STATUS = 0x11,
                   PS_FAST_OUT = 0x12,
                   PS_SET_POS = 0x20,
                   PS_GET_POS = 0x21,
                   PS_SET_MAX = 0x22,
                   PS_GET_MAX = 0x23,
                   PS_SET_HBITS = 0x28,
                   PS_GET_HBITS = 0x29,
                   PS_SET_SPERIOD = 0x30,
                   PS_GET_SPERIOD = 0x31,
                   PS_SET_BACKLASH = 0x32,
                   PS_GET_BACKLASH = 0x33,
                   PS_SET_HYS = 0x34,
                   PS_GET_HYS = 0x35,
                   PS_SET_TMPCO = 0x36,
                   PS_GET_TMPCO = 0x37,
                   PS_SET_TCOMP = 0x38,
                   PS_GET_TCOMP = 0x39,
                   PS_SET_MTRCUR = 0x3a,
                   PS_GET_MTRCUR = 0x3b,
                   PS_GET_WEATHER = 0x43,
                   PS_VERSION = 0x51,
                   PS_SET_MTRPOL = 0x60, // 0=normal, 1=reversed
                   PS_GET_MTRPOL = 0x61,
                   PS_SET_MTRLCK = 0x72,
                   PS_GET_MTRLCK = 0x73,
                   PS_PORT_CTL = 0x80,
                   PS_PORT_STATUS = 0x81,
                   PS_SET_VAR = 0x82,
                   PS_GET_VAR = 0x83,
                   PS_SET_PWM = 0x86,
                   PS_GET_PWM = 0x87,
                   PS_DEW_CTL = 0x90,
                   PS_DEW_STATUS = 0x91,
                   PS_VOLTS = 0xb1,
                   PS_CURRENT = 0xb5,
                   PS_SET_AUTO = 0xc0,
                   PS_GET_AUTO = 0xc1,
                   PS_SET_MTR_LED = 0xC2,
                   PS_GET_MTR_LED = 0xc3,
                   PS_FAULT1 = 0xD1,
                   PS_FAULT2 = 0xD3,
                   PS_SET_ULIMIT = 0xD4,
                   PS_GET_ULIMIT = 0xD5,
                   PS_RESET = 0xee,
                   PS_HALT  = 0xFF
                  } PS_COMMANDS;
                 
        // PS Weather
        typedef enum { PS_TEMP,
                   PS_HUM
                 } PS_WEATHER;
                 
        // PS Move Type
        typedef enum { PS_ABS,
                   PS_MAX
                 } PS_MOVE_TYPE;
       
        // PS Dew Heaters
        typedef enum { PS_DEW1,
                   PS_DEW2
                 } PS_DEW;
        
        PSCTL();

        typedef struct
        {
            bool    state;
            float   current;
            float   levels;
            uint8_t setting;
            bool    autoboot;
            bool    fault1;
            bool    fault2;
        } statusData;
        
        vector<string> Devices = {"Out1", "Out2", "Out3", "Out4",
        "Var", "MP", "USB1", "USB2", "USB3", "USB4", "USB5", "USB6",
        "Dew1", "Dew2", "Temp", "Hum", "IN", "Int", "FM", "Bip", "LED"};

        map <string, statusData> statusMap;
        map <string, statusData> :: iterator itr;
        
        const char *getDefaultName();
        bool    initProperties();
        //void    SetTimer(int POLLMS);
        
        bool    getStatus();

        bool    MoveAbsFocuser(uint32_t targetTicks);
        bool    AbortFocuser();
        bool    SyncFocuser(uint32_t ticks);
        bool    SetFocuserMaxPosition(uint32_t ticks);
        
        bool    Connect();
        bool    Disconnect();

        uint8_t  getFocusStatus();
        uint16_t getPWM();
        uint8_t  getDew(uint8_t device);
        uint32_t getFaultStatus(uint16_t mask);
        void     clearFaultStatus();
        PowerStarProfile    getProfileStatus();

        bool     setDew(uint8_t channel, uint8_t percent);
        bool     setPWM(uint16_t pwmamt);
        bool     setPowerState(string &device, string &action);
        bool     setAutoBoot(string &device, string &action);
        bool     setVar(uint8_t voltage);
        bool     setLED(uint8_t brightness);
        bool     setMultiPort(uint8_t MPtype);
        bool     saveDewPwmFault(PowerStarProfile psProfile);
        uint16_t setUlimit(uint8_t device, uint16_t limit);
        uint16_t getUlimit(uint8_t device);
        
        bool     activateProfile(PowerStarProfile psProfile);

        bool     clearFaults();
        bool     restart();
        bool     lockFocusMtr();
        bool     unLockFocusMtr();
        uint16_t getVersion();
        float    getHumidity();
        float    getTemperature();
        
        // Set/Get Absolute Position
        bool setAbsPosition(uint32_t ticks);
        bool getAbsPosition(uint32_t *ticks);

        // Set/Get Maximum Position
        bool setMaxPosition(uint32_t ticks);
        bool getMaxPosition(uint32_t *ticks);
        
    private:
        
        /**
         * @brief setPosition Set Position (Either Absolute or Maximum)
         * @param ticks desired position
         * @param cmdCode 0x20 to set Absolute position. 0x22 to set Maximum position
         * @return True if successful, false otherwise.
         */
        bool setPosition(uint32_t ticks, uint8_t cmdCode);

        /**
         * @brief getPosition Get Position (Either Absolute or Maximum)
         * @param ticks pointer to store the returned position.
         * @param cmdCode 0x21 to get Absolute position. 0x23 to get Maximum position
         * @return True if successful, false otherwise.
         */
        bool getPosition(uint32_t *ticks, uint8_t cmdCode);
        
        int32_t simPosition { 0 };
        uint32_t targetPosition { 0 };
        uint8_t* response = {0};
        
        bool isConnected;
        
        uint8_t* hidCMD(PS_COMMANDS hcmd, uint8_t hidArg1, uint8_t hidArg2, int numCmd);
        
        hid_device *handle { nullptr };

        // Driver Timeout in ms
        static const uint16_t PS_TIMEOUT { 1000 };
        

};

