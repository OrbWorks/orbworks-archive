#ifndef __PILOT_H__
#include <PalmOS.h>
#include <PalmCompatibility.h>
#endif
#include <FloatMgr.h>
#include <SonyCLIE.h>

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;

#define ver30 sysMakeROMVersion(3,0,0,sysROMStageRelease,0)
#define ver31 sysMakeROMVersion(3,1,0,sysROMStageRelease,0)
#define ver35 sysMakeROMVersion(3,5,0,sysROMStageRelease,0)
#define ver40 sysMakeROMVersion(4,0,0,sysROMStageRelease,0)
#define ver50 sysMakeROMVersion(5,0,0,sysROMStageRelease,0)

#undef ERROR_CHECK_LEVEL
#define ERROR_CHECK_LEVEL 1

#include "compiler.h"
//#include "stdcover.h"
//#include "vector.h" // my own vector class
//#include "string.h"

void c_error(char*, short); // compiler error
void oom(); // out of memory


#define sysAppLaunchCmdCompile (sysAppLaunchCmdCustomBase + 1)
#define sysAppLaunchCmdCompileResult (sysAppLaunchCmdCustomBase + 1)
#define sysAppLaunchCmdCompileAndReturn (sysAppLaunchCmdCustomBase + 2)

//
// resize support
//

typedef enum {
    DIA_TYPE_NONE = 0,
    DIA_TYPE_SONY2,
    DIA_TYPE_PALM10,
    DIA_TYPE_PALM11,
    DIA_TYPE_UNKNOWN
} DIAtype;

typedef enum {
    DIA_STATE_MAX = 0,
    DIA_STATE_MIN,
    DIA_STATE_NO_STATUS_BAR,
    DIA_STATE_USER,
    DIA_STATE_UNDEFINED
} DIAState;

DIAtype InitDIA(void);
void CloseDIA(void);
void SetFormDIAPolicy(UInt16 formID);
bool HandleResizeNotification(UInt32 notificationType);
bool LayoutForm(word formID);
void InitFormDIA(word formID);

void SetDIAState(DIAState state);
DIAState GetDIAState();
bool GetDIARotation();
void SetDIARotation(bool portrait);
bool HandleResizeEvent(word formID, EventPtr pEvent);