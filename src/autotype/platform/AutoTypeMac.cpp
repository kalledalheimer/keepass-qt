/*
  Qt KeePass - macOS Auto-Type Implementation

  Reference: MFC KeePassLibCpp/Util/AppUtil.cpp
*/

#include "AutoTypeMac.h"

#ifdef Q_OS_MAC

#include <QThread>
#include <ApplicationServices/ApplicationServices.h>

// macOS key codes (from HIToolbox/Events.h)
enum {
    kVK_ANSI_A = 0x00,
    kVK_ANSI_S = 0x01,
    kVK_ANSI_D = 0x02,
    kVK_ANSI_F = 0x03,
    kVK_ANSI_H = 0x04,
    kVK_ANSI_G = 0x05,
    kVK_ANSI_Z = 0x06,
    kVK_ANSI_X = 0x07,
    kVK_ANSI_C = 0x08,
    kVK_ANSI_V = 0x09,
    kVK_ANSI_B = 0x0B,
    kVK_ANSI_Q = 0x0C,
    kVK_ANSI_W = 0x0D,
    kVK_ANSI_E = 0x0E,
    kVK_ANSI_R = 0x0F,
    kVK_ANSI_Y = 0x10,
    kVK_ANSI_T = 0x11,
    kVK_ANSI_1 = 0x12,
    kVK_ANSI_2 = 0x13,
    kVK_ANSI_3 = 0x14,
    kVK_ANSI_4 = 0x15,
    kVK_ANSI_6 = 0x16,
    kVK_ANSI_5 = 0x17,
    kVK_ANSI_Equal = 0x18,
    kVK_ANSI_9 = 0x19,
    kVK_ANSI_7 = 0x1A,
    kVK_ANSI_Minus = 0x1B,
    kVK_ANSI_8 = 0x1C,
    kVK_ANSI_0 = 0x1D,
    kVK_ANSI_RightBracket = 0x1E,
    kVK_ANSI_O = 0x1F,
    kVK_ANSI_U = 0x20,
    kVK_ANSI_LeftBracket = 0x21,
    kVK_ANSI_I = 0x22,
    kVK_ANSI_P = 0x23,
    kVK_Return = 0x24,
    kVK_ANSI_L = 0x25,
    kVK_ANSI_J = 0x26,
    kVK_ANSI_Quote = 0x27,
    kVK_ANSI_K = 0x28,
    kVK_ANSI_Semicolon = 0x29,
    kVK_ANSI_Backslash = 0x2A,
    kVK_ANSI_Comma = 0x2B,
    kVK_ANSI_Slash = 0x2C,
    kVK_ANSI_N = 0x2D,
    kVK_ANSI_M = 0x2E,
    kVK_ANSI_Period = 0x2F,
    kVK_Tab = 0x30,
    kVK_Space = 0x31,
    kVK_ANSI_Grave = 0x32,
    kVK_Delete = 0x33,
    kVK_Escape = 0x35,
    kVK_Command = 0x37,
    kVK_Shift = 0x38,
    kVK_CapsLock = 0x39,
    kVK_Option = 0x3A,
    kVK_Control = 0x3B,
    kVK_RightShift = 0x3C,
    kVK_RightOption = 0x3D,
    kVK_RightControl = 0x3E,
    kVK_F1 = 0x7A,
    kVK_F2 = 0x7B,
    kVK_F3 = 0x7C,
    kVK_F4 = 0x7D,
    kVK_F5 = 0x7E,
    kVK_F6 = 0x7F,
    kVK_F7 = 0x80,
    kVK_F8 = 0x81,
    kVK_F9 = 0x82,
    kVK_F10 = 0x83,
    kVK_F11 = 0x84,
    kVK_F12 = 0x85,
    kVK_ForwardDelete = 0x75,
    kVK_Home = 0x73,
    kVK_End = 0x77,
    kVK_PageUp = 0x74,
    kVK_PageDown = 0x79,
    kVK_LeftArrow = 0x7B,
    kVK_RightArrow = 0x7C,
    kVK_DownArrow = 0x7D,
    kVK_UpArrow = 0x7E
};

AutoTypeMac::AutoTypeMac()
{
}

AutoTypeMac::~AutoTypeMac()
{
}

bool AutoTypeMac::performAutoType(const QList<AutoTypeAction>& actions, int defaultDelay)
{
    m_lastError.clear();

    // Check if we have accessibility permissions
    if (!isAvailable()) {
        m_lastError = "Accessibility permissions required. "
                     "Enable in System Preferences > Security & Privacy > Privacy > Accessibility";
        return false;
    }

    // Release all modifier keys before starting
    releaseModifiers();

    // Execute each action
    for (const AutoTypeAction& action : actions) {
        switch (action.type) {
            case AutoTypeAction::Type::Text:
                sendText(action.text);
                break;

            case AutoTypeAction::Type::Key:
                sendKeyPress(action.key);
                break;

            case AutoTypeAction::Type::KeyDown:
                sendKeyDown(action.key);
                break;

            case AutoTypeAction::Type::KeyUp:
                sendKeyUp(action.key);
                break;

            case AutoTypeAction::Type::Delay:
                delay(action.delayMs);
                break;
        }

        // Default delay between actions
        if (defaultDelay > 0) {
            delay(defaultDelay);
        }
    }

    return true;
}

void AutoTypeMac::releaseModifiers()
{
    // Release all modifier keys
    sendKeyUp(AutoTypeKey::Shift);
    sendKeyUp(AutoTypeKey::Control);
    sendKeyUp(AutoTypeKey::Alt);
    sendKeyUp(AutoTypeKey::Command);
}

bool AutoTypeMac::isAvailable() const
{
    // Check if we have accessibility permissions
    // This is required to send keyboard events on macOS
    return AXIsProcessTrusted();
}

QString AutoTypeMac::lastError() const
{
    return m_lastError;
}

void AutoTypeMac::sendKeyPress(AutoTypeKey key)
{
    sendKeyDown(key);
    sendKeyUp(key);
}

void AutoTypeMac::sendKeyDown(AutoTypeKey key)
{
    quint16 keyCode = keyCodeForAutoTypeKey(key);
    if (keyCode == 0xFFFF) {
        return;  // Invalid key
    }

    CGEventRef event = CGEventCreateKeyboardEvent(nullptr, keyCode, true);
    if (event) {
        CGEventPost(kCGHIDEventTap, event);
        CFRelease(event);
    }
}

void AutoTypeMac::sendKeyUp(AutoTypeKey key)
{
    quint16 keyCode = keyCodeForAutoTypeKey(key);
    if (keyCode == 0xFFFF) {
        return;  // Invalid key
    }

    CGEventRef event = CGEventCreateKeyboardEvent(nullptr, keyCode, false);
    if (event) {
        CGEventPost(kCGHIDEventTap, event);
        CFRelease(event);
    }
}

void AutoTypeMac::sendText(const QString& text)
{
    // Type each character
    for (const QChar& ch : text) {
        bool needsShift = false;
        quint16 keyCode = keyCodeForCharacter(ch, needsShift);

        if (keyCode == 0xFFFF) {
            // Use Unicode event for characters we can't map
            CGEventRef event = CGEventCreateKeyboardEvent(nullptr, 0, true);
            if (event) {
                UniChar unicodeChar = ch.unicode();
                CGEventKeyboardSetUnicodeString(event, 1, &unicodeChar);
                CGEventPost(kCGHIDEventTap, event);
                CFRelease(event);
            }
            continue;
        }

        // Press shift if needed
        if (needsShift) {
            sendKeyDown(AutoTypeKey::Shift);
        }

        // Type the character
        CGEventRef keyDown = CGEventCreateKeyboardEvent(nullptr, keyCode, true);
        CGEventRef keyUp = CGEventCreateKeyboardEvent(nullptr, keyCode, false);

        if (keyDown && keyUp) {
            CGEventPost(kCGHIDEventTap, keyDown);
            CGEventPost(kCGHIDEventTap, keyUp);
            CFRelease(keyDown);
            CFRelease(keyUp);
        }

        // Release shift if needed
        if (needsShift) {
            sendKeyUp(AutoTypeKey::Shift);
        }
    }
}

void AutoTypeMac::delay(int ms)
{
    QThread::msleep(ms);
}

quint16 AutoTypeMac::keyCodeForAutoTypeKey(AutoTypeKey key) const
{
    switch (key) {
        case AutoTypeKey::Tab:       return kVK_Tab;
        case AutoTypeKey::Enter:     return kVK_Return;
        case AutoTypeKey::Space:     return kVK_Space;
        case AutoTypeKey::Backspace: return kVK_Delete;
        case AutoTypeKey::Delete:    return kVK_ForwardDelete;
        case AutoTypeKey::Home:      return kVK_Home;
        case AutoTypeKey::End:       return kVK_End;
        case AutoTypeKey::PageUp:    return kVK_PageUp;
        case AutoTypeKey::PageDown:  return kVK_PageDown;
        case AutoTypeKey::Left:      return kVK_LeftArrow;
        case AutoTypeKey::Right:     return kVK_RightArrow;
        case AutoTypeKey::Up:        return kVK_UpArrow;
        case AutoTypeKey::Down:      return kVK_DownArrow;
        case AutoTypeKey::Escape:    return kVK_Escape;
        case AutoTypeKey::F1:        return kVK_F1;
        case AutoTypeKey::F2:        return kVK_F2;
        case AutoTypeKey::F3:        return kVK_F3;
        case AutoTypeKey::F4:        return kVK_F4;
        case AutoTypeKey::F5:        return kVK_F5;
        case AutoTypeKey::F6:        return kVK_F6;
        case AutoTypeKey::F7:        return kVK_F7;
        case AutoTypeKey::F8:        return kVK_F8;
        case AutoTypeKey::F9:        return kVK_F9;
        case AutoTypeKey::F10:       return kVK_F10;
        case AutoTypeKey::F11:       return kVK_F11;
        case AutoTypeKey::F12:       return kVK_F12;
        case AutoTypeKey::Shift:     return kVK_Shift;
        case AutoTypeKey::Control:   return kVK_Control;
        case AutoTypeKey::Alt:       return kVK_Option;
        case AutoTypeKey::Command:   return kVK_Command;
        default:                     return 0xFFFF;  // Invalid
    }
}

quint16 AutoTypeMac::keyCodeForCharacter(QChar ch, bool& needsShift) const
{
    needsShift = false;
    char c = ch.toLatin1();

    // Lowercase letters
    if (c >= 'a' && c <= 'z') {
        return kVK_ANSI_A + (c - 'a');
    }

    // Uppercase letters
    if (c >= 'A' && c <= 'Z') {
        needsShift = true;
        return kVK_ANSI_A + (c - 'A');
    }

    // Numbers
    if (c >= '0' && c <= '9') {
        if (c == '0') return kVK_ANSI_0;
        return kVK_ANSI_1 + (c - '1');
    }

    // Special characters (US keyboard layout)
    switch (c) {
        case ' ':  return kVK_Space;
        case '\t': return kVK_Tab;
        case '\n': return kVK_Return;
        case '\r': return kVK_Return;
        case '-':  return kVK_ANSI_Minus;
        case '=':  return kVK_ANSI_Equal;
        case '[':  return kVK_ANSI_LeftBracket;
        case ']':  return kVK_ANSI_RightBracket;
        case '\\': return kVK_ANSI_Backslash;
        case ';':  return kVK_ANSI_Semicolon;
        case '\'': return kVK_ANSI_Quote;
        case ',':  return kVK_ANSI_Comma;
        case '.':  return kVK_ANSI_Period;
        case '/':  return kVK_ANSI_Slash;
        case '`':  return kVK_ANSI_Grave;

        // Shifted characters
        case '!':  needsShift = true; return kVK_ANSI_1;
        case '@':  needsShift = true; return kVK_ANSI_2;
        case '#':  needsShift = true; return kVK_ANSI_3;
        case '$':  needsShift = true; return kVK_ANSI_4;
        case '%':  needsShift = true; return kVK_ANSI_5;
        case '^':  needsShift = true; return kVK_ANSI_6;
        case '&':  needsShift = true; return kVK_ANSI_7;
        case '*':  needsShift = true; return kVK_ANSI_8;
        case '(':  needsShift = true; return kVK_ANSI_9;
        case ')':  needsShift = true; return kVK_ANSI_0;
        case '_':  needsShift = true; return kVK_ANSI_Minus;
        case '+':  needsShift = true; return kVK_ANSI_Equal;
        case '{':  needsShift = true; return kVK_ANSI_LeftBracket;
        case '}':  needsShift = true; return kVK_ANSI_RightBracket;
        case '|':  needsShift = true; return kVK_ANSI_Backslash;
        case ':':  needsShift = true; return kVK_ANSI_Semicolon;
        case '"':  needsShift = true; return kVK_ANSI_Quote;
        case '<':  needsShift = true; return kVK_ANSI_Comma;
        case '>':  needsShift = true; return kVK_ANSI_Period;
        case '?':  needsShift = true; return kVK_ANSI_Slash;
        case '~':  needsShift = true; return kVK_ANSI_Grave;

        default:   return 0xFFFF;  // Use Unicode event
    }
}

#endif // Q_OS_MAC
