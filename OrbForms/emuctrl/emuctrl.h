
#ifndef EMUCTRL_EXPORT
#define EMUCTRL_EXPORT __declspec(dllimport)
#endif

bool EMUCTRL_EXPORT InstallFile(char*);
bool EMUCTRL_EXPORT HasDatabase(char*);
bool EMUCTRL_EXPORT IsEmulatorRunning();
void EMUCTRL_EXPORT EmulatorReset();
void EMUCTRL_EXPORT LaunchApp(char*);
void EMUCTRL_EXPORT EmulatorStartAppLauncher();
short EMUCTRL_EXPORT OrbFormsRt_ver();
