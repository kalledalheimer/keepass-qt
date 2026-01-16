/*
  Qt KeePass - Window Manager Platform Abstraction

  Cross-platform interface for window enumeration and management.
  Used for global auto-type hotkey functionality.

  Reference: MFC PwSafeDlg.cpp OnHotKey (lines 10331-10560)
*/

#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#include <QString>
#include <QList>

/// Window information structure
struct WindowInfo
{
    QString title;              // Window title text
    quint64 windowId = 0;      // Platform-specific window ID/handle
    QString processName;       // Process name (optional, for filtering)
};

/// Platform-independent window manager interface
class WindowManager
{
public:
    virtual ~WindowManager() = default;

    /// Get the currently focused/foreground window
    /// @return WindowInfo for foreground window, empty if none
    [[nodiscard]] virtual WindowInfo getForegroundWindow() const = 0;

    /// Enumerate all visible windows
    /// @param excludeSelf If true, exclude the calling application's windows
    /// @return List of visible windows
    [[nodiscard]] virtual QList<WindowInfo> enumerateWindows(bool excludeSelf = true) const = 0;

    /// Get window title by window ID
    /// @param windowId Platform-specific window identifier
    /// @return Window title, empty if not found
    [[nodiscard]] virtual QString getWindowTitle(quint64 windowId) const = 0;

    /// Check if window manager is available/functional
    /// @return true if window operations are supported
    [[nodiscard]] virtual bool isAvailable() const = 0;

    /// Get last error message
    [[nodiscard]] virtual QString lastError() const = 0;

    /// Create platform-specific instance
    static WindowManager* create();
};

#endif // WINDOWMANAGER_H
