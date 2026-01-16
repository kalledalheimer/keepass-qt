/*
  Qt KeePass - Plugins Dialog

  Shows list of loaded plugins with options to configure them.
  Reference: MFC WinGUI/PluginsDlg.h
*/

#ifndef PLUGINSDIALOG_H
#define PLUGINSDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QDialogButtonBox>
#include <QPushButton>

class PluginsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PluginsDialog(QWidget *parent = nullptr);

private slots:
    void onOpenPluginFolder();
    void onPluginContextMenu(const QPoint& pos);
    void onConfigurePlugin();
    void onAboutPlugin();
    void onRefreshList();

private:
    void setupUi();
    void loadPluginList();
    [[nodiscard]] quint32 selectedPluginId() const;

    QTableWidget *m_pluginTable;
    QPushButton *m_openFolderButton;
    QPushButton *m_refreshButton;
    QDialogButtonBox *m_buttonBox;
};

#endif // PLUGINSDIALOG_H
