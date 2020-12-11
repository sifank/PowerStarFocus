/***************************************************************
*  Program:      PStui.h
*  Version:      20201208
*  Author:       Sifan S. Kahale
*  Description:  TUI based Power*Star control
****************************************************************/
#pragma once

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
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

const std::map<uint8_t, std::string> MotorMap =
{
    {0, "Idle"},
    {1, "Moving Inwards"},
    {2, "Moving Outwards"},
    {3, "Busy"},
    {4, "Unknown"},
    {5, "Locked"}
};

uint32_t    maxPos = 0;
uint32_t    curPos = 0;
uint32_t    FocusMaxPos;
        
uint32_t    FocusAbsPos;
uint32_t    FocusAbsPosMax;
uint32_t    FocusAbsPosStep;
uint32_t    FocusAbsPosMin;
            
uint32_t    FocusSyncMax;
uint32_t    FocusSyncStep;
uint32_t    FocusSyncMin;

uint32_t    FocusRelPos;
uint32_t    FocusRelPosMax;
uint32_t    FocusRelPosStep;
uint32_t    FocusRelPosMin;        

uint8_t     FocusAbsPosState;
uint8_t     FocusRelPosState;
uint8_t     FocusReverseState;
bool        isConnected;

string      message;
string      device = "";
string      action = "";
string      cimput;

PowerStarProfile PDMS_SI = {"PDMS_SI", 2, 0, 8, 64, 2.2, 50000, 25000, 10.0, 0.0, 0, false, true, 0, 1, 0xffff, 
    "Out1", "Out2", "Out3", "Out4", "Dew1", "Dew2", "VAR", "MP",
    "USB1", "USB2", "USB3", "USB4", "USB5", "USB6"
};
PowerStarProfile HSM_SI  = {"HSM_SI", 1, 0, 8, 80, 1.5, 50000, 25000, 10.0, 0.0, 0, false, true, 0, 1, 0xffff, 
    "Out1", "Out2", "Out3", "Out4", "Dew1", "Dew2", "VAR", "MP",
    "USB1", "USB2", "USB3", "USB4", "USB5", "USB6"
};
PowerStarProfile User1   = {"User1", 5, 0, 32, 0, 5.0, 50000, 25000, 10.0, 0.0, 1, false, true, 0, 0, 0xffff, 
    "Out1", "Out2", "Out3", "Out4", "Dew1", "Dew2", "VAR", "MP",
    "USB1", "USB2", "USB3", "USB4", "USB5", "USB6"
};
PowerStarProfile UNI_12V = {"UNI_12V", 5, 0, 32, 0, 5.0, 50000, 25000, 10.0, 0.0, 1, false, true, 0, 0, 0xffff, 
    "Out1", "Out2", "Out3", "Out4", "Dew1", "Dew2", "VAR", "MP",
    "USB1", "USB2", "USB3", "USB4", "USB5", "USB6"
};

PowerStarProfile curProfile;
PowerStarProfile actProfile;
