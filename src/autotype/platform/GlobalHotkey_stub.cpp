/*
  Qt KeePass - Global Hotkey (Stub Implementation)

  Used on platforms without global hotkey support or as placeholder
  for Windows/Linux until those implementations are complete.
*/

#include "../GlobalHotkey.h"

#if !defined(Q_OS_MAC)

#include <QDebug>

// Stub private implementation
class GlobalHotkey::Private
{
public:
    Private(GlobalHotkey *parent) : q(parent) {}
    ~Private() = default;

    bool setup(const QKeySequence &) { return false; }
    void cleanup() {}

    GlobalHotkey *q;
};

// GlobalHotkey implementation (stub)
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
    delete d;
}

bool GlobalHotkey::registerHotkey(const QKeySequence &keySequence)
{
    Q_UNUSED(keySequence)
    m_lastError = tr("Global hotkeys are not yet implemented on this platform.");
    qWarning() << "GlobalHotkey:" << m_lastError;
    return false;
}

void GlobalHotkey::unregisterHotkey()
{
    m_registered = false;
    m_currentHotkey = QKeySequence();
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
#ifdef Q_OS_WIN
    return false;  // TODO: Implement Windows support
#elif defined(Q_OS_LINUX)
    return false;  // TODO: Implement Linux support
#else
    return false;
#endif
}

QString GlobalHotkey::lastError() const
{
    return m_lastError;
}

#endif // !Q_OS_MAC
