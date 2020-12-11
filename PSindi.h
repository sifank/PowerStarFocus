/*********************************************************************
*  Program:      PSindi.h
*  Version:      20201209
*  Author:       Sifan S. Kahale
*  Description:  Power^Star indi .h file
**********************************************************************/

#pragma once

#include "indifocuser.h"
#include "PScontrol.h"
#include "hidapi.h"
#include <map>
#include <cmath>
#include <cstring>
#include <memory>

typedef enum {     PS_NOT_MOVING,
                   PS_MOVING_IN,
                   PS_MOVING_OUT,
                   PS_BUSY,
                   PS_UNK,
                   PS_LOCKED
} PS_MOTOR;

/**
const std::map<uint8_t, std::string> MotorMap =
{
    {0, "Idle"},
    {1, "Moving Inwards"},
    {2, "Moving Outwards"},
    {3, "Busy"},
    {4, "Unknown"},
    {5, "Locked"}
};
**/

   int  POLLMS = 1000; 

class PWRSTR : public INDI::Focuser

{
    public:

    PSCTL psctl;

    PWRSTR();

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

        uint8_t* response = {0};

        PS_MOTOR m_Motor { PS_NOT_MOVING };
        int32_t simPosition { 0 };
        uint32_t maximumPosition = 0;
        uint32_t relitivePosition = 0;
        uint32_t targetPosition { 0 };

        // Driver Timeout in ms
        static const uint16_t PS_TIMEOUT { 1000 };
        
        PowerStarProfile curProfile;
};

