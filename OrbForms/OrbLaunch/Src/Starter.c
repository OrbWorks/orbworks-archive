/******************************************************************************
 *
 * Copyright (c) 1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Starter.c
 *
 *****************************************************************************/

#include <PalmOS.h>
#include "StarterRsc.h"


/***********************************************************************
 *
 *   Entry Points
 *
 ***********************************************************************/


/***********************************************************************
 *
 *   Internal Structures
 *
 ***********************************************************************/
typedef struct 
    {
    UInt8 replaceme;
    } StarterPreferenceType;

typedef struct 
    {
    UInt8 replaceme;
    } StarterAppInfoType;

typedef StarterAppInfoType* StarterAppInfoPtr;


/***********************************************************************
 *
 *   Global variables
 *
 ***********************************************************************/
//static Boolean HideSecretRecords;


/***********************************************************************
 *
 *   Internal Constants
 *
 ***********************************************************************/
#define appFileCreator				 'STRT'
#define appVersionNum              0x01
#define appPrefID                  0x00
#define appPrefVersionNum          0x01

/***********************************************************************
 *
 * FUNCTION:    StarterPalmMain
 *
 * DESCRIPTION: This is the main entry point for the application.
 *
 * PARAMETERS:  cmd - word value specifying the launch code. 
 *              cmdPB - pointer to a structure that is associated with the launch code. 
 *              launchFlags -  word value providing extra information about the launch.
 *
 * RETURNED:    Result of launch
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
#define sysAppLaunchCmdStartApplet sysAppLaunchCmdCustomBase
#define sysAppLaunchCmdStartAppletWithArg sysAppLaunchCmdCustomBase + 1

static UInt32 StarterPalmMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
    char* name = NULL;
    int size = 32;
    UInt16 orbCmd = sysAppLaunchCmdStartApplet;

    switch (cmd)
        {
        case sysAppLaunchCmdStartAppletWithArg:
            size = 32 + StrLen((char*)cmdPBP) + 1;
            orbCmd = sysAppLaunchCmdStartAppletWithArg;
            
        case sysAppLaunchCmdNormalLaunch:
            name = MemPtrNew(size);
            MemSet(name, size, 0);
            MemMove(name, "AppletName\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 32);
            if (cmd == sysAppLaunchCmdStartAppletWithArg)
                StrCopy(name + 32, (char*)cmdPBP);
            MemPtrSetOwner(name, 0);

            if (!DmFindDatabase(0, "OrbFormsRT")) {
                FrmAlert(NoRuntimeAlert);
                return 1;
            }
            AppLaunchWithCommand('OrbF', orbCmd, name);

        default:
            break;

        }
    
    return errNone;
}


/***********************************************************************
 *
 * FUNCTION:    PilotMain
 *
 * DESCRIPTION: This is the main entry point for the application.
 *
 * PARAMETERS:  cmd - word value specifying the launch code. 
 *              cmdPB - pointer to a structure that is associated with the launch code. 
 *              launchFlags -  word value providing extra information about the launch.
 * RETURNED:    Result of launch
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
UInt32 PilotMain( UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
    return StarterPalmMain(cmd, cmdPBP, launchFlags);
}

