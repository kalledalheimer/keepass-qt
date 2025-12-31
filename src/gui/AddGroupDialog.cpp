/*
  Qt KeePass - Add Group Dialog Implementation
*/

#include "AddGroupDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QDialogButtonBox>

// Reserved group names that cannot be used
// From MFC: PWS_SEARCHGROUP, PWS_BACKUPGROUP
const QStringList AddGroupDialog::ReservedNames = {
    "Search Results",  // PWS_SEARCHGROUP - used for search results
    "Backup"          // PWS_BACKUPGROUP - used for backup entries
};

AddGroupDialog::AddGroupDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi();

    // Connect signals
    connect(m_groupNameEdit, &QLineEdit::textChanged,
            this, &AddGroupDialog::onGroupNameChanged);
    connect(m_okButton, &QPushButton::clicked,
            this, &AddGroupDialog::validateAndAccept);
    connect(m_cancelButton, &QPushButton::clicked,
            this, &QDialog::reject);

    // Initial state
    m_okButton->setEnabled(false);
}

void AddGroupDialog::setupUi()
{
    setWindowTitle(tr("Add Group"));
    setModal(true);
    setMinimumWidth(350);

    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Form layout for input fields
    QFormLayout *formLayout = new QFormLayout();

    // Group name field
    m_groupNameEdit = new QLineEdit(this);
    m_groupNameEdit->setPlaceholderText(tr("Enter group name"));
    formLayout->addRow(tr("&Group name:"), m_groupNameEdit);

    // Icon ID field
    m_iconIdSpin = new QSpinBox(this);
    m_iconIdSpin->setRange(0, 68);  // KeePass 1.x has 69 standard icons (0-68)
    m_iconIdSpin->setValue(48);     // PWM_STD_ICON_GROUP - default folder icon
    m_iconIdSpin->setToolTip(tr("Icon identifier (0-68). Default: 48 (folder)"));
    formLayout->addRow(tr("&Icon ID:"), m_iconIdSpin);

    mainLayout->addLayout(formLayout);

    // Add some spacing
    mainLayout->addSpacing(10);

    // Button box
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_okButton = new QPushButton(tr("OK"), this);
    m_okButton->setDefault(true);
    buttonLayout->addWidget(m_okButton);

    m_cancelButton = new QPushButton(tr("Cancel"), this);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    // Set focus to group name field
    m_groupNameEdit->setFocus();
}

QString AddGroupDialog::getGroupName() const
{
    return m_groupNameEdit->text().trimmed();
}

quint32 AddGroupDialog::getIconId() const
{
    return static_cast<quint32>(m_iconIdSpin->value());
}

void AddGroupDialog::onGroupNameChanged(const QString &text)
{
    // Enable OK button only if name is not empty
    m_okButton->setEnabled(!text.trimmed().isEmpty());
}

void AddGroupDialog::validateAndAccept()
{
    if (validateInput()) {
        accept();
    }
}

bool AddGroupDialog::validateInput()
{
    QString groupName = getGroupName();

    // Check for empty name
    if (groupName.isEmpty()) {
        QMessageBox::warning(this, tr("Add Group"),
                           tr("Enter a group name!"));
        m_groupNameEdit->setFocus();
        return false;
    }

    // Check for reserved names (case-insensitive)
    QString nameLower = groupName.toLower();
    for (const QString &reserved : ReservedNames) {
        if (nameLower == reserved.toLower()) {
            QMessageBox::warning(this, tr("Add Group"),
                               tr("The specified name is reserved.\n\n"
                                  "Please choose a different name."));
            m_groupNameEdit->selectAll();
            m_groupNameEdit->setFocus();
            return false;
        }
    }

    return true;
}
