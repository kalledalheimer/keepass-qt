/*
  Qt KeePass - Options Dialog

  Comprehensive application settings dialog with 6 tabs.
  Ported from MFC: WinGUI/OptionsDlg.h

  Tabs:
  - Security: Lock settings, secure edits, expiration defaults
  - Interface: Grid, fonts, colors, tray options
  - Files: Newline sequence, save options
  - Memory: Clipboard timeout and settings
  - Setup: File associations, URL handlers
  - Advanced: All other application options
*/

#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QRadioButton>
#include <QPushButton>
#include <QLineEdit>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QFont>
#include <QColor>

class OptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OptionsDialog(QWidget* parent = nullptr);

    // Security tab settings
    bool lockOnMinimize() const { return m_lockOnMinimize; }
    bool lockOnWinLock() const { return m_lockOnWinLock; }
    bool lockAfterTime() const { return m_lockAfterTime; }
    int lockAfterSeconds() const { return m_lockAfterSeconds; }
    bool disableUnsafe() const { return m_disableUnsafe; }
    bool secureEdits() const { return m_secureEdits; }
    bool defaultExpire() const { return m_defaultExpire; }
    int defaultExpireDays() const { return m_defaultExpireDays; }

    // Interface tab settings
    bool imageButtons() const { return m_imageButtons; }
    bool entryGrid() const { return m_entryGrid; }
    bool columnAutoSize() const { return m_columnAutoSize; }
    bool minimizeToTray() const { return m_minimizeToTray; }
    bool closeMinimizes() const { return m_closeMinimizes; }
    QFont mainFont() const { return m_mainFont; }
    QFont passwordFont() const { return m_passwordFont; }
    QFont notesFont() const { return m_notesFont; }
    QColor rowHighlightColor() const { return m_rowHighlightColor; }

    // Files tab settings
    int newlineSequence() const { return m_newlineSequence; }
    bool saveOnLockAfterTimeMod() const { return m_saveOnLockAfterTimeMod; }

    // Memory tab settings
    int clipboardTimeoutSeconds() const { return m_clipboardTimeoutSeconds; }
    bool clearClipboardOnDbClose() const { return m_clearClipboardOnDbClose; }
    bool clipboardNoPersist() const { return m_clipboardNoPersist; }

    // Setup tab settings
    bool usePuttyForURLs() const { return m_usePuttyForURLs; }

    // Advanced tab settings
    bool rememberLastFile() const { return m_rememberLastFile; }
    bool autoOpenLastDb() const { return m_autoOpenLastDb; }
    bool startMinimized() const { return m_startMinimized; }
    bool autoSave() const { return m_autoSave; }
    bool singleInstance() const { return m_singleInstance; }
    bool checkForUpdate() const { return m_checkForUpdate; }
    bool autoShowExpired() const { return m_autoShowExpired; }
    bool autoShowExpiredSoon() const { return m_autoShowExpiredSoon; }
    bool backupEntries() const { return m_backupEntries; }
    bool deleteBackupsOnSave() const { return m_deleteBackupsOnSave; }
    bool quickFindInPasswords() const { return m_quickFindInPasswords; }
    bool quickFindIncBackup() const { return m_quickFindIncBackup; }
    bool quickFindIncExpired() const { return m_quickFindIncExpired; }
    bool focusAfterQuickFind() const { return m_focusAfterQuickFind; }
    bool showTrayOnlyIfTrayed() const { return m_showTrayOnlyIfTrayed; }
    bool singleClickTrayIcon() const { return m_singleClickTrayIcon; }
    bool rememberKeySources() const { return m_rememberKeySources; }
    bool minimizeOnLock() const { return m_minimizeOnLock; }
    bool exitInsteadOfLockAfterTime() const { return m_exitInsteadOfLockAfterTime; }
    bool showFullPath() const { return m_showFullPath; }
    bool disableSaveIfNotModified() const { return m_disableSaveIfNotModified; }
    bool useLocalTimeFormat() const { return m_useLocalTimeFormat; }
    bool registerRestoreHotKey() const { return m_registerRestoreHotKey; }
    bool deleteTANsAfterUse() const { return m_deleteTANsAfterUse; }
    bool useTransactedFileWrites() const { return m_useTransactedFileWrites; }
    bool startWithWindows() const { return m_startWithWindows; }
    bool copyURLsToClipboard() const { return m_copyURLsToClipboard; }
    bool dropToBackgroundOnCopy() const { return m_dropToBackgroundOnCopy; }
    bool enableRemoteControl() const { return m_enableRemoteControl; }
    bool alwaysAllowRemoteControl() const { return m_alwaysAllowRemoteControl; }

    // Setters for loading current settings
    void setLockOnMinimize(bool lock);
    void setLockOnWinLock(bool lock);
    void setLockAfterTime(bool lock);
    void setLockAfterSeconds(int seconds);
    void setDisableUnsafe(bool disable);
    void setSecureEdits(bool secure);
    void setDefaultExpire(bool expire);
    void setDefaultExpireDays(int days);
    void setImageButtons(bool enable);
    void setEntryGrid(bool enable);
    void setColumnAutoSize(bool enable);
    void setMinimizeToTray(bool enable);
    void setCloseMinimizes(bool enable);
    void setMainFont(const QFont& font);
    void setPasswordFont(const QFont& font);
    void setNotesFont(const QFont& font);
    void setRowHighlightColor(const QColor& color);
    void setNewlineSequence(int sequence);
    void setSaveOnLockAfterTimeMod(bool save);
    void setClipboardTimeoutSeconds(int seconds);
    void setClearClipboardOnDbClose(bool clear);
    void setClipboardNoPersist(bool noPersist);
    void setUsePuttyForURLs(bool use);
    void setRememberLastFile(bool remember);
    void setAutoOpenLastDb(bool autoOpen);
    void setStartMinimized(bool minimized);
    void setAutoSave(bool autoSave);
    void setSingleInstance(bool single);
    void setCheckForUpdate(bool check);
    void setAutoShowExpired(bool show);
    void setAutoShowExpiredSoon(bool show);
    void setBackupEntries(bool backup);
    void setDeleteBackupsOnSave(bool deleteBackups);
    void setQuickFindInPasswords(bool search);
    void setQuickFindIncBackup(bool include);
    void setQuickFindIncExpired(bool include);
    void setFocusAfterQuickFind(bool focus);
    void setShowTrayOnlyIfTrayed(bool show);
    void setSingleClickTrayIcon(bool singleClick);
    void setRememberKeySources(bool remember);
    void setMinimizeOnLock(bool minimize);
    void setExitInsteadOfLockAfterTime(bool exit);
    void setShowFullPath(bool show);
    void setDisableSaveIfNotModified(bool disable);
    void setUseLocalTimeFormat(bool use);
    void setRegisterRestoreHotKey(bool register_hotkey);
    void setDeleteTANsAfterUse(bool deleteTANs);
    void setUseTransactedFileWrites(bool use);
    void setStartWithWindows(bool start);
    void setCopyURLsToClipboard(bool copy);
    void setDropToBackgroundOnCopy(bool drop);
    void setEnableRemoteControl(bool enable);
    void setAlwaysAllowRemoteControl(bool allow);

protected:
    void accept() override;

private slots:
    void onSelectMainFont();
    void onSelectPasswordFont();
    void onSelectNotesFont();
    void onSelectRowHighlightColor();
    void onLockAfterTimeChanged(int state);
    void onDefaultExpireChanged(int state);
    void onCreateFileAssociation();
    void onDeleteFileAssociation();

private:
    void createSecurityTab();
    void createInterfaceTab();
    void createFilesTab();
    void createMemoryTab();
    void createSetupTab();
    void createAdvancedTab();

    void loadSettings();
    void saveSettings();

    QFont selectFont(const QFont& currentFont, const QString& title);

    // Main tab widget
    QTabWidget* m_tabWidget;

    // Security tab widgets
    QCheckBox* m_checkLockOnMinimize;
    QCheckBox* m_checkLockOnWinLock;
    QCheckBox* m_checkLockAfterTime;
    QSpinBox* m_spinLockAfterSeconds;
    QCheckBox* m_checkDisableUnsafe;
    QCheckBox* m_checkSecureEdits;
    QCheckBox* m_checkDefaultExpire;
    QSpinBox* m_spinDefaultExpireDays;

    // Interface tab widgets
    QCheckBox* m_checkImageButtons;
    QCheckBox* m_checkEntryGrid;
    QCheckBox* m_checkColumnAutoSize;
    QCheckBox* m_checkMinimizeToTray;
    QCheckBox* m_checkCloseMinimizes;
    QPushButton* m_btnSelectMainFont;
    QPushButton* m_btnSelectPasswordFont;
    QPushButton* m_btnSelectNotesFont;
    QPushButton* m_btnSelectRowHighlightColor;
    QLabel* m_labelMainFont;
    QLabel* m_labelPasswordFont;
    QLabel* m_labelNotesFont;

    // Files tab widgets
    QRadioButton* m_radioNewlineWindows;
    QRadioButton* m_radioNewlineUnix;
    QCheckBox* m_checkSaveOnLockAfterTimeMod;

    // Memory tab widgets
    QSpinBox* m_spinClipboardTimeout;
    QCheckBox* m_checkClearClipOnDbClose;
    QCheckBox* m_checkClipNoPersist;

    // Setup tab widgets
    QPushButton* m_btnCreateAssoc;
    QPushButton* m_btnDeleteAssoc;
    QCheckBox* m_checkUsePuttyForURLs;

    // Advanced tab widgets (using checkboxes with list layout)
    QListWidget* m_listAdvanced;
    QCheckBox* m_checkRememberLastFile;
    QCheckBox* m_checkAutoOpenLastDb;
    QCheckBox* m_checkStartMinimized;
    QCheckBox* m_checkAutoSave;
    QCheckBox* m_checkSingleInstance;
    QCheckBox* m_checkCheckForUpdate;
    QCheckBox* m_checkAutoShowExpired;
    QCheckBox* m_checkAutoShowExpiredSoon;
    QCheckBox* m_checkBackupEntries;
    QCheckBox* m_checkDeleteBackupsOnSave;
    QCheckBox* m_checkQuickFindInPasswords;
    QCheckBox* m_checkQuickFindIncBackup;
    QCheckBox* m_checkQuickFindIncExpired;
    QCheckBox* m_checkFocusAfterQuickFind;
    QCheckBox* m_checkShowTrayOnlyIfTrayed;
    QCheckBox* m_checkSingleClickTrayIcon;
    QCheckBox* m_checkRememberKeySources;
    QCheckBox* m_checkMinimizeOnLock;
    QCheckBox* m_checkExitInsteadOfLockAfterTime;
    QCheckBox* m_checkShowFullPath;
    QCheckBox* m_checkDisableSaveIfNotModified;
    QCheckBox* m_checkUseLocalTimeFormat;
    QCheckBox* m_checkRegisterRestoreHotKey;
    QCheckBox* m_checkDeleteTANsAfterUse;
    QCheckBox* m_checkUseTransactedFileWrites;
    QCheckBox* m_checkStartWithWindows;
    QCheckBox* m_checkCopyURLsToClipboard;
    QCheckBox* m_checkDropToBackgroundOnCopy;
    QCheckBox* m_checkEnableRemoteControl;
    QCheckBox* m_checkAlwaysAllowRemoteControl;

    // Settings values
    // Security
    bool m_lockOnMinimize;
    bool m_lockOnWinLock;
    bool m_lockAfterTime;
    int m_lockAfterSeconds;
    bool m_disableUnsafe;
    bool m_secureEdits;
    bool m_defaultExpire;
    int m_defaultExpireDays;

    // Interface
    bool m_imageButtons;
    bool m_entryGrid;
    bool m_columnAutoSize;
    bool m_minimizeToTray;
    bool m_closeMinimizes;
    QFont m_mainFont;
    QFont m_passwordFont;
    QFont m_notesFont;
    QColor m_rowHighlightColor;

    // Files
    int m_newlineSequence;  // 0 = Windows (CRLF), 1 = Unix (LF)
    bool m_saveOnLockAfterTimeMod;

    // Memory
    int m_clipboardTimeoutSeconds;
    bool m_clearClipboardOnDbClose;
    bool m_clipboardNoPersist;

    // Setup
    bool m_usePuttyForURLs;

    // Advanced
    bool m_rememberLastFile;
    bool m_autoOpenLastDb;
    bool m_startMinimized;
    bool m_autoSave;
    bool m_singleInstance;
    bool m_checkForUpdate;
    bool m_autoShowExpired;
    bool m_autoShowExpiredSoon;
    bool m_backupEntries;
    bool m_deleteBackupsOnSave;
    bool m_quickFindInPasswords;
    bool m_quickFindIncBackup;
    bool m_quickFindIncExpired;
    bool m_focusAfterQuickFind;
    bool m_showTrayOnlyIfTrayed;
    bool m_singleClickTrayIcon;
    bool m_rememberKeySources;
    bool m_minimizeOnLock;
    bool m_exitInsteadOfLockAfterTime;
    bool m_showFullPath;
    bool m_disableSaveIfNotModified;
    bool m_useLocalTimeFormat;
    bool m_registerRestoreHotKey;
    bool m_deleteTANsAfterUse;
    bool m_useTransactedFileWrites;
    bool m_startWithWindows;
    bool m_copyURLsToClipboard;
    bool m_dropToBackgroundOnCopy;
    bool m_enableRemoteControl;
    bool m_alwaysAllowRemoteControl;
};

#endif // OPTIONSDIALOG_H
