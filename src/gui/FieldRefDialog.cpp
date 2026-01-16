/*
  Qt KeePass - Field Reference Dialog Implementation

  Reference: MFC WinGUI/FieldRefDlg.cpp
*/

#include "FieldRefDialog.h"
#include "IconManager.h"
#include "../core/PwManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QMessageBox>

FieldRefDialog::FieldRefDialog(PwManager *pwManager, QWidget *parent)
    : QDialog(parent)
    , m_pwManager(pwManager)
{
    setWindowTitle(tr("Insert Field Reference"));
    setMinimumSize(700, 500);

    setupUi();
    populateEntryList();
    updateControls();
}

void FieldRefDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Instructions
    QLabel *instructionsLabel = new QLabel(
        tr("Select an entry below, then choose which field to reference and how to identify the entry.\n"
           "A field reference allows you to use values from other entries."));
    instructionsLabel->setWordWrap(true);
    mainLayout->addWidget(instructionsLabel);

    // Entry table
    m_entryTable = new QTableWidget();
    m_entryTable->setColumnCount(ColCount);
    m_entryTable->setHorizontalHeaderLabels({
        tr("Group"), tr("Title"), tr("User Name"), tr("URL"), tr("Notes"), tr("UUID")
    });
    m_entryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_entryTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_entryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_entryTable->horizontalHeader()->setStretchLastSection(true);
    m_entryTable->verticalHeader()->setVisible(false);

    connect(m_entryTable, &QTableWidget::itemSelectionChanged,
            this, &FieldRefDialog::onEntrySelectionChanged);

    mainLayout->addWidget(m_entryTable, 1);

    // Options row
    QHBoxLayout *optionsLayout = new QHBoxLayout();

    // Field to reference group
    QGroupBox *refFieldBox = new QGroupBox(tr("Field to Reference"));
    QVBoxLayout *refFieldLayout = new QVBoxLayout(refFieldBox);

    m_refFieldGroup = new QButtonGroup(this);
    m_radioRefTitle = new QRadioButton(tr("Title"));
    m_radioRefUsername = new QRadioButton(tr("User Name"));
    m_radioRefPassword = new QRadioButton(tr("Password"));
    m_radioRefUrl = new QRadioButton(tr("URL"));
    m_radioRefNotes = new QRadioButton(tr("Notes"));

    m_refFieldGroup->addButton(m_radioRefTitle, 0);
    m_refFieldGroup->addButton(m_radioRefUsername, 1);
    m_refFieldGroup->addButton(m_radioRefPassword, 2);
    m_refFieldGroup->addButton(m_radioRefUrl, 3);
    m_refFieldGroup->addButton(m_radioRefNotes, 4);

    m_radioRefUsername->setChecked(true);  // Default

    refFieldLayout->addWidget(m_radioRefTitle);
    refFieldLayout->addWidget(m_radioRefUsername);
    refFieldLayout->addWidget(m_radioRefPassword);
    refFieldLayout->addWidget(m_radioRefUrl);
    refFieldLayout->addWidget(m_radioRefNotes);

    optionsLayout->addWidget(refFieldBox);

    // Identify by group
    QGroupBox *idFieldBox = new QGroupBox(tr("Identify Entry By"));
    QVBoxLayout *idFieldLayout = new QVBoxLayout(idFieldBox);

    m_idFieldGroup = new QButtonGroup(this);
    m_radioIdTitle = new QRadioButton(tr("Title"));
    m_radioIdUsername = new QRadioButton(tr("User Name"));
    m_radioIdPassword = new QRadioButton(tr("Password"));
    m_radioIdUrl = new QRadioButton(tr("URL"));
    m_radioIdNotes = new QRadioButton(tr("Notes"));
    m_radioIdUuid = new QRadioButton(tr("UUID (always unique)"));

    m_idFieldGroup->addButton(m_radioIdTitle, 0);
    m_idFieldGroup->addButton(m_radioIdUsername, 1);
    m_idFieldGroup->addButton(m_radioIdPassword, 2);
    m_idFieldGroup->addButton(m_radioIdUrl, 3);
    m_idFieldGroup->addButton(m_radioIdNotes, 4);
    m_idFieldGroup->addButton(m_radioIdUuid, 5);

    m_radioIdUuid->setChecked(true);  // Default to UUID (most reliable)

    idFieldLayout->addWidget(m_radioIdTitle);
    idFieldLayout->addWidget(m_radioIdUsername);
    idFieldLayout->addWidget(m_radioIdPassword);
    idFieldLayout->addWidget(m_radioIdUrl);
    idFieldLayout->addWidget(m_radioIdNotes);
    idFieldLayout->addWidget(m_radioIdUuid);

    optionsLayout->addWidget(idFieldBox);

    mainLayout->addLayout(optionsLayout);

    // Status label
    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet("color: red;");
    mainLayout->addWidget(m_statusLabel);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_helpButton = new QPushButton(tr("&Help"));
    connect(m_helpButton, &QPushButton::clicked, this, &FieldRefDialog::onHelp);
    buttonLayout->addWidget(m_helpButton);

    buttonLayout->addStretch();

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_okButton = buttonBox->button(QDialogButtonBox::Ok);
    m_cancelButton = buttonBox->button(QDialogButtonBox::Cancel);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &FieldRefDialog::validateAndAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    buttonLayout->addWidget(buttonBox);
    mainLayout->addLayout(buttonLayout);
}

void FieldRefDialog::populateEntryList()
{
    m_entryTable->setRowCount(0);

    if (m_pwManager == nullptr) {
        return;
    }

    // Get backup group ID to exclude backup entries
    quint32 backupGroupId = m_pwManager->getGroupId("Backup");

    quint32 entryCount = m_pwManager->getNumberOfEntries();
    for (quint32 i = 0; i < entryCount; ++i) {
        PW_ENTRY* entry = m_pwManager->getEntry(i);
        if (entry == nullptr) {
            continue;
        }

        // Skip backup entries
        if (backupGroupId != 0 && entry->uGroupId == backupGroupId) {
            continue;
        }

        // Skip meta-stream entries
        if (entry->pszTitle != nullptr && QString::fromUtf8(entry->pszTitle) == "Meta-Info") {
            continue;
        }

        int row = m_entryTable->rowCount();
        m_entryTable->insertRow(row);

        // Group name
        QString groupName;
        PW_GROUP* group = m_pwManager->getGroupById(entry->uGroupId);
        if (group != nullptr && group->pszGroupName != nullptr) {
            groupName = QString::fromUtf8(group->pszGroupName);
        }

        // Entry fields
        QString title = entry->pszTitle != nullptr ? QString::fromUtf8(entry->pszTitle) : QString();
        QString username = entry->pszUserName != nullptr ? QString::fromUtf8(entry->pszUserName) : QString();
        QString url = entry->pszURL != nullptr ? QString::fromUtf8(entry->pszURL) : QString();
        QString notes = entry->pszAdditional != nullptr ? QString::fromUtf8(entry->pszAdditional) : QString();

        // UUID as hex string
        QString uuid;
        for (int j = 0; j < 16; ++j) {
            uuid += QString("%1").arg(entry->uuid[j], 2, 16, QChar('0'));
        }

        // Truncate notes for display
        if (notes.length() > 50) {
            notes = notes.left(47) + "...";
        }
        notes.replace('\n', ' ');

        // Create items
        QTableWidgetItem *groupItem = new QTableWidgetItem(groupName);
        QTableWidgetItem *titleItem = new QTableWidgetItem(title);
        QTableWidgetItem *usernameItem = new QTableWidgetItem(username);
        QTableWidgetItem *urlItem = new QTableWidgetItem(url);
        QTableWidgetItem *notesItem = new QTableWidgetItem(notes);
        QTableWidgetItem *uuidItem = new QTableWidgetItem(uuid);

        // Set icon on title
        titleItem->setIcon(IconManager::instance().getEntryIcon(entry->uImageId));

        // Store entry index
        titleItem->setData(Qt::UserRole, i);

        m_entryTable->setItem(row, ColGroup, groupItem);
        m_entryTable->setItem(row, ColTitle, titleItem);
        m_entryTable->setItem(row, ColUsername, usernameItem);
        m_entryTable->setItem(row, ColUrl, urlItem);
        m_entryTable->setItem(row, ColNotes, notesItem);
        m_entryTable->setItem(row, ColUuid, uuidItem);
    }

    // Resize columns to content
    m_entryTable->resizeColumnsToContents();
}

void FieldRefDialog::setDefaultRef(DefaultRef ref)
{
    switch (ref) {
        case DefaultRef::Title:
            m_radioRefTitle->setChecked(true);
            break;
        case DefaultRef::Username:
            m_radioRefUsername->setChecked(true);
            break;
        case DefaultRef::Password:
            m_radioRefPassword->setChecked(true);
            break;
        case DefaultRef::URL:
            m_radioRefUrl->setChecked(true);
            break;
        case DefaultRef::Notes:
            m_radioRefNotes->setChecked(true);
            break;
    }
}

void FieldRefDialog::onEntrySelectionChanged()
{
    updateControls();
}

void FieldRefDialog::updateControls()
{
    bool hasSelection = m_entryTable->currentRow() >= 0;
    m_okButton->setEnabled(hasSelection);

    if (hasSelection) {
        m_statusLabel->clear();
    }
}

PW_ENTRY* FieldRefDialog::getSelectedEntry()
{
    int row = m_entryTable->currentRow();
    if (row < 0 || m_pwManager == nullptr) {
        return nullptr;
    }

    QTableWidgetItem* titleItem = m_entryTable->item(row, ColTitle);
    if (titleItem == nullptr) {
        return nullptr;
    }

    quint32 entryIndex = titleItem->data(Qt::UserRole).toUInt();
    return m_pwManager->getEntry(entryIndex);
}

bool FieldRefDialog::idMatchesMultipleTimes(const QString& value, QChar searchType)
{
    if (m_pwManager == nullptr || value.isEmpty()) {
        return false;
    }

    int matchCount = 0;
    quint32 entryCount = m_pwManager->getNumberOfEntries();

    for (quint32 i = 0; i < entryCount && matchCount < 2; ++i) {
        PW_ENTRY* entry = m_pwManager->getEntry(i);
        if (entry == nullptr) {
            continue;
        }

        QString fieldValue;
        switch (searchType.toLatin1()) {
            case 'T':
                if (entry->pszTitle != nullptr) {
                    fieldValue = QString::fromUtf8(entry->pszTitle);
                }
                break;
            case 'U':
                if (entry->pszUserName != nullptr) {
                    fieldValue = QString::fromUtf8(entry->pszUserName);
                }
                break;
            case 'P':
                if (entry->pszPassword != nullptr) {
                    m_pwManager->unlockEntryPassword(entry);
                    fieldValue = QString::fromUtf8(entry->pszPassword);
                    m_pwManager->lockEntryPassword(entry);
                }
                break;
            case 'A':
                if (entry->pszURL != nullptr) {
                    fieldValue = QString::fromUtf8(entry->pszURL);
                }
                break;
            case 'N':
                if (entry->pszAdditional != nullptr) {
                    fieldValue = QString::fromUtf8(entry->pszAdditional);
                }
                break;
        }

        if (fieldValue.compare(value, Qt::CaseInsensitive) == 0) {
            ++matchCount;
        }
    }

    return matchCount > 1;
}

void FieldRefDialog::validateAndAccept()
{
    PW_ENTRY* entry = getSelectedEntry();
    if (entry == nullptr) {
        QMessageBox::warning(this, tr("Field Reference"),
            tr("Please select an entry."));
        return;
    }

    // Determine field to reference
    QChar refField;
    if (m_radioRefTitle->isChecked()) {
        refField = 'T';
    } else if (m_radioRefUsername->isChecked()) {
        refField = 'U';
    } else if (m_radioRefPassword->isChecked()) {
        refField = 'P';
    } else if (m_radioRefUrl->isChecked()) {
        refField = 'A';
    } else if (m_radioRefNotes->isChecked()) {
        refField = 'N';
    } else {
        refField = 'U';  // Default
    }

    // Determine identification field
    QChar idField;
    QString idValue;

    if (m_radioIdTitle->isChecked()) {
        idField = 'T';
        idValue = entry->pszTitle != nullptr ? QString::fromUtf8(entry->pszTitle) : QString();
    } else if (m_radioIdUsername->isChecked()) {
        idField = 'U';
        idValue = entry->pszUserName != nullptr ? QString::fromUtf8(entry->pszUserName) : QString();
    } else if (m_radioIdPassword->isChecked()) {
        idField = 'P';
        if (entry->pszPassword != nullptr) {
            m_pwManager->unlockEntryPassword(entry);
            idValue = QString::fromUtf8(entry->pszPassword);
            m_pwManager->lockEntryPassword(entry);
        }
    } else if (m_radioIdUrl->isChecked()) {
        idField = 'A';
        idValue = entry->pszURL != nullptr ? QString::fromUtf8(entry->pszURL) : QString();
    } else if (m_radioIdNotes->isChecked()) {
        idField = 'N';
        idValue = entry->pszAdditional != nullptr ? QString::fromUtf8(entry->pszAdditional) : QString();
    } else {
        // UUID - always unique
        idField = 'I';
        for (int i = 0; i < 16; ++i) {
            idValue += QString("%1").arg(entry->uuid[i], 2, 16, QChar('0'));
        }
    }

    // Validate ID value
    if (idValue.isEmpty()) {
        QMessageBox::warning(this, tr("Field Reference"),
            tr("The selected identification field is empty for this entry.\n"
               "Please choose a different identification method."));
        return;
    }

    // Check for illegal characters
    if (idValue.contains('{') || idValue.contains('}') || idValue.contains('\n')) {
        QMessageBox::warning(this, tr("Field Reference"),
            tr("The identification value contains illegal characters ({, }, or newline).\n"
               "Please choose a different identification method (UUID is recommended)."));
        return;
    }

    // Warn if ID is not unique (except for UUID which is always unique)
    if (idField != 'I' && idMatchesMultipleTimes(idValue, idField)) {
        int result = QMessageBox::warning(this, tr("Field Reference"),
            tr("Warning: The selected identification value matches multiple entries.\n"
               "The reference may not return the expected result.\n\n"
               "Do you want to continue anyway?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

        if (result != QMessageBox::Yes) {
            return;
        }
    }

    // Build the field reference string
    m_fieldReference = QString("{REF:%1@%2:%3}").arg(refField).arg(idField).arg(idValue);

    accept();
}

void FieldRefDialog::onHelp()
{
    QMessageBox::information(this, tr("Field Reference Help"),
        tr("<b>Field References</b><br><br>"
           "A field reference allows you to use values from other entries in your database.<br><br>"
           "<b>Syntax:</b> {REF:&lt;Field&gt;@&lt;SearchType&gt;:&lt;SearchValue&gt;}<br><br>"
           "<b>Field codes:</b><br>"
           "&nbsp;&nbsp;T = Title<br>"
           "&nbsp;&nbsp;U = User Name<br>"
           "&nbsp;&nbsp;P = Password<br>"
           "&nbsp;&nbsp;A = URL<br>"
           "&nbsp;&nbsp;N = Notes<br><br>"
           "<b>Search types:</b><br>"
           "&nbsp;&nbsp;T, U, P, A, N = Search by field value<br>"
           "&nbsp;&nbsp;I = Search by UUID (most reliable)<br><br>"
           "<b>Example:</b><br>"
           "{REF:U@T:Gmail} - Gets username from entry titled 'Gmail'"));
}
