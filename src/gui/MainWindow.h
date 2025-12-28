/*
  Qt KeePass - Main Window

  Main application window matching MFC KeePass layout:
  - Menu bar: File, Edit, View, Tools, Help
  - Toolbar with common actions
  - Split view: Groups (tree) | Entries (table)
  - Status bar
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeView>
#include <QTableView>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QMenuBar>
#include <QLabel>

// Forward declarations
class PwManager;
class GroupModel;
class EntryModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    // Window events
    void closeEvent(QCloseEvent *event) override;

private slots:
    // File menu
    void onFileNew();
    void onFileOpen();
    void onFileSave();
    void onFileSaveAs();
    void onFileClose();
    void onFileExit();

    // Edit menu
    void onEditAddGroup();
    void onEditAddEntry();
    void onEditEditEntry();
    void onEditDeleteEntry();
    void onEditDeleteGroup();
    void onEditFind();

    // View menu
    void onViewToolbar();
    void onViewStatusBar();
    void onViewExpandAll();
    void onViewCollapseAll();

    // Tools menu
    void onToolsOptions();
    void onToolsPasswordGenerator();
    void onToolsDatabaseSettings();

    // Help menu
    void onHelpContents();
    void onHelpAbout();

    // Selection changes
    void onGroupSelectionChanged();
    void onEntrySelectionChanged();
    void onEntryDoubleClicked(const QModelIndex &index);

private:
    // UI setup
    void setupUi();
    void createActions();
    void createMenus();
    void createToolBar();
    void createStatusBar();
    void createCentralWidget();
    void loadSettings();
    void saveSettings();

    // Database operations
    bool openDatabase(const QString &filePath);
    bool saveDatabase();
    bool saveDatabaseAs();
    bool closeDatabase();
    bool confirmSaveChanges();

    // UI updates
    void updateWindowTitle();
    void updateActions();
    void updateStatusBar();
    void refreshModels();

    // Members
    PwManager *m_pwManager;
    GroupModel *m_groupModel;
    EntryModel *m_entryModel;

    // UI components
    QSplitter *m_splitter;
    QTreeView *m_groupView;
    QTableView *m_entryView;
    QToolBar *m_toolBar;
    QLabel *m_statusLabel;

    // Actions - File
    QAction *m_actionFileNew;
    QAction *m_actionFileOpen;
    QAction *m_actionFileSave;
    QAction *m_actionFileSaveAs;
    QAction *m_actionFileClose;
    QAction *m_actionFileExit;

    // Actions - Edit
    QAction *m_actionEditAddGroup;
    QAction *m_actionEditAddEntry;
    QAction *m_actionEditEditEntry;
    QAction *m_actionEditDeleteEntry;
    QAction *m_actionEditDeleteGroup;
    QAction *m_actionEditFind;

    // Actions - View
    QAction *m_actionViewToolbar;
    QAction *m_actionViewStatusBar;
    QAction *m_actionViewExpandAll;
    QAction *m_actionViewCollapseAll;

    // Actions - Tools
    QAction *m_actionToolsOptions;
    QAction *m_actionToolsPasswordGenerator;
    QAction *m_actionToolsDatabaseSettings;

    // Actions - Help
    QAction *m_actionHelpContents;
    QAction *m_actionHelpAbout;

    // State
    QString m_currentFilePath;
    bool m_isModified;
    bool m_hasDatabase;
};

#endif // MAINWINDOW_H
