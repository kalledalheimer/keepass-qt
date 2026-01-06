/*
  Qt KeePass - Auto-Type Platform Abstraction

  Cross-platform interface for keyboard simulation.
  Platform-specific implementations handle actual key event generation.

  Reference: MFC KeePassLibCpp/Util/AppUtil.h (CSendKeysEx)
*/

#ifndef AUTOTYPEPLATFORM_H
#define AUTOTYPEPLATFORM_H

#include <QString>
#include <QList>

/// Special key codes for auto-type
enum class AutoTypeKey
{
    Tab,
    Enter,
    Space,
    Backspace,
    Delete,
    Insert,
    Home,
    End,
    PageUp,
    PageDown,
    Left,
    Right,
    Up,
    Down,
    Escape,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    // Modifier keys
    Shift,
    Control,
    Alt,
    Command  // macOS Command key (Windows key on Windows)
};

/// Single auto-type action
struct AutoTypeAction
{
    enum class Type
    {
        Key,        // Press a special key
        Text,       // Type text string
        Delay,      // Wait specified milliseconds
        KeyDown,    // Press key down (without releasing)
        KeyUp       // Release key
    };

    Type type;
    QString text;           // For Text type
    AutoTypeKey key;        // For Key, KeyDown, KeyUp types
    int delayMs;            // For Delay type

    AutoTypeAction() : type(Type::Text), key(AutoTypeKey::Tab), delayMs(0) {}

    static AutoTypeAction makeText(const QString& t) {
        AutoTypeAction action;
        action.type = Type::Text;
        action.text = t;
        return action;
    }

    static AutoTypeAction makeKey(AutoTypeKey k) {
        AutoTypeAction action;
        action.type = Type::Key;
        action.key = k;
        return action;
    }

    static AutoTypeAction makeDelay(int ms) {
        AutoTypeAction action;
        action.type = Type::Delay;
        action.delayMs = ms;
        return action;
    }

    static AutoTypeAction makeKeyDown(AutoTypeKey k) {
        AutoTypeAction action;
        action.type = Type::KeyDown;
        action.key = k;
        return action;
    }

    static AutoTypeAction makeKeyUp(AutoTypeKey k) {
        AutoTypeAction action;
        action.type = Type::KeyUp;
        action.key = k;
        return action;
    }
};

/// Platform-independent auto-type interface
class AutoTypePlatform
{
public:
    virtual ~AutoTypePlatform() = default;

    /// Execute auto-type sequence
    /// @param actions List of actions to perform
    /// @param defaultDelay Default delay between actions in milliseconds
    /// @return true if successful, false on error
    virtual bool performAutoType(const QList<AutoTypeAction>& actions,
                                  int defaultDelay = 10) = 0;

    /// Release all modifier keys
    virtual void releaseModifiers() = 0;

    /// Check if auto-type is available on this platform
    virtual bool isAvailable() const = 0;

    /// Get last error message
    virtual QString lastError() const = 0;

    /// Create platform-specific instance
    static AutoTypePlatform* create();
};

#endif // AUTOTYPEPLATFORM_H
