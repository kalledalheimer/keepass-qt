/*
  Qt KeePass - Window Manager Platform Factory
*/

#include "WindowManager.h"

#ifdef Q_OS_MACOS
#include "WindowManagerMac.h"
#elif defined(Q_OS_WIN)
#include "WindowManagerWindows.h"
#elif defined(Q_OS_LINUX)
#include "WindowManagerLinux.h"
#endif

WindowManager* WindowManager::create()
{
#ifdef Q_OS_MACOS
    return new WindowManagerMac();
#elif defined(Q_OS_WIN)
    return new WindowManagerWindows();
#elif defined(Q_OS_LINUX)
    return new WindowManagerLinux();
#else
    return nullptr;  // Unsupported platform
#endif
}
