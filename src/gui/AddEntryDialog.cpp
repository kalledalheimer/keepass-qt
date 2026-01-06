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
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>

AddEntryDialog::AddEntryDialog(PwManager *pwManager, Mode mode, quint32 idValue, QWidget *parent)
    : QDialog(parent)
    , m_pwManager(pwManager)
    , m_mode(mode)
    , m_selectedGroupId(mode == AddMode ? idValue : 0)
    , m_entryIndex(mode == EditMode ? idValue : 0)
    , m_attachmentModified(false)
{
    setupUi();
    setWindowTitle(mode == AddMode ? tr("Add Entry") : tr("Edit Entry"));
    populateGroupCombo();

    if (mode == AddMode) {
        // Set default values for new entry
        m_iconIdSpin->setValue(0);  // Default entry icon
        m_usernameEdit->setText("");  // TODO: Get default username from database property
        m_passwordEdit->setText(generateRandomPassword());
        m_repeatPasswordEdit->setText(m_passwordEdit->text());

        // Initial state
        m_expirationDateTime->setEnabled(false);
    } else {
        // EditMode: Load entry data
        if (m_pwManager && idValue < m_pwManager->getNumberOfEntries()) {
            PW_ENTRY *entry = m_pwManager->getEntry(idValue);
            if (entry) {
                populateFromEntry(entry);
            }
        }
    }

    // Connect signals
    connect(m_passwordEdit, &QLineEdit::textChanged, this, &AddEntryDialog::onPasswordChanged);
    connect(m_repeatPasswordEdit, &QLineEdit::textChanged, this, &AddEntryDialog::onPasswordChanged);
    connect(m_showPasswordCheck, &QCheckBox::toggled, this, &AddEntryDialog::onShowPasswordToggled);
    connect(m_expiresCheck, &QCheckBox::toggled, this, &AddEntryDialog::onExpiresToggled);
    connect(m_okButton, &QPushButton::clicked, this, &AddEntryDialog::validateAndAccept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    // Connect attachment signals
    connect(m_setAttachmentButton, &QPushButton::clicked, this, &AddEntryDialog::onSetAttachment);
    connect(m_saveAttachmentButton, &QPushButton::clicked, this, &AddEntryDialog::onSaveAttachment);
    connect(m_removeAttachmentButton, &QPushButton::clicked, this, &AddEntryDialog::onRemoveAttachment);

    onPasswordChanged();  // Update validation state
    updateAttachmentControls();  // Update attachment button state
}

void AddEntryDialog::setupUi()
{
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

    // Attachment group
    QGroupBox *attachmentGroup = new QGroupBox(tr("Attachment"), this);
    QVBoxLayout *attachmentLayout = new QVBoxLayout(attachmentGroup);

    // Attachment description (read-only)
    m_attachmentEdit = new QLineEdit(this);
    m_attachmentEdit->setReadOnly(true);
    m_attachmentEdit->setPlaceholderText(tr("No attachment"));
    attachmentLayout->addWidget(m_attachmentEdit);

    // Attachment buttons
    QHBoxLayout *attachmentButtonLayout = new QHBoxLayout();

    m_setAttachmentButton = new QPushButton(tr("Set Attachment..."), this);
    m_setAttachmentButton->setToolTip(tr("Open file and set as attachment"));
    attachmentButtonLayout->addWidget(m_setAttachmentButton);

    m_saveAttachmentButton = new QPushButton(tr("Save Attachment..."), this);
    m_saveAttachmentButton->setToolTip(tr("Save attached file to disk"));
    m_saveAttachmentButton->setEnabled(false);
    attachmentButtonLayout->addWidget(m_saveAttachmentButton);

    m_removeAttachmentButton = new QPushButton(tr("Remove Attachment"), this);
    m_removeAttachmentButton->setToolTip(tr("Remove attachment from this entry"));
    m_removeAttachmentButton->setEnabled(false);
    attachmentButtonLayout->addWidget(m_removeAttachmentButton);

    attachmentButtonLayout->addStretch();
    attachmentLayout->addLayout(attachmentButtonLayout);

    formLayout->addRow(attachmentGroup);

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

void AddEntryDialog::populateFromEntry(PW_ENTRY *entry)
{
    if (!entry) {
        return;
    }

    // Unlock password for editing
    m_pwManager->unlockEntryPassword(entry);

    // Set all fields from entry
    m_titleEdit->setText(QString::fromUtf8(entry->pszTitle));
    m_usernameEdit->setText(QString::fromUtf8(entry->pszUserName));
    m_passwordEdit->setText(QString::fromUtf8(entry->pszPassword));
    m_repeatPasswordEdit->setText(QString::fromUtf8(entry->pszPassword));
    m_urlEdit->setText(QString::fromUtf8(entry->pszURL));
    m_notesEdit->setPlainText(QString::fromUtf8(entry->pszAdditional));
    m_iconIdSpin->setValue(entry->uImageId);

    // Lock password again
    m_pwManager->lockEntryPassword(entry);

    // Set group selection
    for (int i = 0; i < m_groupCombo->count(); ++i) {
        if (m_groupCombo->itemData(i).toUInt() == entry->uGroupId) {
            m_groupCombo->setCurrentIndex(i);
            break;
        }
    }

    // Set expiration
    PW_TIME neverExpire;
    PwManager::getNeverExpireTime(&neverExpire);

    // Check if entry expires (not equal to never-expire time)
    // Compare all fields of PW_TIME
    bool expires = !(entry->tExpire.shYear == neverExpire.shYear &&
                     entry->tExpire.btMonth == neverExpire.btMonth &&
                     entry->tExpire.btDay == neverExpire.btDay &&
                     entry->tExpire.btHour == neverExpire.btHour &&
                     entry->tExpire.btMinute == neverExpire.btMinute &&
                     entry->tExpire.btSecond == neverExpire.btSecond);

    m_expiresCheck->setChecked(expires);
    m_expirationDateTime->setEnabled(expires);

    if (expires) {
        QDateTime expireDateTime = PwUtil::pwTimeToDateTime(&entry->tExpire);
        m_expirationDateTime->setDateTime(expireDateTime);
    }

    // Set attachment
    if (entry->pszBinaryDesc && entry->pszBinaryDesc[0] != '\0') {
        m_originalAttachment = QString::fromUtf8(entry->pszBinaryDesc);
        m_attachmentEdit->setText(m_originalAttachment);
    }

    updateAttachmentControls();
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

//==============================================================================
// Attachment Functions
//==============================================================================

void AddEntryDialog::updateAttachmentControls()
{
    bool hasAttachment = !m_attachmentEdit->text().isEmpty();
    bool hasNewAttachment = !m_attachmentPath.isEmpty();

    // Enable "Set Attachment" always
    m_setAttachmentButton->setEnabled(true);

    // Enable "Save Attachment" only if there's an attachment (original or new)
    // But NOT if we're about to set a new one (user hasn't confirmed yet)
    if (m_mode == EditMode && hasAttachment && !hasNewAttachment) {
        m_saveAttachmentButton->setEnabled(true);
    } else {
        m_saveAttachmentButton->setEnabled(false);
    }

    // Enable "Remove Attachment" if there's an attachment shown
    m_removeAttachmentButton->setEnabled(hasAttachment || hasNewAttachment);
}

void AddEntryDialog::onSetAttachment()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Select File to Attach"),
        QString(),
        tr("All Files (*.*)")
    );

    if (filePath.isEmpty()) {
        return;  // User cancelled
    }

    // Check if there's an existing attachment
    if (!m_attachmentEdit->text().isEmpty()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            tr("Overwrite?"),
            tr("There already is a file attached with this entry.\n\nDo you want to overwrite the current attachment?"),
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply != QMessageBox::Yes) {
            return;
        }
    }

    // Store the file path for processing on accept
    m_attachmentPath = filePath;
    m_attachmentModified = true;

    // Extract filename and show in UI
    QFileInfo fileInfo(filePath);
    m_attachmentEdit->setText(fileInfo.fileName() + tr(" (pending)"));

    updateAttachmentControls();
}

void AddEntryDialog::onSaveAttachment()
{
    // This only works in Edit mode with an existing attachment
    if (m_mode != EditMode || m_entryIndex >= m_pwManager->getNumberOfEntries()) {
        return;
    }

    PW_ENTRY* entry = m_pwManager->getEntry(m_entryIndex);
    if (!entry || !entry->pszBinaryDesc || entry->pszBinaryDesc[0] == '\0') {
        QMessageBox::information(this, tr("Save Attachment"),
                                tr("There is no file attached with this entry."));
        return;
    }

    // Suggest the original filename
    QString suggestedName = QString::fromUtf8(entry->pszBinaryDesc);

    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Save Attachment"),
        suggestedName,
        tr("All Files (*.*)")
    );

    if (filePath.isEmpty()) {
        return;  // User cancelled
    }

    // Save the binary data
    QString errorMsg;
    if (!PwUtil::saveBinaryData(entry, filePath, &errorMsg)) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to save attachment:\n%1").arg(errorMsg));
        return;
    }

    QMessageBox::information(this, tr("Success"),
                           tr("Attachment saved successfully."));
}

void AddEntryDialog::onRemoveAttachment()
{
    // Clear the attachment field and mark as modified
    m_attachmentEdit->clear();
    m_attachmentPath.clear();
    m_attachmentModified = true;

    updateAttachmentControls();
}
