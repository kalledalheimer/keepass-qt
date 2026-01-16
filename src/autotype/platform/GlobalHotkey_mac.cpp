/*
  Qt KeePass - Global Hotkey (macOS Implementation)

  Uses CGEventTap to monitor keyboard events system-wide.
  Requires Accessibility permissions on macOS 10.14+.
*/

#include "../GlobalHotkey.h"

#ifdef Q_OS_MAC

#include <QDebug>
#include <QTimer>
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>

// Convert Qt key to macOS virtual key code
static CGKeyCode qtKeyToMacKeyCode(int qtKey)
{
    switch (qtKey) {
        case Qt::Key_A: return kVK_ANSI_A;
        case Qt::Key_B: return kVK_ANSI_B;
        case Qt::Key_C: return kVK_ANSI_C;
        case Qt::Key_D: return kVK_ANSI_D;
        case Qt::Key_E: return kVK_ANSI_E;
        case Qt::Key_F: return kVK_ANSI_F;
        case Qt::Key_G: return kVK_ANSI_G;
        case Qt::Key_H: return kVK_ANSI_H;
        case Qt::Key_I: return kVK_ANSI_I;
        case Qt::Key_J: return kVK_ANSI_J;
        case Qt::Key_K: return kVK_ANSI_K;
        case Qt::Key_L: return kVK_ANSI_L;
        case Qt::Key_M: return kVK_ANSI_M;
        case Qt::Key_N: return kVK_ANSI_N;
        case Qt::Key_O: return kVK_ANSI_O;
        case Qt::Key_P: return kVK_ANSI_P;
        case Qt::Key_Q: return kVK_ANSI_Q;
        case Qt::Key_R: return kVK_ANSI_R;
        case Qt::Key_S: return kVK_ANSI_S;
        case Qt::Key_T: return kVK_ANSI_T;
        case Qt::Key_U: return kVK_ANSI_U;
        case Qt::Key_V: return kVK_ANSI_V;
        case Qt::Key_W: return kVK_ANSI_W;
        case Qt::Key_X: return kVK_ANSI_X;
        case Qt::Key_Y: return kVK_ANSI_Y;
        case Qt::Key_Z: return kVK_ANSI_Z;
        case Qt::Key_0: return kVK_ANSI_0;
        case Qt::Key_1: return kVK_ANSI_1;
        case Qt::Key_2: return kVK_ANSI_2;
        case Qt::Key_3: return kVK_ANSI_3;
        case Qt::Key_4: return kVK_ANSI_4;
        case Qt::Key_5: return kVK_ANSI_5;
        case Qt::Key_6: return kVK_ANSI_6;
        case Qt::Key_7: return kVK_ANSI_7;
        case Qt::Key_8: return kVK_ANSI_8;
        case Qt::Key_9: return kVK_ANSI_9;
        case Qt::Key_F1: return kVK_F1;
        case Qt::Key_F2: return kVK_F2;
        case Qt::Key_F3: return kVK_F3;
        case Qt::Key_F4: return kVK_F4;
        case Qt::Key_F5: return kVK_F5;
        case Qt::Key_F6: return kVK_F6;
        case Qt::Key_F7: return kVK_F7;
        case Qt::Key_F8: return kVK_F8;
        case Qt::Key_F9: return kVK_F9;
        case Qt::Key_F10: return kVK_F10;
        case Qt::Key_F11: return kVK_F11;
        case Qt::Key_F12: return kVK_F12;
        case Qt::Key_Space: return kVK_Space;
        case Qt::Key_Return: return kVK_Return;
        case Qt::Key_Enter: return kVK_Return;
        case Qt::Key_Tab: return kVK_Tab;
        case Qt::Key_Escape: return kVK_Escape;
        default: return 0xFFFF;  // Invalid
    }
}

// Private implementation
class GlobalHotkey::Private
{
public:
    Private(GlobalHotkey *parent)
        : q(parent)
        , eventTap(nullptr)
        , runLoopSource(nullptr)
        , targetKeyCode(0xFFFF)
        , targetModifiers(0)
    {
    }

    ~Private()
    {
        cleanup();
    }

    bool setup(const QKeySequence &keySequence)
    {
        cleanup();

        if (keySequence.isEmpty()) {
            return false;
        }

        // Parse the key sequence (we only support single key combinations)
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QKeyCombination combo = keySequence[0];
        int key = combo.key();
        Qt::KeyboardModifiers mods = combo.keyboardModifiers();
#else
        int keyWithMods = keySequence[0];
        int key = keyWithMods & ~Qt::KeyboardModifierMask;
        Qt::KeyboardModifiers mods = Qt::KeyboardModifiers(keyWithMods & Qt::KeyboardModifierMask);
#endif

        targetKeyCode = qtKeyToMacKeyCode(key);
        if (targetKeyCode == 0xFFFF) {
            return false;
        }

        // Convert Qt modifiers to CGEventFlags
        targetModifiers = 0;
        if (mods & Qt::ControlModifier) {
            targetModifiers |= kCGEventFlagMaskControl;
        }
        if (mods & Qt::AltModifier) {
            targetModifiers |= kCGEventFlagMaskAlternate;
        }
        if (mods & Qt::ShiftModifier) {
            targetModifiers |= kCGEventFlagMaskShift;
        }
        if (mods & Qt::MetaModifier) {
            targetModifiers |= kCGEventFlagMaskCommand;
        }

        // Check accessibility permissions
        if (AXIsProcessTrusted() == false) {
            qWarning() << "GlobalHotkey: Accessibility permissions not granted";
            // Request permissions (shows dialog to user)
            const void *keys[] = { kAXTrustedCheckOptionPrompt };
            const void *values[] = { kCFBooleanTrue };
            CFDictionaryRef options = CFDictionaryCreate(
                kCFAllocatorDefault, keys, values, 1,
                &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks
            );
            AXIsProcessTrustedWithOptions(options);
            CFRelease(options);
            return false;
        }

        // Create event tap
        CGEventMask eventMask = CGEventMaskBit(kCGEventKeyDown);
        eventTap = CGEventTapCreate(
            kCGSessionEventTap,
            kCGHeadInsertEventTap,
            kCGEventTapOptionDefault,
            eventMask,
            &Private::eventCallback,
            this
        );

        if (eventTap == nullptr) {
            qWarning() << "GlobalHotkey: Failed to create event tap";
            return false;
        }

        // Add to run loop
        runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
        CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
        CGEventTapEnable(eventTap, true);

        qDebug() << "GlobalHotkey: Registered hotkey" << keySequence.toString();
        return true;
    }

    void cleanup()
    {
        if (eventTap != nullptr) {
            CGEventTapEnable(eventTap, false);
            if (runLoopSource != nullptr) {
                CFRunLoopRemoveSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
                CFRelease(runLoopSource);
                runLoopSource = nullptr;
            }
            CFRelease(eventTap);
            eventTap = nullptr;
        }
        targetKeyCode = 0xFFFF;
        targetModifiers = 0;
    }

    static CGEventRef eventCallback(CGEventTapProxy proxy, CGEventType type,
                                    CGEventRef event, void *userInfo)
    {
        Q_UNUSED(proxy)

        Private *self = static_cast<Private*>(userInfo);

        // Handle tap being disabled (e.g., due to timeout)
        if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
            if (self->eventTap != nullptr) {
                CGEventTapEnable(self->eventTap, true);
            }
            return event;
        }

        if (type != kCGEventKeyDown) {
            return event;
        }

        CGKeyCode keyCode = static_cast<CGKeyCode>(CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode));
        CGEventFlags flags = CGEventGetFlags(event);

        // Mask out caps lock and other non-modifier flags
        CGEventFlags modifierMask = kCGEventFlagMaskControl | kCGEventFlagMaskAlternate |
                                    kCGEventFlagMaskShift | kCGEventFlagMaskCommand;
        CGEventFlags currentMods = flags & modifierMask;

        if (keyCode == self->targetKeyCode && currentMods == self->targetModifiers) {
            // Emit signal on main thread
            QTimer::singleShot(0, self->q, &GlobalHotkey::hotkeyTriggered);

            // Consume the event (don't pass it to other apps)
            return nullptr;
        }

        return event;
    }

    GlobalHotkey *q;
    CFMachPortRef eventTap;
    CFRunLoopSourceRef runLoopSource;
    CGKeyCode targetKeyCode;
    CGEventFlags targetModifiers;
};

// GlobalHotkey implementation
GlobalHotkey& GlobalHotkey::instance()
{
    static GlobalHotkey instance;
    return instance;
}

GlobalHotkey::GlobalHotkey()
    : QObject(nullptr)
    , d(new Private(this))
    , m_registered(false)
{
}

GlobalHotkey::~GlobalHotkey()
{
    unregisterHotkey();
    delete d;
}

bool GlobalHotkey::registerHotkey(const QKeySequence &keySequence)
{
    if (keySequence.isEmpty()) {
        m_lastError = tr("Empty key sequence");
        return false;
    }

    // Unregister existing hotkey first
    unregisterHotkey();

    if (d->setup(keySequence)) {
        m_currentHotkey = keySequence;
        m_registered = true;
        m_lastError.clear();
        emit registrationChanged(true);
        return true;
    }

    m_lastError = tr("Failed to register hotkey. Check Accessibility permissions in System Preferences > Security & Privacy > Privacy > Accessibility.");
    return false;
}

void GlobalHotkey::unregisterHotkey()
{
    if (m_registered) {
        d->cleanup();
        m_currentHotkey = QKeySequence();
        m_registered = false;
        emit registrationChanged(false);
    }
}

bool GlobalHotkey::isRegistered() const
{
    return m_registered;
}

QKeySequence GlobalHotkey::currentHotkey() const
{
    return m_currentHotkey;
}

bool GlobalHotkey::isSupported()
{
    return true;  // macOS always supports global hotkeys (with permissions)
}

QString GlobalHotkey::lastError() const
{
    return m_lastError;
}

#endif // Q_OS_MAC
