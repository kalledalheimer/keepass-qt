/*
  Qt KeePass - Window Manager for macOS

  Uses CoreGraphics CGWindowList API for window enumeration.
*/

#include "WindowManagerMac.h"
#include <QCoreApplication>

#ifdef Q_OS_MACOS

#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>

WindowManagerMac::WindowManagerMac()
{
}

WindowInfo WindowManagerMac::getForegroundWindow() const
{
    // Reference: Get the frontmost window using CGWindowListCopyWindowInfo
    // kCGWindowListOptionOnScreenOnly - only visible windows
    // kCGNullWindowID - all windows

    m_lastError.clear();

    // Get list of windows at kCGWindowLayer 0 (normal windows)
    CFArrayRef windowList = CGWindowListCopyWindowInfo(
        kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
        kCGNullWindowID);

    if (!windowList) {
        m_lastError = QStringLiteral("Failed to get window list");
        return {};
    }

    WindowInfo result;
    CFIndex count = CFArrayGetCount(windowList);

    // Find the first window with kCGWindowLayer 0 (normal window layer)
    // and kCGWindowIsOnscreen = true
    for (CFIndex i = 0; i < count; ++i) {
        CFDictionaryRef window = (CFDictionaryRef)CFArrayGetValueAtIndex(windowList, i);

        // Get window layer
        CFNumberRef layerRef = (CFNumberRef)CFDictionaryGetValue(window, kCGWindowLayer);
        int layer = 0;
        if (layerRef) {
            CFNumberGetValue(layerRef, kCFNumberIntType, &layer);
        }

        // Only consider normal windows (layer 0)
        if (layer != 0) {
            continue;
        }

        // Get window ID
        CFNumberRef windowIdRef = (CFNumberRef)CFDictionaryGetValue(window, kCGWindowNumber);
        quint32 windowId = 0;
        if (windowIdRef) {
            CFNumberGetValue(windowIdRef, kCFNumberSInt32Type, &windowId);
        }

        // Get window title (kCGWindowName)
        CFStringRef titleRef = (CFStringRef)CFDictionaryGetValue(window, kCGWindowName);
        if (titleRef) {
            CFIndex length = CFStringGetLength(titleRef);
            CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
            char* buffer = new char[maxSize];
            if (CFStringGetCString(titleRef, buffer, maxSize, kCFStringEncodingUTF8)) {
                result.title = QString::fromUtf8(buffer);
            }
            delete[] buffer;
        }

        // Get owner name (process name)
        CFStringRef ownerRef = (CFStringRef)CFDictionaryGetValue(window, kCGWindowOwnerName);
        if (ownerRef) {
            CFIndex length = CFStringGetLength(ownerRef);
            CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
            char* buffer = new char[maxSize];
            if (CFStringGetCString(ownerRef, buffer, maxSize, kCFStringEncodingUTF8)) {
                result.processName = QString::fromUtf8(buffer);
            }
            delete[] buffer;
        }

        result.windowId = windowId;

        // Found the frontmost window
        break;
    }

    CFRelease(windowList);
    return result;
}

QList<WindowInfo> WindowManagerMac::enumerateWindows(bool excludeSelf) const
{
    // Reference: Enumerate all on-screen windows

    m_lastError.clear();
    QList<WindowInfo> windows;

    QString appName = QCoreApplication::applicationName();

    CFArrayRef windowList = CGWindowListCopyWindowInfo(
        kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
        kCGNullWindowID);

    if (!windowList) {
        m_lastError = QStringLiteral("Failed to get window list");
        return windows;
    }

    CFIndex count = CFArrayGetCount(windowList);

    for (CFIndex i = 0; i < count; ++i) {
        CFDictionaryRef window = (CFDictionaryRef)CFArrayGetValueAtIndex(windowList, i);

        // Get window layer - only consider normal windows (layer 0)
        CFNumberRef layerRef = (CFNumberRef)CFDictionaryGetValue(window, kCGWindowLayer);
        int layer = 0;
        if (layerRef) {
            CFNumberGetValue(layerRef, kCFNumberIntType, &layer);
        }

        if (layer != 0) {
            continue;
        }

        WindowInfo info;

        // Get window ID
        CFNumberRef windowIdRef = (CFNumberRef)CFDictionaryGetValue(window, kCGWindowNumber);
        quint32 windowId = 0;
        if (windowIdRef) {
            CFNumberGetValue(windowIdRef, kCFNumberSInt32Type, &windowId);
        }
        info.windowId = windowId;

        // Get window title
        CFStringRef titleRef = (CFStringRef)CFDictionaryGetValue(window, kCGWindowName);
        if (titleRef) {
            CFIndex length = CFStringGetLength(titleRef);
            CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
            char* buffer = new char[maxSize];
            if (CFStringGetCString(titleRef, buffer, maxSize, kCFStringEncodingUTF8)) {
                info.title = QString::fromUtf8(buffer);
            }
            delete[] buffer;
        }

        // Get owner name (process name)
        CFStringRef ownerRef = (CFStringRef)CFDictionaryGetValue(window, kCGWindowOwnerName);
        if (ownerRef) {
            CFIndex length = CFStringGetLength(ownerRef);
            CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
            char* buffer = new char[maxSize];
            if (CFStringGetCString(ownerRef, buffer, maxSize, kCFStringEncodingUTF8)) {
                info.processName = QString::fromUtf8(buffer);
            }
            delete[] buffer;
        }

        // Skip if excluding self and this is our application
        if (excludeSelf && info.processName == appName) {
            continue;
        }

        // Skip windows without titles (usually system/utility windows)
        if (info.title.isEmpty()) {
            continue;
        }

        windows.append(info);
    }

    CFRelease(windowList);
    return windows;
}

QString WindowManagerMac::getWindowTitle(quint64 windowId) const
{
    // Get title for a specific window ID

    m_lastError.clear();

    CFArrayRef windowList = CGWindowListCopyWindowInfo(
        kCGWindowListOptionAll,
        kCGNullWindowID);

    if (!windowList) {
        m_lastError = QStringLiteral("Failed to get window list");
        return {};
    }

    QString title;
    CFIndex count = CFArrayGetCount(windowList);

    for (CFIndex i = 0; i < count; ++i) {
        CFDictionaryRef window = (CFDictionaryRef)CFArrayGetValueAtIndex(windowList, i);

        // Get window ID
        CFNumberRef windowIdRef = (CFNumberRef)CFDictionaryGetValue(window, kCGWindowNumber);
        quint32 wid = 0;
        if (windowIdRef) {
            CFNumberGetValue(windowIdRef, kCFNumberSInt32Type, &wid);
        }

        if (wid == static_cast<quint32>(windowId)) {
            // Found the window - get title
            CFStringRef titleRef = (CFStringRef)CFDictionaryGetValue(window, kCGWindowName);
            if (titleRef) {
                CFIndex length = CFStringGetLength(titleRef);
                CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
                char* buffer = new char[maxSize];
                if (CFStringGetCString(titleRef, buffer, maxSize, kCFStringEncodingUTF8)) {
                    title = QString::fromUtf8(buffer);
                }
                delete[] buffer;
            }
            break;
        }
    }

    CFRelease(windowList);
    return title;
}

bool WindowManagerMac::isAvailable() const
{
    // CoreGraphics is always available on macOS
    return true;
}

QString WindowManagerMac::lastError() const
{
    return m_lastError;
}

#endif // Q_OS_MACOS
