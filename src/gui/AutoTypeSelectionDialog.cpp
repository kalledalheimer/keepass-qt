/*
  Qt KeePass - Auto-Type Entry Selection Dialog Implementation
*/

#include "AutoTypeSelectionDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <algorithm>

AutoTypeSelectionDialog::AutoTypeSelectionDialog(
    const QList<PW_ENTRY*>& entries,
    const QString& windowTitle,
    bool sortEntries,
    QWidget* parent)
    : QDialog(parent)
    , m_entries(entries)
    , m_windowTitle(windowTitle)
    , m_sortEntries(sortEntries)
    , m_selectedEntry(nullptr)
{
    setupUi();
    populateTable();

    // Sort if requested
    if (m_sortEntries && m_tableEntries->rowCount() > 0) {
        m_tableEntries->sortItems(0, Qt::AscendingOrder);  // Sort by title column
    }

    // Select first entry by default
    if (m_tableEntries->rowCount() > 0) {
        m_tableEntries->selectRow(0);
    }
}

void AutoTypeSelectionDialog::setupUi()
{
    setWindowTitle(tr("Auto-Type Entry Selection"));
    setMinimumWidth(600);
    setMinimumHeight(400);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Message label
    m_labelMessage = new QLabel(this);
    m_labelMessage->setWordWrap(true);
    if (!m_windowTitle.isEmpty()) {
        m_labelMessage->setText(
            tr("Multiple entries match the window \"%1\".\n"
               "Please select the entry to auto-type:")
            .arg(m_windowTitle));
    } else {
        m_labelMessage->setText(
            tr("Multiple entries match the current window.\n"
               "Please select the entry to auto-type:"));
    }
    mainLayout->addWidget(m_labelMessage);

    // Entry table
    m_tableEntries = new QTableWidget(this);
    m_tableEntries->setColumnCount(3);
    m_tableEntries->setHorizontalHeaderLabels({tr("Title"), tr("Username"), tr("URL")});
    m_tableEntries->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableEntries->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableEntries->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableEntries->horizontalHeader()->setStretchLastSection(true);
    m_tableEntries->verticalHeader()->hide();

    // Set column widths
    m_tableEntries->setColumnWidth(0, 200);  // Title
    m_tableEntries->setColumnWidth(1, 150);  // Username

    connect(m_tableEntries, &QTableWidget::cellDoubleClicked,
            this, &AutoTypeSelectionDialog::onEntryDoubleClicked);
    connect(m_tableEntries, &QTableWidget::itemSelectionChanged,
            this, &AutoTypeSelectionDialog::onSelectionChanged);

    mainLayout->addWidget(m_tableEntries);

    // Button box
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttonOk = buttonBox->button(QDialogButtonBox::Ok);
    m_buttonCancel = buttonBox->button(QDialogButtonBox::Cancel);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);

    // Initially disable OK until something is selected
    m_buttonOk->setEnabled(false);
}

void AutoTypeSelectionDialog::populateTable()
{
    m_tableEntries->setRowCount(m_entries.size());

    for (int i = 0; i < m_entries.size(); ++i) {
        PW_ENTRY* entry = m_entries[i];

        // Title
        QString title;
        if (entry->pszTitle) {
            title = QString::fromUtf8(entry->pszTitle);
        }
        QTableWidgetItem* itemTitle = new QTableWidgetItem(title);
        itemTitle->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(entry)));
        m_tableEntries->setItem(i, 0, itemTitle);

        // Username
        QString username;
        if (entry->pszUserName) {
            username = QString::fromUtf8(entry->pszUserName);
        }
        QTableWidgetItem* itemUsername = new QTableWidgetItem(username);
        m_tableEntries->setItem(i, 1, itemUsername);

        // URL
        QString url;
        if (entry->pszURL) {
            url = QString::fromUtf8(entry->pszURL);
        }
        QTableWidgetItem* itemUrl = new QTableWidgetItem(url);
        m_tableEntries->setItem(i, 2, itemUrl);
    }
}

void AutoTypeSelectionDialog::onEntryDoubleClicked(int row, int column)
{
    Q_UNUSED(column);

    if (row >= 0 && row < m_tableEntries->rowCount()) {
        QTableWidgetItem* item = m_tableEntries->item(row, 0);
        if (item) {
            m_selectedEntry = static_cast<PW_ENTRY*>(
                item->data(Qt::UserRole).value<void*>());
            accept();
        }
    }
}

void AutoTypeSelectionDialog::onSelectionChanged()
{
    QList<QTableWidgetItem*> selectedItems = m_tableEntries->selectedItems();
    m_buttonOk->setEnabled(!selectedItems.isEmpty());

    // Update selected entry
    if (!selectedItems.isEmpty()) {
        int row = selectedItems.first()->row();
        QTableWidgetItem* item = m_tableEntries->item(row, 0);
        if (item) {
            m_selectedEntry = static_cast<PW_ENTRY*>(
                item->data(Qt::UserRole).value<void*>());
        }
    } else {
        m_selectedEntry = nullptr;
    }
}
