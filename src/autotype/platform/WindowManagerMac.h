/*
  Qt KeePass - Window Manager for macOS

  Uses CoreGraphics (CGWindowList) API for window enumeration.

  Reference: macOS CGWindow.h documentation
*/

#ifndef WINDOWMANAGERMAC_H
#define WINDOWMANAGERMAC_H

#include "WindowManager.h"

class WindowManagerMac : public WindowManager
{
public:
    WindowManagerMac();
    ~WindowManagerMac() override = default;

    // WindowManager interface
    [[nodiscard]] WindowInfo getForegroundWindow() const override;
    [[nodiscard]] QList<WindowInfo> enumerateWindows(bool excludeSelf = true) const override;
    [[nodiscard]] QString getWindowTitle(quint64 windowId) const override;
    [[nodiscard]] bool isAvailable() const override;
    [[nodiscard]] QString lastError() const override;

private:
    mutable QString m_lastError;
};

#endif // WINDOWMANAGERMAC_H
