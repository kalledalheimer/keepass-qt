/*
  Qt KeePass - Auto-Type Platform Factory
*/

#include "AutoTypePlatform.h"

#ifdef Q_OS_MAC
#include "AutoTypeMac.h"
#endif

AutoTypePlatform* AutoTypePlatform::create()
{
#ifdef Q_OS_MAC
    return new AutoTypeMac();
#else
    // Not implemented for other platforms yet
    return nullptr;
#endif
}
