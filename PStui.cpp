/***************************************************************
*  Program:      npstui.cpp
*  Version:      20210102
*  Author:       Sifan S. Kahale
*  Description:  TUI based Power*Star control
****************************************************************/

#include "PStui.h"

float appVersion = 0.12;

char configFile[50] = "/etc/powerstar.config";
  
//************************************************************
void printMsg(const std::string& Msg) {
    printf("%s - Hit ENTER to continue\n", Msg.c_str());
    getline(cin, message);
}

//************************************************************
bool updateProfile(PSCTL& psctl) {
    FILE *fout = fopen(configFile, "w");
    if (!fout) {
        fclose(fout);
        return false;
    }
    fwrite(&curProfile, sizeof(PowerStarProfile), 1, fout);
    fclose(fout);
    return true;
}

//************************************************************
void askString(char (& fname)[11], const std::string& Msg, bool stateprev=true) {
    if (stateprev)
        printf("Enter Name for %s (10 chars) [%s]: ", Msg.c_str(), fname);
    else
        printf("Enter Name for %s (10 chars): ", Msg.c_str());
    
    getline(cin, message);
    if (strlen(message.c_str()) != 0) {
        strncpy(fname, message.c_str(), 10);
        fname[9] = '\0';
    }
}

//************************************************************
void askYN(bool * request, const std::string& Msg, bool stateprev=true) {
    bool wfin = true;
    while (wfin) {
        if (stateprev)
            printf("Enable %s (Y'es or N'o) [%s]: ", Msg.c_str(), *request ? "yes" : "no");
        else
            printf("Enable %s (Y'es or N'o): ", Msg.c_str());
            
        getline(cin, message);
        if (strlen(message.c_str()) == 0) {
            wfin = false;
            break;
        }
    
        boost::algorithm::to_lower(message);
        char command = message[0];
        switch(command) {
            case 'y' : {
                *request = true;
                wfin = false;
                break;
            }
            case 'n' : {
                *request = false;
                wfin = false;
                break;
            }
            default : {
                printMsg("Answer must be Y'es or N'o");
                break;
            }
        }
    }
}

//************************************************************
void askMask(uint16_t * request, uint16_t mask, const std::string& Msg) {
    bool wfin = true;
    while (wfin) {
        printf("Ignore %s (Y'es or N'o) [%s]: ", Msg.c_str(), (*request & mask) ? "no" : "yes");
        
        getline(cin, message);
        if (strlen(message.c_str()) == 0) {
            wfin = false;
            break;
        }
    
        boost::algorithm::to_lower(message);
        char command = message[0];
        switch(command) {
            case 'n' : {
                *request |= mask;
                wfin = false;
                break;
            }
            case 'y' : {
                *request &= ~mask;
                wfin = false;
                break;
            }
            default : {
                printMsg("Answer must be Y'es or N'o");
                break;
            }
        }
    }
    //printf("DIAG: fault mask leaving: <%04x>\n", *request);
}

//************************************************************
void askUint16(uint16_t * request, int min, int max, const std::string& Msg, bool stateprev = true) {
    while (true) {
        if (stateprev)
            printf("Enter Value for %s (between %i and %i [%u]: ", Msg.c_str(), min, max, *request);
        else
            printf("Enter Value for %s (between %i and %i: ", Msg.c_str(), min, max);
        
        getline(cin, message);
        if (strlen(message.c_str()) != 0) {
            try {
                *request = (uint16_t)stoi(message);
            }
            catch (exception &err) {
                printMsg("Must be an integer number");
                continue;
            }
            
            if (*request < min || *request > max) {
                printMsg("Value out of range");
                continue;
            }
        }
        break;
    }
}

//************************************************************
void askUint8(uint8_t * request, int min, int max, const std::string& Msg, bool stateprev) {
    while (true) {
        if (stateprev)
            printf("Enter Value for %s (between %i and %i) [%u]: ", Msg.c_str(), min, max, *request);
        else
            printf("Enter Value for %s (between %i and %i: ", Msg.c_str(),  min, max);
        
        getline(cin, message);
        if (strlen(message.c_str()) != 0) {
            try {
                *request = (uint16_t)stoi(message);
            }
            catch (exception &err) {
                printMsg("Must be an integer number");
                continue;
            }
            
            if (*request < min || *request > max) {
                printMsg("Value out of range");
                continue;
            }
        }
        break;
    }
}


//************************************************************
void askFloat(float * request, float min, float max, const std::string& Msg) {
    while (true) {
        printf("Enter Value for %s (between %.1f and %.1f) [%.1f]: ", Msg.c_str(), min, max, *request);
        getline(cin, message);
        if (strlen(message.c_str()) != 0) {
            try {
                *request = stof(message);
            }
            catch (exception &err) {
                printMsg("Must be an integer number");
                continue;
            }
            
            if (*request < min || *request > max) {
                printMsg("Value out of range");
                continue;
            }
        }
        break;
    }
}

//************************************************************
void printFaults(PSCTL& psctl) {
    uint32_t faultstat = psctl.getFaultStatus(curProfile.faultMask);
    if ( faultstat ) {
            if (faultstat & 0xff00)
                printf("\033[1;31mFatal-FAULT: <%04X> \033[0m  ", ((faultstat & 0xff00) >> 16));
            
            //TODO gave Non-Fatal msg but no other fault ..?
            if (faultstat & 0x00ff)
                printf("\033[1;31mNon-Fatal FAULT: <%04X> \033[0m \n", (faultstat & 0x00ff));
            
            // level 1 byte 1 (non fatal) faults
            if (faultstat & 0x00000002)
                printf("Over/Under Voltage on 12V input\n");
            else if (faultstat & 0x00000004)
                printf("Over Current on 12V Input\n");
            else if (faultstat & 0x00000008)
                printf("Motor temperature sensor\n");
            else if (faultstat & 0x00000010)
                printf("Bipolar Motor\n");
            else if (faultstat & 0x00000020)
                printf("Internal voltage out of spec\n");
            else if (faultstat & 0x00000040)
                printf("Environment sensor\n");
            else if (faultstat & 0x00000080)
                printf("Variable voltage over/under voltage by 6.25%% \n");
            
            // level 1 byte 2 (non fatal) faults
            else if (faultstat & 0x00000100)
                printf("Out1 over 15A\n");
            else if (faultstat & 0x00000200)
                printf("Out2 over 10A\n");
            else if (faultstat & 0x00000400)
                printf("Out3 over 6A\n");
            else if (faultstat & 0x00000800)
                printf("Out4 over 6A\n");
            else if (faultstat & 0x00001000)
                printf("Dew1 over 6A\n");
            else if (faultstat & 0x00002000)
                printf("Dew2 over 6A\n");
            else if (faultstat & 0x00004000)
                printf("MP over 6A\n");
            else if (faultstat & 0x00008000)
                printf("Position Change\n");
                
            // Fatal faults:
            if (faultstat & 0x00000002)
                printf("Fatal fault(s) occurred, you will need to restart Power*Star\n");
            // level 2 byte 1 (fatal) faults
            else if (faultstat & 0x00010000)
                printf("Fatal: Out1 over 30A\n");
            else if (faultstat & 0x00020000)
                printf("Fatal: Out2 over 20A\n");
            else if (faultstat & 0x00040000)
                printf("Fatal: Out3 over 10A\n");
            else if (faultstat & 0x00080000)
                printf("Fatal: Out4 over 10A\n");
            else if (faultstat & 0x00100000)
                printf("Fatal: Dew1 over 10A\n");
            else if (faultstat & 0x00200000)
                printf("Fatal: Dew2 over 10A\n");
            else if (faultstat & 0x00400000)
                printf("Fatal: VAR over 10A\n");
            else if (faultstat & 0x00800000)
                printf("Fatal: MP over 10A\n");
            
            // level 2 byte 2 (fatal) faults
            else if (faultstat & 0x01000000)
                printf("Fatal: 12V input under-voltage\n");
            else if (faultstat & 0x02000000)
                printf("Fatal: 12V input over-voltage\n");
            else if (faultstat & 0x04000000)
                printf("Fatal: Total current over-current\n");
            else if (faultstat & 0x08000000)
                printf("Fatal: Current sensor\n");
            else if (faultstat & 0x10000000)
                printf("Fatal: Internal 3.3V under-voltage\n");
            else if (faultstat & 0x20000000)
                printf("Fatal: Internal 3.3V over-voltage\n");
            else if (faultstat & 0x40000000)
                printf("Fatal: Internal 5V under-voltage\n");
            // 0x80000000 not used
    }
    //else
        //printf("No Faults\n");
}

//************************************************************
void userLimitsMenu(PSCTL& psctl) {
   while (true) {
       
    psctl.getUserLimitStatus(curUsrLimit);
    rc = system("clear");
    
    printf("Power*Star User Limits\n\n");
    
    printf("                    Actual Request                       Actual Request\n");
    
    printf("Out1 current limit  %5.2f  %5.2f   12V in Low Voltage     %5.2f  %5.2f\n",
           curUsrLimit[5], reqUsrLimit[5], 
           curUsrLimit[0], reqUsrLimit[0]
          );
    printf("Out2 current limit  %5.2f  %5.2f   12V in High Voltage    %5.2f  %5.2f\n",
           curUsrLimit[6], reqUsrLimit[6], 
           curUsrLimit[1], reqUsrLimit[1]
          );
    printf("Out3 current limit  %5.2f  %5.2f   Total current limit    %5.2f  %5.2f\n",
           curUsrLimit[7], reqUsrLimit[7], 
           curUsrLimit[4], reqUsrLimit[4]
          );
    printf("Out4 current limit  %5.2f  %5.2f   VAR Low Voltage        %5.2f  %5.2f\n",
           curUsrLimit[8], reqUsrLimit[8], 
           curUsrLimit[2], reqUsrLimit[2]
          );
    printf("Dew1 current limit  %5.2f  %5.2f   VAR High Voltage       %5.2f  %5.2f\n",
           curUsrLimit[9], reqUsrLimit[9], 
           curUsrLimit[3], reqUsrLimit[3]
          );
    printf("Dew2 current limit  %5.2f  %5.2f   MP current limit       %5.2f  %5.2f\n",
           curUsrLimit[10], reqUsrLimit[10], 
           curUsrLimit[11], reqUsrLimit[11]
          );
    
    printFaults(psctl);
    
    printf("\nActual values will be slightly different from requested\n");
    printf("Cmd:  E'dit, then S'ave, R'eset or B'ack?\n");

        
        printf("Command: ");
        getline(cin, cimput);
        boost::algorithm::to_lower(cimput);
        char command = cimput[0];
        switch(command) {
            // Edit user limits
            case 'e': {
                askFloat(&reqUsrLimit[0], 10, 14, "IN low voltage limit");                
                askFloat(&reqUsrLimit[1], 12, 14.5, "IN high voltage limit");
                askFloat(&reqUsrLimit[4], 0, 22.7, "IN high current limit");
                askFloat(&reqUsrLimit[2], 2.7, 9, "VAR low voltage limit");
                askFloat(&reqUsrLimit[3], 3.3, 10.9, "VAR high voltage limit");
                askFloat(&reqUsrLimit[5], 0, 15, "Out1 current limit");
                askFloat(&reqUsrLimit[6], 0, 10, "Out2 current limit");
                askFloat(&reqUsrLimit[7], 0, 6, "Out3 current limit");
                askFloat(&reqUsrLimit[8], 0, 6, "Out4 current limit");
                askFloat(&reqUsrLimit[9], 0, 5, "Dew1 current limit");
                askFloat(&reqUsrLimit[10], 0, 5, "Dew1 current limit");
                askFloat(&reqUsrLimit[11], 0, 6, "MP current limit");
                break;
            }
            
            // Save user limits
            case 's': {
                psctl.setUserLimitStatus(reqUsrLimit);

                break;
            }

            // Reset user limits to defaults
            case 'r': {
                reqUsrLimit[0] = 14.0;
                reqUsrLimit[1] = 14.5;
                reqUsrLimit[2] = 9.0;
                reqUsrLimit[3] = 10.9;
                reqUsrLimit[4] = 22.7;
                reqUsrLimit[5] = 15.0;
                reqUsrLimit[6] = 10.0;
                reqUsrLimit[7] = reqUsrLimit[8] = reqUsrLimit[11] = 6.0;
                reqUsrLimit[9] = reqUsrLimit[10] = 5.0;
                
                psctl.setUserLimitStatus(reqUsrLimit);
                break;
            }
            
            // return to previous menu
            case 'b': {
                if ( ! updateProfile(psctl)) {
                    printMsg("Error: save could not updated config file");
                    break;
                }

                if ( ! psctl.activateProfile(curProfile))
                    printMsg("Problem activating profile");
                
                break;
            }
                        
            default: {
            }
                        
        }
        if (command == 'b')
            break;
                
    }
}

//************************************************************
void profileMenu(PSCTL& psctl) {
while (true) {

    actProfile = psctl.getProfileStatus();
    
    rc = system("clear");
    printf("Power*Star Focus Motor Profile\n");
    
    printf("                    Actual Requested                       Actual Requested\n");
    
    printf("Backlash:         %8u  %8u   PrefDir:          %8s  %8s\n",
           actProfile.backlash, curProfile.backlash,
           actProfile.prefDir ? "Out" : "In",
           curProfile.prefDir ? "Out" : "In"
    );
    
    printf("Idle Cur:         %8u  %8u   Drv Cur:          %8u  %8u\n",
           actProfile.idleMtrCurrent, curProfile.idleMtrCurrent,
           actProfile.driveMtrCurrent, curProfile.driveMtrCurrent
    );
    
    string mtrBreak;
    if (curProfile.motorBraking == 0)
        mtrBreak = "None";
    else if (curProfile.motorBraking == 1)
        mtrBreak = "Low";
    else
        mtrBreak = "Normal";
    
    string mtrBreakA;
    if (actProfile.motorBraking == 0)
        mtrBreakA = "None";
    else if (actProfile.motorBraking == 1)
        mtrBreakA = "Low";
    else
        mtrBreakA = "Normal";
    
    printf("Mtr Type:         %8s  %8s   Brake Cur:        %8s  %8s\n",
           actProfile.motorType ? "BiPolar" : "Unipolar",
           curProfile.motorType ? "BiPolar" : "Unipolar",
           mtrBreakA.c_str(), mtrBreak.c_str()
    );
    
    printf("Step Per:         %8.1f  %8.1f   Rev Mtr:          %8s  %8s\n",
           actProfile.stepPeriod, curProfile.stepPeriod,
           actProfile.reverseMtr ? "Reverse" : "Normal",
           curProfile.reverseMtr ? "Reverse" : "Normal"
    );
    
    printf("Cur Pos:          %8lu  %8lu   Max Pos:          %8lu  %8lu\n",
           (unsigned long)actProfile.curPosition, (unsigned long)curProfile.curPosition,
           (unsigned long)actProfile.maxPosition, (unsigned long)curProfile.maxPosition
    );
    
    printf("Temp Coef:        %8.1f  %8.1f   Temp Hys:         %8.1f  %8.1f\n",
           actProfile.tempCoef, curProfile.tempCoef,
           actProfile.tempHysterisis, curProfile.tempHysterisis
    );
    
    string tempCompA;
    if (curProfile.tempSensor == 0)
        tempCompA = "Disabled";
    else if (curProfile.tempSensor == 1)
        tempCompA = "Motor";
    else if (curProfile.tempSensor == 2)
        tempCompA = "Env";
    else
        tempCompA = "Unk";
    
    string tempCompP;
    if (actProfile.tempSensor == 0)
        tempCompP = "Disabled";
    else if (actProfile.tempSensor == 1)
        tempCompP = "Motor";
    else
        tempCompP = "Env";
    
    printf("Disable Perm Foc: %8s  %8s   Temp Comp Selection:%8s  %8s\n",
           actProfile.disablePermFocus ? "Yes" : "No",
           curProfile.disablePermFocus ? "Yes" : "No",
           tempCompA.c_str(), tempCompP.c_str()
    );
    
    printf("\nCmd: E'dit, R'eset, S'ave or B'ack?\n");
    
    printf("Command: ");
        getline(cin, cimput);
        boost::algorithm::to_lower(cimput);
        char command = cimput[0];
        switch(command) {
            // Edit focus motor profile
            case 'e': {
                
                // Motor type
                bool wfin = true;
                while (wfin) {
                    string tempCompA;
                    if (curProfile.motorType == 0)
                        tempCompA = "Unipolar";
                    else
                        tempCompA = "BiPolar";

                    printf("Motor Type (U'nipolar or B'ipolar [%s]: ",
                        tempCompA.c_str());
                    
                    getline(cin, cimput);
                    if (strlen(cimput.c_str()) == 0) {
                        wfin = false;
                        break;
                    }
                    
                    boost::algorithm::to_lower(cimput);
                    char command = cimput[0];
                    switch(command) {
                        case 'u' : {
                            curProfile.motorType = 0;
                            wfin = false;
                            break;
                        }
                        case 'b' : {
                            curProfile.motorType = 1;
                            wfin = false;
                            break;
                        }
                        default : {
                            printMsg("Enter U'nipolar or B'ipolar\n");
                            break;
                        }
                    }
                }
                
                // Backlash
                askUint16(&curProfile.backlash, 0, 255, "Backlash amount (0-255)");
                
                // Preferred direction
                wfin = true;
                while (wfin) {
                    string tempCompA;
                    if (curProfile.prefDir == 0)
                        tempCompA = "In";
                    else
                        tempCompA = "Out";

                    printf("Backlash Preferred Direction (I'n or O'out) [%s]: ",
                        tempCompA.c_str());
                    
                    getline(cin, cimput);
                    if (strlen(cimput.c_str()) == 0) {
                        wfin = false;
                        break;
                    }
                    
                    boost::algorithm::to_lower(cimput);
                    char command = cimput[0];
                    switch(command) {
                        case 'i' : {
                            curProfile.prefDir = 0;
                            wfin = false;
                            break;
                        }
                        case 'o' : {
                            curProfile.prefDir = 1;
                            wfin = false;
                            break;
                        }
                        default : {
                            printMsg("Enter I'n or O'out\n");
                            break;
                        }
                    }
                }
                
                // Idle Current
                // for unipolar, off/50%/maximum
                if (curProfile.motorType == 0) { //unipolar
                    askUint8(&curProfile.idleMtrCurrent, 0, 254, "Unipolar Idle Current (0-84 off, 85-160: 50%, 161-254: max)", true);
                    
                }
                else { //bipolar, just ask value
                    askUint8(&curProfile.idleMtrCurrent, 0, 254, "Bipolar Idle Current (0-254)", true);
                }
                
                // Drive (move) Current
                // ignored if Unipolar
                if (! curProfile.motorType == 0) {
                    //bipolar, just ask value
                    askUint8(&curProfile.driveMtrCurrent, 0, 254, "Bipolar Drive Current (0-254)", true);
                }
                
                // Motor motorBraking
                askUint8(&curProfile.motorBraking, 0, 2, "Mtr Braking (0: none, 1: low, 2: idle level)", true);
                
                // Step Period
                // in tenths of a ms, so have to * 10 (from 1 - 100)
                askFloat(&curProfile.stepPeriod, 0.0, 10.0, "Step Period in ms");
                
                // Reverse Motor
                askYN(&curProfile.reverseMtr, "Reverse Motor");
                
                // Temperature Coefficient
                askFloat(&curProfile.tempCoef, 0.0, 255.99, "Temperature Coefficient (degrees C or 0 to disable)");                
                
                // Temperature Hysteresis
                askFloat(&curProfile.tempHysterisis, 0.0, 25.5, "Hysterisis (degrees C)");
                
                // Enable Temperature Compensation
                wfin = true;
                while (wfin) {
                    string tempCompA;
                    if (curProfile.tempSensor == 0)
                        tempCompA = "None";
                    else if (curProfile.tempSensor == 1)
                        tempCompA = "Motor";
                    else if (curProfile.tempSensor == 2)
                        tempCompA = "Env";
                    else
                        tempCompA = "Unk";

                    printf("Temp Comp (N'one, M'otor or E'nv [%s]: ",
                        tempCompA.c_str());
                    
                    getline(cin, cimput);
                    if (strlen(cimput.c_str()) == 0) {
                        wfin = false;
                        break;
                    }
                    
                    boost::algorithm::to_lower(cimput);
                    char command = cimput[0];
                    switch(command) {
                        case 'n' : {
                            curProfile.tempSensor = 0;
                            wfin = false;
                            break;
                        }
                        case 'm' : {
                            curProfile.tempSensor = 1;
                            wfin = false;
                            break;
                        }
                        case 'e' : {
                            curProfile.tempSensor = 2;
                            wfin = false;
                            break;
                        }
                        default : {
                            printMsg("Enter N'one, M'otor or E'nv");
                            break;
                        }
                    }
                }
                    
                // Enable Permanent Focus
                askYN(&curProfile.disablePermFocus, "Disable Permanent Focus?");
                
                break;
            }
            
            // reset (select) focus motor template **********************
            case 'r': {
                    
                printf("Select template: hsm, pdms, uni12, user1? ");
                getline(cin, device);
                boost::algorithm::to_lower(device);

                if (device == "uni12") curProfile = UNI_12V;
                else if (device == "hsm") curProfile = HSM_SI;
                else if (device == "pdms") curProfile = PDMS_SI;
                else if (device == "user1") curProfile = User1;
                else {
                    printMsg("Focus motor profile must be: HSM_SI, PDMS_SI, UNI_12V, USER1");
                    break;
                }
                
                printMsg("Remember to save the profile to activate it");
                break;
              }
              
            // save and activate Profile
            case 's': {
                if ( ! updateProfile(psctl)) {
                    printMsg("Error: save could not updated config file");
                    break;
                }

                if ( ! psctl.activateProfile(curProfile))
                    printMsg("Problem activating profile");
                    
                break;
            }
            
            // return to previous menu
            case 'b': {
                if ( ! updateProfile(psctl)) {
                    printMsg("Error: save could not updated config file");
                    break;
                }

                if ( ! psctl.activateProfile(curProfile))
                    printMsg("Problem activating profile");
                
                break;
            }
            
            default: {
            }
                    
        }
        if (command == 'b')
            break;
                
    }
    
}

//************************************************************
void focusMenu(PSCTL& psctl) {
while (true) {

    actProfile = psctl.getProfileStatus();
    
    rc = system("clear");
    printf("Power*Star Focus Menu\n\n");
    
    uint8_t mtrStatus = psctl.getFocusStatus();
    psctl.getAbsPosition(&curPos);
    psctl.getMaxPosition(&maxPos);
    
    printf("Current Position: %u\n", curPos);
    printf("Max Position:     %u\n", maxPos);
    printf("Status:           %s\n\n", MotorMap.at(mtrStatus).c_str());
    
    printFaults(psctl);
    
    printf("('Enter' to refresh)\n");
    printf("Cmd: I'n O'ut G'oto A'bort L'ock U'nlock\n");
    printf("     C'urrent or M'ax Position, P'rofile, B'ack\n");
    
    printf("Command: ");
        getline(cin, cimput);
        boost::algorithm::to_lower(cimput);
        char command = cimput[0];
        switch(command) {
            // focus goto position *********************************
            case 'g' : {
                uint32_t ncurPos;
                printf("Move to absolute location? ");
                getline(cin, action);

                try {
                    ncurPos = stoi(action);
                }
                catch (exception &err) {
                    printf("ERROR: Current steps must be a number between 1 and %u\n", maxPos);
                    getline(cin, message);
                    break;
                }
                
                if (ncurPos < 1 || ncurPos > maxPos) {
                    printf("ERROR: Current steps must be between 1 and %u\n", maxPos);
                    getline(cin, message);
                    break;
                }
                
                if (! psctl.MoveAbsFocuser(ncurPos) ) {
                    printf("Error, could not move focuser\n");
                    getline(cin, message);
                }
                break;
            }
                    
            // focus go in x steps *********************************
            case 'i' : {
                uint32_t ncurPos;
                printf("Move in how many steps? ");
                getline(cin, action);

                try {
                    ncurPos = stoi(action);
                }
                catch (exception &err) {
                    printf("ERROR: Current steps must be a number between 1 and %u\n", curPos);
                    getline(cin, message);
                    break;
                }
                
                // calculate abs position
                ncurPos = curPos - ncurPos;
                
                if (ncurPos < 1 || ncurPos > curPos) {
                    printf("ERROR: Current steps must be between 1 and %u\n", curPos);
                    getline(cin, message);
                    break;
                }
                
                if (! psctl.MoveAbsFocuser(ncurPos) ) {
                    printf("Error, could not move focuser inward\n");
                    getline(cin, message);
                }
                
                break;  
            }
                    
            // focus go out x steps *********************************
            case 'o' : {
                uint32_t ncurPos;
                printf("Move out how many steps? ");
                getline(cin, action);
                //boost::algorithm::to_lower(action);
                try {
                    ncurPos = stoi(action);
                }
                catch (exception &err) {
                    printf("ERROR: Current steps must be a number between 1 and %u\n", maxPos - curPos);
                    getline(cin, message);
                    break;
                }
            
                if (ncurPos < 1 || ncurPos > (maxPos - curPos)) {
                    printf("ERROR: Current steps must be between 1 and %u\n", (maxPos - curPos));
                    getline(cin, message);
                    break;
                }
                
                // calculate abs position
                ncurPos = curPos + ncurPos;
                
                if (! psctl.MoveAbsFocuser(ncurPos) ) {
                    printf("Error, could not move focuser outward\n");
                    getline(cin, message);
                }
                
                break;
            }
                    
            // focus abort movement ********************************
            case 'a' : {
                if ( ! psctl.AbortFocuser()) {
                     printf("Error, could not abort\n");
                    getline(cin, message);
                }
                break;
            }
                    
            // focus lock motor ************************************
            case 'l' : {
                if ( ! psctl.lockFocusMtr()) {
                    printf("Error, could not lock focus motor\n");
                    getline(cin, message);
                }
                break;
            }
                    
            // focus unlock motor **********************************
            case 'u' : {
                if ( ! psctl.unLockFocusMtr()) {
                    printf("Error, could not unlock focus motor\n");
                    getline(cin, message);
                }
                break;
            }
            
            // set max steps (position) ***********************
            case 'm' : {
                uint32_t nmaxPos;
                printf("Set maximum steps to? ");
                getline(cin, action);
                
                try {
                    nmaxPos = stoi(action);
                }
                catch (exception &err) {
                    printf("ERROR: Maximum steps must be a number between 1 and 60,000\n");
                    getline(cin, message);
                    break;
                }
                
                if (nmaxPos < 1 || nmaxPos > 60000) {
                    printf("ERROR: Maximum steps must be between 1 and 60,000\n");
                    getline(cin, message);
                    break;
                }
                
                if (! psctl.SetFocuserMaxPosition(nmaxPos) ) {
                    printf("Error, could not set max position\n");
                    getline(cin, message);
                }
                
                curProfile.maxPosition = maxPos;
                updateProfile(psctl);
                
                break;
            }
              
            // set current position ***************************
            case 'c' : {
                uint32_t ncurPos;
                printf("Set current position to? ");
                getline(cin, action);

                try {
                    ncurPos = stoi(action);
                }
                catch (exception &err) {
                    printf("ERROR: Current steps must be a number between 1 and %u\n", maxPos);
                    getline(cin, message);
                    break;
                }
                
                if (ncurPos < 1 || ncurPos > maxPos) {
                    printf("ERROR: Current steps must be between 1 and %u\n", maxPos);
                    getline(cin, message);
                    break;
                }
                
                if (! psctl.SyncFocuser(ncurPos) ) {
                    printf("Error, could not set current position\n");
                    getline(cin, message);
                }
                
                break;
            }
            
            // Profile Menu  **********************************
            case 'p' : {
                profileMenu(psctl);
                break;
            }
            
            // return to previous menu
            case 'b': {
                if ( ! updateProfile(psctl)) {
                    printMsg("Error: save could not updated config file");
                    break;
                }

                if ( ! psctl.activateProfile(curProfile))
                    printMsg("Problem activating profile");
                
                break;
            }
            
            default: {
            }
            
        }
        if (command == 'b')
            break;

    }
    
}

//************************************************************
void autobootMenu(PSCTL& psctl) {
while (true) {

    actProfile = psctl.getProfileStatus();
    
    rc = system("clear");
    printf("Power*Star AutoBoot Menu\n");
    printf("\nAutoBoot Status:\n");
    printf("         PORTS                  USB\n");
    printf("%-17s %3s   %-10s    %3s\n",
           curProfile.out1, psctl.statusMap["Out1"].autoboot ? "on" : "off",
           curProfile.usb2, psctl.statusMap["USB2"].autoboot ? "on" : "off"
          );

    printf("%-17s %3s   %-10s    %3s\n",
           curProfile.out2, psctl.statusMap["Out2"].autoboot ? "on" : "off",
           curProfile.usb3, psctl.statusMap["USB3"].autoboot ? "on" : "off"
           );
    
    printf("%-17s %3s   %-10s    %3s\n",
           curProfile.out3, psctl.statusMap["Out3"].autoboot ? "on" : "off",
           curProfile.usb6, psctl.statusMap["USB6"].autoboot ? "on" : "off"
           );
    
    printf("%-17s %3s\n",
           curProfile.out4, psctl.statusMap["Out4"].autoboot ? "on" : "off");
    
    printf("         OTHER                  DEW\n");
    
    printf("%-17s %3s   %-10s    %3s\n",
           curProfile.var, psctl.statusMap["Var"].autoboot ? "on" : "off",
           curProfile.dew1, psctl.statusMap["Dew1"].autoboot ? "on" : "off"
          );
    
    printf("%-17s %3s   %-10s    %3s\n",
           curProfile.mp, psctl.statusMap["MP"].autoboot ? "on" : "off",
           curProfile.dew2, psctl.statusMap["Dew2"].autoboot ? "on" : "off"
           );
    
    printFaults(psctl);
        
    printf("\nCmd: S'et, R'eset, B'ack\n");
    
    printf("Command: ");
        getline(cin, cimput);
        boost::algorithm::to_lower(device);
        char command = cimput[0];

        switch(command) {
            // set auto boot
            case 'a' : {
                printf("Which device: ");
                getline(cin, device);
                
                printf("On or Off? ");
                getline(cin, action);
                
                boost::algorithm::to_lower(device);
                boost::algorithm::to_lower(action);
        
                if ( ! psctl.setAutoBoot(device, action))
                    printMsg("Problem setting Autoboot");
                
                break;
            }
            
            // back
            case 'b' : {
                break;
            }
            
            default: {
            }
        }
        if (command == 'b')
            break;
    }
}

//************************************************************
void faultMenu(PSCTL& psctl) {
while (true) {

    actProfile = psctl.getProfileStatus();
    
    rc = system("clear");
    printf("Power*Star Fault Menu\n");
    printf("\nFaults Ignored:\n");
    printf("Out1 Over Current Fault    %3s  Input Over/Under Voltage Fault %s\n",
           (curProfile.faultMask & 0x0100) ? "No" : "Yes",
           (curProfile.faultMask & 0x0002) ? "No" : "Yes");
    printf("Out2 Over Current Fault    %3s  Input Over Current Fault       %s\n",
           (curProfile.faultMask & 0x0200) ? "No" : "Yes",
           (curProfile.faultMask & 0x0004) ? "No" : "Yes");
    printf("Out3 Over Current Fault    %3s  Motor Temp Fault               %s\n",
           (curProfile.faultMask & 0x0400) ? "No" : "Yes",
           (curProfile.faultMask & 0x0008) ? "No" : "Yes");
    printf("Out4 Over Current Fault    %3s  BiMotor Fault                  %s\n",
           (curProfile.faultMask & 0x0800) ? "No" : "Yes",
           (curProfile.faultMask & 0x0010) ? "No" : "Yes");
    printf("Dew1 Over Current Fault    %3s  Internal power Fault           %s\n",
           (curProfile.faultMask & 0x1000) ? "No" : "Yes",
           (curProfile.faultMask & 0x0020) ? "No" : "Yes");
    printf("Dew2 Over Current Fault    %3s  Env Sensor Fault               %s\n",
           (curProfile.faultMask & 0x2000) ? "No" : "Yes",
           (curProfile.faultMask & 0x0040) ? "No" : "Yes");
    printf("MP Over Current Fault      %3s  Var voltage over/under Fault   %s\n",
           (curProfile.faultMask & 0x4000) ? "No" : "Yes",
           (curProfile.faultMask & 0x0080) ? "No" : "Yes");
    printf("Position Change Fault       %s\n",
           (curProfile.faultMask & 0x8000) ? "No" : "Yes");
    
    printf("\nIgnore Fault Mask:     <%04x>\n",
            curProfile.faultMask
          );
    
    printFaults(psctl);
    
    printf("\nCmd: E'dit or R'eset fault mask, C'lear faults, B'ack\n");
        
        printf("Command: ");
        getline(cin, cimput);
        boost::algorithm::to_lower(cimput);
        char command = cimput[0];
        switch(command) {
            // clear faults
            case 'c' : {
                if ( ! psctl.clearFaults()) {
                    printMsg("Problem clearing faults");
                    break;
                }

                psctl.clearFaultStatus();
                break;
            }
            
            // Set fault mask
            case 'e': { 
                askMask(&curProfile.faultMask, 0x0002, "Input Over/Under Voltage Fault? ");
                askMask(&curProfile.faultMask, 0x0004, "Input Over Current Fault?       ");
                askMask(&curProfile.faultMask, 0x0008, "Motor Temp Fault?               ");
                askMask(&curProfile.faultMask, 0x0010, "BiMotor Fault?                  ");
                askMask(&curProfile.faultMask, 0x0020, "Internal power Fault?           ");
                askMask(&curProfile.faultMask, 0x0040, "Env Sensor Fault?               ");
                askMask(&curProfile.faultMask, 0x0080, "Var voltage over/under Fault?   ");
                               
                askMask(&curProfile.faultMask, 0x0100, "Out1 Over Current Fault?       ");
                askMask(&curProfile.faultMask, 0x0200, "Out2 Over Current Fault?       ");
                askMask(&curProfile.faultMask, 0x0400, "Out3 Over Current Fault?       ");
                askMask(&curProfile.faultMask, 0x0800, "Out4 Over Current Fault?       ");
                askMask(&curProfile.faultMask, 0x1000, "Dew1 Over Current Fault?       ");
                askMask(&curProfile.faultMask, 0x2000, "Dew2 Over Current Fault?       ");
                askMask(&curProfile.faultMask, 0x4000, "MP Over Current Fault?        ");
                askMask(&curProfile.faultMask, 0x8000, "Position Change Fault?         ");
                
                // save fault map to system
                if ( ! psctl.saveDewPwmFault(curProfile))
                        printMsg("Problem saving fault settings");
                break;
            }
            
            // reset (clear) fault mask
            case 'r': {
                curProfile.faultMask = 0xffff;
                // save fault map to system
                if ( ! psctl.saveDewPwmFault(curProfile))
                        printMsg("Problem saving fault settings");
                break;
            }
            
            // return to previous menu
            case 'b': {                
                break;
            }
                        
            default: {
            }
                        
        }
        if (command == 'b')
            break;
   }
}
    
//************************************************************
void settingsMenu(PSCTL& psctl) {
   while (true) {

    actProfile = psctl.getProfileStatus();
    
    rc = system("clear");
    
    printf("Power*Star Settings\n\n");
    
    uint16_t fvir = psctl.getVersion();
    printf("Firmware Ver: %i.%i  TUI Ver %.1f\n\n",
           (fvir & 0xff00) / 256, fvir & 0x00ff, appVersion);
    
    printf("Port Names:\n");
    printf("Out1: %-10s      USB1: %-10s\n",
           curProfile.out1, curProfile.usb1);
    printf("Out2: %-10s      USB2: %-10s\n",
           curProfile.out2, curProfile.usb2);
    printf("Out3: %-10s      USB3: %-10s\n",
           curProfile.out3, curProfile.usb3);
    printf("Out4: %-10s      USB4: %-10s\n",
           curProfile.out4, curProfile.usb4);
    printf("DEW1: %-10s      USB5: %-10s\n",
           curProfile.dew1, curProfile.usb5);
    printf("DEW2: %-10s      USB6: %-10s\n\n",
           curProfile.dew2, curProfile.usb6);
    
    printf("VAR:  %-10s  Set to: %.1fV\n",
           curProfile.var, psctl.statusMap["Var"].levels);
    
    string mpSetting;
    string mpPercent;
    char varField[16];
    if (psctl.statusMap["MP"].setting == 0) {
        mpSetting = "DC";
        snprintf(varField, 16, "%-sV", "12");
        mpPercent = psctl.statusMap["MP"].state ? "on" : "off";
    }
    else if (psctl.statusMap["MP"].setting == 1) {
        mpSetting = "PWM";
        snprintf(varField, 16, "%-i", psctl.getPWM());
        mpPercent = psctl.getPWM() ? varField : "off";
    }
    else {
        mpSetting = "Dew";
        snprintf(varField, 16, "%-i%%", psctl.getDew(2));
        mpPercent = psctl.getDew(2) ? varField : "off";
    }
    printf("MP:   %-10s  Set as: %3s   Value: %5s\n",
           curProfile.mp,
           mpSetting.c_str(),
           varField);
    
    printf("LED Brightness    %i\n",
           (psctl.statusMap["LED"].setting /4));
    
    printFaults(psctl);

    printf("\nCmd: LE'D, L'imits, Set P'ort, U'sb Names, V'ar, M'P\n");
    printf("     A'utoboot, then S'ave or B'ack?\n");
        
        printf("Command: ");
        getline(cin, cimput);
        boost::algorithm::to_lower(cimput);
        char command = cimput[0];
        switch(command) {
            // set auto boot
            case 'a' : {
                autobootMenu(psctl);
                break;
            }
  
            // Port names
            case 'p': {
                askString(curProfile.out1, "Out1");
                askString(curProfile.out2, "Out2");
                askString(curProfile.out3, "Out3");
                askString(curProfile.out4, "Out4");
                askString(curProfile.var, "Var");
                askString(curProfile.mp, "MP");
                askString(curProfile.dew1, "Dew1");
                askString(curProfile.dew2, "Dew2");
                break;
            }
                    
            // USB names
            case 'u': {
                askString(curProfile.usb1, "USB1");
                askString(curProfile.usb2, "USB2");
                askString(curProfile.usb3, "USB3");
                askString(curProfile.usb4, "USB4");
                askString(curProfile.usb5, "USB5");
                askString(curProfile.usb6, "USB6");
                break;
            }
            
            // LED brightness
            case 'e' : {
                uint8_t usract = 0;
                askUint8(&usract, 0, 3, "LED Brightness", false);
                
                // this maps 0-3 to the 0-15 segmented value 
                uint8_t susract = (usract * 4 ) + 1;
                
                if ( ! psctl.setLED(susract)) {
                    printMsg("Problem setting LED brightness");
                    break;
                }

                break;
            }
            
            // save and activate Profile
            case 's': {
                if ( ! updateProfile(psctl)) {
                    printMsg("Error: save could not updated config file");
                    break;
                }

                if ( ! psctl.activateProfile(curProfile))
                    printMsg("Problem activating profile");
                    
            break;
            }
            
            // set variable voltage port
            case 'v' : {
                if (psctl.statusMap["Var"].state) {
                    printMsg("VAR must be off to set it's voltage");
                    break;
                }
                
                askFloat(&psctl.statusMap["Var"].levels, 3.0, 10.0, "Variable voltage");
                
                uint8_t svolt = (uint8_t)(psctl.statusMap["Var"].levels * 10);
                if ( ! psctl.setVar(svolt))
                    printMsg("Problem setting Var voltage");
                
                break;
            }
            
            // set multiport type (DC, PWM, DEW)
            case 'm' : {
                printf("DC, PWM or DEW? ");
                getline(cin, device);
                boost::algorithm::to_lower(device);
            
                uint8_t Rtype = 0;
                if (device == "dc") Rtype = 0;
                else if (device == "pwm") Rtype = 1;
                else if (device == "dew") Rtype = 2;
                else {
                    printMsg("MultiPort types must be: dc | pwm | dew");
                    break;
                }
                
                if ( ! psctl.setMultiPort(Rtype)) {
                    printMsg("Problem setting MultiPort");
                    break;
                }
                
                if (device == "dc") {
                    psctl.statusMap["MP"].state = 1;
                    psctl.statusMap["MP"].levels = 0;
                    if ( ! psctl.saveDewPwmFault(curProfile)) {
                        printMsg("Problem saving PWM settings");
                        break;
                    }
                    break;
                }

                // if pwm ask rate
                if (device == "pwm") {
                    //ask pwm rate
                    printf("PWM rate (0 - 1023): ? ");
                    getline(cin, action);
                    boost::algorithm::to_lower(action);
                    uint16_t usract;
                    try {
                        usract = stoi(action);
                    }
                    catch (exception &err) {
                        printMsg("ERROR: Power must be a number between 0 and 1023");
                        break;
                    }
                    if (usract < 0 || usract > 1023) {
                        printMsg("ERROR: Power must be between 0 and 1023");
                        break;
                    }
        
                    
                    if ( ! psctl.setPWM(usract)) {
                        printMsg("Problem setting PWM");
                        break;
                    }
                    
                    psctl.statusMap["MP"].levels = usract;

                    psctl.statusMap["MP"].state = 0;
                    
                    if ( ! psctl.saveDewPwmFault(curProfile))
                        printMsg("Problem saving PWM settings");
                    
                    break;
                }
                
                // if dew ask %
                if (device == "dew") {
                    printf("Percent Power ? ");
                    getline(cin, action);
                    boost::algorithm::to_lower(action);
                    uint8_t usract;
                    try {
                        usract = stoi(action);
                    }
                    catch (exception &err) {
                        printMsg("ERROR: Percent power must be a number between 0 and 100");
                        break;
                    }
                    if (usract < 0 || usract > 100) {
                        printMsg("ERROR: Percent power must be between 0 and 100");
                        break;
                    }
        
                    if ( ! psctl.setDew(2, usract)) {
                        printMsg("Problem setting Dew");
                        break;
                    }
                    
                    psctl.statusMap["MP"].levels = usract;
                    
                    psctl.statusMap["MP"].state = 0;
                    
                    if ( ! psctl.saveDewPwmFault(curProfile))
                        printMsg("Problem saving Dew settings");
                    
                }
                break;
            }
            
            // set user defined limits
            case 'l': {
                userLimitsMenu(psctl);
                break;
            }
            
            // return to previous menu
            case 'b': {
                if ( ! updateProfile(psctl)) {
                    printMsg("Error: save could not updated config file");
                    break;
                }

                if ( ! psctl.activateProfile(curProfile))
                    printMsg("Problem activating profile");
                
                break;
            }
                        
            default: {
            }
                        
        }
        if (command == 'b')
            break;
                
    }
}
   
//************************************************************
void mainMenu(PSCTL& psctl) {
while (true) {
    psctl.getStatus();
    
    rc = system("clear");

    printf("Power*Star Main Menu\n");
    printf("\nDevice           State Current      F1     F2       USB      State\n");
    
    printf("%-17s %3s   %5.2f    %5s  %5s       %-10s\n",
           curProfile.out1,
           psctl.statusMap["Out1"].state ? "on" : "off",
           psctl.statusMap["Out1"].current,
           psctl.statusMap["Out1"].fault1 ? "\033[1;31mFAULT\033[0m" : "-",   
           psctl.statusMap["Out1"].fault2 ? "\033[1;31mFAULT\033[0m" : "-",
           curProfile.usb1
          );

    printf("%-17s %3s   %5.2f    %5s  %5s       %-10s%3s\n",
           curProfile.out2,
           psctl.statusMap["Out2"].state ? "on" : "off",
           psctl.statusMap["Out2"].current,
           psctl.statusMap["Out2"].fault1 ? "\033[1;31mFAULT\033[0m" : "-",   
           psctl.statusMap["Out2"].fault2 ? "\033[1;31mFAULT\033[0m" : "-",
           curProfile.usb2,
           psctl.statusMap["USB2"].state ? "on" : "off");
    
    printf("%-17s %3s   %5.2f    %5s  %5s       %-10s%3s\n",
           curProfile.out3,
           psctl.statusMap["Out3"].state ? "on" : "off",
           psctl.statusMap["Out3"].current,
           psctl.statusMap["Out3"].fault1 ? "\033[1;31mFAULT\033[0m" : "-",   
           psctl.statusMap["Out3"].fault2 ? "\033[1;31mFAULT\033[0m" : "-",
           curProfile.usb3,
           psctl.statusMap["USB3"].state ? "on" : "off");
    
    printf("%-17s %3s   %5.2f    %5s  %5s       %-10s\n",
           curProfile.out4,
           psctl.statusMap["Out4"].state ? "on" : "off",
           psctl.statusMap["Out4"].current,
           psctl.statusMap["Out4"].fault1 ? "\033[1;31mFAULT\033[0m" : "-",   
           psctl.statusMap["Out4"].fault2 ? "\033[1;31mFAULT\033[0m" : "-",
          curProfile.usb4
          );
    
    int dewSetting = psctl.statusMap["Dew1"].setting;
    string dewPercent = dewSetting ? to_string(psctl.statusMap["Dew1"].setting) : "off";
    printf("%-17s %3s   %5.2f    %5s  %5s       %-10s\n",
           curProfile.dew1,
           dewPercent.c_str(),
           psctl.statusMap["Dew1"].current,
           psctl.statusMap["Dew1"].fault1 ? "\033[1;31mFAULT\033[0m" : "-",   
           psctl.statusMap["Dew1"].fault2 ? "\033[1;31mFAULT\033[0m" : "-",
           curProfile.usb5
           );
    
        
    dewSetting = psctl.statusMap["Dew2"].setting;
    dewPercent = dewSetting ? to_string(psctl.statusMap["Dew2"].setting) : "off";
    printf("%-17s %3s   %5.2f    %5s  %5s       %-10s%3s\n",
           curProfile.dew2,
           dewPercent.c_str(),
           psctl.statusMap["Dew2"].current,
           psctl.statusMap["Dew2"].fault1 ? "\033[1;31mFAULT\033[0m" : "-",   
           psctl.statusMap["Dew2"].fault2 ? "\033[1;31mFAULT\033[0m" : "-",
           curProfile.usb6,
           psctl.statusMap["USB6"].state ? "on" : "off");
    
    float varSetting = psctl.statusMap["Var"].levels;
    char varField[17];
    snprintf(varField, 17, "%s (%.1f)", curProfile.var, varSetting);
    
    printf("%-17s %3s   %5.2f    %5s  %5s\n",
           varField,
           psctl.statusMap["Var"].state ? "on" : "off",
           psctl.statusMap["Var"].current,
           psctl.statusMap["Var"].fault1 ? "\033[1;31mFAULT\033[0m" : "-",   
           psctl.statusMap["Var"].fault2 ? "\033[1;31mFAULT\033[0m" : "-");
    
    string mpSetting;
    string mpPercent;
    if (psctl.statusMap["MP"].setting == 0) {
        mpSetting = "DC";
        mpPercent = psctl.statusMap["MP"].state ? "on" : "off";
    }
    else if (psctl.statusMap["MP"].setting == 1) {
        mpSetting = "PWM";
        char varField[16];
        snprintf(varField, 16, "%i", psctl.getPWM());
        mpPercent = psctl.getPWM() ? varField : "off";
    }
    else {
        mpSetting = "Dew";
        char varField[16];
        snprintf(varField, 16, "%i", psctl.getDew(2));
        mpPercent = psctl.getDew(2) ? varField : "off";
    }
    char mpField[17];
    snprintf(mpField, 17, "%s (%s)", curProfile.mp, mpSetting.c_str());
    printf("%-16s  %3s   %5.2f    %5s  %5s\n",
           mpField,
           mpPercent.c_str(),
           psctl.statusMap["MP"].current,
           psctl.statusMap["MP"].fault1 ? "\033[1;31mFAULT\033[0m" : "-",   
           psctl.statusMap["MP"].fault2 ? "\033[1;31mFAULT\033[0m" : "-");

    float Dp = psctl.statusMap["Temp"].levels - ((100 - psctl.statusMap["Hum"].levels)/5.0);
    float DpDep = psctl.statusMap["Temp"].levels - Dp;
    printf("\nTemp:     %4.1fF  Hum:      %4.1f%%   DewPoint %4.1fF   DP Dep      %4.1fF\n",
           psctl.statusMap["Temp"].levels,
           psctl.statusMap["Hum"].levels,
           Dp,
           DpDep
    );
    
    float pswatts = psctl.statusMap["IN"].levels * psctl.statusMap["IN"].current;
    printf("Volts-in: %5.2fV Amps-in: %5.2fA   Watts    %5.2fW  Amp-Hrs:  %6.1fAH\n",
           psctl.statusMap["IN"].levels,
           psctl.statusMap["IN"].current,
           pswatts,
           0.0
    );

    printFaults(psctl);
        
    printf("\nCmd: P'ower D'ew, F'ocus, H'andle Faults, S'ettings, R'estart Q'uit\n");
    
    printf("Command: ");
        getline(cin, cimput);
        boost::algorithm::to_lower(device);
        char command = cimput[0];

        switch(command) {
            // Change settings
            case 's': {
                settingsMenu(psctl);
                break;              
            }
            
            // Focus Menu
            case 'f': {
                focusMenu(psctl);
                break;              
            }
            
            // Faults Menu
            case 'h': {
                faultMenu(psctl);
                break;              
            }
        
            // turn output pwr and usb on/off
            case 'p': {;
                char pwrdev[11] = " ";
                askString(pwrdev, "device name", false);
                boost::algorithm::to_lower(pwrdev);
                device = string(pwrdev);
                
                if (device == "mp" &&  psctl.statusMap["MP"].setting != 0) {
                    printMsg("MP is not set to DC, use the M' command to change Dew or PWM settings");
                    break;
                }
                
                if (device == "usb1" || device == "usb4" || device == "usb5") {
                    printf("USB1,4,5 are not switchable\n");
                    break;
                }

                bool pwract;
                askYN(&pwract, " ",false);
                if ( pwract)
                    action = "yes";
                else
                    action = "no";

                if (! psctl.setPowerState(device, action))
                    printMsg("Problem configuring power");
                
                break;
            }
            
            // set dew power Settings
            case 'd' : {
                uint8_t udev;
                printf("Dew 1 or 2 ? ");
                getline(cin, device);
                boost::algorithm::to_lower(device);
                try {
                    udev = stoi(device) - 1;
                }
                catch (exception &err) {
                    printMsg("ERROR: Dew Heater must a number, either 1 or 2");
                    //getline(cin, message);
                    break;
                }
                if (udev < 0 || udev > 1) {
                    printMsg("ERROR: Dew Heater must be 1 or 2");
                    //getline(cin, message);
                    break;
                }
                
                printf("Percent Power ? ");
                getline(cin, action);
                boost::algorithm::to_lower(action);
                uint8_t usract;
                try {
                    usract = stoi(action);
                }
                catch (exception &err) {
                    printMsg("ERROR: Percent power must be a number between 0 and 100");
                    //getline(cin, message);
                    break;
                }
                if (usract < 0 || usract > 100) {
                    printMsg("ERROR: Percent power must be between 0 and 100");
                    //getline(cin, message);
                    break;
                }
        
                if ( ! psctl.setDew(udev, usract))
                    printMsg("Problem setting Dew");
                    //getline(cin, message);
                break;
                
                if ( ! psctl.saveDewPwmFault(curProfile))
                        printMsg("Problem saving Dew settings");
                    
                break;
            }
           
            // restart
            case 'r' : {
                psctl.restart();
                psctl.Disconnect();
                printf("Restarting: wait a moment before running this program again\n");
                return;
            }
            
            // quit
            case 'q' : {
                // if profile changes and not saved, ask if should do

                psctl.lockFocusMtr();
                psctl.Disconnect();
                return;
            }
            
            default: {
            }
        }           
      }
}

//************************************************************
int main (int argc, char* argv[])
{    
    PSCTL psctl;
    
    if (psctl.Connect())
    {
     // check that the config file exits, create it if not
     int rc;
     FILE* fin = fopen(configFile, "r");
     if (!fin) {
        printMsg("Error: can not find .ini file - will create one");
        // not found, so create it
        FILE* fin = fopen(configFile, "w+");
        if (!fin){
         	printMsg("Error: need to run this as root to create the config file");
         	return false;
    	}
        fclose(fin);
        chmod(configFile, S_IROTH|S_IWOTH);
        
        // and initialize it to unipolar
        curProfile = UNI_12V;
        if (!updateProfile(psctl)) {
            printMsg("Error: startup could not update config file");
            return false;
        }
     }
     
     // now open it for business
     fin = fopen(configFile, "r");
     rc = fread(&curProfile, sizeof(PowerStarProfile), 1, fin);
     
     if (!rc) {
        printMsg("Error: could not read entire profile");
        return false;
     }
     fclose(fin);
    
    isConnected = true;
    
    // unlock focus motor
    psctl.unLockFocusMtr();
    
    // set requested user limits initially to current limits on P*S
    psctl.getUserLimitStatus(reqUsrLimit);

    mainMenu(psctl);
        
            
    psctl.Disconnect();
        return true;
        
    }
    printf("Error:  could not connect to Power*Star\n");
    return false;
}
