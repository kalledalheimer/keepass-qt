/*
  Qt KeePass - Global Hotkey

  Cross-platform global hotkey registration for Auto-Type trigger.
  Allows the application to respond to hotkeys even when not focused.

  Platform implementations:
  - macOS: CGEventTap (Core Graphics)
  - Windows: RegisterHotKey API (stub)
  - Linux: X11 XGrabKey (stub)
*/

#ifndef GLOBALHOTKEY_H
#define GLOBALHOTKEY_H

#include <QObject>
#include <QKeySequence>

class GlobalHotkey : public QObject
{
    Q_OBJECT

public:
    // Singleton access
    static GlobalHotkey& instance();

    // Register a global hotkey
    // Returns true if successful, false if hotkey is already taken or unavailable
    bool registerHotkey(const QKeySequence &keySequence);

    // Unregister the current hotkey
    void unregisterHotkey();

    // Check if a hotkey is currently registered
    bool isRegistered() const;

    // Get the currently registered hotkey
    QKeySequence currentHotkey() const;

    // Check if global hotkeys are supported on this platform
    static bool isSupported();

    // Get a human-readable error message if registration failed
    QString lastError() const;

signals:
    // Emitted when the global hotkey is pressed
    void hotkeyTriggered();

    // Emitted when registration status changes
    void registrationChanged(bool registered);

private:
    GlobalHotkey();
    ~GlobalHotkey() override;

    // Disable copy
    GlobalHotkey(const GlobalHotkey&) = delete;
    GlobalHotkey& operator=(const GlobalHotkey&) = delete;

    // Platform-specific implementation (pimpl)
    class Private;
    Private *d;

    QKeySequence m_currentHotkey;
    QString m_lastError;
    bool m_registered;
};

#endif // GLOBALHOTKEY_H
