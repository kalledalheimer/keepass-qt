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
