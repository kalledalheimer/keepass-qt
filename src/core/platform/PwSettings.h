/*
  Qt KeePass - Platform Settings

  Cross-platform settings management using QSettings.
  Replaces Windows Registry access from MFC version.

  Platforms:
  - macOS: ~/Library/Preferences/com.keepass.KeePass.plist
  - Linux: ~/.config/KeePass/KeePass.conf
  - Windows: HKEY_CURRENT_USER\Software\KeePass
*/

#ifndef PWSETTINGS_H
#define PWSETTINGS_H

#include <QSettings>
#include <QString>
#include <QVariant>
#include "../PwStructs.h"

class PwSettings
{
public:
    // Singleton access
    static PwSettings& instance();

    // Prevent copying
    PwSettings(const PwSettings&) = delete;
    PwSettings& operator=(const PwSettings&) = delete;

    // Database defaults
    QString getLastDatabasePath() const;
    void setLastDatabasePath(const QString& path);

    int getDefaultKeyRounds() const;
    void setDefaultKeyRounds(int rounds);

    quint32 getAlgorithm() const;  // ALGO_AES or ALGO_TWOFISH
    void setAlgorithm(quint32 algorithm);

    // Security settings
    int getClipboardTimeout() const;  // Milliseconds (default: 12000)
    void setClipboardTimeout(int timeoutMs);

    bool getLockOnMinimize() const;
    void setLockOnMinimize(bool lock);

    bool getLockOnInactivity() const;
    void setLockOnInactivity(bool lock);

    int getInactivityTimeout() const;  // Seconds (default: 300)
    void setInactivityTimeout(int seconds);

    // UI preferences
    bool getRememberWindowSize() const;
    void setRememberWindowSize(bool remember);

    QByteArray getMainWindowGeometry() const;
    void setMainWindowGeometry(const QByteArray& geometry);

    QByteArray getMainWindowState() const;
    void setMainWindowState(const QByteArray& state);

    // Backup settings
    bool getCreateBackups() const;
    void setCreateBackups(bool create);

    QString getBackupDirectory() const;
    void setBackupDirectory(const QString& path);

    // TAN Wizard settings
    QString getTanChars() const;
    void setTanChars(const QString& chars);

    // Auto-Type settings
    [[nodiscard]] QString getDefaultAutoTypeSequence() const;
    void setDefaultAutoTypeSequence(const QString& sequence);

    [[nodiscard]] bool getAutoTypeEnabled() const;
    void setAutoTypeEnabled(bool enabled);

    // Advanced Auto-Type settings
    [[nodiscard]] bool getAutoTypeMinimizeBeforeType() const;  // Minimize vs drop-back
    void setAutoTypeMinimizeBeforeType(bool minimize);

    [[nodiscard]] quint32 getAutoTypeGlobalHotKey() const;  // Global hotkey (Qt::Key + modifiers)
    void setAutoTypeGlobalHotKey(quint32 hotKey);

    [[nodiscard]] bool getAutoTypeSameKeyboardLayout() const;  // Enforce same keyboard layout
    void setAutoTypeSameKeyboardLayout(bool same);

    [[nodiscard]] bool getAutoTypeSortSelectionItems() const;  // Sort entries in selection dialog
    void setAutoTypeSortSelectionItems(bool sort);

    [[nodiscard]] bool getAutoTypeNormalizeDashes() const;  // Normalize dashes in window titles
    void setAutoTypeNormalizeDashes(bool normalize);

    [[nodiscard]] bool getAutoTypeInternetExplorerFix() const;  // IE fix (add delay prefix)
    void setAutoTypeInternetExplorerFix(bool fix);

    // View Options settings
    [[nodiscard]] bool getHidePasswordStars() const;  // Hide passwords with stars (default: true)
    void setHidePasswordStars(bool hide);

    [[nodiscard]] bool getHideUsernameStars() const;  // Hide usernames with stars (default: false)
    void setHideUsernameStars(bool hide);

    // Generic access (for future settings)
    [[nodiscard]] QVariant get(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void set(const QString& key, const QVariant& value);

    // Persistence
    void sync();  // Force write to disk

private:
    PwSettings();
    ~PwSettings();

    QSettings m_settings;

    // Setting keys
    static constexpr const char* KEY_LAST_DB_PATH = "Database/LastPath";
    static constexpr const char* KEY_DEFAULT_KEY_ROUNDS = "Database/DefaultKeyRounds";
    static constexpr const char* KEY_ALGORITHM = "Database/Algorithm";
    static constexpr const char* KEY_CLIPBOARD_TIMEOUT = "Security/ClipboardTimeout";
    static constexpr const char* KEY_LOCK_ON_MINIMIZE = "Security/LockOnMinimize";
    static constexpr const char* KEY_LOCK_ON_INACTIVITY = "Security/LockOnInactivity";
    static constexpr const char* KEY_INACTIVITY_TIMEOUT = "Security/InactivityTimeout";
    static constexpr const char* KEY_REMEMBER_WINDOW_SIZE = "UI/RememberWindowSize";
    static constexpr const char* KEY_MAIN_WINDOW_GEOMETRY = "UI/MainWindowGeometry";
    static constexpr const char* KEY_MAIN_WINDOW_STATE = "UI/MainWindowState";
    static constexpr const char* KEY_CREATE_BACKUPS = "Backup/CreateBackups";
    static constexpr const char* KEY_BACKUP_DIRECTORY = "Backup/Directory";
    static constexpr const char* KEY_TAN_CHARS = "TAN/AcceptableChars";
    static constexpr const char* KEY_AUTO_TYPE_DEFAULT_SEQUENCE = "AutoType/DefaultSequence";
    static constexpr const char* KEY_AUTO_TYPE_ENABLED = "AutoType/Enabled";
    static constexpr const char* KEY_AUTO_TYPE_MINIMIZE_BEFORE = "AutoType/MinimizeBeforeType";
    static constexpr const char* KEY_AUTO_TYPE_GLOBAL_HOTKEY = "AutoType/GlobalHotKey";
    static constexpr const char* KEY_AUTO_TYPE_SAME_KEYBOARD_LAYOUT = "AutoType/SameKeyboardLayout";
    static constexpr const char* KEY_AUTO_TYPE_SORT_SELECTION = "AutoType/SortSelection";
    static constexpr const char* KEY_AUTO_TYPE_NORMALIZE_DASHES = "AutoType/NormalizeDashes";
    static constexpr const char* KEY_AUTO_TYPE_IE_FIX = "AutoType/InternetExplorerFix";
    static constexpr const char* KEY_HIDE_PASSWORD_STARS = "View/HidePasswordStars";
    static constexpr const char* KEY_HIDE_USERNAME_STARS = "View/HideUsernameStars";
};

#endif // PWSETTINGS_H
