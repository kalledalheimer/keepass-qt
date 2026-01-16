/*
  Qt KeePass - Mass Modify Dialog Implementation

  Reference: MFC WinGUI/EntryPropertiesDlg.cpp
*/

#include "MassModifyDialog.h"
#include "IconManager.h"
#include "IconPickerDialog.h"
#include "../core/PwManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QDateTime>

MassModifyDialog::MassModifyDialog(PwManager *pwManager, const QList<quint32>& entryIndices,
                                   QWidget *parent)
    : QDialog(parent)
    , m_pwManager(pwManager)
    , m_entryIndices(entryIndices)
    , m_modifyGroup(false)
    , m_modifyIcon(false)
    , m_modifyExpiration(false)
    , m_deleteAttachments(false)
    , m_newGroupId(0)
    , m_newIconId(0)
    , m_hasExpiration(false)
{
    setWindowTitle(tr("Mass Modify Entries"));
    setMinimumWidth(450);

    // Initialize expiration time to "never"
    PwManager::getNeverExpireTime(&m_expirationTime);

    setupUi();
}

void MassModifyDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Info label
    QLabel *infoLabel = new QLabel(
        tr("Modify properties of %1 selected entries.\n"
           "Check the options you want to change.").arg(m_entryIndices.size()));
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);

    mainLayout->addSpacing(10);

    // Group modification
    QGroupBox *groupBox = new QGroupBox(tr("Group"));
    QHBoxLayout *groupLayout = new QHBoxLayout(groupBox);

    m_checkModifyGroup = new QCheckBox(tr("Move to group:"));
    m_checkModifyGroup->setChecked(false);
    groupLayout->addWidget(m_checkModifyGroup);

    m_groupCombo = new QComboBox();
    m_groupCombo->setEnabled(false);
    populateGroupCombo();
    groupLayout->addWidget(m_groupCombo, 1);

    connect(m_checkModifyGroup, &QCheckBox::toggled, this, &MassModifyDialog::onModifyGroupToggled);
    mainLayout->addWidget(groupBox);

    // Icon modification
    QGroupBox *iconBox = new QGroupBox(tr("Icon"));
    QHBoxLayout *iconLayout = new QHBoxLayout(iconBox);

    m_checkModifyIcon = new QCheckBox(tr("Change icon:"));
    m_checkModifyIcon->setChecked(false);
    iconLayout->addWidget(m_checkModifyIcon);

    m_iconLabel = new QLabel();
    m_iconLabel->setFixedSize(24, 24);
    m_iconLabel->setPixmap(IconManager::instance().getEntryIcon(0).pixmap(16, 16));
    m_iconLabel->setEnabled(false);
    iconLayout->addWidget(m_iconLabel);

    m_selectIconButton = new QPushButton(tr("Select..."));
    m_selectIconButton->setEnabled(false);
    iconLayout->addWidget(m_selectIconButton);
    iconLayout->addStretch();

    connect(m_checkModifyIcon, &QCheckBox::toggled, this, &MassModifyDialog::onModifyIconToggled);
    connect(m_selectIconButton, &QPushButton::clicked, this, &MassModifyDialog::onSelectIcon);
    mainLayout->addWidget(iconBox);

    // Expiration modification
    QGroupBox *expirationBox = new QGroupBox(tr("Expiration"));
    QVBoxLayout *expirationLayout = new QVBoxLayout(expirationBox);

    m_checkModifyExpiration = new QCheckBox(tr("Change expiration:"));
    m_checkModifyExpiration->setChecked(false);
    expirationLayout->addWidget(m_checkModifyExpiration);

    // Expiration options
    QWidget *expirationOptions = new QWidget();
    QVBoxLayout *optionsLayout = new QVBoxLayout(expirationOptions);
    optionsLayout->setContentsMargins(20, 0, 0, 0);

    // Radio buttons
    m_radioExpires = new QRadioButton(tr("Expires on:"));
    m_radioExpires->setChecked(true);
    m_radioExpires->setEnabled(false);
    optionsLayout->addWidget(m_radioExpires);

    // DateTime and preset buttons
    QHBoxLayout *dateTimeLayout = new QHBoxLayout();
    dateTimeLayout->setContentsMargins(20, 0, 0, 0);

    m_expirationDateTime = new QDateTimeEdit(QDateTime::currentDateTime());
    m_expirationDateTime->setCalendarPopup(true);
    m_expirationDateTime->setDisplayFormat("yyyy-MM-dd hh:mm:ss");
    m_expirationDateTime->setEnabled(false);
    dateTimeLayout->addWidget(m_expirationDateTime);
    dateTimeLayout->addStretch();
    optionsLayout->addLayout(dateTimeLayout);

    // Preset buttons
    QHBoxLayout *presetsLayout = new QHBoxLayout();
    presetsLayout->setContentsMargins(20, 0, 0, 0);

    m_btn1Week = new QPushButton(tr("1 Week"));
    m_btn2Weeks = new QPushButton(tr("2 Weeks"));
    m_btn1Month = new QPushButton(tr("1 Month"));
    m_btn3Months = new QPushButton(tr("3 Months"));
    m_btn6Months = new QPushButton(tr("6 Months"));
    m_btn12Months = new QPushButton(tr("1 Year"));
    m_btnNow = new QPushButton(tr("Now"));

    m_btn1Week->setEnabled(false);
    m_btn2Weeks->setEnabled(false);
    m_btn1Month->setEnabled(false);
    m_btn3Months->setEnabled(false);
    m_btn6Months->setEnabled(false);
    m_btn12Months->setEnabled(false);
    m_btnNow->setEnabled(false);

    presetsLayout->addWidget(m_btn1Week);
    presetsLayout->addWidget(m_btn2Weeks);
    presetsLayout->addWidget(m_btn1Month);
    presetsLayout->addWidget(m_btn3Months);
    presetsLayout->addWidget(m_btn6Months);
    presetsLayout->addWidget(m_btn12Months);
    presetsLayout->addWidget(m_btnNow);
    presetsLayout->addStretch();
    optionsLayout->addLayout(presetsLayout);

    // No expiration radio
    m_radioNoExpiration = new QRadioButton(tr("Never expires"));
    m_radioNoExpiration->setEnabled(false);
    optionsLayout->addWidget(m_radioNoExpiration);

    expirationLayout->addWidget(expirationOptions);

    connect(m_checkModifyExpiration, &QCheckBox::toggled, this, &MassModifyDialog::onModifyExpirationToggled);
    connect(m_radioNoExpiration, &QRadioButton::toggled, this, &MassModifyDialog::onNoExpiration);
    connect(m_btn1Week, &QPushButton::clicked, this, &MassModifyDialog::onExpires1Week);
    connect(m_btn2Weeks, &QPushButton::clicked, this, &MassModifyDialog::onExpires2Weeks);
    connect(m_btn1Month, &QPushButton::clicked, this, &MassModifyDialog::onExpires1Month);
    connect(m_btn3Months, &QPushButton::clicked, this, &MassModifyDialog::onExpires3Months);
    connect(m_btn6Months, &QPushButton::clicked, this, &MassModifyDialog::onExpires6Months);
    connect(m_btn12Months, &QPushButton::clicked, this, &MassModifyDialog::onExpires12Months);
    connect(m_btnNow, &QPushButton::clicked, this, &MassModifyDialog::onExpiresNow);

    mainLayout->addWidget(expirationBox);

    // Attachment deletion
    QGroupBox *attachBox = new QGroupBox(tr("Attachments"));
    QHBoxLayout *attachLayout = new QHBoxLayout(attachBox);

    m_checkDeleteAttachments = new QCheckBox(tr("Delete all attachments"));
    attachLayout->addWidget(m_checkDeleteAttachments);
    attachLayout->addStretch();

    mainLayout->addWidget(attachBox);

    mainLayout->addStretch();

    // Buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_okButton = buttonBox->button(QDialogButtonBox::Ok);
    m_cancelButton = buttonBox->button(QDialogButtonBox::Cancel);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &MassModifyDialog::validateAndAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);
}

void MassModifyDialog::populateGroupCombo()
{
    m_groupCombo->clear();

    if (m_pwManager == nullptr) {
        return;
    }

    quint32 groupCount = m_pwManager->getNumberOfGroups();
    for (quint32 i = 0; i < groupCount; ++i) {
        PW_GROUP* group = m_pwManager->getGroup(i);
        if (group != nullptr && group->pszGroupName != nullptr) {
            QString name = QString::fromUtf8(group->pszGroupName);

            // Add indentation based on level
            QString displayName;
            for (quint16 j = 0; j < group->usLevel; ++j) {
                displayName += "    ";
            }
            displayName += name;

            QIcon icon = IconManager::instance().getEntryIcon(group->uImageId);
            m_groupCombo->addItem(icon, displayName, group->uGroupId);
        }
    }

    if (m_groupCombo->count() > 0) {
        m_groupCombo->setCurrentIndex(0);
        m_newGroupId = m_groupCombo->itemData(0).toUInt();
    }
}

void MassModifyDialog::onModifyGroupToggled(bool checked)
{
    m_groupCombo->setEnabled(checked);
}

void MassModifyDialog::onModifyIconToggled(bool checked)
{
    m_iconLabel->setEnabled(checked);
    m_selectIconButton->setEnabled(checked);
}

void MassModifyDialog::onModifyExpirationToggled(bool /*checked*/)
{
    updateExpirationControls();
}

void MassModifyDialog::updateExpirationControls()
{
    bool enabled = m_checkModifyExpiration->isChecked();

    m_radioExpires->setEnabled(enabled);
    m_radioNoExpiration->setEnabled(enabled);

    bool expiresEnabled = enabled && m_radioExpires->isChecked();
    m_expirationDateTime->setEnabled(expiresEnabled);
    m_btn1Week->setEnabled(expiresEnabled);
    m_btn2Weeks->setEnabled(expiresEnabled);
    m_btn1Month->setEnabled(expiresEnabled);
    m_btn3Months->setEnabled(expiresEnabled);
    m_btn6Months->setEnabled(expiresEnabled);
    m_btn12Months->setEnabled(expiresEnabled);
    m_btnNow->setEnabled(expiresEnabled);
}

void MassModifyDialog::onSelectIcon()
{
    IconPickerDialog dialog(static_cast<int>(m_newIconId), this);
    if (dialog.exec() == QDialog::Accepted) {
        m_newIconId = static_cast<quint32>(dialog.selectedIcon());
        m_iconLabel->setPixmap(IconManager::instance().getEntryIcon(m_newIconId).pixmap(16, 16));
    }
}

void MassModifyDialog::setExpireDays(int days)
{
    QDateTime expireDateTime = QDateTime::currentDateTime().addDays(days);
    m_expirationDateTime->setDateTime(expireDateTime);
    m_radioExpires->setChecked(true);
    updateExpirationControls();
}

void MassModifyDialog::onExpires1Week()
{
    setExpireDays(7);
}

void MassModifyDialog::onExpires2Weeks()
{
    setExpireDays(14);
}

void MassModifyDialog::onExpires1Month()
{
    setExpireDays(30);
}

void MassModifyDialog::onExpires3Months()
{
    setExpireDays(90);
}

void MassModifyDialog::onExpires6Months()
{
    setExpireDays(180);
}

void MassModifyDialog::onExpires12Months()
{
    setExpireDays(365);
}

void MassModifyDialog::onExpiresNow()
{
    m_expirationDateTime->setDateTime(QDateTime::currentDateTime());
    m_radioExpires->setChecked(true);
    updateExpirationControls();
}

void MassModifyDialog::onNoExpiration(bool /*checked*/)
{
    updateExpirationControls();
}

void MassModifyDialog::validateAndAccept()
{
    // Gather results
    m_modifyGroup = m_checkModifyGroup->isChecked();
    m_modifyIcon = m_checkModifyIcon->isChecked();
    m_modifyExpiration = m_checkModifyExpiration->isChecked();
    m_deleteAttachments = m_checkDeleteAttachments->isChecked();

    // Check if at least one option is selected
    if (!m_modifyGroup && !m_modifyIcon && !m_modifyExpiration && !m_deleteAttachments) {
        QMessageBox::information(this, tr("Mass Modify"),
            tr("Please select at least one option to modify."));
        return;
    }

    // Get group ID
    if (m_modifyGroup && m_groupCombo->currentIndex() >= 0) {
        m_newGroupId = m_groupCombo->currentData().toUInt();
    }

    // Get expiration
    if (m_modifyExpiration) {
        m_hasExpiration = m_radioExpires->isChecked();

        if (m_hasExpiration) {
            // Convert QDateTime to PW_TIME
            QDateTime dt = m_expirationDateTime->dateTime();
            m_expirationTime.shYear = static_cast<quint16>(dt.date().year());
            m_expirationTime.btMonth = static_cast<quint8>(dt.date().month());
            m_expirationTime.btDay = static_cast<quint8>(dt.date().day());
            m_expirationTime.btHour = static_cast<quint8>(dt.time().hour());
            m_expirationTime.btMinute = static_cast<quint8>(dt.time().minute());
            m_expirationTime.btSecond = static_cast<quint8>(dt.time().second());
        } else {
            // Never expires
            PwManager::getNeverExpireTime(&m_expirationTime);
        }
    }

    accept();
}
