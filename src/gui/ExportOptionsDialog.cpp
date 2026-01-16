/*
  Qt KeePass - Export Options Dialog Implementation
*/

#include "ExportOptionsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>

ExportOptionsDialog::ExportOptionsDialog(quint32 exportFormat, QWidget *parent)
    : QDialog(parent),
      m_exportFormat(exportFormat)
{
    setupUi();
    loadDefaultsForFormat();
}

void ExportOptionsDialog::setupUi()
{
    setWindowTitle(tr("Export Options"));
    resize(400, 500);

    auto *layout = new QVBoxLayout(this);

    // Instruction label
    auto *label = new QLabel(tr("Select fields to include in export:"), this);
    layout->addWidget(label);

    // Field checkboxes in a group box
    auto *fieldsGroup = new QGroupBox(tr("Fields"), this);
    auto *fieldsLayout = new QVBoxLayout(fieldsGroup);

    // Basic fields
    m_cbGroup = new QCheckBox(tr("Group"), this);
    fieldsLayout->addWidget(m_cbGroup);

    m_cbGroupTree = new QCheckBox(tr("Group Tree (full path)"), this);
    fieldsLayout->addWidget(m_cbGroupTree);

    m_cbTitle = new QCheckBox(tr("Title"), this);
    fieldsLayout->addWidget(m_cbTitle);

    m_cbUsername = new QCheckBox(tr("User Name"), this);
    fieldsLayout->addWidget(m_cbUsername);

    m_cbPassword = new QCheckBox(tr("Password"), this);
    fieldsLayout->addWidget(m_cbPassword);

    m_cbURL = new QCheckBox(tr("URL"), this);
    fieldsLayout->addWidget(m_cbURL);

    m_cbNotes = new QCheckBox(tr("Notes"), this);
    fieldsLayout->addWidget(m_cbNotes);

    // Advanced fields group
    auto *advancedGroup = new QGroupBox(tr("Advanced Fields"), this);
    auto *advancedLayout = new QVBoxLayout(advancedGroup);

    m_cbUUID = new QCheckBox(tr("UUID"), this);
    advancedLayout->addWidget(m_cbUUID);

    m_cbIcon = new QCheckBox(tr("Icon"), this);
    advancedLayout->addWidget(m_cbIcon);

    m_cbCreation = new QCheckBox(tr("Creation Time"), this);
    advancedLayout->addWidget(m_cbCreation);

    m_cbLastAccess = new QCheckBox(tr("Last Access Time"), this);
    advancedLayout->addWidget(m_cbLastAccess);

    m_cbLastMod = new QCheckBox(tr("Last Modification Time"), this);
    advancedLayout->addWidget(m_cbLastMod);

    m_cbExpire = new QCheckBox(tr("Expiration Time"), this);
    advancedLayout->addWidget(m_cbExpire);

    m_cbAttachDesc = new QCheckBox(tr("Attachment Description"), this);
    advancedLayout->addWidget(m_cbAttachDesc);

    m_cbAttachment = new QCheckBox(tr("Attachment"), this);
    advancedLayout->addWidget(m_cbAttachment);

    layout->addWidget(fieldsGroup);
    layout->addWidget(advancedGroup);

    // Warning label
    auto *warningLabel = new QLabel(
        tr("<b>Warning:</b> Exporting passwords stores them in plain text."),
        this
    );
    warningLabel->setWordWrap(true);
    layout->addWidget(warningLabel);

    layout->addStretch();

    // Buttons
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this
    );
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(m_buttonBox);
}

void ExportOptionsDialog::loadDefaultsForFormat()
{
    quint32 defaults = 0;

    // Load format-specific defaults
    switch (m_exportFormat) {
        case PWEXP_TXT:
            defaults = PwExportFlags::DEFAULT_TXT;
            break;
        case PWEXP_HTML:
            defaults = PwExportFlags::DEFAULT_HTML;
            break;
        case PWEXP_XML:
            defaults = PwExportFlags::DEFAULT_XML;
            break;
        default:
            // Default to common fields
            defaults = PwExportFlags::GROUP | PwExportFlags::TITLE |
                      PwExportFlags::USERNAME | PwExportFlags::PASSWORD |
                      PwExportFlags::URL | PwExportFlags::NOTES;
            break;
    }

    setSelectedFields(defaults);
}

quint32 ExportOptionsDialog::getSelectedFields() const
{
    quint32 fields = 0;

    if (m_cbGroup->isChecked()) {
        fields |= PwExportFlags::GROUP;
    }
    if (m_cbGroupTree->isChecked()) {
        fields |= PwExportFlags::GROUPTREE;
    }
    if (m_cbTitle->isChecked()) {
        fields |= PwExportFlags::TITLE;
    }
    if (m_cbUsername->isChecked()) {
        fields |= PwExportFlags::USERNAME;
    }
    if (m_cbPassword->isChecked()) {
        fields |= PwExportFlags::PASSWORD;
    }
    if (m_cbURL->isChecked()) {
        fields |= PwExportFlags::URL;
    }
    if (m_cbNotes->isChecked()) {
        fields |= PwExportFlags::NOTES;
    }
    if (m_cbUUID->isChecked()) {
        fields |= PwExportFlags::UUID;
    }
    if (m_cbIcon->isChecked()) {
        fields |= PwExportFlags::IMAGEID;
    }
    if (m_cbCreation->isChecked()) {
        fields |= PwExportFlags::CREATION;
    }
    if (m_cbLastAccess->isChecked()) {
        fields |= PwExportFlags::LASTACCESS;
    }
    if (m_cbLastMod->isChecked()) {
        fields |= PwExportFlags::LASTMOD;
    }
    if (m_cbExpire->isChecked()) {
        fields |= PwExportFlags::EXPIRE;
    }
    if (m_cbAttachDesc->isChecked()) {
        fields |= PwExportFlags::ATTACHDESC;
    }
    if (m_cbAttachment->isChecked()) {
        fields |= PwExportFlags::ATTACHMENT;
    }

    return fields;
}

void ExportOptionsDialog::setSelectedFields(quint32 fields)
{
    m_cbGroup->setChecked((fields & PwExportFlags::GROUP) != 0);
    m_cbGroupTree->setChecked((fields & PwExportFlags::GROUPTREE) != 0);
    m_cbTitle->setChecked((fields & PwExportFlags::TITLE) != 0);
    m_cbUsername->setChecked((fields & PwExportFlags::USERNAME) != 0);
    m_cbPassword->setChecked((fields & PwExportFlags::PASSWORD) != 0);
    m_cbURL->setChecked((fields & PwExportFlags::URL) != 0);
    m_cbNotes->setChecked((fields & PwExportFlags::NOTES) != 0);
    m_cbUUID->setChecked((fields & PwExportFlags::UUID) != 0);
    m_cbIcon->setChecked((fields & PwExportFlags::IMAGEID) != 0);
    m_cbCreation->setChecked((fields & PwExportFlags::CREATION) != 0);
    m_cbLastAccess->setChecked((fields & PwExportFlags::LASTACCESS) != 0);
    m_cbLastMod->setChecked((fields & PwExportFlags::LASTMOD) != 0);
    m_cbExpire->setChecked((fields & PwExportFlags::EXPIRE) != 0);
    m_cbAttachDesc->setChecked((fields & PwExportFlags::ATTACHDESC) != 0);
    m_cbAttachment->setChecked((fields & PwExportFlags::ATTACHMENT) != 0);
}
