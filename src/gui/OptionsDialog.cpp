/*
  Qt KeePass - Options Dialog Implementation

  Comprehensive application settings dialog with 6 tabs.
  Ported from MFC: WinGUI/OptionsDlg.cpp
*/

#include "OptionsDialog.h"
#include "core/platform/PwSettings.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QFontDialog>
#include <QColorDialog>
#include <QMessageBox>
#include <QScrollArea>

OptionsDialog::OptionsDialog(QWidget* parent)
    : QDialog(parent)
    , m_tabWidget(nullptr)
    , m_lockOnMinimize(false)
    , m_lockOnWinLock(false)
    , m_lockAfterTime(false)
    , m_lockAfterSeconds(300)
    , m_disableUnsafe(false)
    , m_secureEdits(false)
    , m_defaultExpire(false)
    , m_defaultExpireDays(365)
    , m_imageButtons(false)
    , m_entryGrid(false)
    , m_columnAutoSize(false)
    , m_minimizeToTray(false)
    , m_closeMinimizes(false)
    , m_rowHighlightColor(238, 238, 255)
    , m_newlineSequence(0)
    , m_saveOnLockAfterTimeMod(false)
    , m_clipboardTimeoutSeconds(10)
    , m_clearClipboardOnDbClose(true)
    , m_clipboardNoPersist(true)
    , m_usePuttyForURLs(false)
    , m_rememberLastFile(true)
    , m_autoOpenLastDb(false)
    , m_startMinimized(false)
    , m_autoSave(false)
    , m_singleInstance(false)
    , m_checkForUpdate(false)
    , m_autoShowExpired(false)
    , m_autoShowExpiredSoon(false)
    , m_backupEntries(true)
    , m_deleteBackupsOnSave(false)
    , m_quickFindInPasswords(false)
    , m_quickFindIncBackup(false)
    , m_quickFindIncExpired(false)
    , m_focusAfterQuickFind(false)
    , m_showTrayOnlyIfTrayed(false)
    , m_singleClickTrayIcon(false)
    , m_rememberKeySources(false)
    , m_minimizeOnLock(false)
    , m_exitInsteadOfLockAfterTime(false)
    , m_showFullPath(false)
    , m_disableSaveIfNotModified(false)
    , m_useLocalTimeFormat(false)
    , m_registerRestoreHotKey(false)
    , m_deleteTANsAfterUse(false)
    , m_useTransactedFileWrites(false)
    , m_startWithWindows(false)
    , m_copyURLsToClipboard(false)
    , m_dropToBackgroundOnCopy(false)
    , m_enableRemoteControl(false)
    , m_alwaysAllowRemoteControl(false)
{
    setWindowTitle(tr("Settings"));
    setMinimumSize(600, 500);
    resize(700, 600);

    // Create main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Create tab widget
    m_tabWidget = new QTabWidget(this);
    mainLayout->addWidget(m_tabWidget);

    // Create all tabs
    createSecurityTab();
    createInterfaceTab();
    createFilesTab();
    createMemoryTab();
    createAutoTypeTab();
    createSetupTab();
    createAdvancedTab();

    // Create button box
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Load current settings
    loadSettings();
}

void OptionsDialog::createSecurityTab()
{
    QWidget* securityTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(securityTab);

    // Lock settings group
    QGroupBox* lockGroup = new QGroupBox(tr("Lock Workspace"), securityTab);
    QVBoxLayout* lockLayout = new QVBoxLayout(lockGroup);

    m_checkLockOnMinimize = new QCheckBox(tr("Lock workspace when minimizing main window"), lockGroup);
    lockLayout->addWidget(m_checkLockOnMinimize);

    m_checkLockOnWinLock = new QCheckBox(tr("Lock workspace when computer is locked"), lockGroup);
    lockLayout->addWidget(m_checkLockOnWinLock);

    // Lock after time
    QHBoxLayout* lockAfterLayout = new QHBoxLayout();
    m_checkLockAfterTime = new QCheckBox(tr("Lock workspace after KeePass inactivity (seconds):"), lockGroup);
    m_spinLockAfterSeconds = new QSpinBox(lockGroup);
    m_spinLockAfterSeconds->setRange(1, 999999);
    m_spinLockAfterSeconds->setValue(300);
    lockAfterLayout->addWidget(m_checkLockAfterTime);
    lockAfterLayout->addWidget(m_spinLockAfterSeconds);
    lockAfterLayout->addStretch();
    lockLayout->addLayout(lockAfterLayout);

    connect(m_checkLockAfterTime, &QCheckBox::checkStateChanged,
            this, &OptionsDialog::onLockAfterTimeChanged);

    layout->addWidget(lockGroup);

    // Security options group
    QGroupBox* securityGroup = new QGroupBox(tr("Security Options"), securityTab);
    QVBoxLayout* securityLayout = new QVBoxLayout(securityGroup);

    m_checkDisableUnsafe = new QCheckBox(tr("Disable all unsafe functions (e.g. 'Send To' menu)"), securityGroup);
    securityLayout->addWidget(m_checkDisableUnsafe);

    m_checkSecureEdits = new QCheckBox(tr("Use secure edit controls (prevent control text display)"), securityGroup);
    securityLayout->addWidget(m_checkSecureEdits);

    layout->addWidget(securityGroup);

    // Default expiration group
    QGroupBox* expireGroup = new QGroupBox(tr("Default Entry Expiration"), securityTab);
    QHBoxLayout* expireLayout = new QHBoxLayout(expireGroup);

    m_checkDefaultExpire = new QCheckBox(tr("Default expiration for new entries (days):"), expireGroup);
    m_spinDefaultExpireDays = new QSpinBox(expireGroup);
    m_spinDefaultExpireDays->setRange(1, 36500);
    m_spinDefaultExpireDays->setValue(365);
    expireLayout->addWidget(m_checkDefaultExpire);
    expireLayout->addWidget(m_spinDefaultExpireDays);
    expireLayout->addStretch();

    connect(m_checkDefaultExpire, &QCheckBox::checkStateChanged,
            this, &OptionsDialog::onDefaultExpireChanged);

    layout->addWidget(expireGroup);

    layout->addStretch();
    m_tabWidget->addTab(securityTab, tr("Security"));
}

void OptionsDialog::createInterfaceTab()
{
    QWidget* interfaceTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(interfaceTab);

    // Display options group
    QGroupBox* displayGroup = new QGroupBox(tr("Display Options"), interfaceTab);
    QVBoxLayout* displayLayout = new QVBoxLayout(displayGroup);

    m_checkImageButtons = new QCheckBox(tr("Use image buttons (toolbar icons)"), displayGroup);
    displayLayout->addWidget(m_checkImageButtons);

    m_checkEntryGrid = new QCheckBox(tr("Show grid lines in entry list"), displayGroup);
    displayLayout->addWidget(m_checkEntryGrid);

    m_checkColumnAutoSize = new QCheckBox(tr("Automatically adjust column widths in entry list"), displayGroup);
    displayLayout->addWidget(m_checkColumnAutoSize);

    layout->addWidget(displayGroup);

    // Window behavior group
    QGroupBox* windowGroup = new QGroupBox(tr("Window Behavior"), interfaceTab);
    QVBoxLayout* windowLayout = new QVBoxLayout(windowGroup);

    m_checkMinimizeToTray = new QCheckBox(tr("Minimize to system tray instead of taskbar"), windowGroup);
    windowLayout->addWidget(m_checkMinimizeToTray);

    m_checkCloseMinimizes = new QCheckBox(tr("Minimize window when clicking close button (instead of exiting)"), windowGroup);
    windowLayout->addWidget(m_checkCloseMinimizes);

    layout->addWidget(windowGroup);

    // Font settings group
    QGroupBox* fontGroup = new QGroupBox(tr("Fonts"), interfaceTab);
    QGridLayout* fontLayout = new QGridLayout(fontGroup);

    fontLayout->addWidget(new QLabel(tr("Main font:"), fontGroup), 0, 0);
    m_btnSelectMainFont = new QPushButton(tr("Select..."), fontGroup);
    m_labelMainFont = new QLabel(tr("(Default)"), fontGroup);
    fontLayout->addWidget(m_btnSelectMainFont, 0, 1);
    fontLayout->addWidget(m_labelMainFont, 0, 2);
    connect(m_btnSelectMainFont, &QPushButton::clicked,
            this, &OptionsDialog::onSelectMainFont);

    fontLayout->addWidget(new QLabel(tr("Password font:"), fontGroup), 1, 0);
    m_btnSelectPasswordFont = new QPushButton(tr("Select..."), fontGroup);
    m_labelPasswordFont = new QLabel(tr("(Default)"), fontGroup);
    fontLayout->addWidget(m_btnSelectPasswordFont, 1, 1);
    fontLayout->addWidget(m_labelPasswordFont, 1, 2);
    connect(m_btnSelectPasswordFont, &QPushButton::clicked,
            this, &OptionsDialog::onSelectPasswordFont);

    fontLayout->addWidget(new QLabel(tr("Notes font:"), fontGroup), 2, 0);
    m_btnSelectNotesFont = new QPushButton(tr("Select..."), fontGroup);
    m_labelNotesFont = new QLabel(tr("(Default)"), fontGroup);
    fontLayout->addWidget(m_btnSelectNotesFont, 2, 1);
    fontLayout->addWidget(m_labelNotesFont, 2, 2);
    connect(m_btnSelectNotesFont, &QPushButton::clicked,
            this, &OptionsDialog::onSelectNotesFont);

    fontLayout->setColumnStretch(2, 1);
    layout->addWidget(fontGroup);

    // Color settings group
    QGroupBox* colorGroup = new QGroupBox(tr("Colors"), interfaceTab);
    QHBoxLayout* colorLayout = new QHBoxLayout(colorGroup);

    colorLayout->addWidget(new QLabel(tr("Row highlight color:"), colorGroup));
    m_btnSelectRowHighlightColor = new QPushButton(tr("Select..."), colorGroup);
    connect(m_btnSelectRowHighlightColor, &QPushButton::clicked,
            this, &OptionsDialog::onSelectRowHighlightColor);
    colorLayout->addWidget(m_btnSelectRowHighlightColor);
    colorLayout->addStretch();

    layout->addWidget(colorGroup);

    layout->addStretch();
    m_tabWidget->addTab(interfaceTab, tr("Interface"));
}

void OptionsDialog::createFilesTab()
{
    QWidget* filesTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(filesTab);

    // Newline sequence group
    QGroupBox* newlineGroup = new QGroupBox(tr("Newline Sequence"), filesTab);
    QVBoxLayout* newlineLayout = new QVBoxLayout(newlineGroup);

    newlineLayout->addWidget(new QLabel(tr("Select the newline sequence for text exports:"), newlineGroup));

    m_radioNewlineWindows = new QRadioButton(tr("Windows (CR+LF)"), newlineGroup);
    m_radioNewlineUnix = new QRadioButton(tr("Unix (LF)"), newlineGroup);
    m_radioNewlineWindows->setChecked(true);

    newlineLayout->addWidget(m_radioNewlineWindows);
    newlineLayout->addWidget(m_radioNewlineUnix);

    layout->addWidget(newlineGroup);

    // Save options group
    QGroupBox* saveGroup = new QGroupBox(tr("Save Options"), filesTab);
    QVBoxLayout* saveLayout = new QVBoxLayout(saveGroup);

    m_checkSaveOnLockAfterTimeMod = new QCheckBox(
        tr("Save database when 'Lock After Time' setting is modified"), saveGroup);
    saveLayout->addWidget(m_checkSaveOnLockAfterTimeMod);

    layout->addWidget(saveGroup);

    layout->addStretch();
    m_tabWidget->addTab(filesTab, tr("Files"));
}

void OptionsDialog::createMemoryTab()
{
    QWidget* memoryTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(memoryTab);

    // Clipboard group
    QGroupBox* clipboardGroup = new QGroupBox(tr("Clipboard"), memoryTab);
    QVBoxLayout* clipboardLayout = new QVBoxLayout(clipboardGroup);

    clipboardLayout->addWidget(new QLabel(
        tr("KeePass can automatically clear the clipboard after a specified time.\n"
           "This increases security by ensuring passwords are not left on the clipboard."),
        clipboardGroup));

    // Clipboard timeout
    QHBoxLayout* timeoutLayout = new QHBoxLayout();
    timeoutLayout->addWidget(new QLabel(tr("Auto-clear clipboard after (seconds):"), clipboardGroup));
    m_spinClipboardTimeout = new QSpinBox(clipboardGroup);
    m_spinClipboardTimeout->setRange(0, 999999);
    m_spinClipboardTimeout->setValue(10);
    timeoutLayout->addWidget(m_spinClipboardTimeout);
    timeoutLayout->addStretch();
    clipboardLayout->addLayout(timeoutLayout);

    m_checkClearClipOnDbClose = new QCheckBox(
        tr("Clear clipboard when closing/locking database"), clipboardGroup);
    m_checkClearClipOnDbClose->setChecked(true);
    clipboardLayout->addWidget(m_checkClearClipOnDbClose);

    m_checkClipNoPersist = new QCheckBox(
        tr("Don't allow clipboard persistence (enhanced security)"), clipboardGroup);
    m_checkClipNoPersist->setChecked(true);
    clipboardLayout->addWidget(m_checkClipNoPersist);

    layout->addWidget(clipboardGroup);

    layout->addStretch();
    m_tabWidget->addTab(memoryTab, tr("Memory"));
}

void OptionsDialog::createAutoTypeTab()
{
    // Reference: MFC/MFC-KeePass/WinGUI/OptionsAutoTypeDlg.cpp
    QWidget* autoTypeTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(autoTypeTab);

    // Auto-Type settings group
    QGroupBox* autoTypeGroup = new QGroupBox(tr("Auto-Type Settings"), autoTypeTab);
    QVBoxLayout* autoTypeLayout = new QVBoxLayout(autoTypeGroup);

    autoTypeLayout->addWidget(new QLabel(
        tr("Auto-Type allows KeePass to automatically type your username and password\n"
           "into login forms. Configure the global auto-type behavior below."),
        autoTypeGroup));

    autoTypeLayout->addSpacing(5);

    // Enable/Disable Auto-Type
    m_checkAutoTypeEnabled = new QCheckBox(
        tr("Enable Auto-Type functionality"), autoTypeGroup);
    m_checkAutoTypeEnabled->setChecked(true);
    autoTypeLayout->addWidget(m_checkAutoTypeEnabled);

    autoTypeLayout->addSpacing(10);

    // Default sequence
    autoTypeLayout->addWidget(new QLabel(
        tr("Default Auto-Type Sequence:"), autoTypeGroup));

    m_editDefaultAutoTypeSequence = new QLineEdit(autoTypeGroup);
    m_editDefaultAutoTypeSequence->setPlaceholderText(
        tr("e.g., {USERNAME}{TAB}{PASSWORD}{ENTER}"));
    m_editDefaultAutoTypeSequence->setText(QStringLiteral("{USERNAME}{TAB}{PASSWORD}{ENTER}"));
    autoTypeLayout->addWidget(m_editDefaultAutoTypeSequence);

    autoTypeLayout->addSpacing(10);

    // Global hotkey configuration
    autoTypeLayout->addWidget(new QLabel(
        tr("Global Auto-Type Hotkey:"), autoTypeGroup));

    QHBoxLayout* hotkeyLayout = new QHBoxLayout();
    m_hotkeyEdit = new QKeySequenceEdit(autoTypeGroup);
    m_hotkeyEdit->setKeySequence(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_A));
    m_hotkeyEdit->setToolTip(
        tr("Press the desired key combination to set the global hotkey.\n"
           "This hotkey will trigger Auto-Type from any application.\n"
           "Note: Requires Accessibility permissions on macOS."));
    hotkeyLayout->addWidget(m_hotkeyEdit);
    hotkeyLayout->addStretch();
    autoTypeLayout->addLayout(hotkeyLayout);

    autoTypeLayout->addWidget(new QLabel(
        tr("Available placeholders:\n"
           "  {USERNAME}, {PASSWORD}, {TITLE}, {URL}, {NOTES}\n"
           "  {TAB}, {ENTER}, {SPACE}, {BACKSPACE}, {DELETE}\n"
           "  {LEFT}, {RIGHT}, {UP}, {DOWN}, {HOME}, {END}\n"
           "  {F1} - {F12}, {DELAY X} (delay X milliseconds)"),
        autoTypeGroup));

    layout->addWidget(autoTypeGroup);

    // Advanced settings group
    QGroupBox* advancedGroup = new QGroupBox(tr("Advanced Options"), autoTypeTab);
    QVBoxLayout* advancedLayout = new QVBoxLayout(advancedGroup);

    // Minimize before auto-type
    m_checkAutoTypeMinimizeBeforeType = new QCheckBox(
        tr("Use alternative method (minimize KeePass window)"), advancedGroup);
    m_checkAutoTypeMinimizeBeforeType->setToolTip(
        tr("When enabled, KeePass minimizes before auto-typing.\n"
           "When disabled, KeePass just drops to the background."));
    m_checkAutoTypeMinimizeBeforeType->setChecked(true);
    advancedLayout->addWidget(m_checkAutoTypeMinimizeBeforeType);

    // Same keyboard layout
    m_checkAutoTypeSameKeyboardLayout = new QCheckBox(
        tr("Use same keyboard layout as KeePass window"), advancedGroup);
    m_checkAutoTypeSameKeyboardLayout->setToolTip(
        tr("Ensures auto-type uses the same keyboard layout that KeePass is using.\n"
           "Helps prevent issues with different keyboard layouts."));
    m_checkAutoTypeSameKeyboardLayout->setChecked(true);
    advancedLayout->addWidget(m_checkAutoTypeSameKeyboardLayout);

    // Sort selection items
    m_checkAutoTypeSortSelection = new QCheckBox(
        tr("Sort entries in auto-type selection dialog"), advancedGroup);
    m_checkAutoTypeSortSelection->setToolTip(
        tr("When multiple entries match, sort them by title in the selection dialog."));
    m_checkAutoTypeSortSelection->setChecked(true);
    advancedLayout->addWidget(m_checkAutoTypeSortSelection);

    // Normalize dashes
    m_checkAutoTypeNormalizeDashes = new QCheckBox(
        tr("Normalize dashes in window titles"), advancedGroup);
    m_checkAutoTypeNormalizeDashes->setToolTip(
        tr("Converts different types of dashes (em-dash, en-dash, minus, etc.)\n"
           "to a standard form for better window title matching."));
    m_checkAutoTypeNormalizeDashes->setChecked(true);
    advancedLayout->addWidget(m_checkAutoTypeNormalizeDashes);

    // Internet Explorer fix
    m_checkAutoTypeIEFix = new QCheckBox(
        tr("Internet Explorer compatibility mode"), advancedGroup);
    m_checkAutoTypeIEFix->setToolTip(
        tr("Adds a prefix to work around Internet Explorer auto-complete issues.\n"
           "Only enable if you experience problems with IE or similar browsers."));
    m_checkAutoTypeIEFix->setChecked(false);
    advancedLayout->addWidget(m_checkAutoTypeIEFix);

    layout->addWidget(advancedGroup);

    layout->addStretch();
    m_tabWidget->addTab(autoTypeTab, tr("Auto-Type"));
}

void OptionsDialog::createSetupTab()
{
    QWidget* setupTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(setupTab);

    // File association group
    QGroupBox* assocGroup = new QGroupBox(tr("File Associations"), setupTab);
    QVBoxLayout* assocLayout = new QVBoxLayout(assocGroup);

    assocLayout->addWidget(new QLabel(
        tr("Register KeePass as the default application for .kdb files."), assocGroup));

    m_btnCreateAssoc = new QPushButton(tr("Create File Association"), assocGroup);
    m_btnDeleteAssoc = new QPushButton(tr("Delete File Association"), assocGroup);

    connect(m_btnCreateAssoc, &QPushButton::clicked,
            this, &OptionsDialog::onCreateFileAssociation);
    connect(m_btnDeleteAssoc, &QPushButton::clicked,
            this, &OptionsDialog::onDeleteFileAssociation);

    assocLayout->addWidget(m_btnCreateAssoc);
    assocLayout->addWidget(m_btnDeleteAssoc);

    layout->addWidget(assocGroup);

    // URL handler group
    QGroupBox* urlGroup = new QGroupBox(tr("URL Handlers"), setupTab);
    QVBoxLayout* urlLayout = new QVBoxLayout(urlGroup);

    m_checkUsePuttyForURLs = new QCheckBox(
        tr("Use PuTTY for SSH URLs (ssh://host URLs will launch PuTTY)"), urlGroup);
    urlLayout->addWidget(m_checkUsePuttyForURLs);

    layout->addWidget(urlGroup);

    layout->addStretch();
    m_tabWidget->addTab(setupTab, tr("Setup"));
}

void OptionsDialog::createAdvancedTab()
{
    QWidget* advancedTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(advancedTab);

    // Create scroll area for all checkboxes
    QScrollArea* scrollArea = new QScrollArea(advancedTab);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget* scrollWidget = new QWidget();
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollWidget);

    // Integration group
    QGroupBox* integrationGroup = new QGroupBox(tr("Integration"), scrollWidget);
    QVBoxLayout* integrationLayout = new QVBoxLayout(integrationGroup);

    m_checkStartWithWindows = new QCheckBox(
        tr("Start KeePass at system startup (for current user)"), integrationGroup);
    integrationLayout->addWidget(m_checkStartWithWindows);

    m_checkCopyURLsToClipboard = new QCheckBox(
        tr("Copy URLs to clipboard instead of opening them (exception: cmd:// URLs)"), integrationGroup);
    integrationLayout->addWidget(m_checkCopyURLsToClipboard);

    m_checkDropToBackgroundOnCopy = new QCheckBox(
        tr("Drop to background when copying data to the clipboard"), integrationGroup);
    integrationLayout->addWidget(m_checkDropToBackgroundOnCopy);

    m_checkEnableRemoteControl = new QCheckBox(
        tr("Enable remote control (allow applications to communicate with KeePass)"), integrationGroup);
    integrationLayout->addWidget(m_checkEnableRemoteControl);

    m_checkAlwaysAllowRemoteControl = new QCheckBox(
        tr("Always grant full access through remote control (not recommended)"), integrationGroup);
    integrationLayout->addWidget(m_checkAlwaysAllowRemoteControl);

    scrollLayout->addWidget(integrationGroup);

    // Start and exit group
    QGroupBox* startExitGroup = new QGroupBox(tr("Start and Exit"), scrollWidget);
    QVBoxLayout* startExitLayout = new QVBoxLayout(startExitGroup);

    m_checkRememberLastFile = new QCheckBox(tr("Remember last opened file"), startExitGroup);
    startExitLayout->addWidget(m_checkRememberLastFile);

    m_checkAutoOpenLastDb = new QCheckBox(
        tr("Automatically open last used database on startup"), startExitGroup);
    startExitLayout->addWidget(m_checkAutoOpenLastDb);

    m_checkStartMinimized = new QCheckBox(tr("Start minimized and locked"), startExitGroup);
    startExitLayout->addWidget(m_checkStartMinimized);

    m_checkAutoSave = new QCheckBox(
        tr("Automatically save when closing/locking the database"), startExitGroup);
    startExitLayout->addWidget(m_checkAutoSave);

    m_checkSingleInstance = new QCheckBox(tr("Limit to single instance"), startExitGroup);
    startExitLayout->addWidget(m_checkSingleInstance);

    m_checkCheckForUpdate = new QCheckBox(tr("Check for updates at KeePass startup"), startExitGroup);
    startExitLayout->addWidget(m_checkCheckForUpdate);

    scrollLayout->addWidget(startExitGroup);

    // Immediately after opening database group
    QGroupBox* afterOpenGroup = new QGroupBox(tr("Immediately After Opening a Database"), scrollWidget);
    QVBoxLayout* afterOpenLayout = new QVBoxLayout(afterOpenGroup);

    m_checkAutoShowExpired = new QCheckBox(tr("Show expired entries (if any)"), afterOpenGroup);
    afterOpenLayout->addWidget(m_checkAutoShowExpired);

    m_checkAutoShowExpiredSoon = new QCheckBox(
        tr("Show entries that will expire soon (if any)"), afterOpenGroup);
    afterOpenLayout->addWidget(m_checkAutoShowExpiredSoon);

    scrollLayout->addWidget(afterOpenGroup);

    // Backup group
    QGroupBox* backupGroup = new QGroupBox(tr("Backup"), scrollWidget);
    QVBoxLayout* backupLayout = new QVBoxLayout(backupGroup);

    m_checkBackupEntries = new QCheckBox(
        tr("Save backups of modified entries into the 'Backup' group"), backupGroup);
    m_checkBackupEntries->setChecked(true);
    backupLayout->addWidget(m_checkBackupEntries);

    m_checkDeleteBackupsOnSave = new QCheckBox(
        tr("Delete all backup entries before saving the database"), backupGroup);
    backupLayout->addWidget(m_checkDeleteBackupsOnSave);

    scrollLayout->addWidget(backupGroup);

    // Quick search group
    QGroupBox* quickSearchGroup = new QGroupBox(tr("Quick Search (Toolbar)"), scrollWidget);
    QVBoxLayout* quickSearchLayout = new QVBoxLayout(quickSearchGroup);

    m_checkQuickFindInPasswords = new QCheckBox(
        tr("Search for passwords in quick searches"), quickSearchGroup);
    quickSearchLayout->addWidget(m_checkQuickFindInPasswords);

    m_checkQuickFindIncBackup = new QCheckBox(
        tr("Include backup entries in quick searches"), quickSearchGroup);
    quickSearchLayout->addWidget(m_checkQuickFindIncBackup);

    m_checkQuickFindIncExpired = new QCheckBox(
        tr("Include expired entries in quick searches"), quickSearchGroup);
    quickSearchLayout->addWidget(m_checkQuickFindIncExpired);

    m_checkFocusAfterQuickFind = new QCheckBox(
        tr("Focus entry list after a successful quick search"), quickSearchGroup);
    quickSearchLayout->addWidget(m_checkFocusAfterQuickFind);

    scrollLayout->addWidget(quickSearchGroup);

    // Tray icon group
    QGroupBox* trayGroup = new QGroupBox(tr("Tray Icon"), scrollWidget);
    QVBoxLayout* trayLayout = new QVBoxLayout(trayGroup);

    m_checkShowTrayOnlyIfTrayed = new QCheckBox(
        tr("Show tray icon only if main window has been sent to tray"), trayGroup);
    trayLayout->addWidget(m_checkShowTrayOnlyIfTrayed);

    m_checkSingleClickTrayIcon = new QCheckBox(
        tr("Single left click instead of double-click for default tray icon action"), trayGroup);
    trayLayout->addWidget(m_checkSingleClickTrayIcon);

    scrollLayout->addWidget(trayGroup);

    // Advanced options group
    QGroupBox* advancedGroup = new QGroupBox(tr("Advanced"), scrollWidget);
    QVBoxLayout* advancedLayout = new QVBoxLayout(advancedGroup);

    m_checkRememberKeySources = new QCheckBox(
        tr("Remember key sources (key file paths, provider names, ...)"), advancedGroup);
    advancedLayout->addWidget(m_checkRememberKeySources);

    m_checkMinimizeOnLock = new QCheckBox(
        tr("Minimize main window after locking the workspace"), advancedGroup);
    advancedLayout->addWidget(m_checkMinimizeOnLock);

    m_checkExitInsteadOfLockAfterTime = new QCheckBox(
        tr("Exit program instead of locking the workspace after the specified time"), advancedGroup);
    advancedLayout->addWidget(m_checkExitInsteadOfLockAfterTime);

    m_checkShowFullPath = new QCheckBox(
        tr("Show full path in the title bar (instead of file name only)"), advancedGroup);
    advancedLayout->addWidget(m_checkShowFullPath);

    m_checkDisableSaveIfNotModified = new QCheckBox(
        tr("Disable 'Save' button if the database hasn't been modified"), advancedGroup);
    advancedLayout->addWidget(m_checkDisableSaveIfNotModified);

    m_checkUseLocalTimeFormat = new QCheckBox(
        tr("Use local date/time format instead of ISO notation"), advancedGroup);
    advancedLayout->addWidget(m_checkUseLocalTimeFormat);

    m_checkRegisterRestoreHotKey = new QCheckBox(
        tr("Register Ctrl+Alt+K hot key (brings the KeePass window to front)"), advancedGroup);
    advancedLayout->addWidget(m_checkRegisterRestoreHotKey);

    m_checkDeleteTANsAfterUse = new QCheckBox(
        tr("Delete TAN entries immediately after using them"), advancedGroup);
    advancedLayout->addWidget(m_checkDeleteTANsAfterUse);

    m_checkUseTransactedFileWrites = new QCheckBox(
        tr("Use file transactions for writing databases"), advancedGroup);
    advancedLayout->addWidget(m_checkUseTransactedFileWrites);

    scrollLayout->addWidget(advancedGroup);
    scrollLayout->addStretch();

    scrollWidget->setLayout(scrollLayout);
    scrollArea->setWidget(scrollWidget);

    layout->addWidget(scrollArea);
    m_tabWidget->addTab(advancedTab, tr("Advanced"));
}

void OptionsDialog::accept()
{
    // Collect all settings from UI elements to member variables
    // Security
    m_lockOnMinimize = m_checkLockOnMinimize->isChecked();
    m_lockOnWinLock = m_checkLockOnWinLock->isChecked();
    m_lockAfterTime = m_checkLockAfterTime->isChecked();
    m_lockAfterSeconds = m_spinLockAfterSeconds->value();
    m_disableUnsafe = m_checkDisableUnsafe->isChecked();
    m_secureEdits = m_checkSecureEdits->isChecked();
    m_defaultExpire = m_checkDefaultExpire->isChecked();
    m_defaultExpireDays = m_spinDefaultExpireDays->value();

    // Interface
    m_imageButtons = m_checkImageButtons->isChecked();
    m_entryGrid = m_checkEntryGrid->isChecked();
    m_columnAutoSize = m_checkColumnAutoSize->isChecked();
    m_minimizeToTray = m_checkMinimizeToTray->isChecked();
    m_closeMinimizes = m_checkCloseMinimizes->isChecked();
    // Fonts are already updated in setter methods

    // Files
    m_newlineSequence = m_radioNewlineWindows->isChecked() ? 0 : 1;
    m_saveOnLockAfterTimeMod = m_checkSaveOnLockAfterTimeMod->isChecked();

    // Memory
    m_clipboardTimeoutSeconds = m_spinClipboardTimeout->value();
    m_clearClipboardOnDbClose = m_checkClearClipOnDbClose->isChecked();
    m_clipboardNoPersist = m_checkClipNoPersist->isChecked();

    // Auto-Type
    m_autoTypeEnabled = m_checkAutoTypeEnabled->isChecked();
    m_defaultAutoTypeSequence = m_editDefaultAutoTypeSequence->text();
    m_autoTypeGlobalHotkey = m_hotkeyEdit->keySequence();
    m_autoTypeMinimizeBeforeType = m_checkAutoTypeMinimizeBeforeType->isChecked();
    m_autoTypeSameKeyboardLayout = m_checkAutoTypeSameKeyboardLayout->isChecked();
    m_autoTypeSortSelection = m_checkAutoTypeSortSelection->isChecked();
    m_autoTypeNormalizeDashes = m_checkAutoTypeNormalizeDashes->isChecked();
    m_autoTypeIEFix = m_checkAutoTypeIEFix->isChecked();

    // Setup
    m_usePuttyForURLs = m_checkUsePuttyForURLs->isChecked();

    // Advanced
    m_rememberLastFile = m_checkRememberLastFile->isChecked();
    m_autoOpenLastDb = m_checkAutoOpenLastDb->isChecked();
    m_startMinimized = m_checkStartMinimized->isChecked();
    m_autoSave = m_checkAutoSave->isChecked();
    m_singleInstance = m_checkSingleInstance->isChecked();
    m_checkForUpdate = m_checkCheckForUpdate->isChecked();
    m_autoShowExpired = m_checkAutoShowExpired->isChecked();
    m_autoShowExpiredSoon = m_checkAutoShowExpiredSoon->isChecked();
    m_backupEntries = m_checkBackupEntries->isChecked();
    m_deleteBackupsOnSave = m_checkDeleteBackupsOnSave->isChecked();
    m_quickFindInPasswords = m_checkQuickFindInPasswords->isChecked();
    m_quickFindIncBackup = m_checkQuickFindIncBackup->isChecked();
    m_quickFindIncExpired = m_checkQuickFindIncExpired->isChecked();
    m_focusAfterQuickFind = m_checkFocusAfterQuickFind->isChecked();
    m_showTrayOnlyIfTrayed = m_checkShowTrayOnlyIfTrayed->isChecked();
    m_singleClickTrayIcon = m_checkSingleClickTrayIcon->isChecked();
    m_rememberKeySources = m_checkRememberKeySources->isChecked();
    m_minimizeOnLock = m_checkMinimizeOnLock->isChecked();
    m_exitInsteadOfLockAfterTime = m_checkExitInsteadOfLockAfterTime->isChecked();
    m_showFullPath = m_checkShowFullPath->isChecked();
    m_disableSaveIfNotModified = m_checkDisableSaveIfNotModified->isChecked();
    m_useLocalTimeFormat = m_checkUseLocalTimeFormat->isChecked();
    m_registerRestoreHotKey = m_checkRegisterRestoreHotKey->isChecked();
    m_deleteTANsAfterUse = m_checkDeleteTANsAfterUse->isChecked();
    m_useTransactedFileWrites = m_checkUseTransactedFileWrites->isChecked();
    m_startWithWindows = m_checkStartWithWindows->isChecked();
    m_copyURLsToClipboard = m_checkCopyURLsToClipboard->isChecked();
    m_dropToBackgroundOnCopy = m_checkDropToBackgroundOnCopy->isChecked();
    m_enableRemoteControl = m_checkEnableRemoteControl->isChecked();
    m_alwaysAllowRemoteControl = m_checkAlwaysAllowRemoteControl->isChecked();

    // Save all settings to disk
    saveSettings();

    // Call base class accept
    QDialog::accept();
}

void OptionsDialog::loadSettings()
{
    PwSettings& settings = PwSettings::instance();

    // Security settings
    setLockOnMinimize(settings.get("Security/LockOnMinimize", false).toBool());
    setLockOnWinLock(settings.get("Security/LockOnWinLock", false).toBool());
    setLockAfterTime(settings.get("Security/LockAfterTime", false).toBool());
    setLockAfterSeconds(settings.get("Security/LockAfterSeconds", 300).toInt());
    setDisableUnsafe(settings.get("Security/DisableUnsafe", false).toBool());
    setSecureEdits(settings.get("Security/SecureEdits", false).toBool());
    setDefaultExpire(settings.get("Security/DefaultExpire", false).toBool());
    setDefaultExpireDays(settings.get("Security/DefaultExpireDays", 365).toInt());

    // Interface settings
    setImageButtons(settings.get("Interface/ImageButtons", false).toBool());
    setEntryGrid(settings.get("Interface/EntryGrid", false).toBool());
    setColumnAutoSize(settings.get("Interface/ColumnAutoSize", false).toBool());
    setMinimizeToTray(settings.get("Interface/MinimizeToTray", false).toBool());
    setCloseMinimizes(settings.get("Interface/CloseMinimizes", false).toBool());

    QFont mainFont;
    if (mainFont.fromString(settings.get("Interface/MainFont", "").toString())) {
        setMainFont(mainFont);
    }

    QFont passwordFont;
    if (passwordFont.fromString(settings.get("Interface/PasswordFont", "").toString())) {
        setPasswordFont(passwordFont);
    }

    QFont notesFont;
    if (notesFont.fromString(settings.get("Interface/NotesFont", "").toString())) {
        setNotesFont(notesFont);
    }

    QColor rowColor(settings.get("Interface/RowHighlightColor", m_rowHighlightColor).value<QColor>());
    setRowHighlightColor(rowColor);

    // Files settings
    setNewlineSequence(settings.get("Files/NewlineSequence", 0).toInt());
    setSaveOnLockAfterTimeMod(settings.get("Files/SaveOnLockAfterTimeMod", false).toBool());

    // Memory settings
    setClipboardTimeoutSeconds(settings.get("Memory/ClipboardTimeout", 10).toInt());
    setClearClipboardOnDbClose(settings.get("Memory/ClearClipOnDbClose", true).toBool());
    setClipboardNoPersist(settings.get("Memory/ClipNoPersist", true).toBool());

    // Auto-Type settings
    setAutoTypeEnabled(settings.getAutoTypeEnabled());
    setDefaultAutoTypeSequence(settings.getDefaultAutoTypeSequence());
    // Load global hotkey - convert from quint32 to QKeySequence
    quint32 hotkeyValue = settings.getAutoTypeGlobalHotKey();
    if (hotkeyValue != 0) {
        setAutoTypeGlobalHotkey(QKeySequence(static_cast<int>(hotkeyValue)));
    } else {
        // Default hotkey: Ctrl+Alt+A
        setAutoTypeGlobalHotkey(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_A));
    }
    setAutoTypeMinimizeBeforeType(settings.getAutoTypeMinimizeBeforeType());
    setAutoTypeSameKeyboardLayout(settings.getAutoTypeSameKeyboardLayout());
    setAutoTypeSortSelection(settings.getAutoTypeSortSelectionItems());
    setAutoTypeNormalizeDashes(settings.getAutoTypeNormalizeDashes());
    setAutoTypeIEFix(settings.getAutoTypeInternetExplorerFix());

    // Setup settings
    setUsePuttyForURLs(settings.get("Setup/UsePuttyForURLs", false).toBool());

    // Advanced settings
    setRememberLastFile(settings.get("Advanced/RememberLastFile", true).toBool());
    setAutoOpenLastDb(settings.get("Advanced/AutoOpenLastDb", false).toBool());
    setStartMinimized(settings.get("Advanced/StartMinimized", false).toBool());
    setAutoSave(settings.get("Advanced/AutoSave", false).toBool());
    setSingleInstance(settings.get("Advanced/SingleInstance", false).toBool());
    setCheckForUpdate(settings.get("Advanced/CheckForUpdate", false).toBool());
    setAutoShowExpired(settings.get("Advanced/AutoShowExpired", false).toBool());
    setAutoShowExpiredSoon(settings.get("Advanced/AutoShowExpiredSoon", false).toBool());
    setBackupEntries(settings.get("Advanced/BackupEntries", true).toBool());
    setDeleteBackupsOnSave(settings.get("Advanced/DeleteBackupsOnSave", false).toBool());
    setQuickFindInPasswords(settings.get("Advanced/QuickFindInPasswords", false).toBool());
    setQuickFindIncBackup(settings.get("Advanced/QuickFindIncBackup", false).toBool());
    setQuickFindIncExpired(settings.get("Advanced/QuickFindIncExpired", false).toBool());
    setFocusAfterQuickFind(settings.get("Advanced/FocusAfterQuickFind", false).toBool());
    setShowTrayOnlyIfTrayed(settings.get("Advanced/ShowTrayOnlyIfTrayed", false).toBool());
    setSingleClickTrayIcon(settings.get("Advanced/SingleClickTrayIcon", false).toBool());
    setRememberKeySources(settings.get("Advanced/RememberKeySources", false).toBool());
    setMinimizeOnLock(settings.get("Advanced/MinimizeOnLock", false).toBool());
    setExitInsteadOfLockAfterTime(settings.get("Advanced/ExitInsteadOfLockAfterTime", false).toBool());
    setShowFullPath(settings.get("Advanced/ShowFullPath", false).toBool());
    setDisableSaveIfNotModified(settings.get("Advanced/DisableSaveIfNotModified", false).toBool());
    setUseLocalTimeFormat(settings.get("Advanced/UseLocalTimeFormat", false).toBool());
    setRegisterRestoreHotKey(settings.get("Advanced/RegisterRestoreHotKey", false).toBool());
    setDeleteTANsAfterUse(settings.get("Advanced/DeleteTANsAfterUse", false).toBool());
    setUseTransactedFileWrites(settings.get("Advanced/UseTransactedFileWrites", false).toBool());
    setStartWithWindows(settings.get("Advanced/StartWithWindows", false).toBool());
    setCopyURLsToClipboard(settings.get("Advanced/CopyURLsToClipboard", false).toBool());
    setDropToBackgroundOnCopy(settings.get("Advanced/DropToBackgroundOnCopy", false).toBool());
    setEnableRemoteControl(settings.get("Advanced/EnableRemoteControl", false).toBool());
    setAlwaysAllowRemoteControl(settings.get("Advanced/AlwaysAllowRemoteControl", false).toBool());
}

void OptionsDialog::saveSettings()
{
    PwSettings& settings = PwSettings::instance();

    // Security settings
    settings.set("Security/LockOnMinimize", m_lockOnMinimize);
    settings.set("Security/LockOnWinLock", m_lockOnWinLock);
    settings.set("Security/LockAfterTime", m_lockAfterTime);
    settings.set("Security/LockAfterSeconds", m_lockAfterSeconds);
    settings.set("Security/DisableUnsafe", m_disableUnsafe);
    settings.set("Security/SecureEdits", m_secureEdits);
    settings.set("Security/DefaultExpire", m_defaultExpire);
    settings.set("Security/DefaultExpireDays", m_defaultExpireDays);

    // Interface settings
    settings.set("Interface/ImageButtons", m_imageButtons);
    settings.set("Interface/EntryGrid", m_entryGrid);
    settings.set("Interface/ColumnAutoSize", m_columnAutoSize);
    settings.set("Interface/MinimizeToTray", m_minimizeToTray);
    settings.set("Interface/CloseMinimizes", m_closeMinimizes);
    settings.set("Interface/MainFont", m_mainFont.toString());
    settings.set("Interface/PasswordFont", m_passwordFont.toString());
    settings.set("Interface/NotesFont", m_notesFont.toString());
    settings.set("Interface/RowHighlightColor", m_rowHighlightColor);

    // Files settings
    settings.set("Files/NewlineSequence", m_newlineSequence);
    settings.set("Files/SaveOnLockAfterTimeMod", m_saveOnLockAfterTimeMod);

    // Memory settings
    settings.set("Memory/ClipboardTimeout", m_clipboardTimeoutSeconds);
    settings.set("Memory/ClearClipOnDbClose", m_clearClipboardOnDbClose);
    settings.set("Memory/ClipNoPersist", m_clipboardNoPersist);

    // Auto-Type settings
    settings.setAutoTypeEnabled(m_autoTypeEnabled);
    settings.setDefaultAutoTypeSequence(m_defaultAutoTypeSequence);
    // Save global hotkey - convert QKeySequence to quint32
    if (!m_autoTypeGlobalHotkey.isEmpty()) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        settings.setAutoTypeGlobalHotKey(static_cast<quint32>(m_autoTypeGlobalHotkey[0].toCombined()));
#else
        settings.setAutoTypeGlobalHotKey(static_cast<quint32>(m_autoTypeGlobalHotkey[0]));
#endif
    } else {
        settings.setAutoTypeGlobalHotKey(0);
    }
    settings.setAutoTypeMinimizeBeforeType(m_autoTypeMinimizeBeforeType);
    settings.setAutoTypeSameKeyboardLayout(m_autoTypeSameKeyboardLayout);
    settings.setAutoTypeSortSelectionItems(m_autoTypeSortSelection);
    settings.setAutoTypeNormalizeDashes(m_autoTypeNormalizeDashes);
    settings.setAutoTypeInternetExplorerFix(m_autoTypeIEFix);

    // Setup settings
    settings.set("Setup/UsePuttyForURLs", m_usePuttyForURLs);

    // Advanced settings
    settings.set("Advanced/RememberLastFile", m_rememberLastFile);
    settings.set("Advanced/AutoOpenLastDb", m_autoOpenLastDb);
    settings.set("Advanced/StartMinimized", m_startMinimized);
    settings.set("Advanced/AutoSave", m_autoSave);
    settings.set("Advanced/SingleInstance", m_singleInstance);
    settings.set("Advanced/CheckForUpdate", m_checkForUpdate);
    settings.set("Advanced/AutoShowExpired", m_autoShowExpired);
    settings.set("Advanced/AutoShowExpiredSoon", m_autoShowExpiredSoon);
    settings.set("Advanced/BackupEntries", m_backupEntries);
    settings.set("Advanced/DeleteBackupsOnSave", m_deleteBackupsOnSave);
    settings.set("Advanced/QuickFindInPasswords", m_quickFindInPasswords);
    settings.set("Advanced/QuickFindIncBackup", m_quickFindIncBackup);
    settings.set("Advanced/QuickFindIncExpired", m_quickFindIncExpired);
    settings.set("Advanced/FocusAfterQuickFind", m_focusAfterQuickFind);
    settings.set("Advanced/ShowTrayOnlyIfTrayed", m_showTrayOnlyIfTrayed);
    settings.set("Advanced/SingleClickTrayIcon", m_singleClickTrayIcon);
    settings.set("Advanced/RememberKeySources", m_rememberKeySources);
    settings.set("Advanced/MinimizeOnLock", m_minimizeOnLock);
    settings.set("Advanced/ExitInsteadOfLockAfterTime", m_exitInsteadOfLockAfterTime);
    settings.set("Advanced/ShowFullPath", m_showFullPath);
    settings.set("Advanced/DisableSaveIfNotModified", m_disableSaveIfNotModified);
    settings.set("Advanced/UseLocalTimeFormat", m_useLocalTimeFormat);
    settings.set("Advanced/RegisterRestoreHotKey", m_registerRestoreHotKey);
    settings.set("Advanced/DeleteTANsAfterUse", m_deleteTANsAfterUse);
    settings.set("Advanced/UseTransactedFileWrites", m_useTransactedFileWrites);
    settings.set("Advanced/StartWithWindows", m_startWithWindows);
    settings.set("Advanced/CopyURLsToClipboard", m_copyURLsToClipboard);
    settings.set("Advanced/DropToBackgroundOnCopy", m_dropToBackgroundOnCopy);
    settings.set("Advanced/EnableRemoteControl", m_enableRemoteControl);
    settings.set("Advanced/AlwaysAllowRemoteControl", m_alwaysAllowRemoteControl);

    settings.sync();
}

// Slot implementations
void OptionsDialog::onSelectMainFont()
{
    QFont newFont = selectFont(m_mainFont, tr("Select Main Font"));
    setMainFont(newFont);
}

void OptionsDialog::onSelectPasswordFont()
{
    QFont newFont = selectFont(m_passwordFont, tr("Select Password Font"));
    setPasswordFont(newFont);
}

void OptionsDialog::onSelectNotesFont()
{
    QFont newFont = selectFont(m_notesFont, tr("Select Notes Font"));
    setNotesFont(newFont);
}

void OptionsDialog::onSelectRowHighlightColor()
{
    QColor newColor = QColorDialog::getColor(m_rowHighlightColor, this,
                                              tr("Select Row Highlight Color"));
    if (newColor.isValid()) {
        setRowHighlightColor(newColor);
    }
}

void OptionsDialog::onLockAfterTimeChanged(int state)
{
    m_spinLockAfterSeconds->setEnabled(state == Qt::Checked);
}

void OptionsDialog::onDefaultExpireChanged(int state)
{
    m_spinDefaultExpireDays->setEnabled(state == Qt::Checked);
}

void OptionsDialog::onCreateFileAssociation()
{
    QMessageBox::information(this, tr("File Association"),
        tr("File association creation is platform-specific and not yet implemented."));
}

void OptionsDialog::onDeleteFileAssociation()
{
    QMessageBox::information(this, tr("File Association"),
        tr("File association deletion is platform-specific and not yet implemented."));
}

QFont OptionsDialog::selectFont(const QFont& currentFont, const QString& title)
{
    bool ok;
    QFont newFont = QFontDialog::getFont(&ok, currentFont, this, title);
    if (ok) {
        return newFont;
    }
    return currentFont;
}

// Setter implementations (Security)
void OptionsDialog::setLockOnMinimize(bool lock)
{
    m_lockOnMinimize = lock;
    m_checkLockOnMinimize->setChecked(lock);
}

void OptionsDialog::setLockOnWinLock(bool lock)
{
    m_lockOnWinLock = lock;
    m_checkLockOnWinLock->setChecked(lock);
}

void OptionsDialog::setLockAfterTime(bool lock)
{
    m_lockAfterTime = lock;
    m_checkLockAfterTime->setChecked(lock);
    m_spinLockAfterSeconds->setEnabled(lock);
}

void OptionsDialog::setLockAfterSeconds(int seconds)
{
    m_lockAfterSeconds = seconds;
    m_spinLockAfterSeconds->setValue(seconds);
}

void OptionsDialog::setDisableUnsafe(bool disable)
{
    m_disableUnsafe = disable;
    m_checkDisableUnsafe->setChecked(disable);
}

void OptionsDialog::setSecureEdits(bool secure)
{
    m_secureEdits = secure;
    m_checkSecureEdits->setChecked(secure);
}

void OptionsDialog::setDefaultExpire(bool expire)
{
    m_defaultExpire = expire;
    m_checkDefaultExpire->setChecked(expire);
    m_spinDefaultExpireDays->setEnabled(expire);
}

void OptionsDialog::setDefaultExpireDays(int days)
{
    m_defaultExpireDays = days;
    m_spinDefaultExpireDays->setValue(days);
}

// Setter implementations (Interface)
void OptionsDialog::setImageButtons(bool enable)
{
    m_imageButtons = enable;
    m_checkImageButtons->setChecked(enable);
}

void OptionsDialog::setEntryGrid(bool enable)
{
    m_entryGrid = enable;
    m_checkEntryGrid->setChecked(enable);
}

void OptionsDialog::setColumnAutoSize(bool enable)
{
    m_columnAutoSize = enable;
    m_checkColumnAutoSize->setChecked(enable);
}

void OptionsDialog::setMinimizeToTray(bool enable)
{
    m_minimizeToTray = enable;
    m_checkMinimizeToTray->setChecked(enable);
}

void OptionsDialog::setCloseMinimizes(bool enable)
{
    m_closeMinimizes = enable;
    m_checkCloseMinimizes->setChecked(enable);
}

void OptionsDialog::setMainFont(const QFont& font)
{
    m_mainFont = font;
    m_labelMainFont->setText(QString("%1, %2pt").arg(font.family()).arg(font.pointSize()));
}

void OptionsDialog::setPasswordFont(const QFont& font)
{
    m_passwordFont = font;
    m_labelPasswordFont->setText(QString("%1, %2pt").arg(font.family()).arg(font.pointSize()));
}

void OptionsDialog::setNotesFont(const QFont& font)
{
    m_notesFont = font;
    m_labelNotesFont->setText(QString("%1, %2pt").arg(font.family()).arg(font.pointSize()));
}

void OptionsDialog::setRowHighlightColor(const QColor& color)
{
    m_rowHighlightColor = color;
    // Update button background to show color
    QPalette palette = m_btnSelectRowHighlightColor->palette();
    palette.setColor(QPalette::Button, color);
    m_btnSelectRowHighlightColor->setPalette(palette);
}

// Setter implementations (Files)
void OptionsDialog::setNewlineSequence(int sequence)
{
    m_newlineSequence = sequence;
    if (sequence == 0) {
        m_radioNewlineWindows->setChecked(true);
    } else {
        m_radioNewlineUnix->setChecked(true);
    }
}

void OptionsDialog::setSaveOnLockAfterTimeMod(bool save)
{
    m_saveOnLockAfterTimeMod = save;
    m_checkSaveOnLockAfterTimeMod->setChecked(save);
}

// Setter implementations (Memory)
void OptionsDialog::setClipboardTimeoutSeconds(int seconds)
{
    m_clipboardTimeoutSeconds = seconds;
    m_spinClipboardTimeout->setValue(seconds);
}

void OptionsDialog::setClearClipboardOnDbClose(bool clear)
{
    m_clearClipboardOnDbClose = clear;
    m_checkClearClipOnDbClose->setChecked(clear);
}

void OptionsDialog::setClipboardNoPersist(bool noPersist)
{
    m_clipboardNoPersist = noPersist;
    m_checkClipNoPersist->setChecked(noPersist);
}

// Setter implementations (Setup)
void OptionsDialog::setUsePuttyForURLs(bool use)
{
    m_usePuttyForURLs = use;
    m_checkUsePuttyForURLs->setChecked(use);
}

// Setter implementations (Advanced) - continued in next part due to length
void OptionsDialog::setRememberLastFile(bool remember)
{
    m_rememberLastFile = remember;
    m_checkRememberLastFile->setChecked(remember);
}

void OptionsDialog::setAutoOpenLastDb(bool autoOpen)
{
    m_autoOpenLastDb = autoOpen;
    m_checkAutoOpenLastDb->setChecked(autoOpen);
}

void OptionsDialog::setStartMinimized(bool minimized)
{
    m_startMinimized = minimized;
    m_checkStartMinimized->setChecked(minimized);
}

void OptionsDialog::setAutoSave(bool autoSave)
{
    m_autoSave = autoSave;
    m_checkAutoSave->setChecked(autoSave);
}

void OptionsDialog::setSingleInstance(bool single)
{
    m_singleInstance = single;
    m_checkSingleInstance->setChecked(single);
}

void OptionsDialog::setCheckForUpdate(bool check)
{
    m_checkForUpdate = check;
    m_checkCheckForUpdate->setChecked(check);
}

void OptionsDialog::setAutoShowExpired(bool show)
{
    m_autoShowExpired = show;
    m_checkAutoShowExpired->setChecked(show);
}

void OptionsDialog::setAutoShowExpiredSoon(bool show)
{
    m_autoShowExpiredSoon = show;
    m_checkAutoShowExpiredSoon->setChecked(show);
}

void OptionsDialog::setBackupEntries(bool backup)
{
    m_backupEntries = backup;
    m_checkBackupEntries->setChecked(backup);
}

void OptionsDialog::setDeleteBackupsOnSave(bool deleteBackups)
{
    m_deleteBackupsOnSave = deleteBackups;
    m_checkDeleteBackupsOnSave->setChecked(deleteBackups);
}

void OptionsDialog::setQuickFindInPasswords(bool search)
{
    m_quickFindInPasswords = search;
    m_checkQuickFindInPasswords->setChecked(search);
}

void OptionsDialog::setQuickFindIncBackup(bool include)
{
    m_quickFindIncBackup = include;
    m_checkQuickFindIncBackup->setChecked(include);
}

void OptionsDialog::setQuickFindIncExpired(bool include)
{
    m_quickFindIncExpired = include;
    m_checkQuickFindIncExpired->setChecked(include);
}

void OptionsDialog::setFocusAfterQuickFind(bool focus)
{
    m_focusAfterQuickFind = focus;
    m_checkFocusAfterQuickFind->setChecked(focus);
}

void OptionsDialog::setShowTrayOnlyIfTrayed(bool show)
{
    m_showTrayOnlyIfTrayed = show;
    m_checkShowTrayOnlyIfTrayed->setChecked(show);
}

void OptionsDialog::setSingleClickTrayIcon(bool singleClick)
{
    m_singleClickTrayIcon = singleClick;
    m_checkSingleClickTrayIcon->setChecked(singleClick);
}

void OptionsDialog::setRememberKeySources(bool remember)
{
    m_rememberKeySources = remember;
    m_checkRememberKeySources->setChecked(remember);
}

void OptionsDialog::setMinimizeOnLock(bool minimize)
{
    m_minimizeOnLock = minimize;
    m_checkMinimizeOnLock->setChecked(minimize);
}

void OptionsDialog::setExitInsteadOfLockAfterTime(bool exit)
{
    m_exitInsteadOfLockAfterTime = exit;
    m_checkExitInsteadOfLockAfterTime->setChecked(exit);
}

void OptionsDialog::setShowFullPath(bool show)
{
    m_showFullPath = show;
    m_checkShowFullPath->setChecked(show);
}

void OptionsDialog::setDisableSaveIfNotModified(bool disable)
{
    m_disableSaveIfNotModified = disable;
    m_checkDisableSaveIfNotModified->setChecked(disable);
}

void OptionsDialog::setUseLocalTimeFormat(bool use)
{
    m_useLocalTimeFormat = use;
    m_checkUseLocalTimeFormat->setChecked(use);
}

void OptionsDialog::setRegisterRestoreHotKey(bool register_hotkey)
{
    m_registerRestoreHotKey = register_hotkey;
    m_checkRegisterRestoreHotKey->setChecked(register_hotkey);
}

void OptionsDialog::setDeleteTANsAfterUse(bool deleteTANs)
{
    m_deleteTANsAfterUse = deleteTANs;
    m_checkDeleteTANsAfterUse->setChecked(deleteTANs);
}

void OptionsDialog::setUseTransactedFileWrites(bool use)
{
    m_useTransactedFileWrites = use;
    m_checkUseTransactedFileWrites->setChecked(use);
}

void OptionsDialog::setStartWithWindows(bool start)
{
    m_startWithWindows = start;
    m_checkStartWithWindows->setChecked(start);
}

void OptionsDialog::setCopyURLsToClipboard(bool copy)
{
    m_copyURLsToClipboard = copy;
    m_checkCopyURLsToClipboard->setChecked(copy);
}

void OptionsDialog::setDropToBackgroundOnCopy(bool drop)
{
    m_dropToBackgroundOnCopy = drop;
    m_checkDropToBackgroundOnCopy->setChecked(drop);
}

void OptionsDialog::setEnableRemoteControl(bool enable)
{
    m_enableRemoteControl = enable;
    m_checkEnableRemoteControl->setChecked(enable);
}

void OptionsDialog::setAlwaysAllowRemoteControl(bool allow)
{
    m_alwaysAllowRemoteControl = allow;
    m_checkAlwaysAllowRemoteControl->setChecked(allow);
}

// Auto-Type getters

bool OptionsDialog::autoTypeEnabled() const
{
    return m_autoTypeEnabled;
}

QString OptionsDialog::defaultAutoTypeSequence() const
{
    return m_defaultAutoTypeSequence;
}

void OptionsDialog::setAutoTypeEnabled(bool enabled)
{
    m_autoTypeEnabled = enabled;
    m_checkAutoTypeEnabled->setChecked(enabled);
}

void OptionsDialog::setDefaultAutoTypeSequence(const QString& sequence)
{
    m_defaultAutoTypeSequence = sequence;
    m_editDefaultAutoTypeSequence->setText(sequence);
}

QKeySequence OptionsDialog::autoTypeGlobalHotkey() const
{
    return m_autoTypeGlobalHotkey;
}

void OptionsDialog::setAutoTypeGlobalHotkey(const QKeySequence& hotkey)
{
    m_autoTypeGlobalHotkey = hotkey;
    m_hotkeyEdit->setKeySequence(hotkey);
}

void OptionsDialog::setAutoTypeMinimizeBeforeType(bool minimize)
{
    m_autoTypeMinimizeBeforeType = minimize;
    m_checkAutoTypeMinimizeBeforeType->setChecked(minimize);
}

void OptionsDialog::setAutoTypeSameKeyboardLayout(bool same)
{
    m_autoTypeSameKeyboardLayout = same;
    m_checkAutoTypeSameKeyboardLayout->setChecked(same);
}

void OptionsDialog::setAutoTypeSortSelection(bool sort)
{
    m_autoTypeSortSelection = sort;
    m_checkAutoTypeSortSelection->setChecked(sort);
}

void OptionsDialog::setAutoTypeNormalizeDashes(bool normalize)
{
    m_autoTypeNormalizeDashes = normalize;
    m_checkAutoTypeNormalizeDashes->setChecked(normalize);
}

void OptionsDialog::setAutoTypeIEFix(bool fix)
{
    m_autoTypeIEFix = fix;
    m_checkAutoTypeIEFix->setChecked(fix);
}
