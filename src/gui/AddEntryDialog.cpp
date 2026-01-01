/*
  Qt KeePass - Add Entry Dialog Implementation
*/

#include "AddEntryDialog.h"
#include "../core/PwManager.h"
#include "../core/util/PwUtil.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QDateTime>
#include <QRandomGenerator>

AddEntryDialog::AddEntryDialog(PwManager *pwManager, quint32 selectedGroupId, QWidget *parent)
    : QDialog(parent)
    , m_pwManager(pwManager)
    , m_selectedGroupId(selectedGroupId)
{
    setupUi();
    populateGroupCombo();

    // Set default values
    m_iconIdSpin->setValue(0);  // Default entry icon
    m_usernameEdit->setText("");  // TODO: Get default username from database property
    m_passwordEdit->setText(generateRandomPassword());
    m_repeatPasswordEdit->setText(m_passwordEdit->text());

    // Connect signals
    connect(m_passwordEdit, &QLineEdit::textChanged, this, &AddEntryDialog::onPasswordChanged);
    connect(m_repeatPasswordEdit, &QLineEdit::textChanged, this, &AddEntryDialog::onPasswordChanged);
    connect(m_showPasswordCheck, &QCheckBox::toggled, this, &AddEntryDialog::onShowPasswordToggled);
    connect(m_expiresCheck, &QCheckBox::toggled, this, &AddEntryDialog::onExpiresToggled);
    connect(m_okButton, &QPushButton::clicked, this, &AddEntryDialog::validateAndAccept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    // Initial state
    m_expirationDateTime->setEnabled(false);
    onPasswordChanged();  // Update validation state
}

void AddEntryDialog::setupUi()
{
    setWindowTitle(tr("Add Entry"));
    setModal(true);
    setMinimumWidth(500);

    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Form layout for input fields
    QFormLayout *formLayout = new QFormLayout();

    // Group selection
    m_groupCombo = new QComboBox(this);
    formLayout->addRow(tr("&Group:"), m_groupCombo);

    // Icon ID
    m_iconIdSpin = new QSpinBox(this);
    m_iconIdSpin->setRange(0, 68);  // KeePass 1.x has 69 icons (0-68)
    m_iconIdSpin->setValue(0);      // Default entry icon
    m_iconIdSpin->setToolTip(tr("Icon identifier (0-68). Default: 0 (key icon)"));
    formLayout->addRow(tr("&Icon ID:"), m_iconIdSpin);

    // Title
    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setPlaceholderText(tr("Enter title"));
    formLayout->addRow(tr("&Title:"), m_titleEdit);

    // Username
    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText(tr("Enter username"));
    formLayout->addRow(tr("&User name:"), m_usernameEdit);

    // Password group
    QGroupBox *passwordGroup = new QGroupBox(tr("Password"), this);
    QVBoxLayout *passwordLayout = new QVBoxLayout(passwordGroup);

    QFormLayout *passwordFormLayout = new QFormLayout();

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    passwordFormLayout->addRow(tr("&Password:"), m_passwordEdit);

    m_repeatPasswordEdit = new QLineEdit(this);
    m_repeatPasswordEdit->setEchoMode(QLineEdit::Password);
    passwordFormLayout->addRow(tr("&Repeat:"), m_repeatPasswordEdit);

    passwordLayout->addLayout(passwordFormLayout);

    m_showPasswordCheck = new QCheckBox(tr("Show password"), this);
    passwordLayout->addWidget(m_showPasswordCheck);

    formLayout->addRow(passwordGroup);

    // URL
    m_urlEdit = new QLineEdit(this);
    m_urlEdit->setPlaceholderText(tr("Enter URL"));
    formLayout->addRow(tr("&URL:"), m_urlEdit);

    // Notes
    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setPlaceholderText(tr("Enter notes"));
    m_notesEdit->setMaximumHeight(100);
    formLayout->addRow(tr("&Notes:"), m_notesEdit);

    // Expiration group
    QGroupBox *expirationGroup = new QGroupBox(tr("Expiration"), this);
    QVBoxLayout *expirationLayout = new QVBoxLayout(expirationGroup);

    m_expiresCheck = new QCheckBox(tr("Entry expires"), this);
    expirationLayout->addWidget(m_expiresCheck);

    m_expirationDateTime = new QDateTimeEdit(this);
    m_expirationDateTime->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    m_expirationDateTime->setCalendarPopup(true);

    // Set default expiration to 1 year from now at 23:59:59
    QDateTime defaultExpiration = QDateTime::currentDateTime().addYears(1);
    defaultExpiration.setTime(QTime(23, 59, 59));
    m_expirationDateTime->setDateTime(defaultExpiration);

    expirationLayout->addWidget(m_expirationDateTime);

    formLayout->addRow(expirationGroup);

    mainLayout->addLayout(formLayout);

    // Button box
    mainLayout->addSpacing(10);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_okButton = new QPushButton(tr("OK"), this);
    m_okButton->setDefault(true);
    buttonLayout->addWidget(m_okButton);

    m_cancelButton = new QPushButton(tr("Cancel"), this);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    // Set focus to title field
    m_titleEdit->setFocus();
}

void AddEntryDialog::populateGroupCombo()
{
    if (!m_pwManager) {
        return;
    }

    m_groupCombo->clear();

    quint32 numGroups = m_pwManager->getNumberOfGroups();
    int selectedIndex = -1;

    for (quint32 i = 0; i < numGroups; ++i) {
        PW_GROUP *group = m_pwManager->getGroup(i);
        if (group) {
            QString groupName = QString::fromUtf8(group->pszGroupName);

            // Skip reserved "Search Results" group (cannot store entries)
            if (groupName.toLower() == "search results") {
                continue;
            }

            // Add group with its ID as user data
            m_groupCombo->addItem(groupName, group->uGroupId);

            // Select this group if it matches the selectedGroupId
            if (group->uGroupId == m_selectedGroupId) {
                selectedIndex = m_groupCombo->count() - 1;
            }
        }
    }

    // Set selection
    if (selectedIndex >= 0) {
        m_groupCombo->setCurrentIndex(selectedIndex);
    } else if (m_groupCombo->count() > 0) {
        m_groupCombo->setCurrentIndex(0);  // Select first valid group
    }
}

QString AddEntryDialog::generateRandomPassword()
{
    // Simple random password generation (16 characters)
    // TODO: Use proper password generator matching MFC version
    const QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
    QString password;

    for (int i = 0; i < 16; ++i) {
        int index = QRandomGenerator::global()->bounded(chars.length());
        password.append(chars.at(index));
    }

    return password;
}

void AddEntryDialog::onPasswordChanged()
{
    QString password = m_passwordEdit->text();
    QString repeatPassword = m_repeatPasswordEdit->text();

    // Enable OK button only if passwords match (when repeat field has text)
    bool passwordsMatch = repeatPassword.isEmpty() || (password == repeatPassword);
    m_okButton->setEnabled(passwordsMatch);

    // TODO: Update password quality meter
}

void AddEntryDialog::onShowPasswordToggled(bool checked)
{
    QLineEdit::EchoMode mode = checked ? QLineEdit::Normal : QLineEdit::Password;
    m_passwordEdit->setEchoMode(mode);
    m_repeatPasswordEdit->setEchoMode(mode);
}

void AddEntryDialog::onExpiresToggled(bool checked)
{
    m_expirationDateTime->setEnabled(checked);
}

void AddEntryDialog::validateAndAccept()
{
    if (validateInput()) {
        accept();
    }
}

bool AddEntryDialog::validateInput()
{
    // Check if group is selected
    if (m_groupCombo->currentIndex() < 0) {
        QMessageBox::warning(this, tr("Add Entry"),
                           tr("Please select a group for this entry."));
        m_groupCombo->setFocus();
        return false;
    }

    // Check if passwords match
    if (m_passwordEdit->text() != m_repeatPasswordEdit->text()) {
        QMessageBox::warning(this, tr("Add Entry"),
                           tr("Password and repeated password aren't identical!"));
        m_repeatPasswordEdit->selectAll();
        m_repeatPasswordEdit->setFocus();
        return false;
    }

    return true;
}

// Getters

QString AddEntryDialog::getTitle() const
{
    return m_titleEdit->text();
}

QString AddEntryDialog::getUsername() const
{
    return m_usernameEdit->text();
}

QString AddEntryDialog::getPassword() const
{
    return m_passwordEdit->text();
}

QString AddEntryDialog::getUrl() const
{
    return m_urlEdit->text();
}

QString AddEntryDialog::getNotes() const
{
    return m_notesEdit->toPlainText();
}

quint32 AddEntryDialog::getGroupId() const
{
    if (m_groupCombo->currentIndex() >= 0) {
        return m_groupCombo->currentData().toUInt();
    }
    return 0;
}

quint32 AddEntryDialog::getIconId() const
{
    return static_cast<quint32>(m_iconIdSpin->value());
}

PW_TIME AddEntryDialog::getExpirationTime() const
{
    PW_TIME pwTime;

    if (m_expiresCheck->isChecked()) {
        QDateTime dt = m_expirationDateTime->dateTime();
        PwUtil::dateTimeToPwTime(dt, &pwTime);
    } else {
        // Never expire time
        PwManager::getNeverExpireTime(&pwTime);
    }

    return pwTime;
}

bool AddEntryDialog::hasExpiration() const
{
    return m_expiresCheck->isChecked();
}
