/*
  Qt KeePass - Plugins Dialog Implementation

  Reference: MFC WinGUI/PluginsDlg.cpp
*/

#include "PluginsDialog.h"
#include "../plugins/PluginManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QDesktopServices>
#include <QUrl>
#include <QLabel>
#include <QGroupBox>
#include <QMessageBox>

PluginsDialog::PluginsDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi();
    loadPluginList();
}

void PluginsDialog::setupUi()
{
    setWindowTitle(tr("Plugins"));
    resize(600, 400);

    auto *layout = new QVBoxLayout(this);

    // Info label
    auto *infoLabel = new QLabel(
        tr("Plugins extend KeePass functionality. Place plugin files in the plugins folder."),
        this
    );
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);

    // Plugin table
    m_pluginTable = new QTableWidget(this);
    m_pluginTable->setColumnCount(4);
    m_pluginTable->setHorizontalHeaderLabels({
        tr("Name"), tr("Version"), tr("Author"), tr("Description")
    });
    m_pluginTable->horizontalHeader()->setStretchLastSection(true);
    m_pluginTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_pluginTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_pluginTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_pluginTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pluginTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pluginTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pluginTable->setContextMenuPolicy(Qt::CustomContextMenu);
    m_pluginTable->verticalHeader()->setVisible(false);
    m_pluginTable->setAlternatingRowColors(true);

    connect(m_pluginTable, &QTableWidget::customContextMenuRequested,
            this, &PluginsDialog::onPluginContextMenu);

    layout->addWidget(m_pluginTable);

    // Button row
    auto *buttonLayout = new QHBoxLayout();

    m_openFolderButton = new QPushButton(tr("Open Plugin Folder"), this);
    m_openFolderButton->setToolTip(tr("Open the plugins folder in file manager"));
    connect(m_openFolderButton, &QPushButton::clicked, this, &PluginsDialog::onOpenPluginFolder);
    buttonLayout->addWidget(m_openFolderButton);

    m_refreshButton = new QPushButton(tr("Refresh"), this);
    m_refreshButton->setToolTip(tr("Reload the plugin list"));
    connect(m_refreshButton, &QPushButton::clicked, this, &PluginsDialog::onRefreshList);
    buttonLayout->addWidget(m_refreshButton);

    buttonLayout->addStretch();

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    buttonLayout->addWidget(m_buttonBox);

    layout->addLayout(buttonLayout);

    // Status label
    auto *statusGroup = new QGroupBox(tr("Status"), this);
    auto *statusLayout = new QVBoxLayout(statusGroup);
    auto *statusLabel = new QLabel(this);
    statusLayout->addWidget(statusLabel);
    layout->addWidget(statusGroup);

    // Update status when selection changes
    connect(m_pluginTable, &QTableWidget::itemSelectionChanged, this, [this, statusLabel]() {
        quint32 pluginId = selectedPluginId();
        const PluginInstance* plugin = PluginManager::instance().getPluginById(pluginId);
        if (plugin != nullptr) {
            QString status = tr("Plugin ID: %1\nFile: %2")
                .arg(plugin->pluginId)
                .arg(plugin->filePath);
            if (!plugin->info.website.isEmpty()) {
                status += tr("\nWebsite: %1").arg(plugin->info.website);
            }
            statusLabel->setText(status);
        } else {
            statusLabel->setText(tr("No plugin selected"));
        }
    });
}

void PluginsDialog::loadPluginList()
{
    m_pluginTable->setRowCount(0);

    PluginManager& pm = PluginManager::instance();
    int count = pm.pluginCount();

    for (int i = 0; i < count; ++i) {
        const PluginInstance* plugin = pm.getPlugin(i);
        if (plugin == nullptr) {
            continue;
        }

        int row = m_pluginTable->rowCount();
        m_pluginTable->insertRow(row);

        // Name
        auto *nameItem = new QTableWidgetItem(plugin->info.name);
        if (!plugin->info.icon.isNull()) {
            nameItem->setIcon(plugin->info.icon);
        }
        nameItem->setData(Qt::UserRole, plugin->pluginId);
        m_pluginTable->setItem(row, 0, nameItem);

        // Version
        m_pluginTable->setItem(row, 1, new QTableWidgetItem(plugin->info.version));

        // Author
        m_pluginTable->setItem(row, 2, new QTableWidgetItem(plugin->info.author));

        // Description
        m_pluginTable->setItem(row, 3, new QTableWidgetItem(plugin->info.description));
    }

    // Show message if no plugins
    if (count == 0) {
        int row = m_pluginTable->rowCount();
        m_pluginTable->insertRow(row);
        auto *item = new QTableWidgetItem(tr("No plugins loaded"));
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        item->setForeground(Qt::gray);
        m_pluginTable->setItem(row, 0, item);
        m_pluginTable->setSpan(row, 0, 1, 4);
    }
}

quint32 PluginsDialog::selectedPluginId() const
{
    QList<QTableWidgetItem*> selected = m_pluginTable->selectedItems();
    if (selected.isEmpty()) {
        return 0;
    }

    // Get plugin ID from first column's UserRole data
    int row = selected.first()->row();
    QTableWidgetItem* nameItem = m_pluginTable->item(row, 0);
    if (nameItem != nullptr) {
        return nameItem->data(Qt::UserRole).toUInt();
    }
    return 0;
}

void PluginsDialog::onOpenPluginFolder()
{
    QString folder = PluginManager::instance().primaryPluginDirectory();
    QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}

void PluginsDialog::onPluginContextMenu(const QPoint& pos)
{
    quint32 pluginId = selectedPluginId();
    if (pluginId == 0) {
        return;
    }

    QMenu menu(this);

    QAction* configAction = menu.addAction(tr("Configure..."));
    connect(configAction, &QAction::triggered, this, &PluginsDialog::onConfigurePlugin);

    QAction* aboutAction = menu.addAction(tr("About..."));
    connect(aboutAction, &QAction::triggered, this, &PluginsDialog::onAboutPlugin);

    menu.exec(m_pluginTable->viewport()->mapToGlobal(pos));
}

void PluginsDialog::onConfigurePlugin()
{
    quint32 pluginId = selectedPluginId();
    if (pluginId != 0) {
        PluginManager::instance().sendEventToPlugin(
            pluginId, KpPluginEvent::DirectConfig, nullptr, nullptr
        );
    }
}

void PluginsDialog::onAboutPlugin()
{
    quint32 pluginId = selectedPluginId();
    if (pluginId == 0) {
        return;
    }

    const PluginInstance* plugin = PluginManager::instance().getPluginById(pluginId);
    if (plugin == nullptr) {
        return;
    }

    // First try to let plugin show its own about dialog
    bool handled = PluginManager::instance().sendEventToPlugin(
        pluginId, KpPluginEvent::PluginInfo, nullptr, nullptr
    );

    // If plugin didn't handle it, show default about dialog
    if (!handled) {
        QString message = tr("<b>%1</b><br>"
                            "Version: %2<br>"
                            "Author: %3<br><br>"
                            "%4")
            .arg(plugin->info.name.toHtmlEscaped())
            .arg(plugin->info.version.toHtmlEscaped())
            .arg(plugin->info.author.toHtmlEscaped())
            .arg(plugin->info.description.toHtmlEscaped());

        if (!plugin->info.website.isEmpty()) {
            message += tr("<br><br><a href=\"%1\">%1</a>")
                .arg(plugin->info.website.toHtmlEscaped());
        }

        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("About %1").arg(plugin->info.name));
        msgBox.setTextFormat(Qt::RichText);
        msgBox.setText(message);
        msgBox.setIcon(QMessageBox::Information);
        if (!plugin->info.icon.isNull()) {
            msgBox.setIconPixmap(plugin->info.icon.pixmap(64, 64));
        }
        msgBox.exec();
    }
}

void PluginsDialog::onRefreshList()
{
    // Note: This just refreshes the display, doesn't reload plugins
    // Reloading would require unloading first which could be disruptive
    loadPluginList();
}
