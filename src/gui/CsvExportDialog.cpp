/*
  Qt KeePass - CSV Export Dialog Implementation
*/

#include "CsvExportDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>

CsvExportDialog::CsvExportDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi();
}

CsvExportDialog::~CsvExportDialog()
{
}

void CsvExportDialog::setupUi()
{
    setWindowTitle(tr("CSV Export Options"));
    setModal(true);
    resize(400, 450);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Info label
    QLabel *infoLabel = new QLabel(tr("Select the fields to include in the CSV export:"), this);
    mainLayout->addWidget(infoLabel);

    mainLayout->addSpacing(10);

    // Field selection group
    QGroupBox *fieldsGroup = new QGroupBox(tr("Fields to Export"), this);
    QGridLayout *fieldsLayout = new QGridLayout(fieldsGroup);

    m_checkGroup = new QCheckBox(tr("Group"), fieldsGroup);
    m_checkGroup->setChecked(false);
    fieldsLayout->addWidget(m_checkGroup, 0, 0);

    m_checkTitle = new QCheckBox(tr("Title"), fieldsGroup);
    m_checkTitle->setChecked(true);
    fieldsLayout->addWidget(m_checkTitle, 0, 1);

    m_checkUsername = new QCheckBox(tr("Username"), fieldsGroup);
    m_checkUsername->setChecked(true);
    fieldsLayout->addWidget(m_checkUsername, 1, 0);

    m_checkPassword = new QCheckBox(tr("Password"), fieldsGroup);
    m_checkPassword->setChecked(true);
    fieldsLayout->addWidget(m_checkPassword, 1, 1);

    m_checkUrl = new QCheckBox(tr("URL"), fieldsGroup);
    m_checkUrl->setChecked(true);
    fieldsLayout->addWidget(m_checkUrl, 2, 0);

    m_checkNotes = new QCheckBox(tr("Notes"), fieldsGroup);
    m_checkNotes->setChecked(true);
    fieldsLayout->addWidget(m_checkNotes, 2, 1);

    m_checkUuid = new QCheckBox(tr("UUID"), fieldsGroup);
    m_checkUuid->setChecked(false);
    fieldsLayout->addWidget(m_checkUuid, 3, 0);

    m_checkCreationTime = new QCheckBox(tr("Creation Time"), fieldsGroup);
    m_checkCreationTime->setChecked(false);
    fieldsLayout->addWidget(m_checkCreationTime, 3, 1);

    m_checkLastModTime = new QCheckBox(tr("Last Modification"), fieldsGroup);
    m_checkLastModTime->setChecked(false);
    fieldsLayout->addWidget(m_checkLastModTime, 4, 0);

    m_checkLastAccessTime = new QCheckBox(tr("Last Access"), fieldsGroup);
    m_checkLastAccessTime->setChecked(false);
    fieldsLayout->addWidget(m_checkLastAccessTime, 4, 1);

    m_checkExpireTime = new QCheckBox(tr("Expiration Time"), fieldsGroup);
    m_checkExpireTime->setChecked(false);
    fieldsLayout->addWidget(m_checkExpireTime, 5, 0);

    mainLayout->addWidget(fieldsGroup);

    // Selection buttons
    QHBoxLayout *selectionLayout = new QHBoxLayout();
    m_btnSelectAll = new QPushButton(tr("Select All"), this);
    m_btnDeselectAll = new QPushButton(tr("Deselect All"), this);
    selectionLayout->addWidget(m_btnSelectAll);
    selectionLayout->addWidget(m_btnDeselectAll);
    selectionLayout->addStretch();
    mainLayout->addLayout(selectionLayout);

    mainLayout->addStretch();

    // Dialog buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_okButton = new QPushButton(tr("OK"), this);
    m_okButton->setDefault(true);
    buttonLayout->addWidget(m_okButton);

    m_cancelButton = new QPushButton(tr("Cancel"), this);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    // Connections
    connect(m_btnSelectAll, &QPushButton::clicked, this, &CsvExportDialog::onSelectAll);
    connect(m_btnDeselectAll, &QPushButton::clicked, this, &CsvExportDialog::onDeselectAll);
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

CsvExportOptions CsvExportDialog::getExportOptions() const
{
    CsvExportOptions options;
    options.includeGroup = m_checkGroup->isChecked();
    options.includeTitle = m_checkTitle->isChecked();
    options.includeUsername = m_checkUsername->isChecked();
    options.includePassword = m_checkPassword->isChecked();
    options.includeUrl = m_checkUrl->isChecked();
    options.includeNotes = m_checkNotes->isChecked();
    options.includeUuid = m_checkUuid->isChecked();
    options.includeCreationTime = m_checkCreationTime->isChecked();
    options.includeLastModTime = m_checkLastModTime->isChecked();
    options.includeLastAccessTime = m_checkLastAccessTime->isChecked();
    options.includeExpireTime = m_checkExpireTime->isChecked();
    return options;
}

void CsvExportDialog::onSelectAll()
{
    m_checkGroup->setChecked(true);
    m_checkTitle->setChecked(true);
    m_checkUsername->setChecked(true);
    m_checkPassword->setChecked(true);
    m_checkUrl->setChecked(true);
    m_checkNotes->setChecked(true);
    m_checkUuid->setChecked(true);
    m_checkCreationTime->setChecked(true);
    m_checkLastModTime->setChecked(true);
    m_checkLastAccessTime->setChecked(true);
    m_checkExpireTime->setChecked(true);
}

void CsvExportDialog::onDeselectAll()
{
    m_checkGroup->setChecked(false);
    m_checkTitle->setChecked(false);
    m_checkUsername->setChecked(false);
    m_checkPassword->setChecked(false);
    m_checkUrl->setChecked(false);
    m_checkNotes->setChecked(false);
    m_checkUuid->setChecked(false);
    m_checkCreationTime->setChecked(false);
    m_checkLastModTime->setChecked(false);
    m_checkLastAccessTime->setChecked(false);
    m_checkExpireTime->setChecked(false);
}
