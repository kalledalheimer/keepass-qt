/*
  Qt KeePass - Change Master Key Dialog Implementation
*/

#include "ChangeMasterKeyDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QMessageBox>
#include <QGroupBox>

ChangeMasterKeyDialog::ChangeMasterKeyDialog(QWidget *parent)
    : QDialog(parent)
    , m_newPasswordEdit(nullptr)
    , m_confirmPasswordEdit(nullptr)
    , m_showPasswordCheckBox(nullptr)
    , m_newPasswordLabel(nullptr)
    , m_confirmPasswordLabel(nullptr)
    , m_warningLabel(nullptr)
    , m_infoLabel(nullptr)
    , m_okButton(nullptr)
    , m_cancelButton(nullptr)
{
    setupUi();
}

ChangeMasterKeyDialog::~ChangeMasterKeyDialog()
{
}

void ChangeMasterKeyDialog::setupUi()
{
    setWindowTitle(tr("Change Master Key"));
    setModal(true);
    resize(500, 300);

    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Info label
    m_infoLabel = new QLabel(tr(
        "<b>Change Master Password</b><br><br>"
        "You are about to change the master password for this database.<br>"
        "The database will be re-encrypted with the new password.<br><br>"
        "<b>Warning:</b> Make sure you remember your new password. "
        "If you forget it, you will <b>permanently lose access</b> to your database!"
    ), this);
    m_infoLabel->setWordWrap(true);
    m_infoLabel->setStyleSheet("QLabel { padding: 10px; background-color: #ffffcc; border: 1px solid #ccccaa; border-radius: 4px; }");
    mainLayout->addWidget(m_infoLabel);

    mainLayout->addSpacing(10);

    // Password fields group
    QGroupBox *passwordGroup = new QGroupBox(tr("New Master Password"), this);
    QGridLayout *passwordLayout = new QGridLayout(passwordGroup);

    // New password
    m_newPasswordLabel = new QLabel(tr("&New password:"), passwordGroup);
    m_newPasswordEdit = new QLineEdit(passwordGroup);
    m_newPasswordEdit->setEchoMode(QLineEdit::Password);
    m_newPasswordEdit->setMinimumWidth(300);
    m_newPasswordLabel->setBuddy(m_newPasswordEdit);
    passwordLayout->addWidget(m_newPasswordLabel, 0, 0);
    passwordLayout->addWidget(m_newPasswordEdit, 0, 1);

    // Confirm password
    m_confirmPasswordLabel = new QLabel(tr("&Confirm password:"), passwordGroup);
    m_confirmPasswordEdit = new QLineEdit(passwordGroup);
    m_confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    m_confirmPasswordEdit->setMinimumWidth(300);
    m_confirmPasswordLabel->setBuddy(m_confirmPasswordEdit);
    passwordLayout->addWidget(m_confirmPasswordLabel, 1, 0);
    passwordLayout->addWidget(m_confirmPasswordEdit, 1, 1);

    // Show password checkbox
    m_showPasswordCheckBox = new QCheckBox(tr("&Show password"), passwordGroup);
    passwordLayout->addWidget(m_showPasswordCheckBox, 2, 1);

    mainLayout->addWidget(passwordGroup);

    // Warning label (initially hidden)
    m_warningLabel = new QLabel(this);
    m_warningLabel->setStyleSheet("QLabel { color: red; }");
    m_warningLabel->setWordWrap(true);
    m_warningLabel->setVisible(false);
    mainLayout->addWidget(m_warningLabel);

    mainLayout->addStretch();

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_okButton = new QPushButton(tr("OK"), this);
    m_okButton->setDefault(true);
    m_okButton->setEnabled(false);  // Disabled until valid input
    buttonLayout->addWidget(m_okButton);

    m_cancelButton = new QPushButton(tr("Cancel"), this);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    // Connections
    connect(m_showPasswordCheckBox, &QCheckBox::toggled,
            this, &ChangeMasterKeyDialog::onShowPasswordToggled);
    connect(m_newPasswordEdit, &QLineEdit::textChanged,
            this, &ChangeMasterKeyDialog::onPasswordChanged);
    connect(m_confirmPasswordEdit, &QLineEdit::textChanged,
            this, &ChangeMasterKeyDialog::onPasswordChanged);
    connect(m_okButton, &QPushButton::clicked,
            this, &ChangeMasterKeyDialog::onOkClicked);
    connect(m_cancelButton, &QPushButton::clicked,
            this, &QDialog::reject);

    // Focus on new password field
    m_newPasswordEdit->setFocus();
}

QString ChangeMasterKeyDialog::getNewPassword() const
{
    return m_newPasswordEdit->text();
}

void ChangeMasterKeyDialog::onShowPasswordToggled(bool checked)
{
    QLineEdit::EchoMode mode = checked ? QLineEdit::Normal : QLineEdit::Password;
    m_newPasswordEdit->setEchoMode(mode);
    m_confirmPasswordEdit->setEchoMode(mode);
}

void ChangeMasterKeyDialog::onPasswordChanged()
{
    updateOkButton();
}

void ChangeMasterKeyDialog::updateOkButton()
{
    QString newPassword = m_newPasswordEdit->text();
    QString confirmPassword = m_confirmPasswordEdit->text();

    // Clear warning
    m_warningLabel->setVisible(false);

    // Check if passwords are empty
    if (newPassword.isEmpty()) {
        m_okButton->setEnabled(false);
        return;
    }

    // Check if passwords match
    if (newPassword != confirmPassword) {
        m_warningLabel->setText(tr("Passwords do not match."));
        m_warningLabel->setVisible(true);
        m_okButton->setEnabled(false);
        return;
    }

    // Warn about weak passwords
    if (newPassword.length() < 8) {
        m_warningLabel->setText(tr("Warning: Password is short (less than 8 characters). Consider using a longer password for better security."));
        m_warningLabel->setStyleSheet("QLabel { color: orange; }");
        m_warningLabel->setVisible(true);
    }

    // Enable OK button when passwords match and are not empty
    m_okButton->setEnabled(true);
}

void ChangeMasterKeyDialog::onOkClicked()
{
    QString newPassword = m_newPasswordEdit->text();
    QString confirmPassword = m_confirmPasswordEdit->text();

    // Final validation
    if (newPassword.isEmpty()) {
        QMessageBox::warning(this, tr("Empty Password"),
                           tr("The password cannot be empty.\n\n"
                              "Please enter a master password."));
        m_newPasswordEdit->setFocus();
        return;
    }

    if (newPassword != confirmPassword) {
        QMessageBox::warning(this, tr("Password Mismatch"),
                           tr("The passwords you entered do not match.\n\n"
                              "Please make sure both password fields contain the same password."));
        m_confirmPasswordEdit->clear();
        m_confirmPasswordEdit->setFocus();
        return;
    }

    // Confirm password change
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Confirm Password Change"),
        tr("Are you sure you want to change the master password?\n\n"
           "The database will be re-encrypted with the new password.\n"
           "Make sure you remember your new password!"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        accept();
    }
}
