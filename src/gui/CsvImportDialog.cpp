/*
  Qt KeePass - CSV Import Dialog Implementation
*/

#include "CsvImportDialog.h"
#include "../core/PwManager.h"
#include "../core/PwStructs.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>

CsvImportDialog::CsvImportDialog(PwManager *pwManager, QWidget *parent)
    : QDialog(parent)
    , m_pwManager(pwManager)
{
    setupUi();
    populateGroups();
}

CsvImportDialog::~CsvImportDialog()
{
}

void CsvImportDialog::setupUi()
{
    setWindowTitle(tr("CSV Import Options"));
    setModal(true);
    resize(450, 200);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Info label
    m_infoLabel = new QLabel(tr(
        "<b>CSV Import</b><br><br>"
        "This will import entries from a CSV file.<br>"
        "The CSV file should have the following columns (in order):<br>"
        "<b>Title, Username, Password, URL, Notes</b><br><br>"
        "Select the group where entries will be imported:"
    ), this);
    m_infoLabel->setWordWrap(true);
    mainLayout->addWidget(m_infoLabel);

    mainLayout->addSpacing(10);

    // Group selection
    QFormLayout *formLayout = new QFormLayout();
    m_comboGroup = new QComboBox(this);
    formLayout->addRow(tr("Import to group:"), m_comboGroup);
    mainLayout->addLayout(formLayout);

    mainLayout->addStretch();

    // Dialog buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_okButton = new QPushButton(tr("OK"), this);
    m_okButton->setDefault(true);
    m_okButton->setEnabled(false);  // Enabled when group is selected
    buttonLayout->addWidget(m_okButton);

    m_cancelButton = new QPushButton(tr("Cancel"), this);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    // Connections
    connect(m_comboGroup, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                m_okButton->setEnabled(index >= 0 && m_comboGroup->currentData().toUInt() != 0);
            });
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void CsvImportDialog::populateGroups()
{
    if (!m_pwManager) {
        return;
    }

    m_comboGroup->clear();

    quint32 numGroups = m_pwManager->getNumberOfGroups();
    for (quint32 i = 0; i < numGroups; ++i) {
        PW_GROUP *group = m_pwManager->getGroup(i);
        if (group && group->pszGroupName) {
            QString groupName = QString::fromUtf8(group->pszGroupName);

            // Indent based on tree level
            QString indent;
            for (int j = 0; j < group->usLevel; ++j) {
                indent += "  ";
            }

            m_comboGroup->addItem(indent + groupName, group->uGroupId);
        }
    }

    // Select first group by default
    if (m_comboGroup->count() > 0) {
        m_comboGroup->setCurrentIndex(0);
    }
}

CsvImportOptions CsvImportDialog::getImportOptions() const
{
    CsvImportOptions options;
    options.targetGroupId = m_comboGroup->currentData().toUInt();

    // Use default column mapping: Title, Username, Password, URL, Notes
    options.titleColumn = 0;
    options.usernameColumn = 1;
    options.passwordColumn = 2;
    options.urlColumn = 3;
    options.notesColumn = 4;

    return options;
}
