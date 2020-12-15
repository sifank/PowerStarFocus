/*******************************************************************************
  Copyright(c) 2019 Jasem Mutlaq. All rights reserved.

 (Originally) Starlight Instruments EFS Focuser

 Modified by Sifan Kahale for Starlight Instruments for the PowerHub

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

#pragma once

#include "indifocuser.h"
#include "hidapi.h"
#include <map>

class PWRHB : public INDI::Focuser

{
    public:

    // PowerHub State
    typedef enum { PH_NOOP,
                   PH_IN,
                   PH_OUT,
                   PH_GOTO,
                   PH_CMD_POS,
                   PH_CMD_MAX,
                   PH_MTR_CMD = 0x10,
                   PH_FAST_IN  = 0x11,
                   PH_GET_STATUS = 0x11,
                   PH_FAST_OUT = 0x12,
                   PH_SET_POS = 0x20,
                   PH_GET_POS = 0x21,
                   PH_SET_MAX = 0x22,
                   PH_GET_MAX = 0x23,
                   PH_SET_HBITS = 0x28,
                   PH_GET_HBITS = 0x29,
                   PH_COMP = 0x38,
                   PH_GET_WEATHER = 0x43,
                   PH_VERSION = 0x51,
                   PH_MTR_LCK = 0x72,
                   PH_PORT_CTL = 0x80,
                   PH_DEW_CTL = 0x90,
                   PH_MTR_TYP = 0xC2,
                   PH_FAULT1 = 0xD1,
                   PH_FAULT2 = 0xD3,
                   PH_CLEAR_FAULT = 0xee,
                   PH_HALT     = 0xFF
                  } PH_COMMANDS;


    // PH EFS Motor State
    typedef enum { PH_NOT_MOVING,
                   PH_MOVING_IN,
                   PH_MOVING_OUT,
                   PH_LOCKED = 5,
                   PH_UNK = -3
                 } PH_MOTOR;
                 
    // PH Weather
    typedef enum { PH_TEMP,
                   PH_HUM
                 } PH_WEATHER;
                 
    // PH Move Type
    typedef enum { PH_ABS,
                   PH_MAX
                 } PH_MOVE_TYPE;
       

    // PH Dew Heaters
    typedef enum { PH_DEW1,
                   PH_DEW2
                 } PH_DEW;


        PWRHB();

        const char *getDefaultName() override;
        virtual bool initProperties() override;

        virtual bool Connect() override;
        virtual bool Disconnect() override;

        virtual void TimerHit() override;

        virtual IPState MoveAbsFocuser(uint32_t targetTicks) override;
        virtual IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks) override;
        virtual bool AbortFocuser() override;
        virtual bool SyncFocuser(uint32_t ticks) override;
        virtual bool SetFocuserMaxPosition(uint32_t ticks) override;
        
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

        // Set/Get Absolute Position
        bool setAbsPosition(uint32_t ticks);
        bool getAbsPosition(uint32_t *ticks);

        // Set/Get Maximum Position
        bool setMaxPosition(uint32_t ticks);
        bool getMaxPosition(uint32_t *ticks);

        float getHumidity();
        float getTemperature();
        uint8_t* hidCMD(PH_COMMANDS hcmd, uint8_t hidArg1, uint8_t hidArg2, int numCmd);
        uint8_t getStatus();

        hid_device *handle { nullptr };
        PH_MOTOR m_Motor { PH_NOT_MOVING };
        int32_t simPosition { 0 };
        uint32_t targetPosition { 0 };

        // Driver Timeout in ms
        static const uint16_t PH_TIMEOUT { 1000 };

        static const std::map<PH_COMMANDS, std::string> CommandsMap;
        static const std::map<PH_MOTOR, std::string> MotorMap;

};

