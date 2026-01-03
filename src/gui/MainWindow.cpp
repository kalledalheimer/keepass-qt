/*
  Qt KeePass - Main Window Implementation
*/

#include "MainWindow.h"
#include "GroupModel.h"
#include "EntryModel.h"
#include "MasterKeyDialog.h"
#include "AddGroupDialog.h"
#include "AddEntryDialog.h"
#include "FindDialog.h"
#include "PasswordGeneratorDialog.h"
#include "IconManager.h"
#include "../core/PwManager.h"
#include "../core/platform/PwSettings.h"
#include "../core/util/PwUtil.h"

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
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QDebug>
#include <QClipboard>
#include <QCryptographicHash>

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
    , m_clipboardTimer(new QTimer(this))
    , m_clipboardCountdown(-1)
    , m_clipboardTimeoutSecs(11)  // Default: 10+1 seconds (matching MFC)
{
    // Connect clipboard timer
    connect(m_clipboardTimer, &QTimer::timeout, this, &MainWindow::onClipboardTimer);
    m_clipboardTimer->setInterval(1000);  // 1 second

    setupUi();
    loadSettings();
    updateWindowTitle();
    updateActions();
}

MainWindow::~MainWindow()
{
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

    m_actionEditDeleteEntry = new QAction(iconMgr.getToolbarIcon("tb_delet"), tr("&Delete Entry"), this);
    m_actionEditDeleteEntry->setShortcut(Qt::Key_Delete);
    m_actionEditDeleteEntry->setStatusTip(tr("Delete the selected entry"));
    m_actionEditDeleteEntry->setEnabled(false);
    connect(m_actionEditDeleteEntry, &QAction::triggered, this, &MainWindow::onEditDeleteEntry);

    m_actionEditDeleteGroup = new QAction(iconMgr.getToolbarIcon("tb_delet"), tr("Delete &Group"), this);
    m_actionEditDeleteGroup->setStatusTip(tr("Delete the selected group"));
    m_actionEditDeleteGroup->setEnabled(false);
    connect(m_actionEditDeleteGroup, &QAction::triggered, this, &MainWindow::onEditDeleteGroup);

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

    // Help menu actions
    m_actionHelpContents = new QAction(tr("&Contents"), this);
    m_actionHelpContents->setShortcut(QKeySequence::HelpContents);
    m_actionHelpContents->setStatusTip(tr("Show help contents"));
    connect(m_actionHelpContents, &QAction::triggered, this, &MainWindow::onHelpContents);

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
    fileMenu->addAction(m_actionFileClose);
    fileMenu->addSeparator();
    fileMenu->addAction(m_actionFileExit);

    // Edit menu
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(m_actionEditAddGroup);
    editMenu->addAction(m_actionEditAddEntry);
    editMenu->addSeparator();
    editMenu->addAction(m_actionEditEditEntry);
    editMenu->addSeparator();
    editMenu->addAction(m_actionEditDeleteEntry);
    editMenu->addAction(m_actionEditDeleteGroup);
    editMenu->addSeparator();
    editMenu->addAction(m_actionEditCopyUsername);
    editMenu->addAction(m_actionEditCopyPassword);
    editMenu->addSeparator();
    editMenu->addAction(m_actionEditFind);

    // View menu
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(m_actionViewToolbar);
    viewMenu->addAction(m_actionViewStatusBar);
    viewMenu->addSeparator();
    viewMenu->addAction(m_actionViewExpandAll);
    viewMenu->addAction(m_actionViewCollapseAll);

    // Tools menu
    QMenu *toolsMenu = menuBar()->addMenu(tr("&Tools"));
    toolsMenu->addAction(m_actionToolsPasswordGenerator);
    toolsMenu->addAction(m_actionToolsDatabaseSettings);
    toolsMenu->addSeparator();
    toolsMenu->addAction(m_actionToolsOptions);

    // Help menu
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(m_actionHelpContents);
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
        title += tr(" - KeePass Password Safe");
    }

    setWindowTitle(title);
}

void MainWindow::updateActions()
{
    bool hasSelection = m_entryView && m_entryView->selectionModel()->hasSelection();

    m_actionFileSave->setEnabled(m_hasDatabase && m_isModified);
    m_actionFileSaveAs->setEnabled(m_hasDatabase);
    m_actionFileClose->setEnabled(m_hasDatabase);

    m_actionEditAddGroup->setEnabled(m_hasDatabase);
    m_actionEditAddEntry->setEnabled(m_hasDatabase);
    m_actionEditEditEntry->setEnabled(hasSelection);
    m_actionEditDeleteEntry->setEnabled(hasSelection);
    m_actionEditDeleteGroup->setEnabled(m_hasDatabase);
    m_actionEditFind->setEnabled(m_hasDatabase);
    m_actionEditCopyUsername->setEnabled(hasSelection);
    m_actionEditCopyPassword->setEnabled(hasSelection);

    m_actionViewExpandAll->setEnabled(m_hasDatabase);
    m_actionViewCollapseAll->setEnabled(m_hasDatabase);

    m_actionToolsPasswordGenerator->setEnabled(m_hasDatabase);
    m_actionToolsDatabaseSettings->setEnabled(m_hasDatabase);
}

void MainWindow::updateStatusBar()
{
    if (!m_pwManager) {
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
    if (m_groupModel) {
        m_groupModel->refresh();
    }

    if (m_entryModel) {
        m_entryModel->refresh();
    }

    // Expand the root group by default
    if (m_groupView) {
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
    } else if (result == QMessageBox::Discard) {
        return true;
    } else {
        return false;
    }
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
        if (group) {
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
    if (!entry) {
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
    if (entry->pszBinaryDesc && entry->pBinaryData && entry->uBinaryDataLen > 0) {
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
    if (entryTemplate.pBinaryData) {
        delete[] entryTemplate.pBinaryData;
    }

    if (!success) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to update entry."));
        m_statusLabel->setText(tr("Failed to update entry"));
        return;
    }

    // Update UI
    m_isModified = true;
    refreshModels();
    updateWindowTitle();
    updateActions();
    m_statusLabel->setText(tr("Entry '%1' updated successfully").arg(title));
}

void MainWindow::onEditDeleteEntry()
{
    // Check if database is open
    if (!m_pwManager || !m_hasDatabase) {
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
    if (!entry) {
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
    if (!m_pwManager || !m_hasDatabase) {
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

void MainWindow::onEditFind()
{
    // Reference: MFC/MFC-KeePass/WinGUI/PwSafeDlg.cpp _Find method
    if (!m_pwManager || !m_hasDatabase) {
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
    // TODO: Implement
    QMessageBox::information(this, tr("Not Implemented"), tr("Tools > Options will be implemented later"));
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
    // TODO: Implement
}

void MainWindow::onHelpContents()
{
    QMessageBox::information(this, tr("Help"),
                           tr("KeePass Qt - Password Safe\n\n"
                              "Visit https://keepass.info for documentation."));
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
    if (!m_pwManager || !m_hasDatabase) {
        return;
    }

    // Get selected group
    QModelIndex currentIndex = m_groupView->currentIndex();

    if (currentIndex.isValid()) {
        PW_GROUP *group = m_groupModel->getGroup(currentIndex);
        if (group) {
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
    if (m_pwManager) {
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
    if (!m_entryView || !m_entryView->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndex index = m_entryView->selectionModel()->currentIndex();
    if (!index.isValid()) {
        return;
    }

    // Get the entry from the model
    PW_ENTRY* entry = m_entryModel->getEntry(index);
    if (!entry) {
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
    if (!m_entryView || !m_entryView->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndex index = m_entryView->selectionModel()->currentIndex();
    if (!index.isValid()) {
        return;
    }

    // Get the entry from the model
    PW_ENTRY* entry = m_entryModel->getEntry(index);
    if (!entry) {
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
