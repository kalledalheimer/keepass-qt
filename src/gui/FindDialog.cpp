/*
  Qt KeePass - Find Dialog Implementation
  Reference: MFC/MFC-KeePass/WinGUI/FindInDbDlg.cpp
*/

#include "FindDialog.h"
#include "../core/PwManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

FindDialog::FindDialog(PwManager* pwManager, QWidget* parent)
    : QDialog(parent)
    , m_pwManager(pwManager)
{
    setupUI();
    setWindowTitle(tr("Find"));
}

void FindDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Search string input
    QLabel* searchLabel = new QLabel(tr("Find:"), this);
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Enter search text..."));

    mainLayout->addWidget(searchLabel);
    mainLayout->addWidget(m_searchEdit);

    // Search in fields group
    QGroupBox* fieldsGroup = new QGroupBox(tr("Search in:"), this);
    QVBoxLayout* fieldsLayout = new QVBoxLayout(fieldsGroup);

    m_titleCheck = new QCheckBox(tr("Title"), this);
    m_usernameCheck = new QCheckBox(tr("Username"), this);
    m_urlCheck = new QCheckBox(tr("URL"), this);
    m_passwordCheck = new QCheckBox(tr("Password"), this);
    m_notesCheck = new QCheckBox(tr("Notes"), this);
    m_uuidCheck = new QCheckBox(tr("UUID"), this);
    m_groupNameCheck = new QCheckBox(tr("Group Name"), this);

    // Set defaults: search in title, username, URL, notes (matching MFC defaults)
    m_titleCheck->setChecked(true);
    m_usernameCheck->setChecked(true);
    m_urlCheck->setChecked(true);
    m_notesCheck->setChecked(true);
    m_passwordCheck->setChecked(false);
    m_uuidCheck->setChecked(false);
    m_groupNameCheck->setChecked(false);

    fieldsLayout->addWidget(m_titleCheck);
    fieldsLayout->addWidget(m_usernameCheck);
    fieldsLayout->addWidget(m_urlCheck);
    fieldsLayout->addWidget(m_passwordCheck);
    fieldsLayout->addWidget(m_notesCheck);
    fieldsLayout->addWidget(m_uuidCheck);
    fieldsLayout->addWidget(m_groupNameCheck);

    mainLayout->addWidget(fieldsGroup);

    // Options group
    QGroupBox* optionsGroup = new QGroupBox(tr("Options:"), this);
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);

    m_caseSensitiveCheck = new QCheckBox(tr("Case sensitive"), this);
    m_regexCheck = new QCheckBox(tr("Regular expression"), this);
    m_excludeBackupsCheck = new QCheckBox(tr("Exclude backup entries"), this);
    m_excludeExpiredCheck = new QCheckBox(tr("Exclude expired entries"), this);

    // Set defaults (matching MFC defaults)
    m_caseSensitiveCheck->setChecked(false);
    m_regexCheck->setChecked(false);
    m_excludeBackupsCheck->setChecked(true);
    m_excludeExpiredCheck->setChecked(false);

    optionsLayout->addWidget(m_caseSensitiveCheck);
    optionsLayout->addWidget(m_regexCheck);
    optionsLayout->addWidget(m_excludeBackupsCheck);
    optionsLayout->addWidget(m_excludeExpiredCheck);

    mainLayout->addWidget(optionsGroup);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_okButton = new QPushButton(tr("Find"), this);
    m_cancelButton = new QPushButton(tr("Cancel"), this);

    m_okButton->setDefault(true);

    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    // Connect signals
    connect(m_okButton, &QPushButton::clicked, this, &FindDialog::onOK);
    connect(m_cancelButton, &QPushButton::clicked, this, &FindDialog::onCancel);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &FindDialog::onOK);

    // Set minimum width
    setMinimumWidth(350);
}

QString FindDialog::searchString() const
{
    return m_searchEdit->text();
}

quint32 FindDialog::searchFlags() const
{
    quint32 flags = 0;

    if (m_titleCheck->isChecked())
        flags |= PWMF_TITLE;
    if (m_usernameCheck->isChecked())
        flags |= PWMF_USER;
    if (m_urlCheck->isChecked())
        flags |= PWMF_URL;
    if (m_passwordCheck->isChecked())
        flags |= PWMF_PASSWORD;
    if (m_notesCheck->isChecked())
        flags |= PWMF_ADDITIONAL;
    if (m_uuidCheck->isChecked())
        flags |= PWMF_UUID;
    if (m_groupNameCheck->isChecked())
        flags |= PWMF_GROUPNAME;

    if (m_regexCheck->isChecked())
        flags |= PWMS_REGEX;

    return flags;
}

bool FindDialog::isCaseSensitive() const
{
    return m_caseSensitiveCheck->isChecked();
}

bool FindDialog::isRegexEnabled() const
{
    return m_regexCheck->isChecked();
}

bool FindDialog::excludeBackups() const
{
    return m_excludeBackupsCheck->isChecked();
}

bool FindDialog::excludeExpired() const
{
    return m_excludeExpiredCheck->isChecked();
}

void FindDialog::onOK()
{
    // Validate search string
    if (m_searchEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Find"),
                           tr("Please enter a search string."));
        m_searchEdit->setFocus();
        return;
    }

    // Validate at least one field is selected
    quint32 flags = searchFlags();
    if ((flags & (PWMF_TITLE | PWMF_USER | PWMF_URL | PWMF_PASSWORD |
                  PWMF_ADDITIONAL | PWMF_UUID | PWMF_GROUPNAME)) == 0) {
        QMessageBox::warning(this, tr("Find"),
                           tr("Please select at least one field to search."));
        return;
    }

    accept();
}

void FindDialog::onCancel()
{
    reject();
}
