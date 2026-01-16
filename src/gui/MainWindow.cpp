/*
  Qt KeePass - Main Window Implementation
*/

#include "MainWindow.h"
#include "GroupModel.h"
#include "EntryModel.h"
#include "MasterKeyDialog.h"
#include "ChangeMasterKeyDialog.h"
#include "AddGroupDialog.h"
#include "AddEntryDialog.h"
#include "FindDialog.h"
#include "PasswordGeneratorDialog.h"
#include "DatabaseSettingsDialog.h"
#include "TanWizardDialog.h"
#include "OptionsDialog.h"
#include "CsvExportDialog.h"
#include "CsvImportDialog.h"
#include "ExportOptionsDialog.h"
#include "IconManager.h"
#include "LanguagesDialog.h"
#include "../core/PwManager.h"
#include "TranslationManager.h"
#include "../core/platform/PwSettings.h"
#include "../core/util/PwUtil.h"
#include "../core/util/CsvUtil.h"
#include "../core/io/PwExport.h"
#include "../core/io/PwImport.h"
#include "../autotype/AutoTypeSequence.h"
#include "../autotype/AutoTypeConfig.h"
#include "../autotype/platform/AutoTypePlatform.h"
#include "../autotype/GlobalHotkey.h"
#include "../plugins/PluginManager.h"
#include "PluginsDialog.h"

#include <QApplication>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QTreeView>
#include <QTableView>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>
#include <QHeaderView>
#include <QFile>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QInputDialog>
#include <QPageLayout>
#include <QTextDocument>
#include <QTemporaryFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QDebug>
#include <QClipboard>
#include <QCryptographicHash>
#include <QEvent>
#include <QDesktopServices>
#include <QProcess>
#include <QUrl>
#include <QThread>

#include "../core/PwStructs.h"
#include <cstring>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_pwManager(new PwManager())
    , m_groupModel(nullptr)
    , m_entryModel(nullptr)
    , m_splitter(nullptr)
    , m_groupView(nullptr)
    , m_entryView(nullptr)
    , m_toolBar(nullptr)
    , m_statusLabel(nullptr)
    , m_isModified(false)
    , m_hasDatabase(false)
    , m_isLocked(false)
    , m_clipboardTimer(new QTimer(this))
    , m_clipboardCountdown(-1)
    , m_clipboardTimeoutSecs(11)  // Default: 10+1 seconds (matching MFC)
    , m_inactivityTimer(new QTimer(this))
    , m_inactivityTimeoutMs(300000)  // Default: 5 minutes
    , m_systemTrayIcon(nullptr)
    , m_trayIconMenu(nullptr)
{
    // Connect clipboard timer
    connect(m_clipboardTimer, &QTimer::timeout, this, &MainWindow::onClipboardTimer);
    m_clipboardTimer->setInterval(1000);  // 1 second

    // Connect inactivity timer
    connect(m_inactivityTimer, &QTimer::timeout, this, &MainWindow::onInactivityTimer);
    m_inactivityTimer->setSingleShot(true);

    // Install event filter for activity tracking
    qApp->installEventFilter(this);

    setupUi();
    createSystemTrayIcon();
    setupGlobalHotkey();

    // Initialize plugin system
    PluginManager::instance().setDatabaseManager(m_pwManager);
    PluginManager::instance().loadAllPlugins();
    PluginManager::instance().broadcastEvent(KpPluginEvent::DelayedInit);

    loadSettings();
    updateWindowTitle();
    updateActions();
}

MainWindow::~MainWindow()
{
    // Shutdown plugins
    PluginManager::instance().broadcastEvent(KpPluginEvent::Cleanup);
    PluginManager::instance().unloadAllPlugins();

    saveSettings();
    delete m_pwManager;
}

void MainWindow::setupUi()
{
    // Set window properties
    setWindowTitle(tr("KeePass Password Safe"));
    resize(1024, 768);

    // Create UI components
    createActions();
    createMenus();
    createToolBar();
    createStatusBar();
    createCentralWidget();
}

void MainWindow::createActions()
{
    // Get icon manager instance
    IconManager &iconMgr = IconManager::instance();

    // File menu actions
    m_actionFileNew = new QAction(iconMgr.getToolbarIcon("tb_new"), tr("&New Database..."), this);
    m_actionFileNew->setShortcut(QKeySequence::New);
    m_actionFileNew->setStatusTip(tr("Create a new password database"));
    connect(m_actionFileNew, &QAction::triggered, this, &MainWindow::onFileNew);

    m_actionFileOpen = new QAction(iconMgr.getToolbarIcon("tb_open"), tr("&Open Database..."), this);
    m_actionFileOpen->setShortcut(QKeySequence::New);
    m_actionFileOpen->setStatusTip(tr("Open an existing password database"));
    connect(m_actionFileOpen, &QAction::triggered, this, &MainWindow::onFileOpen);

    m_actionFileSave = new QAction(iconMgr.getToolbarIcon("tb_save"), tr("&Save Database"), this);
    m_actionFileSave->setShortcut(QKeySequence::Save);
    m_actionFileSave->setStatusTip(tr("Save the current password database"));
    m_actionFileSave->setEnabled(false);
    connect(m_actionFileSave, &QAction::triggered, this, &MainWindow::onFileSave);

    m_actionFileSaveAs = new QAction(tr("Save Database &As..."), this);
    m_actionFileSaveAs->setShortcut(QKeySequence::SaveAs);
    m_actionFileSaveAs->setStatusTip(tr("Save the database with a new name"));
    m_actionFileSaveAs->setEnabled(false);
    connect(m_actionFileSaveAs, &QAction::triggered, this, &MainWindow::onFileSaveAs);

    m_actionFileClose = new QAction(tr("&Close Database"), this);
    m_actionFileClose->setShortcut(QKeySequence::Close);
    m_actionFileClose->setStatusTip(tr("Close the current database"));
    m_actionFileClose->setEnabled(false);
    connect(m_actionFileClose, &QAction::triggered, this, &MainWindow::onFileClose);

    m_actionFileLockWorkspace = new QAction(iconMgr.getToolbarIcon("tb_lock"), tr("&Lock Workspace"), this);
    m_actionFileLockWorkspace->setShortcut(QKeySequence(tr("Ctrl+L")));
    m_actionFileLockWorkspace->setStatusTip(tr("Lock the workspace to protect your passwords"));
    m_actionFileLockWorkspace->setEnabled(false);
    connect(m_actionFileLockWorkspace, &QAction::triggered, this, &MainWindow::onFileLockWorkspace);

    m_actionFileChangeMasterKey = new QAction(tr("Change &Master Key..."), this);
    m_actionFileChangeMasterKey->setStatusTip(tr("Change the master password for this database"));
    m_actionFileChangeMasterKey->setEnabled(false);
    connect(m_actionFileChangeMasterKey, &QAction::triggered, this, &MainWindow::onFileChangeMasterKey);

    m_actionFileExportHtml = new QAction(tr("Export to &HTML..."), this);
    m_actionFileExportHtml->setStatusTip(tr("Export database entries to HTML file"));
    m_actionFileExportHtml->setEnabled(false);
    connect(m_actionFileExportHtml, &QAction::triggered, this, &MainWindow::onFileExportHtml);

    m_actionFileExportXml = new QAction(tr("Export to &XML..."), this);
    m_actionFileExportXml->setStatusTip(tr("Export database entries to XML file"));
    m_actionFileExportXml->setEnabled(false);
    connect(m_actionFileExportXml, &QAction::triggered, this, &MainWindow::onFileExportXml);

    m_actionFileExportTxt = new QAction(tr("Export to &TXT..."), this);
    m_actionFileExportTxt->setStatusTip(tr("Export database entries to plain text file"));
    m_actionFileExportTxt->setEnabled(false);
    connect(m_actionFileExportTxt, &QAction::triggered, this, &MainWindow::onFileExportTxt);

    m_actionFileExportCsv = new QAction(tr("Export to &CSV..."), this);
    m_actionFileExportCsv->setStatusTip(tr("Export database entries to CSV file"));
    m_actionFileExportCsv->setEnabled(false);
    connect(m_actionFileExportCsv, &QAction::triggered, this, &MainWindow::onFileExportCsv);

    m_actionFileImportCsv = new QAction(tr("&Import from CSV..."), this);
    m_actionFileImportCsv->setStatusTip(tr("Import entries from CSV file"));
    m_actionFileImportCsv->setEnabled(false);
    connect(m_actionFileImportCsv, &QAction::triggered, this, &MainWindow::onFileImportCsv);

    m_actionFileImportCodeWallet = new QAction(tr("Import from Code&Wallet TXT..."), this);
    m_actionFileImportCodeWallet->setStatusTip(tr("Import entries from CodeWallet TXT export"));
    m_actionFileImportCodeWallet->setEnabled(false);
    connect(m_actionFileImportCodeWallet, &QAction::triggered, this, &MainWindow::onFileImportCodeWallet);

    m_actionFileImportPwSafe = new QAction(tr("Import from Password &Safe TXT..."), this);
    m_actionFileImportPwSafe->setStatusTip(tr("Import entries from Password Safe v3 TXT export"));
    m_actionFileImportPwSafe->setEnabled(false);
    connect(m_actionFileImportPwSafe, &QAction::triggered, this, &MainWindow::onFileImportPwSafe);

    m_actionFileImportKeePass = new QAction(tr("Import/&Merge KeePass KDB..."), this);
    m_actionFileImportKeePass->setStatusTip(tr("Merge entries from another KeePass 1.x database"));
    m_actionFileImportKeePass->setEnabled(false);
    connect(m_actionFileImportKeePass, &QAction::triggered, this, &MainWindow::onFileImportKeePass);

    m_actionFilePrint = new QAction(tr("&Print..."), this);
    m_actionFilePrint->setShortcut(QKeySequence::Print);
    m_actionFilePrint->setStatusTip(tr("Print the database entries"));
    m_actionFilePrint->setEnabled(false);
    connect(m_actionFilePrint, &QAction::triggered, this, &MainWindow::onFilePrint);

    m_actionFilePrintPreview = new QAction(tr("Print Pre&view..."), this);
    m_actionFilePrintPreview->setStatusTip(tr("Preview the database entries before printing"));
    m_actionFilePrintPreview->setEnabled(false);
    connect(m_actionFilePrintPreview, &QAction::triggered, this, &MainWindow::onFilePrintPreview);

    m_actionFileExit = new QAction(tr("E&xit"), this);
    m_actionFileExit->setShortcut(QKeySequence::Quit);
    m_actionFileExit->setStatusTip(tr("Exit the application"));
    connect(m_actionFileExit, &QAction::triggered, this, &MainWindow::onFileExit);

    // Edit menu actions
    m_actionEditAddGroup = new QAction(iconMgr.getGroupIcon(IconManager::ICON_GROUP), tr("Add &Group..."), this);
    m_actionEditAddGroup->setStatusTip(tr("Add a new group"));
    m_actionEditAddGroup->setEnabled(false);
    connect(m_actionEditAddGroup, &QAction::triggered, this, &MainWindow::onEditAddGroup);

    m_actionEditAddEntry = new QAction(iconMgr.getToolbarIcon("tb_adden"), tr("Add &Entry..."), this);
    m_actionEditAddEntry->setShortcut(Qt::Key_Insert);
    m_actionEditAddEntry->setStatusTip(tr("Add a new entry"));
    m_actionEditAddEntry->setEnabled(false);
    connect(m_actionEditAddEntry, &QAction::triggered, this, &MainWindow::onEditAddEntry);

    m_actionEditEditEntry = new QAction(iconMgr.getToolbarIcon("tb_edite"), tr("&Edit Entry..."), this);
    m_actionEditEditEntry->setShortcut(Qt::Key_Return);
    m_actionEditEditEntry->setStatusTip(tr("Edit the selected entry"));
    m_actionEditEditEntry->setEnabled(false);
    connect(m_actionEditEditEntry, &QAction::triggered, this, &MainWindow::onEditEditEntry);

    m_actionEditDuplicateEntry = new QAction(tr("D&uplicate Entry"), this);
    m_actionEditDuplicateEntry->setShortcut(Qt::CTRL | Qt::Key_K);
    m_actionEditDuplicateEntry->setStatusTip(tr("Duplicate the selected entry"));
    m_actionEditDuplicateEntry->setEnabled(false);
    connect(m_actionEditDuplicateEntry, &QAction::triggered, this, &MainWindow::onEditDuplicateEntry);

    m_actionEditDeleteEntry = new QAction(iconMgr.getToolbarIcon("tb_delet"), tr("&Delete Entry"), this);
    m_actionEditDeleteEntry->setShortcut(Qt::Key_Delete);
    m_actionEditDeleteEntry->setStatusTip(tr("Delete the selected entry"));
    m_actionEditDeleteEntry->setEnabled(false);
    connect(m_actionEditDeleteEntry, &QAction::triggered, this, &MainWindow::onEditDeleteEntry);

    m_actionEditDeleteGroup = new QAction(iconMgr.getToolbarIcon("tb_delet"), tr("Delete &Group"), this);
    m_actionEditDeleteGroup->setStatusTip(tr("Delete the selected group"));
    m_actionEditDeleteGroup->setEnabled(false);
    connect(m_actionEditDeleteGroup, &QAction::triggered, this, &MainWindow::onEditDeleteGroup);

    // Group management actions
    // Reference: MFC OnGroupMoveUp, OnGroupMoveDown, OnGroupMoveLeft, OnGroupMoveRight, OnGroupSort
    m_actionEditMoveGroupUp = new QAction(tr("Move Group &Up"), this);
    m_actionEditMoveGroupUp->setStatusTip(tr("Move the selected group up"));
    m_actionEditMoveGroupUp->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Up));
    m_actionEditMoveGroupUp->setEnabled(false);
    connect(m_actionEditMoveGroupUp, &QAction::triggered, this, &MainWindow::onEditMoveGroupUp);

    m_actionEditMoveGroupDown = new QAction(tr("Move Group &Down"), this);
    m_actionEditMoveGroupDown->setStatusTip(tr("Move the selected group down"));
    m_actionEditMoveGroupDown->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Down));
    m_actionEditMoveGroupDown->setEnabled(false);
    connect(m_actionEditMoveGroupDown, &QAction::triggered, this, &MainWindow::onEditMoveGroupDown);

    m_actionEditMoveGroupLeft = new QAction(tr("Move Group &Left"), this);
    m_actionEditMoveGroupLeft->setStatusTip(tr("Decrease group tree level (move left)"));
    m_actionEditMoveGroupLeft->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Left));
    m_actionEditMoveGroupLeft->setEnabled(false);
    connect(m_actionEditMoveGroupLeft, &QAction::triggered, this, &MainWindow::onEditMoveGroupLeft);

    m_actionEditMoveGroupRight = new QAction(tr("Move Group &Right"), this);
    m_actionEditMoveGroupRight->setStatusTip(tr("Increase group tree level (move right)"));
    m_actionEditMoveGroupRight->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Right));
    m_actionEditMoveGroupRight->setEnabled(false);
    connect(m_actionEditMoveGroupRight, &QAction::triggered, this, &MainWindow::onEditMoveGroupRight);

    m_actionEditSortGroups = new QAction(tr("&Sort Groups Alphabetically"), this);
    m_actionEditSortGroups->setStatusTip(tr("Sort all groups alphabetically"));
    m_actionEditSortGroups->setEnabled(false);
    connect(m_actionEditSortGroups, &QAction::triggered, this, &MainWindow::onEditSortGroups);

    // Entry management actions
    // Reference: MFC OnPwlistMoveUp, OnPwlistMoveDown
    m_actionEditMoveEntryUp = new QAction(tr("Move Entry U&p"), this);
    m_actionEditMoveEntryUp->setStatusTip(tr("Move the selected entry up within its group"));
    m_actionEditMoveEntryUp->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Up));
    m_actionEditMoveEntryUp->setEnabled(false);
    connect(m_actionEditMoveEntryUp, &QAction::triggered, this, &MainWindow::onEditMoveEntryUp);

    m_actionEditMoveEntryDown = new QAction(tr("Move Entry Do&wn"), this);
    m_actionEditMoveEntryDown->setStatusTip(tr("Move the selected entry down within its group"));
    m_actionEditMoveEntryDown->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Down));
    m_actionEditMoveEntryDown->setEnabled(false);
    connect(m_actionEditMoveEntryDown, &QAction::triggered, this, &MainWindow::onEditMoveEntryDown);

    m_actionEditFind = new QAction(iconMgr.getToolbarIcon("tb_find"), tr("&Find..."), this);
    m_actionEditFind->setShortcut(QKeySequence::Find);
    m_actionEditFind->setStatusTip(tr("Find entries"));
    m_actionEditFind->setEnabled(false);
    connect(m_actionEditFind, &QAction::triggered, this, &MainWindow::onEditFind);

    m_actionEditCopyUsername = new QAction(iconMgr.getToolbarIcon("tb_copy_username"), tr("Copy &Username"), this);
    m_actionEditCopyUsername->setShortcut(Qt::CTRL | Qt::Key_B);
    m_actionEditCopyUsername->setStatusTip(tr("Copy username to clipboard"));
    m_actionEditCopyUsername->setEnabled(false);
    connect(m_actionEditCopyUsername, &QAction::triggered, this, &MainWindow::onEditCopyUsername);

    m_actionEditCopyPassword = new QAction(iconMgr.getToolbarIcon("tb_copy_password"), tr("Copy &Password"), this);
    m_actionEditCopyPassword->setShortcut(QKeySequence::Copy);  // Ctrl+C
    m_actionEditCopyPassword->setStatusTip(tr("Copy password to clipboard"));
    m_actionEditCopyPassword->setEnabled(false);
    connect(m_actionEditCopyPassword, &QAction::triggered, this, &MainWindow::onEditCopyPassword);

    m_actionEditVisitUrl = new QAction(tr("&Visit URL"), this);
    m_actionEditVisitUrl->setShortcut(Qt::CTRL | Qt::Key_U);
    m_actionEditVisitUrl->setStatusTip(tr("Open URL in default browser"));
    m_actionEditVisitUrl->setEnabled(false);
    connect(m_actionEditVisitUrl, &QAction::triggered, this, &MainWindow::onEditVisitUrl);

    m_actionEditAutoType = new QAction(tr("Perform &Auto-Type"), this);
    m_actionEditAutoType->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_V);
    m_actionEditAutoType->setStatusTip(tr("Auto-type username and password"));
    m_actionEditAutoType->setEnabled(false);
    connect(m_actionEditAutoType, &QAction::triggered, this, &MainWindow::onEditAutoType);

    // View menu actions
    m_actionViewToolbar = new QAction(tr("&Toolbar"), this);
    m_actionViewToolbar->setCheckable(true);
    m_actionViewToolbar->setChecked(true);
    connect(m_actionViewToolbar, &QAction::triggered, this, &MainWindow::onViewToolbar);

    m_actionViewStatusBar = new QAction(tr("&Status Bar"), this);
    m_actionViewStatusBar->setCheckable(true);
    m_actionViewStatusBar->setChecked(true);
    connect(m_actionViewStatusBar, &QAction::triggered, this, &MainWindow::onViewStatusBar);

    m_actionViewExpandAll = new QAction(tr("&Expand All"), this);
    m_actionViewExpandAll->setStatusTip(tr("Expand all groups"));
    m_actionViewExpandAll->setEnabled(false);
    connect(m_actionViewExpandAll, &QAction::triggered, this, &MainWindow::onViewExpandAll);

    m_actionViewCollapseAll = new QAction(tr("&Collapse All"), this);
    m_actionViewCollapseAll->setStatusTip(tr("Collapse all groups"));
    m_actionViewCollapseAll->setEnabled(false);
    connect(m_actionViewCollapseAll, &QAction::triggered, this, &MainWindow::onViewCollapseAll);

    // Column visibility actions
    m_actionViewColumnTitle = new QAction(tr("Show &Title Column"), this);
    m_actionViewColumnTitle->setCheckable(true);
    connect(m_actionViewColumnTitle, &QAction::triggered, this, &MainWindow::onViewColumnTitle);

    m_actionViewColumnUsername = new QAction(tr("Show &User Name Column"), this);
    m_actionViewColumnUsername->setCheckable(true);
    connect(m_actionViewColumnUsername, &QAction::triggered, this, &MainWindow::onViewColumnUsername);

    m_actionViewColumnURL = new QAction(tr("Show U&RL Column"), this);
    m_actionViewColumnURL->setCheckable(true);
    connect(m_actionViewColumnURL, &QAction::triggered, this, &MainWindow::onViewColumnURL);

    m_actionViewColumnPassword = new QAction(tr("Show &Password Column"), this);
    m_actionViewColumnPassword->setCheckable(true);
    connect(m_actionViewColumnPassword, &QAction::triggered, this, &MainWindow::onViewColumnPassword);

    m_actionViewColumnNotes = new QAction(tr("Show &Notes Column"), this);
    m_actionViewColumnNotes->setCheckable(true);
    connect(m_actionViewColumnNotes, &QAction::triggered, this, &MainWindow::onViewColumnNotes);

    m_actionViewColumnCreation = new QAction(tr("Show &Creation Time Column"), this);
    m_actionViewColumnCreation->setCheckable(true);
    connect(m_actionViewColumnCreation, &QAction::triggered, this, &MainWindow::onViewColumnCreation);

    m_actionViewColumnLastMod = new QAction(tr("Show Last &Modification Column"), this);
    m_actionViewColumnLastMod->setCheckable(true);
    connect(m_actionViewColumnLastMod, &QAction::triggered, this, &MainWindow::onViewColumnLastMod);

    m_actionViewColumnLastAccess = new QAction(tr("Show Last &Access Column"), this);
    m_actionViewColumnLastAccess->setCheckable(true);
    connect(m_actionViewColumnLastAccess, &QAction::triggered, this, &MainWindow::onViewColumnLastAccess);

    m_actionViewColumnExpires = new QAction(tr("Show &Expires Column"), this);
    m_actionViewColumnExpires->setCheckable(true);
    connect(m_actionViewColumnExpires, &QAction::triggered, this, &MainWindow::onViewColumnExpires);

    m_actionViewColumnUUID = new QAction(tr("Show UUI&D Column"), this);
    m_actionViewColumnUUID->setCheckable(true);
    connect(m_actionViewColumnUUID, &QAction::triggered, this, &MainWindow::onViewColumnUUID);

    m_actionViewColumnAttachment = new QAction(tr("Show &Attachment Column"), this);
    m_actionViewColumnAttachment->setCheckable(true);
    connect(m_actionViewColumnAttachment, &QAction::triggered, this, &MainWindow::onViewColumnAttachment);

    // View options - star hiding (matching MFC ID_VIEW_HIDESTARS, ID_VIEW_HIDEUSERS)
    m_actionViewHidePasswordStars = new QAction(tr("&Hide Passwords"), this);
    m_actionViewHidePasswordStars->setCheckable(true);
    m_actionViewHidePasswordStars->setStatusTip(tr("Hide passwords with asterisks"));
    connect(m_actionViewHidePasswordStars, &QAction::triggered, this, &MainWindow::onViewHidePasswordStars);

    m_actionViewHideUsernameStars = new QAction(tr("Hide &Usernames"), this);
    m_actionViewHideUsernameStars->setCheckable(true);
    m_actionViewHideUsernameStars->setStatusTip(tr("Hide usernames with asterisks"));
    connect(m_actionViewHideUsernameStars, &QAction::triggered, this, &MainWindow::onViewHideUsernameStars);

    // Tools menu actions
    m_actionToolsOptions = new QAction(tr("&Options..."), this);
    m_actionToolsOptions->setStatusTip(tr("Configure application settings"));
    connect(m_actionToolsOptions, &QAction::triggered, this, &MainWindow::onToolsOptions);

    m_actionToolsPasswordGenerator = new QAction(tr("&Password Generator..."), this);
    m_actionToolsPasswordGenerator->setStatusTip(tr("Generate a random password"));
    m_actionToolsPasswordGenerator->setEnabled(false);
    connect(m_actionToolsPasswordGenerator, &QAction::triggered, this, &MainWindow::onToolsPasswordGenerator);

    m_actionToolsDatabaseSettings = new QAction(tr("&Database Settings..."), this);
    m_actionToolsDatabaseSettings->setStatusTip(tr("Configure database settings"));
    m_actionToolsDatabaseSettings->setEnabled(false);
    connect(m_actionToolsDatabaseSettings, &QAction::triggered, this, &MainWindow::onToolsDatabaseSettings);

    m_actionToolsTanWizard = new QAction(tr("&TAN Wizard..."), this);
    m_actionToolsTanWizard->setStatusTip(tr("Create multiple TAN entries at once"));
    m_actionToolsTanWizard->setEnabled(false);
    connect(m_actionToolsTanWizard, &QAction::triggered, this, &MainWindow::onToolsTanWizard);

    m_actionToolsRepairDatabase = new QAction(tr("&Repair Database..."), this);
    m_actionToolsRepairDatabase->setStatusTip(tr("Attempt to open a corrupted database file"));
    m_actionToolsRepairDatabase->setEnabled(true);  // Always enabled (opens file with no DB open)
    connect(m_actionToolsRepairDatabase, &QAction::triggered, this, &MainWindow::onToolsRepairDatabase);

    m_actionToolsShowExpiredEntries = new QAction(tr("Show &Expired Entries"), this);
    m_actionToolsShowExpiredEntries->setStatusTip(tr("Show all expired entries"));
    m_actionToolsShowExpiredEntries->setEnabled(false);
    connect(m_actionToolsShowExpiredEntries, &QAction::triggered, this, &MainWindow::onToolsShowExpiredEntries);

    m_actionToolsShowExpiringSoon = new QAction(tr("Show Entries Expiring &Soon"), this);
    m_actionToolsShowExpiringSoon->setStatusTip(tr("Show entries that will expire soon"));
    m_actionToolsShowExpiringSoon->setEnabled(false);
    connect(m_actionToolsShowExpiringSoon, &QAction::triggered, this, &MainWindow::onToolsShowExpiringSoon);

    m_actionToolsPlugins = new QAction(tr("&Plugins..."), this);
    m_actionToolsPlugins->setStatusTip(tr("Manage KeePass plugins"));
    connect(m_actionToolsPlugins, &QAction::triggered, this, &MainWindow::onToolsPlugins);

    // Help menu actions
    m_actionHelpContents = new QAction(tr("&Contents"), this);
    m_actionHelpContents->setShortcut(QKeySequence::HelpContents);
    m_actionHelpContents->setStatusTip(tr("Show help contents"));
    connect(m_actionHelpContents, &QAction::triggered, this, &MainWindow::onHelpContents);

    m_actionHelpLanguages = new QAction(tr("&Languages..."), this);
    m_actionHelpLanguages->setStatusTip(tr("Select application language"));
    connect(m_actionHelpLanguages, &QAction::triggered, this, &MainWindow::onHelpLanguages);

    m_actionHelpAbout = new QAction(tr("&About KeePass"), this);
    m_actionHelpAbout->setStatusTip(tr("Show information about KeePass"));
    connect(m_actionHelpAbout, &QAction::triggered, this, &MainWindow::onHelpAbout);
}

void MainWindow::createMenus()
{
    // File menu
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(m_actionFileNew);
    fileMenu->addAction(m_actionFileOpen);
    fileMenu->addSeparator();
    fileMenu->addAction(m_actionFileSave);
    fileMenu->addAction(m_actionFileSaveAs);
    fileMenu->addSeparator();

    // Import submenu
    QMenu *importMenu = fileMenu->addMenu(tr("&Import"));
    importMenu->addAction(m_actionFileImportCsv);
    importMenu->addAction(m_actionFileImportCodeWallet);
    importMenu->addAction(m_actionFileImportPwSafe);
    importMenu->addSeparator();
    importMenu->addAction(m_actionFileImportKeePass);

    // Export submenu
    QMenu *exportMenu = fileMenu->addMenu(tr("&Export"));
    exportMenu->addAction(m_actionFileExportCsv);
    exportMenu->addAction(m_actionFileExportHtml);
    exportMenu->addAction(m_actionFileExportXml);
    exportMenu->addAction(m_actionFileExportTxt);

    fileMenu->addSeparator();
    fileMenu->addAction(m_actionFilePrint);
    fileMenu->addAction(m_actionFilePrintPreview);
    fileMenu->addSeparator();
    fileMenu->addAction(m_actionFileClose);
    fileMenu->addAction(m_actionFileLockWorkspace);
    fileMenu->addAction(m_actionFileChangeMasterKey);
    fileMenu->addSeparator();
    fileMenu->addAction(m_actionFileExit);

    // Edit menu
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(m_actionEditAddGroup);
    editMenu->addAction(m_actionEditAddEntry);
    editMenu->addSeparator();
    editMenu->addAction(m_actionEditEditEntry);
    editMenu->addAction(m_actionEditDuplicateEntry);
    editMenu->addSeparator();
    editMenu->addAction(m_actionEditDeleteEntry);
    editMenu->addAction(m_actionEditDeleteGroup);
    editMenu->addSeparator();
    editMenu->addAction(m_actionEditMoveGroupUp);
    editMenu->addAction(m_actionEditMoveGroupDown);
    editMenu->addAction(m_actionEditMoveGroupLeft);
    editMenu->addAction(m_actionEditMoveGroupRight);
    editMenu->addSeparator();
    editMenu->addAction(m_actionEditSortGroups);
    editMenu->addSeparator();
    editMenu->addAction(m_actionEditMoveEntryUp);
    editMenu->addAction(m_actionEditMoveEntryDown);
    editMenu->addSeparator();
    editMenu->addAction(m_actionEditCopyUsername);
    editMenu->addAction(m_actionEditCopyPassword);
    editMenu->addAction(m_actionEditVisitUrl);
    editMenu->addAction(m_actionEditAutoType);
    editMenu->addSeparator();
    editMenu->addAction(m_actionEditFind);

    // View menu
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(m_actionViewToolbar);
    viewMenu->addAction(m_actionViewStatusBar);
    viewMenu->addSeparator();
    viewMenu->addAction(m_actionViewExpandAll);
    viewMenu->addAction(m_actionViewCollapseAll);
    viewMenu->addSeparator();

    // Column visibility submenu
    QMenu *columnsMenu = viewMenu->addMenu(tr("&Columns"));
    columnsMenu->addAction(m_actionViewColumnTitle);
    columnsMenu->addAction(m_actionViewColumnUsername);
    columnsMenu->addAction(m_actionViewColumnURL);
    columnsMenu->addAction(m_actionViewColumnPassword);
    columnsMenu->addAction(m_actionViewColumnNotes);
    columnsMenu->addSeparator();
    columnsMenu->addAction(m_actionViewColumnCreation);
    columnsMenu->addAction(m_actionViewColumnLastMod);
    columnsMenu->addAction(m_actionViewColumnLastAccess);
    columnsMenu->addAction(m_actionViewColumnExpires);
    columnsMenu->addSeparator();
    columnsMenu->addAction(m_actionViewColumnUUID);
    columnsMenu->addAction(m_actionViewColumnAttachment);

    // Star hiding options (matching MFC View menu)
    viewMenu->addSeparator();
    viewMenu->addAction(m_actionViewHidePasswordStars);
    viewMenu->addAction(m_actionViewHideUsernameStars);

    // Tools menu
    QMenu *toolsMenu = menuBar()->addMenu(tr("&Tools"));
    toolsMenu->addAction(m_actionToolsPasswordGenerator);
    toolsMenu->addAction(m_actionToolsDatabaseSettings);
    toolsMenu->addAction(m_actionToolsTanWizard);
    toolsMenu->addSeparator();
    toolsMenu->addAction(m_actionToolsRepairDatabase);
    toolsMenu->addSeparator();
    toolsMenu->addAction(m_actionToolsShowExpiredEntries);
    toolsMenu->addAction(m_actionToolsShowExpiringSoon);
    toolsMenu->addSeparator();

    // Plugin submenu - dynamic menu items from loaded plugins
    m_pluginMenu = toolsMenu->addMenu(tr("Pl&ugins"));
    PluginManager::instance().buildPluginMenu(m_pluginMenu);
    m_pluginMenu->addAction(m_actionToolsPlugins);

    toolsMenu->addSeparator();
    toolsMenu->addAction(m_actionToolsOptions);

    // Help menu
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(m_actionHelpContents);
    helpMenu->addSeparator();
    helpMenu->addAction(m_actionHelpLanguages);
    helpMenu->addSeparator();
    helpMenu->addAction(m_actionHelpAbout);
}

void MainWindow::createToolBar()
{
    m_toolBar = addToolBar(tr("Main Toolbar"));
    m_toolBar->setObjectName("MainToolBar");

    m_toolBar->addAction(m_actionFileNew);
    m_toolBar->addAction(m_actionFileOpen);
    m_toolBar->addAction(m_actionFileSave);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_actionEditAddGroup);
    m_toolBar->addAction(m_actionEditAddEntry);
    m_toolBar->addAction(m_actionEditEditEntry);
    m_toolBar->addAction(m_actionEditDeleteEntry);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_actionEditCopyUsername);
    m_toolBar->addAction(m_actionEditCopyPassword);
}

void MainWindow::createStatusBar()
{
    m_statusLabel = new QLabel(tr("Ready"));
    statusBar()->addWidget(m_statusLabel, 1);
}

void MainWindow::createCentralWidget()
{
    // Create models
    m_groupModel = new GroupModel(m_pwManager, this);
    m_entryModel = new EntryModel(m_pwManager, this);

    // Create views
    m_groupView = new QTreeView(this);
    m_groupView->setModel(m_groupModel);
    m_groupView->setHeaderHidden(true);
    m_groupView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_groupView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onGroupSelectionChanged);

    m_entryView = new QTableView(this);
    m_entryView->setModel(m_entryModel);
    m_entryView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_entryView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_entryView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_entryView->verticalHeader()->setVisible(false);
    m_entryView->horizontalHeader()->setStretchLastSection(true);
    connect(m_entryView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onEntrySelectionChanged);
    connect(m_entryView, &QTableView::doubleClicked,
            this, &MainWindow::onEntryDoubleClicked);

    // Create splitter
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->addWidget(m_groupView);
    m_splitter->addWidget(m_entryView);
    m_splitter->setStretchFactor(0, 1);  // Group view
    m_splitter->setStretchFactor(1, 3);  // Entry view (3x wider)

    setCentralWidget(m_splitter);
}

void MainWindow::loadSettings()
{
    PwSettings &settings = PwSettings::instance();

    // Restore window geometry
    if (settings.getRememberWindowSize()) {
        QByteArray geometry = settings.getMainWindowGeometry();
        if (!geometry.isEmpty()) {
            restoreGeometry(geometry);
        }

        QByteArray state = settings.getMainWindowState();
        if (!state.isEmpty()) {
            restoreState(state);
        }
    }
}

void MainWindow::saveSettings()
{
    PwSettings &settings = PwSettings::instance();

    // Save window geometry
    if (settings.getRememberWindowSize()) {
        settings.setMainWindowGeometry(saveGeometry());
        settings.setMainWindowState(saveState());
    }

    settings.sync();
}

void MainWindow::updateWindowTitle()
{
    QString title = tr("KeePass Password Safe");

    if (!m_currentFilePath.isEmpty()) {
        QFileInfo fileInfo(m_currentFilePath);
        title = fileInfo.fileName();
        if (m_isModified) {
            title += tr(" *");
        }
        if (m_isLocked) {
            title += tr(" [LOCKED]");
        }
        title += tr(" - KeePass Password Safe");
    }

    setWindowTitle(title);
}

void MainWindow::updateActions()
{
    bool hasSelection = (m_entryView != nullptr) && m_entryView->selectionModel()->hasSelection();
    bool unlocked = m_hasDatabase && !m_isLocked;

    // File menu - most actions disabled when locked
    m_actionFileSave->setEnabled(unlocked && m_isModified);
    m_actionFileSaveAs->setEnabled(unlocked);
    m_actionFileClose->setEnabled(unlocked);
    m_actionFileLockWorkspace->setEnabled(m_hasDatabase);  // Can lock/unlock anytime
    m_actionFileChangeMasterKey->setEnabled(unlocked);  // Can only change when unlocked
    m_actionFileExportHtml->setEnabled(unlocked);  // Can export when unlocked
    m_actionFileExportXml->setEnabled(unlocked);  // Can export when unlocked
    m_actionFileExportTxt->setEnabled(unlocked);  // Can export when unlocked
    m_actionFileExportCsv->setEnabled(unlocked);  // Can export when unlocked
    m_actionFileImportCsv->setEnabled(unlocked);  // Can import when unlocked
    m_actionFileImportCodeWallet->setEnabled(unlocked);  // Can import when unlocked
    m_actionFileImportPwSafe->setEnabled(unlocked);  // Can import when unlocked
    m_actionFileImportKeePass->setEnabled(unlocked);  // Can import when unlocked
    m_actionFilePrint->setEnabled(unlocked);  // Can print when unlocked
    m_actionFilePrintPreview->setEnabled(unlocked);  // Can print preview when unlocked

    // Edit menu - all disabled when locked
    m_actionEditAddGroup->setEnabled(unlocked);
    m_actionEditAddEntry->setEnabled(unlocked);
    m_actionEditEditEntry->setEnabled(unlocked && hasSelection);
    m_actionEditDuplicateEntry->setEnabled(unlocked && hasSelection);
    m_actionEditDeleteEntry->setEnabled(unlocked && hasSelection);
    m_actionEditDeleteGroup->setEnabled(unlocked);

    // Group management actions - enabled only when a group is selected
    bool hasGroupSelection = unlocked && m_groupView->currentIndex().isValid();
    m_actionEditMoveGroupUp->setEnabled(hasGroupSelection);
    m_actionEditMoveGroupDown->setEnabled(hasGroupSelection);
    m_actionEditMoveGroupLeft->setEnabled(hasGroupSelection);
    m_actionEditMoveGroupRight->setEnabled(hasGroupSelection);
    m_actionEditSortGroups->setEnabled(unlocked);  // Always enabled when unlocked

    // Entry management actions - enabled only when an entry is selected
    m_actionEditMoveEntryUp->setEnabled(unlocked && hasSelection);
    m_actionEditMoveEntryDown->setEnabled(unlocked && hasSelection);

    m_actionEditFind->setEnabled(unlocked);
    m_actionEditCopyUsername->setEnabled(unlocked && hasSelection);
    m_actionEditCopyPassword->setEnabled(unlocked && hasSelection);
    m_actionEditVisitUrl->setEnabled(unlocked && hasSelection);
    m_actionEditAutoType->setEnabled(unlocked && hasSelection);

    // View menu - some view options work when locked
    m_actionViewExpandAll->setEnabled(m_hasDatabase);
    m_actionViewCollapseAll->setEnabled(m_hasDatabase);

    // Column visibility actions - sync with model state
    if (m_entryModel != nullptr) {
        m_actionViewColumnTitle->setChecked(m_entryModel->isColumnVisible(EntryModel::ColumnTitle));
        m_actionViewColumnUsername->setChecked(m_entryModel->isColumnVisible(EntryModel::ColumnUsername));
        m_actionViewColumnURL->setChecked(m_entryModel->isColumnVisible(EntryModel::ColumnURL));
        m_actionViewColumnPassword->setChecked(m_entryModel->isColumnVisible(EntryModel::ColumnPassword));
        m_actionViewColumnNotes->setChecked(m_entryModel->isColumnVisible(EntryModel::ColumnNotes));
        m_actionViewColumnCreation->setChecked(m_entryModel->isColumnVisible(EntryModel::ColumnCreationTime));
        m_actionViewColumnLastMod->setChecked(m_entryModel->isColumnVisible(EntryModel::ColumnLastModification));
        m_actionViewColumnLastAccess->setChecked(m_entryModel->isColumnVisible(EntryModel::ColumnLastAccess));
        m_actionViewColumnExpires->setChecked(m_entryModel->isColumnVisible(EntryModel::ColumnExpires));
        m_actionViewColumnUUID->setChecked(m_entryModel->isColumnVisible(EntryModel::ColumnUUID));
        m_actionViewColumnAttachment->setChecked(m_entryModel->isColumnVisible(EntryModel::ColumnAttachment));
    }

    // Star hiding actions - sync with settings (Reference: MFC m_bPasswordStars, m_bUserStars)
    m_actionViewHidePasswordStars->setChecked(PwSettings::instance().getHidePasswordStars());
    m_actionViewHideUsernameStars->setChecked(PwSettings::instance().getHideUsernameStars());

    // Tools menu - disabled when locked
    m_actionToolsPasswordGenerator->setEnabled(unlocked);
    m_actionToolsDatabaseSettings->setEnabled(unlocked);
    m_actionToolsTanWizard->setEnabled(unlocked);
    m_actionToolsRepairDatabase->setEnabled(!m_hasDatabase && !m_isLocked);  // Only enabled when NO DB is open
    m_actionToolsShowExpiredEntries->setEnabled(unlocked);
    m_actionToolsShowExpiringSoon->setEnabled(unlocked);
}

void MainWindow::updateStatusBar()
{
    if (m_pwManager == nullptr) {
        m_statusLabel->setText(tr("Ready"));
        return;
    }

    quint32 numGroups = m_pwManager->getNumberOfGroups();
    quint32 numEntries = m_pwManager->getNumberOfEntries();

    m_statusLabel->setText(tr("%1 groups, %2 entries").arg(numGroups).arg(numEntries));
}

void MainWindow::refreshModels()
{
    // Notify models that the database has changed
    if (m_groupModel != nullptr) {
        m_groupModel->refresh();
    }

    if (m_entryModel != nullptr) {
        m_entryModel->refresh();
    }

    // Expand the root group by default
    if (m_groupView != nullptr) {
        m_groupView->expandToDepth(0);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (confirmSaveChanges()) {
        saveSettings();
        event->accept();
    } else {
        event->ignore();
    }
}

bool MainWindow::confirmSaveChanges()
{
    if (!m_isModified) {
        return true;
    }

    QMessageBox::StandardButton result = QMessageBox::question(
        this,
        tr("Save Changes?"),
        tr("The database has been modified. Do you want to save your changes?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
    );

    if (result == QMessageBox::Save) {
        return saveDatabase();
    }
    if (result == QMessageBox::Discard) {
        return true;
    }
    return false;
}

// Slot implementations (placeholders for now)

void MainWindow::onFileNew()
{
    // Check if we need to save current database
    if (!confirmSaveChanges()) {
        return;
    }

    // Show master key dialog
    MasterKeyDialog dialog(MasterKeyDialog::CreateNew, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString password = dialog.getPassword();

    // Close any existing database
    closeDatabase();

    // Create new database
    m_pwManager->newDatabase();

    // Set master key
    if (m_pwManager->setMasterKey(password, true, "", false, "") != PWE_SUCCESS) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to set master password."));
        m_pwManager->newDatabase(); // Reset
        return;
    }

    // Add default groups matching MFC KeePass behavior
    // MFC creates: General (root) + Windows, Network, Internet, eMail, Homebanking (subgroups)
    PW_GROUP groupTemplate;
    std::memset(&groupTemplate, 0, sizeof(PW_GROUP));

    // Set common timestamps
    QDateTime now = QDateTime::currentDateTime();
    PwUtil::dateTimeToPwTime(now, &groupTemplate.tCreation);
    PwUtil::dateTimeToPwTime(now, &groupTemplate.tLastMod);
    PwUtil::dateTimeToPwTime(now, &groupTemplate.tLastAccess);
    PwManager::getNeverExpireTime(&groupTemplate.tExpire);

    // Helper lambda to add a group
    auto addDefaultGroup = [&](const QString& name, quint32 imageId, quint16 level, quint32 flags = 0) {
        QByteArray nameUtf8 = name.toUtf8();
        groupTemplate.pszGroupName = new char[nameUtf8.size() + 1];
        std::strcpy(groupTemplate.pszGroupName, nameUtf8.constData());
        groupTemplate.uImageId = imageId;
        groupTemplate.usLevel = level;
        groupTemplate.uGroupId = 0; // Auto-assign ID
        groupTemplate.dwFlags = flags;

        bool success = m_pwManager->addGroup(&groupTemplate);
        delete[] groupTemplate.pszGroupName;
        return success;
    };

    // Create root group "General" (expanded by default)
    if (!addDefaultGroup(tr("General"), 48, 0, PWGF_EXPANDED)) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to create default groups."));
        m_pwManager->newDatabase();
        return;
    }

    // Create subgroups (level 1 - children of General)
    addDefaultGroup(tr("Windows"), 38, 1);
    addDefaultGroup(tr("Network"), 3, 1);
    addDefaultGroup(tr("Internet"), 1, 1);
    addDefaultGroup(tr("eMail"), 19, 1);
    addDefaultGroup(tr("Homebanking"), 37, 1);

    // Mark as modified and update UI
    m_hasDatabase = true;
    m_isModified = true;
    m_currentFilePath.clear();
    refreshModels();
    updateWindowTitle();
    updateActions();
    updateStatusBar();

    m_statusLabel->setText(tr("New database created"));
}

void MainWindow::onFileOpen()
{
    // Check if we need to save current database
    if (!confirmSaveChanges()) {
        m_statusLabel->setText(tr("Open cancelled"));
        qDebug() << "Open cancelled by confirmSaveChanges";
        return;
    }

    m_statusLabel->setText(tr("Select a database file..."));
    qApp->processEvents(); // Force status bar update

    qDebug() << "Opening file dialog at:" << QDir::homePath();

    // Show file open dialog
    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Open Database"),
        QDir::homePath(), // Start at home directory
        tr("KeePass Databases (*.kdb);;All Files (*)"),
        nullptr,
        QFileDialog::DontUseNativeDialog  // Try non-native dialog first
    );

    qDebug() << "Open dialog returned:" << filePath;

    if (filePath.isEmpty()) {
        m_statusLabel->setText(tr("No file selected"));
        qDebug() << "User cancelled open dialog";
        return; // User cancelled
    }

    // Open the database
    if (!openDatabase(filePath)) {
        // Error already shown in openDatabase()
        return;
    }

    m_statusLabel->setText(tr("Database opened successfully"));
}

void MainWindow::onFileSave()
{
    saveDatabase();
}

void MainWindow::onFileSaveAs()
{
    saveDatabaseAs();
}

void MainWindow::onFileClose()
{
    if (confirmSaveChanges()) {
        closeDatabase();
    }
}

void MainWindow::onFileLockWorkspace()
{
    if (!m_hasDatabase || m_isLocked) {
        return;
    }

    lockWorkspace();
}

void MainWindow::lockWorkspace()
{
    if (!m_hasDatabase || m_isLocked) {
        return;
    }

    // Clear clipboard if enabled in settings
    PwSettings& settings = PwSettings::instance();
    bool clearClipOnLock = settings.get("Memory/ClearClipOnDbClose", true).toBool();
    if (clearClipOnLock) {
        clearClipboardIfOwner();
    }

    // Set locked state
    m_isLocked = true;

    // Stop inactivity timer while locked
    m_inactivityTimer->stop();

    // Update UI to locked state
    updateActions();
    updateWindowTitle();

    // Clear entry model data (hide passwords)
    if (m_entryModel != nullptr) {
        m_entryModel->setIndexFilter(QList<quint32>());  // Show nothing
    }

    // Update status
    m_statusLabel->setText(tr("Workspace locked. Press Ctrl+L or use File > Lock Workspace to unlock."));

    // Try to unlock immediately
    QTimer::singleShot(100, this, [this]() {
        if (m_isLocked) {
            if (!unlockWorkspace()) {
                // User cancelled unlock - stay locked
                m_statusLabel->setText(tr("Workspace is locked"));
            }
        }
    });
}

bool MainWindow::unlockWorkspace()
{
    if (!m_hasDatabase || !m_isLocked) {
        return true;
    }

    // Show unlock dialog (reuse MasterKeyDialog in unlock mode)
    MasterKeyDialog dialog(MasterKeyDialog::OpenExisting, this);
    dialog.setWindowTitle(tr("Unlock Workspace"));

    if (dialog.exec() != QDialog::Accepted) {
        return false;  // User cancelled
    }

    // Get password for verification
    QString password = dialog.getPassword();

    // TODO: For proper security, we should verify the password against the stored master key hash
    // For now, we accept any non-empty password to unlock
    // This is a simplification - in production, we would:
    // 1. Store the master key hash when database is opened
    // 2. Verify the entered password produces the same hash
    // 3. Only unlock if hashes match
    if (password.isEmpty()) {
        return false;  // Don't unlock with empty password
    }

    // Set unlocked state
    m_isLocked = false;

    // Restore UI
    updateActions();
    updateWindowTitle();

    // Restore entry view
    if (m_entryModel != nullptr) {
        m_entryModel->clearIndexFilter();
        onGroupSelectionChanged();  // Refresh current group
    }

    // Restart inactivity timer if enabled
    PwSettings& settings = PwSettings::instance();
    bool lockAfterTime = settings.get("Security/LockAfterTime", false).toBool();
    if (lockAfterTime) {
        int timeoutSecs = settings.get("Security/LockAfterSeconds", 300).toInt();
        m_inactivityTimeoutMs = timeoutSecs * 1000;
        startInactivityTimer();
    }

    m_statusLabel->setText(tr("Workspace unlocked"));
    return true;
}

void MainWindow::onFileChangeMasterKey()
{
    // Reference: MFC WinGUI/PwSafeDlg.cpp OnFileChangeKey
    if (!m_hasDatabase || m_isLocked) {
        return;
    }

    // Show Change Master Key dialog
    ChangeMasterKeyDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        m_statusLabel->setText(tr("Master key change cancelled"));
        return;
    }

    QString newPassword = dialog.getNewPassword();

    // Set the new master key
    int result = m_pwManager->setMasterKey(newPassword, true, "", false, "");
    if (result != PWE_SUCCESS) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to set new master password.\n\nError code: %1").arg(result));
        m_statusLabel->setText(tr("Failed to change master key"));
        return;
    }

    // Save the database with the new key
    // This will re-encrypt the database with the new master password
    if (!saveDatabase()) {
        QMessageBox::warning(this, tr("Warning"),
                           tr("Master key changed, but failed to save database.\n\n"
                              "Please save the database manually to apply the new password."));
        m_statusLabel->setText(tr("Master key changed (save failed)"));
        return;
    }

    // Success
    QMessageBox::information(this, tr("Success"),
                           tr("Master password changed successfully.\n\n"
                              "The database has been saved with the new password."));
    m_statusLabel->setText(tr("Master key changed successfully"));
}

void MainWindow::onFileExportCsv()
{
    // Reference: MFC WinGUI/PwSafeDlg.cpp OnFileDbSettings
    if (!m_hasDatabase || m_isLocked) {
        return;
    }

    // Show export options dialog
    CsvExportDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        m_statusLabel->setText(tr("CSV export cancelled"));
        return;
    }

    CsvExportOptions options = dialog.getExportOptions();

    // Show file save dialog
    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Export to CSV"),
        QDir::homePath() + "/export.csv",
        tr("CSV Files (*.csv);;All Files (*)"),
        nullptr,
        QFileDialog::DontUseNativeDialog
    );

    if (filePath.isEmpty()) {
        m_statusLabel->setText(tr("CSV export cancelled"));
        return;
    }

    // Add .csv extension if not present
    if (!filePath.endsWith(".csv", Qt::CaseInsensitive)) {
        filePath += ".csv";
    }

    // Export to CSV
    QString errorMsg;
    if (!CsvUtil::exportToCSV(filePath, m_pwManager, options, &errorMsg)) {
        QMessageBox::critical(this, tr("Export Failed"),
                            tr("Failed to export to CSV:\n\n%1").arg(errorMsg));
        m_statusLabel->setText(tr("CSV export failed"));
        return;
    }

    // Success
    QMessageBox::information(this, tr("Export Successful"),
                           tr("Database exported to CSV successfully:\n\n%1").arg(filePath));
    m_statusLabel->setText(tr("CSV export successful"));
}

void MainWindow::onFileExportHtml()
{
    if (!m_hasDatabase || m_isLocked) {
        return;
    }

    // Show export options dialog
    ExportOptionsDialog dialog(PWEXP_HTML, this);
    if (dialog.exec() != QDialog::Accepted) {
        m_statusLabel->setText(tr("HTML export cancelled"));
        return;
    }

    quint32 fieldFlags = dialog.getSelectedFields();

    // Show file save dialog
    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Export to HTML"),
        QDir::homePath() + "/export.html",
        tr("HTML Files (*.html *.htm);;All Files (*)"),
        nullptr,
        QFileDialog::DontUseNativeDialog
    );

    if (filePath.isEmpty()) {
        m_statusLabel->setText(tr("HTML export cancelled"));
        return;
    }

    // Add .html extension if not present
    if (!filePath.endsWith(".html", Qt::CaseInsensitive) &&
        !filePath.endsWith(".htm", Qt::CaseInsensitive)) {
        filePath += ".html";
    }

    // Export to HTML
    if (!PwExport::exportDatabase(m_pwManager, filePath, PWEXP_HTML, fieldFlags)) {
        QMessageBox::critical(this, tr("Export Failed"),
                            tr("Failed to export to HTML:\n\n%1").arg(filePath));
        m_statusLabel->setText(tr("HTML export failed"));
        return;
    }

    // Success
    QMessageBox::information(this, tr("Export Successful"),
                           tr("Database exported to HTML successfully:\n\n%1").arg(filePath));
    m_statusLabel->setText(tr("HTML export successful"));
}

void MainWindow::onFileExportXml()
{
    if (!m_hasDatabase || m_isLocked) {
        return;
    }

    // Show export options dialog
    ExportOptionsDialog dialog(PWEXP_XML, this);
    if (dialog.exec() != QDialog::Accepted) {
        m_statusLabel->setText(tr("XML export cancelled"));
        return;
    }

    quint32 fieldFlags = dialog.getSelectedFields();

    // Show file save dialog
    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Export to XML"),
        QDir::homePath() + "/export.xml",
        tr("XML Files (*.xml);;All Files (*)"),
        nullptr,
        QFileDialog::DontUseNativeDialog
    );

    if (filePath.isEmpty()) {
        m_statusLabel->setText(tr("XML export cancelled"));
        return;
    }

    // Add .xml extension if not present
    if (!filePath.endsWith(".xml", Qt::CaseInsensitive)) {
        filePath += ".xml";
    }

    // Export to XML
    if (!PwExport::exportDatabase(m_pwManager, filePath, PWEXP_XML, fieldFlags)) {
        QMessageBox::critical(this, tr("Export Failed"),
                            tr("Failed to export to XML:\n\n%1").arg(filePath));
        m_statusLabel->setText(tr("XML export failed"));
        return;
    }

    // Success
    QMessageBox::information(this, tr("Export Successful"),
                           tr("Database exported to XML successfully:\n\n%1").arg(filePath));
    m_statusLabel->setText(tr("XML export successful"));
}

void MainWindow::onFileExportTxt()
{
    if (!m_hasDatabase || m_isLocked) {
        return;
    }

    // Show export options dialog
    ExportOptionsDialog dialog(PWEXP_TXT, this);
    if (dialog.exec() != QDialog::Accepted) {
        m_statusLabel->setText(tr("TXT export cancelled"));
        return;
    }

    quint32 fieldFlags = dialog.getSelectedFields();

    // Show file save dialog
    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Export to TXT"),
        QDir::homePath() + "/export.txt",
        tr("Text Files (*.txt);;All Files (*)"),
        nullptr,
        QFileDialog::DontUseNativeDialog
    );

    if (filePath.isEmpty()) {
        m_statusLabel->setText(tr("TXT export cancelled"));
        return;
    }

    // Add .txt extension if not present
    if (!filePath.endsWith(".txt", Qt::CaseInsensitive)) {
        filePath += ".txt";
    }

    // Export to TXT
    if (!PwExport::exportDatabase(m_pwManager, filePath, PWEXP_TXT, fieldFlags)) {
        QMessageBox::critical(this, tr("Export Failed"),
                            tr("Failed to export to TXT:\n\n%1").arg(filePath));
        m_statusLabel->setText(tr("TXT export failed"));
        return;
    }

    // Success
    QMessageBox::information(this, tr("Export Successful"),
                           tr("Database exported to TXT successfully:\n\n%1").arg(filePath));
    m_statusLabel->setText(tr("TXT export successful"));
}

void MainWindow::onFilePrint()
{
    // Reference: MFC WinGUI/PwSafeDlg.cpp OnFilePrint
    if (!m_hasDatabase || m_isLocked) {
        return;
    }

    // Show export options dialog to choose fields
    ExportOptionsDialog dialog(PWEXP_HTML, this);
    dialog.setWindowTitle(tr("Print Options"));
    if (dialog.exec() != QDialog::Accepted) {
        m_statusLabel->setText(tr("Print cancelled"));
        return;
    }

    quint32 fieldFlags = dialog.getSelectedFields();

    // Create printer
    QPrinter printer{QPrinter::HighResolution};
    printer.setPageOrientation(QPageLayout::Orientation::Portrait);

    // Show print dialog
    QPrintDialog printDialog(&printer, this);
    printDialog.setWindowTitle(tr("Print Database"));

    if (printDialog.exec() != QDialog::Accepted) {
        m_statusLabel->setText(tr("Print cancelled"));
        return;
    }

    // Generate HTML and print
    QString html = generateHtmlForPrint(fieldFlags);
    if (html.isEmpty()) {
        QMessageBox::critical(this, tr("Print Failed"),
                            tr("Failed to generate content for printing."));
        m_statusLabel->setText(tr("Print failed"));
        return;
    }

    // Print the document
    QTextDocument document;
    document.setHtml(html);
    document.print(&printer);

    m_statusLabel->setText(tr("Print successful"));
}

void MainWindow::onFilePrintPreview()
{
    // Reference: MFC WinGUI/PwSafeDlg.cpp OnFilePrintPreview
    if (!m_hasDatabase || m_isLocked) {
        return;
    }

    // Show export options dialog to choose fields
    ExportOptionsDialog dialog(PWEXP_HTML, this);
    dialog.setWindowTitle(tr("Print Preview Options"));
    if (dialog.exec() != QDialog::Accepted) {
        m_statusLabel->setText(tr("Print preview cancelled"));
        return;
    }

    quint32 fieldFlags = dialog.getSelectedFields();

    // Generate HTML
    QString html = generateHtmlForPrint(fieldFlags);
    if (html.isEmpty()) {
        QMessageBox::critical(this, tr("Print Preview Failed"),
                            tr("Failed to generate content for preview."));
        m_statusLabel->setText(tr("Print preview failed"));
        return;
    }

    // Show print preview dialog
    QPrintPreviewDialog preview(this);
    preview.setWindowTitle(tr("Print Preview - KeePass"));

    // Connect preview to print handler
    connect(&preview, &QPrintPreviewDialog::paintRequested, this,
            [html](QPrinter *printer) {
                QTextDocument document;
                document.setHtml(html);
                document.print(printer);
            });

    preview.exec();
    m_statusLabel->setText(tr("Print preview closed"));
}

void MainWindow::onFileImportCsv()
{
    // Reference: MFC WinGUI/PwSafeDlg.cpp OnFileImport
    if (!m_hasDatabase || m_isLocked) {
        return;
    }

    // Show import options dialog
    CsvImportDialog dialog(m_pwManager, this);
    if (dialog.exec() != QDialog::Accepted) {
        m_statusLabel->setText(tr("CSV import cancelled"));
        return;
    }

    CsvImportOptions options = dialog.getImportOptions();

    // Show file open dialog
    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Import from CSV"),
        QDir::homePath(),
        tr("CSV Files (*.csv);;All Files (*)"),
        nullptr,
        QFileDialog::DontUseNativeDialog
    );

    if (filePath.isEmpty()) {
        m_statusLabel->setText(tr("CSV import cancelled"));
        return;
    }

    // Import from CSV
    int entriesImported = 0;
    QString errorMsg;
    if (!CsvUtil::importFromCSV(filePath, m_pwManager, options, &entriesImported, &errorMsg)) {
        QMessageBox::critical(this, tr("Import Failed"),
                            tr("Failed to import from CSV:\n\n%1").arg(errorMsg));
        m_statusLabel->setText(tr("CSV import failed"));
        return;
    }

    // Update UI
    m_isModified = true;
    refreshModels();
    updateWindowTitle();
    updateActions();
    updateStatusBar();

    // Success
    QMessageBox::information(this, tr("Import Successful"),
                           tr("Imported %n entr(ies) from CSV successfully:\n\n%1", "", entriesImported).arg(filePath));
    m_statusLabel->setText(tr("Imported %n entr(ies) from CSV", "", entriesImported));
}

void MainWindow::onFileImportCodeWallet()
{
    // Reference: MFC WinGUI/PwSafeDlg.cpp OnFileImport (PWIMP_CWALLET)

    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Import from CodeWallet TXT"),
        QDir::homePath(),
        tr("Text Files (*.txt);;All Files (*)"),
        nullptr,
        QFileDialog::DontUseNativeDialog
    );

    if (filePath.isEmpty()) {
        m_statusLabel->setText(tr("Import cancelled"));
        return;
    }

    // Import from CodeWallet
    QString errorMsg;
    if (!PwImport::importFromFile(m_pwManager, filePath, PWIMP_CWALLET, &errorMsg)) {
        QMessageBox::critical(this, tr("Import Failed"),
                            tr("Failed to import from CodeWallet TXT:\n\n%1").arg(errorMsg));
        m_statusLabel->setText(tr("CodeWallet import failed"));
        return;
    }

    // Update UI
    m_isModified = true;
    refreshModels();
    updateWindowTitle();
    updateActions();
    updateStatusBar();

    // Success
    QMessageBox::information(this, tr("Import Successful"),
                           tr("Successfully imported entries from CodeWallet TXT file:\n\n%1").arg(filePath));
    m_statusLabel->setText(tr("Imported from CodeWallet TXT"));
}

void MainWindow::onFileImportPwSafe()
{
    // Reference: MFC WinGUI/PwSafeDlg.cpp OnFileImport (PWIMP_PWSAFE)

    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Import from Password Safe TXT"),
        QDir::homePath(),
        tr("Text Files (*.txt);;All Files (*)"),
        nullptr,
        QFileDialog::DontUseNativeDialog
    );

    if (filePath.isEmpty()) {
        m_statusLabel->setText(tr("Import cancelled"));
        return;
    }

    // Import from Password Safe
    QString errorMsg;
    if (!PwImport::importFromFile(m_pwManager, filePath, PWIMP_PWSAFE, &errorMsg)) {
        QMessageBox::critical(this, tr("Import Failed"),
                            tr("Failed to import from Password Safe TXT:\n\n%1").arg(errorMsg));
        m_statusLabel->setText(tr("Password Safe import failed"));
        return;
    }

    // Update UI
    m_isModified = true;
    refreshModels();
    updateWindowTitle();
    updateActions();
    updateStatusBar();

    // Success
    QMessageBox::information(this, tr("Import Successful"),
                           tr("Successfully imported entries from Password Safe TXT file:\n\n%1").arg(filePath));
    m_statusLabel->setText(tr("Imported from Password Safe TXT"));
}

void MainWindow::onFileImportKeePass()
{
    // Reference: MFC WinGUI/PwSafeDlg.cpp OnFileImport (PWIMP_KEEPASS)

    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Merge KeePass Database"),
        QDir::homePath(),
        tr("KeePass Database (*.kdb);;All Files (*)"),
        nullptr,
        QFileDialog::DontUseNativeDialog
    );

    if (filePath.isEmpty()) {
        m_statusLabel->setText(tr("Merge cancelled"));
        return;
    }

    // Ask for the master password of the source database
    MasterKeyDialog keyDialog(MasterKeyDialog::OpenExisting, this);
    keyDialog.setWindowTitle(tr("Enter Master Password for Source Database"));
    if (keyDialog.exec() != QDialog::Accepted) {
        m_statusLabel->setText(tr("Merge cancelled"));
        return;
    }

    QString masterPassword = keyDialog.getPassword();

    // Ask for merge mode
    QStringList modeOptions;
    modeOptions << tr("Create new UUIDs for all imported entries (safe, no conflicts)");
    modeOptions << tr("Overwrite existing entries unconditionally");
    modeOptions << tr("Overwrite only if source entry is newer");

    bool ok = false;
    QString selected = QInputDialog::getItem(
        this,
        tr("Merge Mode"),
        tr("How should entries with the same UUID be handled?"),
        modeOptions,
        0,
        false,
        &ok
    );

    if (!ok) {
        m_statusLabel->setText(tr("Merge cancelled"));
        return;
    }

    KdbMergeMode mergeMode = KdbMergeMode::CREATE_NEW_UUIDS;
    if (selected == modeOptions.at(1)) {
        mergeMode = KdbMergeMode::OVERWRITE_ALWAYS;
    } else if (selected == modeOptions.at(2)) {
        mergeMode = KdbMergeMode::OVERWRITE_IF_NEWER;
    }

    // Merge the database
    QString errorMsg;
    if (!PwImport::mergeDatabase(m_pwManager, filePath, masterPassword, mergeMode, &errorMsg)) {
        QMessageBox::critical(this, tr("Merge Failed"),
                            tr("Failed to merge KeePass database:\n\n%1").arg(errorMsg));
        m_statusLabel->setText(tr("Database merge failed"));
        return;
    }

    // Update UI
    m_isModified = true;
    refreshModels();
    updateWindowTitle();
    updateActions();
    updateStatusBar();

    // Success
    QMessageBox::information(this, tr("Merge Successful"),
                           tr("Successfully merged entries from KeePass database:\n\n%1").arg(filePath));
    m_statusLabel->setText(tr("Merged KeePass database"));
}

void MainWindow::onFileExit()
{
    close();
}

void MainWindow::onEditAddGroup()
{
    // Show Add Group dialog
    AddGroupDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        m_statusLabel->setText(tr("Add group cancelled"));
        return;
    }

    // Get dialog results
    QString groupName = dialog.getGroupName();
    quint32 iconId = dialog.getIconId();

    // Create group template
    PW_GROUP groupTemplate;
    std::memset(&groupTemplate, 0, sizeof(PW_GROUP));

    // Set timestamps
    QDateTime now = QDateTime::currentDateTime();
    PwUtil::dateTimeToPwTime(now, &groupTemplate.tCreation);
    PwUtil::dateTimeToPwTime(now, &groupTemplate.tLastMod);
    PwUtil::dateTimeToPwTime(now, &groupTemplate.tLastAccess);
    PwManager::getNeverExpireTime(&groupTemplate.tExpire);

    // Set group properties
    QByteArray nameUtf8 = groupName.toUtf8();
    groupTemplate.pszGroupName = new char[nameUtf8.size() + 1];
    std::strcpy(groupTemplate.pszGroupName, nameUtf8.constData());

    groupTemplate.uImageId = iconId;
    groupTemplate.uGroupId = 0;     // 0 = auto-assign new ID
    groupTemplate.usLevel = 0;      // Top-level group
    groupTemplate.dwFlags = 0;      // No special flags

    // Add group to database
    bool success = m_pwManager->addGroup(&groupTemplate);

    // Clean up allocated memory
    delete[] groupTemplate.pszGroupName;

    if (!success) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to add group."));
        m_statusLabel->setText(tr("Failed to add group"));
        return;
    }

    // Update UI
    m_isModified = true;
    refreshModels();
    updateWindowTitle();
    updateActions();
    m_statusLabel->setText(tr("Group '%1' added successfully").arg(groupName));

    // TODO: Select the newly added group in the tree view
}

void MainWindow::onEditAddEntry()
{
    // Get currently selected group ID (if any)
    quint32 selectedGroupId = 0;
    QModelIndex currentIndex = m_groupView->currentIndex();
    if (currentIndex.isValid()) {
        PW_GROUP *group = m_groupModel->getGroup(currentIndex);
        if (group != nullptr) {
            selectedGroupId = group->uGroupId;
        }
    }

    // Show Add Entry dialog
    AddEntryDialog dialog(m_pwManager, AddEntryDialog::AddMode, selectedGroupId, this);
    if (dialog.exec() != QDialog::Accepted) {
        m_statusLabel->setText(tr("Add entry cancelled"));
        return;
    }

    // Get dialog results
    QString title = dialog.getTitle();
    QString username = dialog.getUsername();
    QString password = dialog.getPassword();
    QString url = dialog.getUrl();
    QString notes = dialog.getNotes();
    quint32 groupId = dialog.getGroupId();
    quint32 iconId = dialog.getIconId();
    PW_TIME expireTime = dialog.getExpirationTime();

    // Create entry template
    PW_ENTRY entryTemplate;
    std::memset(&entryTemplate, 0, sizeof(PW_ENTRY));

    // Set timestamps
    QDateTime now = QDateTime::currentDateTime();
    PwUtil::dateTimeToPwTime(now, &entryTemplate.tCreation);
    PwUtil::dateTimeToPwTime(now, &entryTemplate.tLastMod);
    PwUtil::dateTimeToPwTime(now, &entryTemplate.tLastAccess);
    entryTemplate.tExpire = expireTime;

    // Set strings (must allocate memory)
    QByteArray titleUtf8 = title.toUtf8();
    entryTemplate.pszTitle = new char[titleUtf8.size() + 1];
    std::strcpy(entryTemplate.pszTitle, titleUtf8.constData());

    QByteArray usernameUtf8 = username.toUtf8();
    entryTemplate.pszUserName = new char[usernameUtf8.size() + 1];
    std::strcpy(entryTemplate.pszUserName, usernameUtf8.constData());

    QByteArray passwordUtf8 = password.toUtf8();
    entryTemplate.pszPassword = new char[passwordUtf8.size() + 1];
    std::strcpy(entryTemplate.pszPassword, passwordUtf8.constData());
    entryTemplate.uPasswordLen = passwordUtf8.size();

    QByteArray urlUtf8 = url.toUtf8();
    entryTemplate.pszURL = new char[urlUtf8.size() + 1];
    std::strcpy(entryTemplate.pszURL, urlUtf8.constData());

    QByteArray notesUtf8 = notes.toUtf8();
    entryTemplate.pszAdditional = new char[notesUtf8.size() + 1];
    std::strcpy(entryTemplate.pszAdditional, notesUtf8.constData());

    // Set empty binary data
    entryTemplate.pszBinaryDesc = new char[1];
    entryTemplate.pszBinaryDesc[0] = '\0';
    entryTemplate.pBinaryData = nullptr;
    entryTemplate.uBinaryDataLen = 0;

    // Set other properties
    entryTemplate.uGroupId = groupId;
    entryTemplate.uImageId = iconId;

    // UUID will be auto-generated by addEntry() if it's all zeros

    // Add entry to database
    bool success = m_pwManager->addEntry(&entryTemplate);

    // Clean up allocated memory
    delete[] entryTemplate.pszTitle;
    delete[] entryTemplate.pszUserName;
    delete[] entryTemplate.pszPassword;
    delete[] entryTemplate.pszURL;
    delete[] entryTemplate.pszAdditional;
    delete[] entryTemplate.pszBinaryDesc;

    if (!success) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to add entry."));
        m_statusLabel->setText(tr("Failed to add entry"));
        return;
    }

    // Process attachment if one was set
    if (dialog.isAttachmentModified()) {
        QString attachmentPath = dialog.getAttachmentPath();

        if (!attachmentPath.isEmpty()) {
            // Find the newly added entry (it's the last one)
            quint32 numEntries = m_pwManager->getNumberOfEntries();
            if (numEntries > 0) {
                PW_ENTRY* newEntry = m_pwManager->getEntry(numEntries - 1);
                if (newEntry != nullptr) {
                    QString errorMsg;
                    if (!PwUtil::attachFileAsBinaryData(newEntry, attachmentPath, &errorMsg)) {
                        QMessageBox::warning(this, tr("Warning"),
                                           tr("Entry added but failed to attach file:\n%1").arg(errorMsg));
                    }
                }
            }
        }
    }

    // Update UI
    m_isModified = true;
    refreshModels();
    updateWindowTitle();
    updateActions();
    m_statusLabel->setText(tr("Entry '%1' added successfully").arg(title));

    // TODO: Select the newly added entry in the list view
}

void MainWindow::onEditEditEntry()
{
    // Get currently selected entry
    QModelIndex currentIndex = m_entryView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::information(this, tr("Edit Entry"),
                               tr("Please select an entry to edit."));
        return;
    }

    // Get the entry from the model
    PW_ENTRY *entry = m_entryModel->getEntry(currentIndex);
    if (entry == nullptr) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to get entry data."));
        return;
    }

    // Find the entry index in PwManager
    quint32 entryIndex = 0;
    quint32 numEntries = m_pwManager->getNumberOfEntries();
    bool found = false;

    for (quint32 i = 0; i < numEntries; ++i) {
        if (m_pwManager->getEntry(i) == entry) {
            entryIndex = i;
            found = true;
            break;
        }
    }

    if (!found) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to locate entry."));
        return;
    }

    // Show Edit Entry dialog
    AddEntryDialog dialog(m_pwManager, AddEntryDialog::EditMode, entryIndex, this);
    if (dialog.exec() != QDialog::Accepted) {
        m_statusLabel->setText(tr("Edit cancelled"));
        return;
    }

    // Get updated values from dialog
    QString title = dialog.getTitle();
    QString username = dialog.getUsername();
    QString password = dialog.getPassword();
    QString url = dialog.getUrl();
    QString notes = dialog.getNotes();
    quint32 groupId = dialog.getGroupId();
    quint32 iconId = dialog.getIconId();
    PW_TIME expireTime = dialog.getExpirationTime();

    // Update entry with setEntry()
    PW_ENTRY entryTemplate;
    std::memset(&entryTemplate, 0, sizeof(PW_ENTRY));

    // Copy UUID and creation time from original entry
    std::memcpy(entryTemplate.uuid, entry->uuid, 16);
    entryTemplate.tCreation = entry->tCreation;

    // Update modification and access times
    QDateTime now = QDateTime::currentDateTime();
    PwUtil::dateTimeToPwTime(now, &entryTemplate.tLastMod);
    PwUtil::dateTimeToPwTime(now, &entryTemplate.tLastAccess);
    entryTemplate.tExpire = expireTime;

    // Set strings (must allocate memory)
    QByteArray titleUtf8 = title.toUtf8();
    entryTemplate.pszTitle = new char[titleUtf8.size() + 1];
    std::strcpy(entryTemplate.pszTitle, titleUtf8.constData());

    QByteArray usernameUtf8 = username.toUtf8();
    entryTemplate.pszUserName = new char[usernameUtf8.size() + 1];
    std::strcpy(entryTemplate.pszUserName, usernameUtf8.constData());

    QByteArray passwordUtf8 = password.toUtf8();
    entryTemplate.pszPassword = new char[passwordUtf8.size() + 1];
    std::strcpy(entryTemplate.pszPassword, passwordUtf8.constData());
    entryTemplate.uPasswordLen = passwordUtf8.size();

    QByteArray urlUtf8 = url.toUtf8();
    entryTemplate.pszURL = new char[urlUtf8.size() + 1];
    std::strcpy(entryTemplate.pszURL, urlUtf8.constData());

    QByteArray notesUtf8 = notes.toUtf8();
    entryTemplate.pszAdditional = new char[notesUtf8.size() + 1];
    std::strcpy(entryTemplate.pszAdditional, notesUtf8.constData());

    // Copy binary data from original entry
    if (entry->pszBinaryDesc != nullptr && entry->pBinaryData != nullptr && entry->uBinaryDataLen > 0) {
        size_t descLen = std::strlen(entry->pszBinaryDesc);
        entryTemplate.pszBinaryDesc = new char[descLen + 1];
        std::strcpy(entryTemplate.pszBinaryDesc, entry->pszBinaryDesc);

        entryTemplate.uBinaryDataLen = entry->uBinaryDataLen;
        entryTemplate.pBinaryData = new BYTE[entry->uBinaryDataLen];
        std::memcpy(entryTemplate.pBinaryData, entry->pBinaryData, entry->uBinaryDataLen);
    } else {
        entryTemplate.pszBinaryDesc = new char[1];
        entryTemplate.pszBinaryDesc[0] = '\0';
        entryTemplate.pBinaryData = nullptr;
        entryTemplate.uBinaryDataLen = 0;
    }

    // Set other properties
    entryTemplate.uGroupId = groupId;
    entryTemplate.uImageId = iconId;

    // Update entry in database
    bool success = m_pwManager->setEntry(entryIndex, &entryTemplate);

    // Clean up allocated memory
    delete[] entryTemplate.pszTitle;
    delete[] entryTemplate.pszUserName;
    delete[] entryTemplate.pszPassword;
    delete[] entryTemplate.pszURL;
    delete[] entryTemplate.pszAdditional;
    delete[] entryTemplate.pszBinaryDesc;
    delete[] entryTemplate.pBinaryData;

    if (!success) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to update entry."));
        m_statusLabel->setText(tr("Failed to update entry"));
        return;
    }

    // Process attachment changes
    if (dialog.isAttachmentModified()) {
        QString attachmentPath = dialog.getAttachmentPath();
        PW_ENTRY* updatedEntry = m_pwManager->getEntry(entryIndex);

        if (updatedEntry) {
            if (!attachmentPath.isEmpty()) {
                // Attach new file
                QString errorMsg;
                if (!PwUtil::attachFileAsBinaryData(updatedEntry, attachmentPath, &errorMsg)) {
                    QMessageBox::warning(this, tr("Warning"),
                                       tr("Entry updated but failed to attach file:\n%1").arg(errorMsg));
                }
            } else {
                // Remove attachment
                PwUtil::removeBinaryData(updatedEntry);
            }
        }
    }

    // Update UI
    m_isModified = true;
    refreshModels();
    updateWindowTitle();
    updateActions();
    m_statusLabel->setText(tr("Entry '%1' updated successfully").arg(title));
}

void MainWindow::onEditDuplicateEntry()
{
    // Get currently selected entry
    QModelIndex currentIndex = m_entryView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::information(this, tr("Duplicate Entry"),
                               tr("Please select an entry to duplicate."));
        return;
    }

    // Get the entry from the model
    PW_ENTRY *originalEntry = m_entryModel->getEntry(currentIndex);
    if (originalEntry == nullptr) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to get entry data."));
        return;
    }

    // Unlock the password before copying
    m_pwManager->unlockEntryPassword(originalEntry);

    // Create duplicate entry template (copy all fields)
    PW_ENTRY entryTemplate;
    std::memset(&entryTemplate, 0, sizeof(PW_ENTRY));

    // Set new timestamps
    QDateTime now = QDateTime::currentDateTime();
    PwUtil::dateTimeToPwTime(now, &entryTemplate.tCreation);
    PwUtil::dateTimeToPwTime(now, &entryTemplate.tLastMod);
    PwUtil::dateTimeToPwTime(now, &entryTemplate.tLastAccess);
    entryTemplate.tExpire = originalEntry->tExpire;  // Keep original expiration

    // Copy title and append " - Copy"
    QString originalTitle = QString::fromUtf8(originalEntry->pszTitle);
    QString newTitle = originalTitle + tr(" - Copy");
    QByteArray titleUtf8 = newTitle.toUtf8();
    entryTemplate.pszTitle = new char[titleUtf8.size() + 1];
    std::strcpy(entryTemplate.pszTitle, titleUtf8.constData());

    // Copy username
    QByteArray usernameUtf8 = QString::fromUtf8(originalEntry->pszUserName).toUtf8();
    entryTemplate.pszUserName = new char[usernameUtf8.size() + 1];
    std::strcpy(entryTemplate.pszUserName, usernameUtf8.constData());

    // Copy password
    QByteArray passwordUtf8 = QString::fromUtf8(originalEntry->pszPassword).toUtf8();
    entryTemplate.pszPassword = new char[passwordUtf8.size() + 1];
    std::strcpy(entryTemplate.pszPassword, passwordUtf8.constData());
    entryTemplate.uPasswordLen = passwordUtf8.size();

    // Copy URL
    QByteArray urlUtf8 = QString::fromUtf8(originalEntry->pszURL).toUtf8();
    entryTemplate.pszURL = new char[urlUtf8.size() + 1];
    std::strcpy(entryTemplate.pszURL, urlUtf8.constData());

    // Copy notes
    QByteArray notesUtf8 = QString::fromUtf8(originalEntry->pszAdditional).toUtf8();
    entryTemplate.pszAdditional = new char[notesUtf8.size() + 1];
    std::strcpy(entryTemplate.pszAdditional, notesUtf8.constData());

    // Copy binary data if present
    if (originalEntry->pszBinaryDesc != nullptr && originalEntry->pBinaryData != nullptr &&
        originalEntry->uBinaryDataLen > 0) {
        size_t descLen = std::strlen(originalEntry->pszBinaryDesc);
        entryTemplate.pszBinaryDesc = new char[descLen + 1];
        std::strcpy(entryTemplate.pszBinaryDesc, originalEntry->pszBinaryDesc);

        entryTemplate.uBinaryDataLen = originalEntry->uBinaryDataLen;
        entryTemplate.pBinaryData = new quint8[originalEntry->uBinaryDataLen];
        std::memcpy(entryTemplate.pBinaryData, originalEntry->pBinaryData,
                   originalEntry->uBinaryDataLen);
    } else {
        entryTemplate.pszBinaryDesc = new char[1];
        entryTemplate.pszBinaryDesc[0] = '\0';
        entryTemplate.pBinaryData = nullptr;
        entryTemplate.uBinaryDataLen = 0;
    }

    // Set other properties (copy from original)
    entryTemplate.uGroupId = originalEntry->uGroupId;  // Same group
    entryTemplate.uImageId = originalEntry->uImageId;  // Same icon

    // UUID will be auto-generated by addEntry() (memset to 0 above)

    // Lock original password again
    m_pwManager->lockEntryPassword(originalEntry);

    // Add the duplicate entry to database
    bool success = m_pwManager->addEntry(&entryTemplate);

    // Clean up allocated memory
    delete[] entryTemplate.pszTitle;
    delete[] entryTemplate.pszUserName;
    delete[] entryTemplate.pszPassword;
    delete[] entryTemplate.pszURL;
    delete[] entryTemplate.pszAdditional;
    delete[] entryTemplate.pszBinaryDesc;
    delete[] entryTemplate.pBinaryData;

    if (!success) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to duplicate entry."));
        m_statusLabel->setText(tr("Failed to duplicate entry"));
        return;
    }

    // Update UI
    m_isModified = true;
    refreshModels();
    updateWindowTitle();
    updateActions();
    m_statusLabel->setText(tr("Entry '%1' duplicated successfully").arg(newTitle));

    // TODO: Select the newly added entry in the list view
}

void MainWindow::onEditDeleteEntry()
{
    // Check if database is open
    if ((m_pwManager == nullptr) || !m_hasDatabase) {
        return;
    }

    // Get selected entry
    QModelIndex currentIndex = m_entryView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::information(this, tr("Delete Entry"),
                               tr("Please select an entry to delete."));
        return;
    }

    PW_ENTRY *entry = m_entryModel->getEntry(currentIndex);
    if (entry == nullptr) {
        return;
    }

    // Get entry title for confirmation dialog
    QString title = QString::fromUtf8(entry->pszTitle);
    if (title.isEmpty()) {
        title = tr("(Untitled)");
    }

    // Confirm deletion
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Delete Entry"),
        tr("Are you sure you want to delete the entry \"%1\"?\n\nThis operation cannot be undone.").arg(title),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply != QMessageBox::Yes) {
        m_statusLabel->setText(tr("Delete cancelled"));
        return;
    }

    // Find entry index in PwManager
    quint32 entryIndex = 0;
    bool found = false;
    quint32 numEntries = m_pwManager->getNumberOfEntries();

    for (quint32 i = 0; i < numEntries; ++i) {
        if (m_pwManager->getEntry(i) == entry) {
            entryIndex = i;
            found = true;
            break;
        }
    }

    if (!found) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to find entry in database."));
        m_statusLabel->setText(tr("Failed to delete entry"));
        return;
    }

    // Delete entry from database
    bool success = m_pwManager->deleteEntry(entryIndex);

    if (!success) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to delete entry."));
        m_statusLabel->setText(tr("Failed to delete entry"));
        return;
    }

    // Update UI
    m_isModified = true;
    refreshModels();
    updateWindowTitle();
    updateActions();
    m_statusLabel->setText(tr("Entry '%1' deleted successfully").arg(title));
}

void MainWindow::onEditDeleteGroup()
{
    // Check if database is open
    if ((m_pwManager == nullptr) || !m_hasDatabase) {
        return;
    }

    // Get selected group
    QModelIndex currentIndex = m_groupView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::information(this, tr("Delete Group"),
                               tr("Please select a group to delete."));
        return;
    }

    PW_GROUP *group = m_groupModel->getGroup(currentIndex);
    if (!group) {
        return;
    }

    // Get group name for confirmation dialog
    QString groupName = QString::fromUtf8(group->pszGroupName);
    if (groupName.isEmpty()) {
        groupName = tr("(Untitled)");
    }

    // Prevent deletion of top-level group if only one exists
    quint32 numGroups = m_pwManager->getNumberOfGroups();
    if (numGroups <= 1) {
        QMessageBox::warning(this, tr("Delete Group"),
                           tr("Cannot delete the only remaining group."));
        return;
    }

    // Check how many entries are in this group
    quint32 numEntries = m_pwManager->getNumberOfItemsInGroupN(group->uGroupId);
    bool createBackup = false;

    // If group has entries, ask about backup
    if (numEntries > 0) {
        QString message;
        if (numEntries == 1) {
            message = tr("The group \"%1\" contains 1 entry.\n\n"
                       "Do you want to create a backup of the entry before deleting the group?\n\n"
                       "Yes = Move entry to backup group\n"
                       "No = Delete entry permanently\n"
                       "Cancel = Don't delete the group").arg(groupName);
        } else {
            message = tr("The group \"%1\" contains %2 entries.\n\n"
                       "Do you want to create a backup of the entries before deleting the group?\n\n"
                       "Yes = Move entries to backup group\n"
                       "No = Delete entries permanently\n"
                       "Cancel = Don't delete the group").arg(groupName).arg(numEntries);
        }

        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            tr("Delete Group"),
            message,
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
            QMessageBox::Cancel
        );

        if (reply == QMessageBox::Cancel) {
            m_statusLabel->setText(tr("Delete cancelled"));
            return;
        }

        createBackup = (reply == QMessageBox::Yes);
    } else {
        // Empty group - simple confirmation
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            tr("Delete Group"),
            tr("Are you sure you want to delete the group \"%1\"?\n\nThis operation cannot be undone.").arg(groupName),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );

        if (reply != QMessageBox::Yes) {
            m_statusLabel->setText(tr("Delete cancelled"));
            return;
        }
    }

    // Delete group from database
    bool success = m_pwManager->deleteGroupById(group->uGroupId, createBackup);

    if (!success) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to delete group."));
        m_statusLabel->setText(tr("Failed to delete group"));
        return;
    }

    // Update UI
    m_isModified = true;
    refreshModels();
    updateWindowTitle();
    updateActions();

    if (numEntries > 0 && createBackup) {
        m_statusLabel->setText(tr("Group '%1' deleted (%2 entries backed up)").arg(groupName).arg(numEntries));
    } else if (numEntries > 0) {
        m_statusLabel->setText(tr("Group '%1' and %2 entries deleted").arg(groupName).arg(numEntries));
    } else {
        m_statusLabel->setText(tr("Group '%1' deleted successfully").arg(groupName));
    }
}

void MainWindow::onEditMoveGroupUp()
{
    // Reference: MFC/MFC-KeePass/WinGUI/PwSafeDlg.cpp OnGroupMoveUp (line ~7115)
    if ((m_pwManager == nullptr) || !m_hasDatabase) {
        return;
    }

    // Get selected group
    QModelIndex currentIndex = m_groupView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::information(this, tr("Move Group"),
                               tr("Please select a group to move."));
        return;
    }

    PW_GROUP *group = m_groupModel->getGroup(currentIndex);
    if (!group) {
        return;
    }

    // Move group up using MFC method: moveGroupExDir with direction -1
    bool success = m_pwManager->moveGroupExDir(group->uGroupId, -1);

    if (!success) {
        QMessageBox::information(this, tr("Move Group"),
                               tr("Cannot move this group up (already at top of its level)."));
        return;
    }

    // Update UI
    m_isModified = true;
    m_groupModel->refresh();
    updateWindowTitle();
    updateActions();

    // Re-select the moved group
    QModelIndex newIndex = m_groupModel->indexForGroup(group->uGroupId);
    if (newIndex.isValid()) {
        m_groupView->setCurrentIndex(newIndex);
        m_groupView->scrollTo(newIndex);
    }

    m_statusLabel->setText(tr("Group moved up"));
}

void MainWindow::onEditMoveGroupDown()
{
    // Reference: MFC/MFC-KeePass/WinGUI/PwSafeDlg.cpp OnGroupMoveDown
    if ((m_pwManager == nullptr) || !m_hasDatabase) {
        return;
    }

    // Get selected group
    QModelIndex currentIndex = m_groupView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::information(this, tr("Move Group"),
                               tr("Please select a group to move."));
        return;
    }

    PW_GROUP *group = m_groupModel->getGroup(currentIndex);
    if (!group) {
        return;
    }

    // Move group down using MFC method: moveGroupExDir with direction +1
    bool success = m_pwManager->moveGroupExDir(group->uGroupId, +1);

    if (!success) {
        QMessageBox::information(this, tr("Move Group"),
                               tr("Cannot move this group down (already at bottom of its level)."));
        return;
    }

    // Update UI
    m_isModified = true;
    m_groupModel->refresh();
    updateWindowTitle();
    updateActions();

    // Re-select the moved group
    QModelIndex newIndex = m_groupModel->indexForGroup(group->uGroupId);
    if (newIndex.isValid()) {
        m_groupView->setCurrentIndex(newIndex);
        m_groupView->scrollTo(newIndex);
    }

    m_statusLabel->setText(tr("Group moved down"));
}

void MainWindow::onEditMoveGroupLeft()
{
    // Reference: MFC/MFC-KeePass/WinGUI/PwSafeDlg.cpp OnGroupMoveLeft
    // Decrease tree level (move to parent's level)
    if ((m_pwManager == nullptr) || !m_hasDatabase) {
        return;
    }

    // Get selected group
    QModelIndex currentIndex = m_groupView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::information(this, tr("Move Group"),
                               tr("Please select a group to move."));
        return;
    }

    PW_GROUP *group = m_groupModel->getGroup(currentIndex);
    if (!group) {
        return;
    }

    // Cannot move left if already at top level (level 0)
    if (group->usLevel == 0) {
        QMessageBox::information(this, tr("Move Group"),
                               tr("Cannot move group left (already at top level)."));
        return;
    }

    // Decrease tree level
    group->usLevel--;

    // Update UI
    m_isModified = true;
    m_groupModel->refresh();
    updateWindowTitle();
    updateActions();

    // Re-select the moved group
    QModelIndex newIndex = m_groupModel->indexForGroup(group->uGroupId);
    if (newIndex.isValid()) {
        m_groupView->setCurrentIndex(newIndex);
        m_groupView->scrollTo(newIndex);
        m_groupView->expand(newIndex);
    }

    m_statusLabel->setText(tr("Group tree level decreased"));
}

void MainWindow::onEditMoveGroupRight()
{
    // Reference: MFC/MFC-KeePass/WinGUI/PwSafeDlg.cpp OnGroupMoveRight
    // Increase tree level (make it a child of previous sibling)
    if ((m_pwManager == nullptr) || !m_hasDatabase) {
        return;
    }

    // Get selected group
    QModelIndex currentIndex = m_groupView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::information(this, tr("Move Group"),
                               tr("Please select a group to move."));
        return;
    }

    PW_GROUP *group = m_groupModel->getGroup(currentIndex);
    if (!group) {
        return;
    }

    // Find the group index in the flat list
    quint32 groupIndex = m_pwManager->getGroupByIdN(group->uGroupId);

    // Cannot move right if it's the first group at this level
    if (groupIndex == 0) {
        QMessageBox::information(this, tr("Move Group"),
                               tr("Cannot move group right (no previous sibling)."));
        return;
    }

    // Get previous group
    PW_GROUP *prevGroup = m_pwManager->getGroup(groupIndex - 1);
    if (!prevGroup || prevGroup->usLevel != group->usLevel) {
        QMessageBox::information(this, tr("Move Group"),
                               tr("Cannot move group right (no previous sibling at same level)."));
        return;
    }

    // Increase tree level
    group->usLevel++;

    // Update UI
    m_isModified = true;
    m_groupModel->refresh();
    updateWindowTitle();
    updateActions();

    // Re-select the moved group and expand parent
    QModelIndex newIndex = m_groupModel->indexForGroup(group->uGroupId);
    if (newIndex.isValid()) {
        QModelIndex parentIndex = newIndex.parent();
        if (parentIndex.isValid()) {
            m_groupView->expand(parentIndex);
        }
        m_groupView->setCurrentIndex(newIndex);
        m_groupView->scrollTo(newIndex);
    }

    m_statusLabel->setText(tr("Group tree level increased"));
}

void MainWindow::onEditSortGroups()
{
    // Reference: MFC/MFC-KeePass/WinGUI/PwSafeDlg.cpp OnGroupSort
    if ((m_pwManager == nullptr) || !m_hasDatabase) {
        return;
    }

    // Confirm sort operation
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Sort Groups"),
        tr("This will sort all groups alphabetically.\n\n"
           "Do you want to continue?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes
    );

    if (reply != QMessageBox::Yes) {
        m_statusLabel->setText(tr("Sort cancelled"));
        return;
    }

    // Get currently selected group ID to restore selection after sort
    quint32 selectedGroupId = 0;
    QModelIndex currentIndex = m_groupView->currentIndex();
    if (currentIndex.isValid()) {
        PW_GROUP *group = m_groupModel->getGroup(currentIndex);
        if (group) {
            selectedGroupId = group->uGroupId;
        }
    }

    // Sort all groups alphabetically
    m_pwManager->sortGroupList();

    // Update UI
    m_isModified = true;
    m_groupModel->refresh();
    updateWindowTitle();
    updateActions();

    // Restore selection if possible
    if (selectedGroupId != 0) {
        QModelIndex newIndex = m_groupModel->indexForGroup(selectedGroupId);
        if (newIndex.isValid()) {
            m_groupView->setCurrentIndex(newIndex);
            m_groupView->scrollTo(newIndex);
        }
    }

    m_statusLabel->setText(tr("Groups sorted alphabetically"));
}

void MainWindow::onEditMoveEntryUp()
{
    // Reference: MFC/MFC-KeePass/WinGUI/PwSafeDlg.cpp OnPwlistMoveUp (line ~6540)
    if ((m_pwManager == nullptr) || !m_hasDatabase) {
        return;
    }

    // Get selected entry
    QModelIndex currentIndex = m_entryView->currentIndex();
    if (!currentIndex.isValid()) {
        return;
    }

    int row = currentIndex.row();
    if (row <= 0) {
        // Already at top
        return;
    }

    // Get current entry to determine group ID
    const PW_ENTRY* entry = m_entryModel->getEntry(currentIndex);
    if (entry == nullptr) {
        return;
    }

    // Move entry up within its group
    m_pwManager->moveEntry(entry->uGroupId, row, row - 1);

    // Update UI
    m_isModified = true;
    m_entryModel->refresh();
    updateWindowTitle();

    // Re-select the moved entry
    QModelIndex newIndex = m_entryModel->index(row - 1, 0);
    if (newIndex.isValid()) {
        m_entryView->setCurrentIndex(newIndex);
        m_entryView->scrollTo(newIndex);
    }

    m_statusLabel->setText(tr("Entry moved up"));
}

void MainWindow::onEditMoveEntryDown()
{
    // Reference: MFC/MFC-KeePass/WinGUI/PwSafeDlg.cpp OnPwlistMoveDown (line ~6595)
    if ((m_pwManager == nullptr) || !m_hasDatabase) {
        return;
    }

    // Get selected entry
    QModelIndex currentIndex = m_entryView->currentIndex();
    if (!currentIndex.isValid()) {
        return;
    }

    int row = currentIndex.row();
    int rowCount = m_entryModel->rowCount();
    if (row >= rowCount - 1) {
        // Already at bottom
        return;
    }

    // Get current entry to determine group ID
    const PW_ENTRY* entry = m_entryModel->getEntry(currentIndex);
    if (entry == nullptr) {
        return;
    }

    // Move entry down within its group
    m_pwManager->moveEntry(entry->uGroupId, row, row + 1);

    // Update UI
    m_isModified = true;
    m_entryModel->refresh();
    updateWindowTitle();

    // Re-select the moved entry
    QModelIndex newIndex = m_entryModel->index(row + 1, 0);
    if (newIndex.isValid()) {
        m_entryView->setCurrentIndex(newIndex);
        m_entryView->scrollTo(newIndex);
    }

    m_statusLabel->setText(tr("Entry moved down"));
}

void MainWindow::onEditFind()
{
    // Reference: MFC/MFC-KeePass/WinGUI/PwSafeDlg.cpp _Find method
    if ((m_pwManager == nullptr) || !m_hasDatabase) {
        return;
    }

    // Show Find dialog
    FindDialog findDialog(m_pwManager, this);
    if (findDialog.exec() != QDialog::Accepted) {
        return;
    }

    // Get search parameters
    QString searchString = findDialog.searchString();
    quint32 searchFlags = findDialog.searchFlags();
    bool caseSensitive = findDialog.isCaseSensitive();
    bool excludeBackups = findDialog.excludeBackups();
    bool excludeExpired = findDialog.excludeExpired();

    // Perform search for ALL matches (not just first)
    QString error;
    QList<quint32> results = m_pwManager->findAll(searchString, caseSensitive, searchFlags,
                                                   excludeBackups, excludeExpired, &error);

    if (results.isEmpty()) {
        // Not found
        if (!error.isEmpty()) {
            QMessageBox::warning(this, tr("Find"), tr("Search error: %1").arg(error));
        } else {
            QMessageBox::information(this, tr("Find"),
                tr("No entries found matching '%1'").arg(searchString));
        }
        m_statusLabel->setText(tr("Search completed - no matches found"));
        return;
    }

    // Create "Search Results" group if it doesn't exist
    // Reference: MFC uses PWS_SEARCHGROUP = "Search Results" with icon 40
    quint32 searchGroupId = m_pwManager->getGroupId(PWS_SEARCHGROUP);
    if (searchGroupId == 0xFFFFFFFF) {
        PW_GROUP searchGroup;
        std::memset(&searchGroup, 0, sizeof(PW_GROUP));
        searchGroup.pszGroupName = const_cast<char*>(PWS_SEARCHGROUP);
        searchGroup.uGroupId = 0;  // 0 = auto-generate ID
        searchGroup.uImageId = 40;  // MFC uses icon 40 for search results
        searchGroup.usLevel = 0;
        searchGroup.dwFlags = 0;

        PwUtil::getCurrentTime(&searchGroup.tCreation);
        searchGroup.tLastAccess = searchGroup.tCreation;
        searchGroup.tLastMod = searchGroup.tCreation;
        PwManager::getNeverExpireTime(&searchGroup.tExpire);

        if (m_pwManager->addGroup(&searchGroup)) {
            searchGroupId = m_pwManager->getGroupId(PWS_SEARCHGROUP);
            // Refresh group model to show the new group
            refreshModels();
        }
    }

    // Display all matching entries using index filter
    m_entryModel->clearGroupFilter();
    m_entryModel->setIndexFilter(results);

    // Select the first result
    if (!results.isEmpty()) {
        m_entryView->selectRow(0);
        m_entryView->scrollTo(m_entryModel->index(0, 0));
        m_entryView->setFocus();
    }

    // Update status bar
    m_statusLabel->setText(tr("Found %n matching entr(ies)", "", results.count()));

    // Show result message
    QMessageBox::information(this, tr("Find"),
        tr("Found %n matching entr(ies) for '%1'", "", results.count()).arg(searchString));
}

void MainWindow::onViewToolbar()
{
    m_toolBar->setVisible(m_actionViewToolbar->isChecked());
}

void MainWindow::onViewStatusBar()
{
    statusBar()->setVisible(m_actionViewStatusBar->isChecked());
}

void MainWindow::onViewExpandAll()
{
    m_groupView->expandAll();
}

void MainWindow::onViewCollapseAll()
{
    m_groupView->collapseAll();
}

void MainWindow::onToolsOptions()
{
    OptionsDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        // Note: Settings are automatically saved by OptionsDialog when OK is clicked
        // Some settings might require application restart to take effect

        // Re-register global hotkey if settings changed
        GlobalHotkey& hotkey = GlobalHotkey::instance();
        hotkey.unregisterHotkey();

        PwSettings& settings = PwSettings::instance();
        if (settings.getAutoTypeEnabled()) {
            QKeySequence keySeq = dialog.autoTypeGlobalHotkey();
            if (!keySeq.isEmpty()) {
                if (!hotkey.registerHotkey(keySeq)) {
                    qWarning() << "Failed to register global hotkey:" << hotkey.lastError();
                }
            }
        }

        m_statusLabel->setText(tr("Settings saved successfully"));
    }
}

void MainWindow::onToolsPasswordGenerator()
{
    PasswordGeneratorDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString password = dialog.generatedPassword();
        // Password generated successfully
        // User can copy it from the dialog before closing
        m_statusLabel->setText(tr("Password generated successfully"));
    }
}

void MainWindow::onToolsDatabaseSettings()
{
    // Reference: MFC shows DbSettingsDlg
    if ((m_pwManager == nullptr) || !m_hasDatabase) {
        return;
    }

    DatabaseSettingsDialog dialog(m_pwManager, this);

    // Load current settings
    dialog.setEncryptionAlgorithm(m_pwManager->getAlgorithm());
    dialog.setKeyTransformRounds(m_pwManager->getKeyEncRounds());
    dialog.setDefaultUsername(m_pwManager->getDefaultUserName());

    // Convert QColor to DWORD (0x00RRGGBB), or DWORD_MAX if invalid
    QColor color = m_pwManager->getColor();
    quint32 colorValue;
    if (color.isValid()) {
        colorValue = (color.red() << 16) | (color.green() << 8) | color.blue();
    }
    else {
        colorValue = 0xFFFFFFFF;  // No custom color
    }
    dialog.setDatabaseColor(colorValue);

    // Show dialog
    if (dialog.exec() == QDialog::Accepted) {
        // Apply new settings
        m_pwManager->setAlgorithm(dialog.encryptionAlgorithm());
        m_pwManager->setKeyEncRounds(dialog.keyTransformRounds());
        m_pwManager->setDefaultUserName(dialog.defaultUsername());

        // Convert DWORD to QColor
        quint32 newColor = dialog.databaseColor();
        if (newColor == 0xFFFFFFFF) {
            m_pwManager->setColor(QColor());  // Invalid color = no custom color
        }
        else {
            QColor qcolor(
                (newColor >> 16) & 0xFF,  // R
                (newColor >> 8) & 0xFF,   // G
                newColor & 0xFF           // B
            );
            m_pwManager->setColor(qcolor);
        }

        // Mark database as modified
        m_isModified = true;
        updateWindowTitle();
        updateActions();

        m_statusLabel->setText(tr("Database settings updated"));
    }
}

void MainWindow::onToolsTanWizard()
{
    // Reference: MFC/MFC-KeePass/WinGUI/PwSafeDlg.cpp OnExtrasTanWizard
    if ((m_pwManager == nullptr) || !m_hasDatabase || m_isLocked) {
        return;
    }

    // Get currently selected group
    QModelIndex groupIndex = m_groupView->currentIndex();
    if (!groupIndex.isValid()) {
        QMessageBox::warning(this, tr("TAN Wizard"),
            tr("Please select a group where the TAN entries should be added."));
        return;
    }

    PW_GROUP* group = m_groupModel->getGroup(groupIndex);
    if (group == nullptr) {
        return;
    }

    quint32 groupId = group->uGroupId;

    QString groupName = QString::fromUtf8(group->pszGroupName);

    // Show TAN Wizard dialog
    TanWizardDialog dialog(groupName, this);
    if (dialog.exec() == QDialog::Accepted) {
        // Get parsed TANs from dialog
        QStringList tanList = dialog.getTanList();
        bool useNumbering = dialog.useSequentialNumbering();
        int startNumber = dialog.getStartNumber();

        if (tanList.isEmpty()) {
            QMessageBox::information(this, tr("TAN Wizard"),
                tr("No valid TANs were found in the input."));
            return;
        }

        // Create entries for each TAN
        PW_ENTRY entryTemplate;
        std::memset(&entryTemplate, 0, sizeof(PW_ENTRY));

        // Get current time for all entries
        PW_TIME currentTime;
        PwUtil::getCurrentTime(&currentTime);

        entryTemplate.tCreation = currentTime;
        entryTemplate.tLastMod = currentTime;
        entryTemplate.tLastAccess = currentTime;
        entryTemplate.uGroupId = groupId;
        entryTemplate.uImageId = 29;  // TAN icon (MFC uses 29)

        // Set never-expire time
        PwManager::getNeverExpireTime(&entryTemplate.tExpire);

        // Add each TAN as a separate entry
        int tanNumber = startNumber;
        for (const QString& tan : tanList) {
            // Title is always "<TAN>"
            QByteArray titleBytes = QString("<TAN>").toUtf8();
            entryTemplate.pszTitle = const_cast<char*>(titleBytes.constData());

            // Password is the TAN code
            QByteArray passwordBytes = tan.toUtf8();
            entryTemplate.pszPassword = const_cast<char*>(passwordBytes.constData());
            entryTemplate.uPasswordLen = passwordBytes.length();

            // Username is the sequential number (if enabled)
            QByteArray usernameBytes;
            if (useNumbering) {
                usernameBytes = QString::number(tanNumber).toUtf8();
                entryTemplate.pszUserName = const_cast<char*>(usernameBytes.constData());
                tanNumber++;
            } else {
                entryTemplate.pszUserName = const_cast<char*>("");
            }

            // URL and notes are empty
            entryTemplate.pszURL = const_cast<char*>("");
            entryTemplate.pszAdditional = const_cast<char*>("");

            // No binary attachment
            entryTemplate.pBinaryData = nullptr;
            entryTemplate.pszBinaryDesc = const_cast<char*>("");
            entryTemplate.uBinaryDataLen = 0;

            // UUID will be generated by AddEntry
            std::memset(&entryTemplate.uuid[0], 0, 16);

            // Add the entry
            bool success = m_pwManager->addEntry(&entryTemplate);
            if (!success) {
                QMessageBox::warning(this, tr("TAN Wizard"),
                    tr("Failed to add TAN entry: %1").arg(tan));
                break;
            }
        }

        // Update UI
        m_isModified = true;
        refreshModels();
        updateWindowTitle();
        updateActions();

        m_statusLabel->setText(tr("Added %1 TAN entries to group '%2'")
            .arg(tanList.count()).arg(groupName));
    }
}

void MainWindow::onToolsRepairDatabase()
{
    // Reference: MFC/MFC-KeePass/WinGUI/PwSafeDlg.cpp OnExtrasRepairDb

    // Cannot repair if a database is already open
    if (m_hasDatabase) {
        QMessageBox::warning(this, tr("Repair Database"),
            tr("Please close the current database before attempting to repair another database."));
        return;
    }

    // Show warning dialog explaining the risks
    QString warningTitle = tr("Warning!");
    QString warningMsg = tr(
        "In repair mode, the integrity of the data is not checked (in order to rescue "
        "as much data as possible). When no integrity checks are performed, corrupted/"
        "malicious data might be incorporated into the database.\n\n"
        "Thus the repair functionality should only be used when there really is no other "
        "solution. If you use it, afterwards you should thoroughly check your whole database "
        "for corrupted/malicious data.\n\n"
        "Are you sure you want to attempt to repair a database file?");

    QMessageBox::StandardButton reply = QMessageBox::warning(this,
        warningTitle, warningMsg,
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return;
    }

    // Select database file to repair
    QString filePath = QFileDialog::getOpenFileName(this,
        tr("Select Database to Repair"),
        QDir::homePath(),
        tr("KeePass Databases (*.kdb);;All Files (*)"));

    if (filePath.isEmpty()) {
        return;
    }

    // Open master key dialog
    MasterKeyDialog keyDialog(MasterKeyDialog::OpenExisting, this);
    keyDialog.setWindowTitle(tr("Enter Master Key to Repair Database"));

    if (keyDialog.exec() != QDialog::Accepted) {
        return;
    }

    // Set master key
    QString password = keyDialog.getPassword();
    int keyResult = m_pwManager->setMasterKey(password, true, "", false, "");

    if (keyResult != PWE_SUCCESS) {
        QMessageBox::critical(this, tr("Error"),
            tr("Failed to set master key."));
        return;
    }

    // Attempt to open database in repair mode
    PWDB_REPAIR_INFO repairInfo;
    int result = m_pwManager->openDatabase(filePath, &repairInfo);

    if (result != PWE_SUCCESS) {
        // Even repair mode failed
        QString errorMsg;
        switch (result) {
        case PWE_INVALID_KEY:
            errorMsg = tr("The master password or key file is incorrect.");
            break;
        case PWE_FILEERROR_READ:
            errorMsg = tr("Could not open the file. It may be in use by another application.");
            break;
        case PWE_CRYPT_ERROR:
            errorMsg = tr("Decryption failed. The file may be severely corrupted.");
            break;
        default:
            errorMsg = tr("Failed to open database (Error code: %1)").arg(result);
            break;
        }

        QMessageBox::critical(this, tr("Repair Failed"), errorMsg);
        return;
    }

    // Database opened successfully in repair mode
    m_currentFilePath = filePath;
    m_hasDatabase = true;
    m_isModified = true;  // Mark as modified since we rescued data

    // Update UI
    refreshModels();
    updateWindowTitle();
    updateActions();
    updateStatusBar();

    // Show success message with repair statistics
    QString successMsg = tr(
        "Database opened in repair mode.\n\n"
        "Statistics:\n"
        "- Groups recovered: %1\n"
        "- Entries recovered: %2\n"
        "- Recognized meta streams: %3\n\n"
        "IMPORTANT: Please review all data for corruption and save to a new file.")
        .arg(m_pwManager->getNumberOfGroups())
        .arg(m_pwManager->getNumberOfEntries())
        .arg(repairInfo.dwRecognizedMetaStreamCount);

    QMessageBox::information(this, tr("Repair Successful"), successMsg);

    m_statusLabel->setText(tr("Database opened in repair mode - VERIFY ALL DATA"));
}

void MainWindow::onHelpContents()
{
    QMessageBox::information(this, tr("Help"),
                           tr("KeePass Qt - Password Safe\n\n"
                              "Visit https://keepass.info for documentation."));
}

void MainWindow::onHelpLanguages()
{
    LanguagesDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString selectedLang = dialog.selectedLanguage();
        if (!selectedLang.isEmpty()) {
            // Save the selection
            PwSettings::instance().set("Language", selectedLang);

            // Apply the language change
            TranslationManager &tm = TranslationManager::instance();
            if (tm.setLanguage(selectedLang)) {
                // Inform user that restart may be needed for full effect
                QMessageBox::information(this, tr("Language Changed"),
                    tr("Language has been changed to %1.\n\n"
                       "Some changes may require restarting the application.")
                    .arg(tm.languageInfo(selectedLang).name));
            }
        }
    }
}

void MainWindow::onHelpAbout()
{
    QMessageBox::about(this, tr("About KeePass"),
                      tr("<h2>KeePass Password Safe</h2>"
                         "<p>Version 1.43.0 (Qt Port)</p>"
                         "<p>A free, open source password manager.</p>"
                         "<p>Copyright © 2025</p>"
                         "<p>Licensed under GPL v2+</p>"));
}

void MainWindow::onGroupSelectionChanged()
{
    if ((m_pwManager == nullptr) || !m_hasDatabase) {
        return;
    }

    // Get selected group
    QModelIndex currentIndex = m_groupView->currentIndex();

    if (currentIndex.isValid()) {
        PW_GROUP *group = m_groupModel->getGroup(currentIndex);
        if (group != nullptr) {
            // Filter entries to show only those in selected group
            m_entryModel->setGroupFilter(group->uGroupId);

            // Update status bar
            quint32 numEntries = m_pwManager->getNumberOfItemsInGroupN(group->uGroupId);
            QString groupName = QString::fromUtf8(group->pszGroupName);
            if (numEntries == 1) {
                m_statusLabel->setText(tr("Group '%1': 1 entry").arg(groupName));
            } else {
                m_statusLabel->setText(tr("Group '%1': %2 entries").arg(groupName).arg(numEntries));
            }
        } else {
            // No valid group - clear filter
            m_entryModel->clearGroupFilter();
            m_statusLabel->setText(tr("Ready"));
        }
    } else {
        // No selection - show all entries
        m_entryModel->clearGroupFilter();
        m_statusLabel->setText(tr("Ready"));
    }

    updateActions();
}

void MainWindow::onEntrySelectionChanged()
{
    updateActions();
}

void MainWindow::onEntryDoubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index);
    // Open edit entry dialog
    onEditEditEntry();
}

bool MainWindow::openDatabase(const QString &filePath)
{
    // Check if file exists
    if (!QFile::exists(filePath)) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Database file does not exist:\n%1").arg(filePath));
        return false;
    }

    // Show master key dialog
    MasterKeyDialog dialog(MasterKeyDialog::OpenExisting, this);
    if (dialog.exec() != QDialog::Accepted) {
        return false; // User cancelled
    }

    QString password = dialog.getPassword();

    // Close any existing database
    closeDatabase();

    // Set the master key
    int keyResult = m_pwManager->setMasterKey(password, true, "", false, "");
    if (keyResult != PWE_SUCCESS) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to set master password.\n\nError code: %1").arg(keyResult));
        return false;
    }

    // Try to open the database
    int result = m_pwManager->openDatabase(filePath, nullptr);

    if (result != PWE_SUCCESS) {
        // Handle different error codes
        QString errorMsg;
        switch (result) {
            case PWE_INVALID_KEY:
                errorMsg = tr("Invalid master password.\n\nPlease check your password and try again.");
                break;
            case PWE_INVALID_FILEHEADER:
                errorMsg = tr("Invalid database file header.\n\nThe file may be corrupted or not a valid KeePass database.");
                break;
            case PWE_INVALID_FILESIGNATURE:
                errorMsg = tr("Invalid file signature.\n\nThis is not a valid KeePass database file.");
                break;
            case PWE_FILEERROR_READ:
                errorMsg = tr("Cannot read database file.\n\nCheck file permissions and try again.");
                break;
            case PWE_UNSUPPORTED_KDBX:
                errorMsg = tr("Unsupported database format.\n\nThis appears to be a KeePass 2.x (KDBX) file.\nThis version only supports KDB v1.x format.");
                break;
            case PWE_INVALID_FILESTRUCTURE:
                errorMsg = tr("Invalid file structure.\n\nThe database file is corrupted or malformed.");
                break;
            default:
                errorMsg = tr("Failed to open database.\n\nError code: %1").arg(result);
                break;
        }

        QMessageBox::critical(this, tr("Error Opening Database"), errorMsg);
        return false;
    }

    // Success! Update UI
    m_hasDatabase = true;
    m_currentFilePath = filePath;
    m_isModified = false;
    refreshModels();
    updateWindowTitle();
    updateActions();
    updateStatusBar();

    return true;
}

bool MainWindow::saveDatabase()
{
    // If no file path, use Save As
    if (m_currentFilePath.isEmpty()) {
        m_statusLabel->setText(tr("Choose save location..."));
        return saveDatabaseAs();
    }

    m_statusLabel->setText(tr("Saving database..."));

    // Save to current file
    int result = m_pwManager->saveDatabase(m_currentFilePath, nullptr);

    if (result != PWE_SUCCESS) {
        // Handle different error codes
        QString errorMsg;
        switch (result) {
            case PWE_NOFILEACCESS_WRITE:
                errorMsg = tr("Cannot write to database file.\n\nCheck file permissions and disk space.");
                break;
            case PWE_FILEERROR_WRITE:
                errorMsg = tr("Error writing database file.\n\nThe file may be in use or the disk may be full.");
                break;
            case PWE_INVALID_PARAM:
                errorMsg = tr("Invalid parameters.\n\nCannot save database.");
                break;
            default:
                errorMsg = tr("Failed to save database.\n\nError code: %1").arg(result);
                break;
        }

        QMessageBox::critical(this, tr("Error Saving Database"), errorMsg);
        return false;
    }

    // Success! Update UI
    m_isModified = false;
    updateWindowTitle();
    updateActions();
    m_statusLabel->setText(tr("Database saved successfully"));

    return true;
}

bool MainWindow::saveDatabaseAs()
{
    m_statusLabel->setText(tr("Choose save location..."));
    qApp->processEvents(); // Force status bar update

    // Show file save dialog
    QString defaultPath = m_currentFilePath;
    if (defaultPath.isEmpty()) {
        defaultPath = QDir::homePath() + "/Untitled.kdb";
    }

    qDebug() << "Opening save dialog with default path:" << defaultPath;

    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Save Database As"),
        defaultPath,
        tr("KeePass Databases (*.kdb);;All Files (*)"),
        nullptr,
        QFileDialog::DontUseNativeDialog  // Try non-native dialog first
    );

    qDebug() << "Save dialog returned:" << filePath;

    if (filePath.isEmpty()) {
        m_statusLabel->setText(tr("Save cancelled"));
        qDebug() << "User cancelled save dialog";
        return false; // User cancelled
    }

    m_statusLabel->setText(tr("Saving database..."));

    // Add .kdb extension if not present
    if (!filePath.endsWith(".kdb", Qt::CaseInsensitive)) {
        filePath += ".kdb";
    }

    // Save to new file
    int result = m_pwManager->saveDatabase(filePath, nullptr);

    if (result != PWE_SUCCESS) {
        QString errorMsg;
        switch (result) {
            case PWE_NOFILEACCESS_WRITE:
                errorMsg = tr("Cannot write to database file.\n\nCheck file permissions and disk space.");
                break;
            case PWE_FILEERROR_WRITE:
                errorMsg = tr("Error writing database file.\n\nThe file may be in use or the disk may be full.");
                break;
            case PWE_INVALID_PARAM:
                errorMsg = tr("Invalid parameters.\n\nCannot save database.");
                break;
            default:
                errorMsg = tr("Failed to save database.\n\nError code: %1").arg(result);
                break;
        }

        QMessageBox::critical(this, tr("Error Saving Database"), errorMsg);
        return false;
    }

    // Success! Update file path and UI
    m_currentFilePath = filePath;
    m_isModified = false;
    updateWindowTitle();
    updateActions();
    m_statusLabel->setText(tr("Database saved successfully"));

    return true;
}

bool MainWindow::closeDatabase()
{
    m_hasDatabase = false;
    m_isModified = false;
    m_currentFilePath.clear();

    // Clear the database
    if (m_pwManager != nullptr) {
        m_pwManager->newDatabase(); // Reset to empty state
    }

    refreshModels();
    updateWindowTitle();
    updateActions();
    updateStatusBar();

    return true;
}

//==============================================================================
// Clipboard Operations
//==============================================================================

void MainWindow::onEditCopyUsername()
{
    // Reference: MFC OnPwlistCopyUser
    if ((m_entryView == nullptr) || !m_entryView->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndex index = m_entryView->selectionModel()->currentIndex();
    if (!index.isValid()) {
        return;
    }

    // Get the entry from the model
    PW_ENTRY* entry = m_entryModel->getEntry(index);
    if (entry == nullptr) {
        return;
    }

    // Copy username to clipboard
    QString username = QString::fromUtf8(entry->pszUserName);
    copyToClipboard(username);

    m_statusLabel->setText(tr("Field copied to clipboard."));
    startClipboardTimer();
}

void MainWindow::onEditCopyPassword()
{
    // Reference: MFC OnPwlistCopyPw
    if ((m_entryView == nullptr) || !m_entryView->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndex index = m_entryView->selectionModel()->currentIndex();
    if (!index.isValid()) {
        return;
    }

    // Get the entry from the model
    PW_ENTRY* entry = m_entryModel->getEntry(index);
    if (entry == nullptr) {
        return;
    }

    // Unlock password, copy, then lock again (like MFC)
    m_pwManager->unlockEntryPassword(entry);
    QString password = QString::fromUtf8(entry->pszPassword);
    m_pwManager->lockEntryPassword(entry);

    copyToClipboard(password);

    m_statusLabel->setText(tr("Field copied to clipboard."));
    startClipboardTimer();
}

void MainWindow::copyToClipboard(const QString &text)
{
    // Reference: MFC CopyStringToClipboard
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(text);

    // Store hash of clipboard content to track ownership
    QByteArray utf8Data = text.toUtf8();
    m_clipboardHash = QCryptographicHash::hash(utf8Data, QCryptographicHash::Sha256);
}

void MainWindow::clearClipboardIfOwner()
{
    // Reference: MFC ClearClipboardIfOwner
    QClipboard* clipboard = QApplication::clipboard();
    QString clipboardText = clipboard->text();

    // Check if we still own the clipboard by comparing hashes
    QByteArray currentHash = QCryptographicHash::hash(
        clipboardText.toUtf8(), QCryptographicHash::Sha256);

    if (currentHash == m_clipboardHash) {
        // We own it, clear it
        clipboard->clear();
        m_statusLabel->setText(tr("Clipboard cleared."));
    }

    // Clear our hash
    m_clipboardHash.clear();
}

void MainWindow::onEditVisitUrl()
{
    // Get currently selected entry
    if ((m_entryView == nullptr) || !m_entryView->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndex index = m_entryView->selectionModel()->currentIndex();
    if (!index.isValid()) {
        return;
    }

    PW_ENTRY* entry = m_entryModel->getEntry(index);
    if (entry == nullptr) {
        return;
    }

    // Get URL from entry
    QString url = QString::fromUtf8(entry->pszURL).trimmed();
    if (url.isEmpty()) {
        QMessageBox::information(this, tr("Visit URL"),
                               tr("This entry does not have a URL."));
        return;
    }

    // Open the URL
    openUrl(url);

    m_statusLabel->setText(tr("Opening URL: %1").arg(url));
}

void MainWindow::openUrl(const QString& url)
{
    // Reference: MFC OpenUrlEx and ParseAndOpenURLWithEntryInfo

    if (url.isEmpty()) {
        return;
    }

    QString processedUrl = url.trimmed();

    // Check if it's a cmd:// URL (execute command)
    if (processedUrl.toLower().startsWith("cmd://")) {
        QString command = processedUrl.mid(6);  // Remove "cmd://" prefix

        if (command.isEmpty()) {
            QMessageBox::warning(this, tr("Visit URL"),
                               tr("Empty command in cmd:// URL."));
            return;
        }

        // Execute command
        bool success = QProcess::startDetached(command);

        if (!success) {
            QMessageBox::critical(this, tr("Error"),
                                tr("Failed to execute command: %1").arg(command));
        }

        return;
    }

    // For regular URLs, add http:// prefix if no protocol is specified
    if (!processedUrl.contains("://") &&
        !processedUrl.startsWith("\\\\")) {  // Not a UNC path
        processedUrl = "http://" + processedUrl;
    }

    // Open URL with default browser/application
    bool success = QDesktopServices::openUrl(QUrl(processedUrl));

    if (!success) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to open URL: %1").arg(processedUrl));
    }
}

void MainWindow::onEditAutoType()
{
    // Reference: MFC CPwSafeDlg::OnPwlistAutoType (PwSafeDlg.cpp:10138)

    // Get currently selected entry
    if ((m_entryView == nullptr) || !m_entryView->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndex index = m_entryView->selectionModel()->currentIndex();
    if (!index.isValid()) {
        return;
    }

    PW_ENTRY* entry = m_entryModel->getEntry(index);
    if (entry == nullptr) {
        return;
    }

    // Create platform auto-type instance
    QScopedPointer<AutoTypePlatform> autoType(AutoTypePlatform::create());
    if (!autoType) {
        QMessageBox::critical(this, tr("Auto-Type"),
                            tr("Auto-Type is not supported on this platform."));
        return;
    }

    // Check if auto-type is available (accessibility permissions on macOS)
    if (!autoType->isAvailable()) {
        // Show warning but continue anyway - auto-type will attempt and show result
        QMessageBox::StandardButton reply = QMessageBox::warning(this, tr("Auto-Type"),
                           tr("Auto-Type requires accessibility permissions.\n\n"
                              "On macOS: The app needs to be in:\n"
                              "System Preferences > Security & Privacy > "
                              "Privacy > Accessibility\n\n"
                              "For unsigned apps, you may need to:\n"
                              "1. Try using auto-type once (it will fail)\n"
                              "2. Check if KeePass appears in the list\n"
                              "3. Grant permission and try again\n\n"
                              "Continue anyway?"),
                           QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::No) {
            return;
        }
    }

    // Get auto-type sequence - check for custom sequence in entry notes
    QString sequence;
    QString windowTitle;

    // Parse auto-type configuration from entry notes
    if (entry->pszAdditional != nullptr) {
        QString notes = QString::fromUtf8(entry->pszAdditional);
        AutoTypeConfig::parseFromNotes(notes, sequence, windowTitle);
    }

    // If no custom sequence, use default from settings or built-in default
    if (sequence.isEmpty()) {
        sequence = PwSettings::instance().getDefaultAutoTypeSequence();
        if (sequence.isEmpty()) {
            sequence = AutoTypeSequence::defaultSequence();
        }
    }

    // Parse and compile the sequence
    AutoTypeSequence parser;
    QList<AutoTypeAction> actions = parser.compile(sequence, entry, m_pwManager);

    if (actions.isEmpty()) {
        QMessageBox::critical(this, tr("Auto-Type"),
                            tr("Failed to parse auto-type sequence:\n%1").arg(parser.lastError()));
        return;
    }

    // Hide/minimize window before auto-typing
    // This allows auto-type to work in the previously focused window
    m_statusLabel->setText(tr("Performing auto-type..."));

    // Process events to ensure status bar updates
    QApplication::processEvents();

    // Choose method based on settings: minimize or drop-back
    // Reference: MFC ATM_MINIMIZE vs ATM_DROPBACK (PwSafeDlg.cpp:10042-10072)
    bool minimizeMethod = PwSettings::instance().getAutoTypeMinimizeBeforeType();
    bool wasVisible = isVisible();

    if (minimizeMethod) {
        // Minimize method: Minimize window to taskbar/tray
        // This is the default and more reliable method
        showMinimized();
    } else {
        // Drop-back method: Hide window without minimizing
        // The OS will automatically activate the window below
        hide();
    }

    // Process events to ensure window state change completes
    QApplication::processEvents();

    // Wait for window to hide and previous window to activate
    // Increased delay to ensure proper window switching
    QThread::msleep(800);

    // Perform auto-type
    bool success = autoType->performAutoType(actions);

    if (!success) {
        // Restore window based on method used
        if (minimizeMethod) {
            showNormal();
            activateWindow();
        } else {
            // Drop-back method: restore visibility if it was visible
            if (wasVisible) {
                show();
                activateWindow();
            }
        }

        QMessageBox::critical(this, tr("Auto-Type"),
                            tr("Auto-Type failed:\n%1").arg(autoType->lastError()));
        return;
    }

    // Restore window after successful auto-type
    // For minimize method, leave minimized (user can restore manually)
    // For drop-back method, restore visibility
    if (!minimizeMethod && wasVisible) {
        show();
        // Don't activate - let target window keep focus
    }

    // Update last access time
    PW_TIME tNow;
    PwUtil::dateTimeToPwTime(QDateTime::currentDateTime(), &tNow);
    entry->tLastAccess = tNow;

    m_statusLabel->setText(tr("Auto-type completed"));
}

void MainWindow::startClipboardTimer()
{
    // Reference: MFC sets m_nClipboardCountdown to m_dwClipboardSecs
    m_clipboardCountdown = m_clipboardTimeoutSecs;

    // Start the timer if not already running
    if (!m_clipboardTimer->isActive()) {
        m_clipboardTimer->start();
    }
}

void MainWindow::onClipboardTimer()
{
    // Reference: MFC OnTimer APPWND_TIMER_ID handler
    if (m_clipboardCountdown == -1) {
        // Timer not active for clipboard
        return;
    }

    --m_clipboardCountdown;

    if (m_clipboardCountdown == -1) {
        // Time to clear clipboard
        clearClipboardIfOwner();
        m_clipboardTimer->stop();
    }
    else if (m_clipboardCountdown == 0) {
        m_statusLabel->setText(tr("Clipboard cleared."));
    }
    else {
        // Update status with countdown
        m_statusLabel->setText(
            tr("Field copied to clipboard. Clipboard will be cleared in %1 seconds.")
            .arg(m_clipboardCountdown));
    }
}

void MainWindow::onInactivityTimer()
{
    // Auto-lock on inactivity timeout
    if (m_hasDatabase && !m_isLocked) {
        lockWorkspace();
    }
}

void MainWindow::resetInactivityTimer()
{
    if (!m_hasDatabase || m_isLocked) {
        return;
    }

    PwSettings& settings = PwSettings::instance();
    bool lockAfterTime = settings.get("Security/LockAfterTime", false).toBool();

    if (lockAfterTime) {
        m_inactivityTimer->stop();
        m_inactivityTimer->start(m_inactivityTimeoutMs);
    }
}

void MainWindow::startInactivityTimer()
{
    if (!m_hasDatabase || m_isLocked) {
        return;
    }

    PwSettings& settings = PwSettings::instance();
    bool lockAfterTime = settings.get("Security/LockAfterTime", false).toBool();

    if (lockAfterTime) {
        m_inactivityTimer->start(m_inactivityTimeoutMs);
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // Track user activity for auto-lock timer
    if (m_hasDatabase && !m_isLocked) {
        switch (event->type()) {
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease:
            case QEvent::KeyPress:
            case QEvent::KeyRelease:
            case QEvent::Wheel:
                resetInactivityTimer();
                break;
            default:
                break;
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized()) {
            // Check if we should minimize to tray
            PwSettings& settings = PwSettings::instance();
            bool minimizeToTray = settings.get("Interface/MinimizeToTray", false).toBool();

            if (minimizeToTray && (m_systemTrayIcon != nullptr) && m_systemTrayIcon->isVisible()) {
                // Hide window and show in tray
                hide();
                event->ignore();
                return;
            }

            // Check if we should lock on minimize
            bool lockOnMinimize = settings.get("Security/LockOnMinimize", false).toBool();
            if (lockOnMinimize && m_hasDatabase && !m_isLocked) {
                lockWorkspace();
            }
        }
    }

    QMainWindow::changeEvent(event);
}

//==============================================================================
// System Tray Icon
//==============================================================================

void MainWindow::createSystemTrayIcon()
{
    // Check if system tray is available
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qWarning("System tray is not available on this system");
        return;
    }

    // Create tray icon menu
    m_trayIconMenu = new QMenu(this);

    // Create tray actions
    m_actionTrayRestore = new QAction(tr("&Restore"), this);
    connect(m_actionTrayRestore, &QAction::triggered, this, &MainWindow::onTrayRestore);

    m_actionTrayLock = new QAction(tr("&Lock Workspace"), this);
    connect(m_actionTrayLock, &QAction::triggered, this, &MainWindow::onTrayLock);

    m_actionTrayExit = new QAction(tr("E&xit"), this);
    connect(m_actionTrayExit, &QAction::triggered, this, &MainWindow::onTrayExit);

    // Build menu
    m_trayIconMenu->addAction(m_actionTrayRestore);
    m_trayIconMenu->addAction(m_actionTrayLock);
    m_trayIconMenu->addSeparator();
    m_trayIconMenu->addAction(m_actionTrayExit);

    // Create system tray icon
    m_systemTrayIcon = new QSystemTrayIcon(this);
    m_systemTrayIcon->setContextMenu(m_trayIconMenu);
    m_systemTrayIcon->setToolTip(tr("KeePass Password Safe"));

    // Set initial icon
    updateTrayIcon();

    // Connect activation signal
    connect(m_systemTrayIcon, &QSystemTrayIcon::activated,
            this, &MainWindow::onTrayIconActivated);

    // Initially hide the tray icon (will be shown based on settings)
    // updateTrayIcon() will be called from loadSettings()
}

void MainWindow::updateTrayIcon()
{
    if (m_systemTrayIcon == nullptr) {
        return;
    }

    // Update icon based on lock state
    QIcon icon;
    if (m_isLocked) {
        icon = QIcon(":/icons/locked.png");
        m_systemTrayIcon->setToolTip(tr("KeePass Password Safe [LOCKED]"));
        m_actionTrayLock->setText(tr("&Unlock Workspace"));
    } else {
        icon = QIcon(":/icons/keepass.png");
        m_systemTrayIcon->setToolTip(tr("KeePass Password Safe"));
        m_actionTrayLock->setText(tr("&Lock Workspace"));
    }

    if (!icon.isNull()) {
        m_systemTrayIcon->setIcon(icon);
    }

    // Update visibility based on settings
    PwSettings& settings = PwSettings::instance();
    bool showTrayOnlyIfTrayed = settings.get("Advanced/ShowTrayOnlyIfTrayed", false).toBool();
    bool minimizeToTray = settings.get("Interface/MinimizeToTray", false).toBool();

    // Show tray icon if:
    // - Not using "show only when trayed" mode, OR
    // - Using "show only when trayed" AND window is hidden/minimized
    if (!showTrayOnlyIfTrayed || (showTrayOnlyIfTrayed && (!isVisible() || isMinimized()))) {
        if (minimizeToTray || m_hasDatabase) {
            m_systemTrayIcon->show();
        }
    } else {
        m_systemTrayIcon->hide();
    }
}

void MainWindow::showTrayIcon()
{
    if (m_systemTrayIcon != nullptr) {
        m_systemTrayIcon->show();
    }
}

void MainWindow::hideTrayIcon()
{
    if (m_systemTrayIcon != nullptr) {
        m_systemTrayIcon->hide();
    }
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    PwSettings& settings = PwSettings::instance();
    bool singleClickTrayIcon = settings.get("Advanced/SingleClickTrayIcon", false).toBool();

    // Handle activation based on settings
    if (singleClickTrayIcon) {
        // Single click to restore
        if (reason == QSystemTrayIcon::Trigger) {
            onTrayRestore();
        }
    } else {
        // Double click to restore (default)
        if (reason == QSystemTrayIcon::DoubleClick) {
            onTrayRestore();
        }
    }
}

void MainWindow::onTrayRestore()
{
    // Restore window from tray
    show();
    setWindowState(windowState() & ~Qt::WindowMinimized);
    raise();
    activateWindow();
    updateTrayIcon();
}

void MainWindow::onTrayLock()
{
    if (m_isLocked) {
        // Unlock
        unlockWorkspace();
    } else {
        // Lock
        onFileLockWorkspace();
    }
    updateTrayIcon();
}

void MainWindow::onTrayExit()
{
    // Exit application
    close();
}

// Global hotkey methods

void MainWindow::setupGlobalHotkey()
{
    // Reference: MFC CPwSafeDlg implements global hotkey via RegisterHotKey
    // We use CGEventTap on macOS (in GlobalHotkey_mac.cpp)

    GlobalHotkey& hotkey = GlobalHotkey::instance();

    // Connect the hotkey signal to our handler
    connect(&hotkey, &GlobalHotkey::hotkeyTriggered,
            this, &MainWindow::onGlobalHotkeyTriggered);

    // Load hotkey configuration from settings
    PwSettings& settings = PwSettings::instance();
    quint32 hotkeyValue = settings.getAutoTypeGlobalHotKey();

    // Default: Ctrl+Alt+A (if no hotkey is configured)
    QKeySequence keySeq;
    if (hotkeyValue != 0) {
        keySeq = QKeySequence(static_cast<int>(hotkeyValue));
    } else {
        keySeq = QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_A);
    }

    // Only register if auto-type is enabled
    if (settings.getAutoTypeEnabled() && !keySeq.isEmpty()) {
        if (!hotkey.registerHotkey(keySeq)) {
            qWarning() << "Failed to register global hotkey:" << hotkey.lastError();
        }
    }
}

void MainWindow::onGlobalHotkeyTriggered()
{
    // Reference: MFC CPwSafeDlg::OnHotKey (PwSafeDlg.cpp:6148)
    // When global hotkey is pressed, trigger auto-type for the selected entry

    // Check if auto-type is enabled
    PwSettings& settings = PwSettings::instance();
    if (!settings.getAutoTypeEnabled()) {
        return;
    }

    // Check if database is open and not locked
    if (!m_hasDatabase || m_isLocked) {
        // Bring window to front and show unlock dialog
        show();
        setWindowState(windowState() & ~Qt::WindowMinimized);
        raise();
        activateWindow();

        if (m_isLocked) {
            unlockWorkspace();
        }
        return;
    }

    // Trigger auto-type
    // For now, we use the selected entry approach
    // A full implementation would match entries by the currently active window title
    onEditAutoType();
}

// Column visibility slots

void MainWindow::onViewColumnTitle(bool checked)
{
    if (m_entryModel != nullptr) {
        m_entryModel->setColumnVisible(EntryModel::ColumnTitle, checked);
    }
}

void MainWindow::onViewColumnUsername(bool checked)
{
    if (m_entryModel != nullptr) {
        m_entryModel->setColumnVisible(EntryModel::ColumnUsername, checked);
    }
}

void MainWindow::onViewColumnURL(bool checked)
{
    if (m_entryModel != nullptr) {
        m_entryModel->setColumnVisible(EntryModel::ColumnURL, checked);
    }
}

void MainWindow::onViewColumnPassword(bool checked)
{
    if (m_entryModel != nullptr) {
        m_entryModel->setColumnVisible(EntryModel::ColumnPassword, checked);
    }
}

void MainWindow::onViewColumnNotes(bool checked)
{
    if (m_entryModel != nullptr) {
        m_entryModel->setColumnVisible(EntryModel::ColumnNotes, checked);
    }
}

void MainWindow::onViewColumnCreation(bool checked)
{
    if (m_entryModel != nullptr) {
        m_entryModel->setColumnVisible(EntryModel::ColumnCreationTime, checked);
    }
}

void MainWindow::onViewColumnLastMod(bool checked)
{
    if (m_entryModel != nullptr) {
        m_entryModel->setColumnVisible(EntryModel::ColumnLastModification, checked);
    }
}

void MainWindow::onViewColumnLastAccess(bool checked)
{
    if (m_entryModel != nullptr) {
        m_entryModel->setColumnVisible(EntryModel::ColumnLastAccess, checked);
    }
}

void MainWindow::onViewColumnExpires(bool checked)
{
    if (m_entryModel != nullptr) {
        m_entryModel->setColumnVisible(EntryModel::ColumnExpires, checked);
    }
}

void MainWindow::onViewColumnUUID(bool checked)
{
    if (m_entryModel != nullptr) {
        m_entryModel->setColumnVisible(EntryModel::ColumnUUID, checked);
    }
}

void MainWindow::onViewColumnAttachment(bool checked)
{
    if (m_entryModel != nullptr) {
        m_entryModel->setColumnVisible(EntryModel::ColumnAttachment, checked);
    }
}

void MainWindow::onViewHidePasswordStars(bool checked)
{
    // Reference: MFC OnViewHideStars (PwSafeDlg.cpp:2656-2697)
    PwSettings::instance().setHidePasswordStars(checked);
    PwSettings::instance().sync();

    // Force refresh of entry view to show/hide passwords
    if (m_entryModel != nullptr) {
        m_entryModel->refresh();
    }
}

void MainWindow::onViewHideUsernameStars(bool checked)
{
    // Reference: MFC OnViewHideUsers (PwSafeDlg.cpp:9178-9225)
    PwSettings::instance().setHideUsernameStars(checked);
    PwSettings::instance().sync();

    // Force refresh of entry view to show/hide usernames
    if (m_entryModel != nullptr) {
        m_entryModel->refresh();
    }
}

// Database Tools

void MainWindow::onToolsShowExpiredEntries()
{
    // Reference: MFC/MFC-KeePass/WinGUI/PwSafeDlg.cpp OnExtrasShowExpired
    if ((m_pwManager == nullptr) || !m_hasDatabase || m_isLocked) {
        return;
    }

    // Find all expired entries
    QList<quint32> expiredIndices = m_pwManager->findExpiredEntries(true, true);

    if (expiredIndices.isEmpty()) {
        QMessageBox::information(this, tr("Expired Entries"),
            tr("There are no expired entries in this database."));
        return;
    }

    // Show expired entries in the entry list by applying index filter
    m_entryModel->setIndexFilter(expiredIndices);

    // Update status bar to show count
    m_statusLabel->setText(tr("Found %1 expired entries").arg(expiredIndices.count()));
}

void MainWindow::onToolsShowExpiringSoon()
{
    // Reference: MFC/MFC-KeePass/WinGUI/PwSafeDlg.cpp _ShowExpiredEntries with bShowSoonToExpire
    if ((m_pwManager == nullptr) || !m_hasDatabase || m_isLocked) {
        return;
    }

    // Default: 7 days (MFC uses _GetSoonToExpireDays() which defaults to 7)
    int days = 7;

    // Find entries expiring within the next 7 days
    QList<quint32> expiringIndices = m_pwManager->findSoonToExpireEntries(days, true, true);

    if (expiringIndices.isEmpty()) {
        QMessageBox::information(this, tr("Expiring Soon"),
            tr("There are no entries expiring within the next %1 days.").arg(days));
        return;
    }

    // Show expiring entries in the entry list by applying index filter
    m_entryModel->setIndexFilter(expiringIndices);

    // Update status bar to show count
    m_statusLabel->setText(tr("Found %1 entries expiring within %2 days").arg(expiringIndices.count()).arg(days));
}

void MainWindow::onToolsPlugins()
{
    PluginsDialog dialog(this);
    dialog.exec();
}

// Helper: Generate HTML for printing
QString MainWindow::generateHtmlForPrint(quint32 fieldFlags)
{
    // Generate HTML using PwExport to a temporary file
    QTemporaryFile tempFile;
    tempFile.setFileTemplate(QDir::tempPath() + "/keepass_print_XXXXXX.html");
    tempFile.setAutoRemove(true);

    if (!tempFile.open()) {
        qWarning() << "Failed to create temporary file for printing";
        return QString();
    }

    QString tempPath = tempFile.fileName();
    tempFile.close();  // Close so PwExport can write to it

    // Export to HTML
    if (!PwExport::exportDatabase(m_pwManager, tempPath, PWEXP_HTML, fieldFlags)) {
        qWarning() << "Failed to export HTML for printing";
        return QString();
    }

    // Read the HTML content
    QFile file(tempPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to read temporary HTML file";
        return QString();
    }

    QString html = QString::fromUtf8(file.readAll());
    file.close();

    // Clean up temp file
    QFile::remove(tempPath);

    return html;
}
