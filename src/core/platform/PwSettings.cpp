/*
  Qt KeePass - Platform Settings Implementation
*/

#include "PwSettings.h"
#include <QCoreApplication>
#include <QStandardPaths>

PwSettings::PwSettings()
    : m_settings("KeePass", "KeePass")
{
    // Set application metadata for QSettings
    QCoreApplication::setOrganizationName("KeePass");
    QCoreApplication::setApplicationName("KeePass");
}

PwSettings::~PwSettings()
{
    // Ensure settings are written
    sync();
}

PwSettings& PwSettings::instance()
{
    static PwSettings instance;
    return instance;
}

// Database defaults

QString PwSettings::getLastDatabasePath() const
{
    return m_settings.value(KEY_LAST_DB_PATH, QString()).toString();
}

void PwSettings::setLastDatabasePath(const QString& path)
{
    m_settings.setValue(KEY_LAST_DB_PATH, path);
}

int PwSettings::getDefaultKeyRounds() const
{
    return m_settings.value(KEY_DEFAULT_KEY_ROUNDS, 600000).toInt();
}

void PwSettings::setDefaultKeyRounds(int rounds)
{
    m_settings.setValue(KEY_DEFAULT_KEY_ROUNDS, rounds);
}

quint32 PwSettings::getAlgorithm() const
{
    // Default to AES (algorithm value 0)
    return m_settings.value(KEY_ALGORITHM, 0).toUInt();
}

void PwSettings::setAlgorithm(quint32 algorithm)
{
    m_settings.setValue(KEY_ALGORITHM, algorithm);
}

// Security settings

int PwSettings::getClipboardTimeout() const
{
    return m_settings.value(KEY_CLIPBOARD_TIMEOUT, 12000).toInt();
}

void PwSettings::setClipboardTimeout(int timeoutMs)
{
    m_settings.setValue(KEY_CLIPBOARD_TIMEOUT, timeoutMs);
}

bool PwSettings::getLockOnMinimize() const
{
    return m_settings.value(KEY_LOCK_ON_MINIMIZE, false).toBool();
}

void PwSettings::setLockOnMinimize(bool lock)
{
    m_settings.setValue(KEY_LOCK_ON_MINIMIZE, lock);
}

bool PwSettings::getLockOnInactivity() const
{
    return m_settings.value(KEY_LOCK_ON_INACTIVITY, false).toBool();
}

void PwSettings::setLockOnInactivity(bool lock)
{
    m_settings.setValue(KEY_LOCK_ON_INACTIVITY, lock);
}

int PwSettings::getInactivityTimeout() const
{
    return m_settings.value(KEY_INACTIVITY_TIMEOUT, 300).toInt();
}

void PwSettings::setInactivityTimeout(int seconds)
{
    m_settings.setValue(KEY_INACTIVITY_TIMEOUT, seconds);
}

// UI preferences

bool PwSettings::getRememberWindowSize() const
{
    return m_settings.value(KEY_REMEMBER_WINDOW_SIZE, true).toBool();
}

void PwSettings::setRememberWindowSize(bool remember)
{
    m_settings.setValue(KEY_REMEMBER_WINDOW_SIZE, remember);
}

QByteArray PwSettings::getMainWindowGeometry() const
{
    return m_settings.value(KEY_MAIN_WINDOW_GEOMETRY, QByteArray()).toByteArray();
}

void PwSettings::setMainWindowGeometry(const QByteArray& geometry)
{
    m_settings.setValue(KEY_MAIN_WINDOW_GEOMETRY, geometry);
}

QByteArray PwSettings::getMainWindowState() const
{
    return m_settings.value(KEY_MAIN_WINDOW_STATE, QByteArray()).toByteArray();
}

void PwSettings::setMainWindowState(const QByteArray& state)
{
    m_settings.setValue(KEY_MAIN_WINDOW_STATE, state);
}

// Backup settings

bool PwSettings::getCreateBackups() const
{
    return m_settings.value(KEY_CREATE_BACKUPS, true).toBool();
}

void PwSettings::setCreateBackups(bool create)
{
    m_settings.setValue(KEY_CREATE_BACKUPS, create);
}

QString PwSettings::getBackupDirectory() const
{
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                          + "/KeePass Backups";
    return m_settings.value(KEY_BACKUP_DIRECTORY, defaultPath).toString();
}

void PwSettings::setBackupDirectory(const QString& path)
{
    m_settings.setValue(KEY_BACKUP_DIRECTORY, path);
}

// TAN Wizard settings

QString PwSettings::getTanChars() const
{
    return m_settings.value(KEY_TAN_CHARS, QString()).toString();
}

void PwSettings::setTanChars(const QString& chars)
{
    m_settings.setValue(KEY_TAN_CHARS, chars);
}

// Auto-Type Settings

QString PwSettings::getDefaultAutoTypeSequence() const
{
    return m_settings.value(KEY_AUTO_TYPE_DEFAULT_SEQUENCE,
                           QStringLiteral("{USERNAME}{TAB}{PASSWORD}{ENTER}")).toString();
}

void PwSettings::setDefaultAutoTypeSequence(const QString& sequence)
{
    m_settings.setValue(KEY_AUTO_TYPE_DEFAULT_SEQUENCE, sequence);
}

bool PwSettings::getAutoTypeEnabled() const
{
    return m_settings.value(KEY_AUTO_TYPE_ENABLED, true).toBool();
}

void PwSettings::setAutoTypeEnabled(bool enabled)
{
    m_settings.setValue(KEY_AUTO_TYPE_ENABLED, enabled);
}

// Advanced Auto-Type Settings

bool PwSettings::getAutoTypeMinimizeBeforeType() const
{
    // Default: true (minimize method, matching MFC default)
    return m_settings.value(KEY_AUTO_TYPE_MINIMIZE_BEFORE, true).toBool();
}

void PwSettings::setAutoTypeMinimizeBeforeType(bool minimize)
{
    m_settings.setValue(KEY_AUTO_TYPE_MINIMIZE_BEFORE, minimize);
}

quint32 PwSettings::getAutoTypeGlobalHotKey() const
{
    // Default: 0 (no hotkey set)
    // Format: lower 16 bits = key code, upper 16 bits = modifiers
    return m_settings.value(KEY_AUTO_TYPE_GLOBAL_HOTKEY, 0).toUInt();
}

void PwSettings::setAutoTypeGlobalHotKey(quint32 hotKey)
{
    m_settings.setValue(KEY_AUTO_TYPE_GLOBAL_HOTKEY, hotKey);
}

bool PwSettings::getAutoTypeSameKeyboardLayout() const
{
    // Default: true (enforce same keyboard layout)
    return m_settings.value(KEY_AUTO_TYPE_SAME_KEYBOARD_LAYOUT, true).toBool();
}

void PwSettings::setAutoTypeSameKeyboardLayout(bool same)
{
    m_settings.setValue(KEY_AUTO_TYPE_SAME_KEYBOARD_LAYOUT, same);
}

bool PwSettings::getAutoTypeSortSelectionItems() const
{
    // Default: true (sort entries in selection dialog)
    return m_settings.value(KEY_AUTO_TYPE_SORT_SELECTION, true).toBool();
}

void PwSettings::setAutoTypeSortSelectionItems(bool sort)
{
    m_settings.setValue(KEY_AUTO_TYPE_SORT_SELECTION, sort);
}

bool PwSettings::getAutoTypeNormalizeDashes() const
{
    // Default: true (normalize different dash types)
    return m_settings.value(KEY_AUTO_TYPE_NORMALIZE_DASHES, true).toBool();
}

void PwSettings::setAutoTypeNormalizeDashes(bool normalize)
{
    m_settings.setValue(KEY_AUTO_TYPE_NORMALIZE_DASHES, normalize);
}

bool PwSettings::getAutoTypeInternetExplorerFix() const
{
    // Default: false (IE fix not needed for most applications)
    return m_settings.value(KEY_AUTO_TYPE_IE_FIX, false).toBool();
}

void PwSettings::setAutoTypeInternetExplorerFix(bool fix)
{
    m_settings.setValue(KEY_AUTO_TYPE_IE_FIX, fix);
}

// View Options settings

bool PwSettings::getHidePasswordStars() const
{
    // Default: true (hide passwords with stars)
    // Reference: MFC PWMKEY_HIDESTARS, default TRUE
    return m_settings.value(KEY_HIDE_PASSWORD_STARS, true).toBool();
}

void PwSettings::setHidePasswordStars(bool hide)
{
    m_settings.setValue(KEY_HIDE_PASSWORD_STARS, hide);
}

bool PwSettings::getHideUsernameStars() const
{
    // Default: false (show usernames)
    // Reference: MFC PWMKEY_HIDEUSERS, default FALSE
    return m_settings.value(KEY_HIDE_USERNAME_STARS, false).toBool();
}

void PwSettings::setHideUsernameStars(bool hide)
{
    m_settings.setValue(KEY_HIDE_USERNAME_STARS, hide);
}

// Generic access

QVariant PwSettings::get(const QString& key, const QVariant& defaultValue) const
{
    return m_settings.value(key, defaultValue);
}

void PwSettings::set(const QString& key, const QVariant& value)
{
    m_settings.setValue(key, value);
}

// Persistence

void PwSettings::sync()
{
    m_settings.sync();
}
