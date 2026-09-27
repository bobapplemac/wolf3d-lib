#ifndef WG_LINUX_CONSOLE_H
#define WG_LINUX_CONSOLE_H

int WG_LinuxConsoleSetDRMDevice(const char *path);
int WG_LinuxConsoleAddInputDevice(const char *path);
int WG_LinuxConsoleSetALSADevice(const char *name);
void WG_LinuxConsoleDisableAudio(void);

#endif
