/*
  Qt KeePass - Master Key Dialog Implementation
*/

#include "MasterKeyDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QMessageBox>

MasterKeyDialog::MasterKeyDialog(Mode mode, QWidget *parent)
    : QDialog(parent)
    , m_mode(mode)
    , m_passwordEdit(nullptr)
    , m_confirmPasswordEdit(nullptr)
    , m_showPasswordCheckBox(nullptr)
    , m_passwordLabel(nullptr)
    , m_confirmPasswordLabel(nullptr)
    , m_warningLabel(nullptr)
    , m_okButton(nullptr)
    , m_cancelButton(nullptr)
{
    setupUi();

    if (m_mode == CreateNew) {
        setWindowTitle(tr("Create Master Key"));
    } else {
        setWindowTitle(tr("Enter Master Key"));
    }
}

MasterKeyDialog::~MasterKeyDialog()
{
}

QString MasterKeyDialog::getPassword() const
{
    return m_passwordEdit->text();
}

void MasterKeyDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Information label
    QLabel *infoLabel = new QLabel(this);
    if (m_mode == CreateNew) {
        infoLabel->setText(tr("Enter a master password for the new database.\n"
                             "This password will be required to open the database."));
    } else {
        infoLabel->setText(tr("Enter the master password to open the database."));
    }
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);

    mainLayout->addSpacing(10);

    // Form layout for password fields
    QFormLayout *formLayout = new QFormLayout();

    // Password field
    m_passwordLabel = new QLabel(tr("&Password:"), this);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setMinimumWidth(300);
    m_passwordLabel->setBuddy(m_passwordEdit);
    connect(m_passwordEdit, &QLineEdit::textChanged, this, &MasterKeyDialog::onPasswordChanged);
    formLayout->addRow(m_passwordLabel, m_passwordEdit);

    // Confirm password field (only for CreateNew mode)
    if (m_mode == CreateNew) {
        m_confirmPasswordLabel = new QLabel(tr("&Confirm Password:"), this);
        m_confirmPasswordEdit = new QLineEdit(this);
        m_confirmPasswordEdit->setEchoMode(QLineEdit::Password);
        m_confirmPasswordLabel->setBuddy(m_confirmPasswordEdit);
        connect(m_confirmPasswordEdit, &QLineEdit::textChanged, this, &MasterKeyDialog::onPasswordChanged);
        formLayout->addRow(m_confirmPasswordLabel, m_confirmPasswordEdit);
    }

    mainLayout->addLayout(formLayout);

    // Show password checkbox
    m_showPasswordCheckBox = new QCheckBox(tr("&Show password"), this);
    connect(m_showPasswordCheckBox, &QCheckBox::toggled, this, &MasterKeyDialog::onShowPasswordToggled);
    mainLayout->addWidget(m_showPasswordCheckBox);

    // Warning label (for password mismatch)
    if (m_mode == CreateNew) {
        m_warningLabel = new QLabel(this);
        m_warningLabel->setStyleSheet("QLabel { color: red; }");
        m_warningLabel->setVisible(false);
        mainLayout->addWidget(m_warningLabel);
    }

    mainLayout->addSpacing(10);

    // Button box
    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    m_okButton = buttonBox->addButton(QDialogButtonBox::Ok);
    m_cancelButton = buttonBox->addButton(QDialogButtonBox::Cancel);

    connect(m_okButton, &QPushButton::clicked, this, &MasterKeyDialog::onOkClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);

    // Initial state
    updateOkButton();

    // Set focus to password field
    m_passwordEdit->setFocus();

    // Set reasonable dialog size
    setMinimumWidth(400);
}

void MasterKeyDialog::onShowPasswordToggled(bool checked)
{
    QLineEdit::EchoMode mode = checked ? QLineEdit::Normal : QLineEdit::Password;
    m_passwordEdit->setEchoMode(mode);

    if (m_confirmPasswordEdit) {
        m_confirmPasswordEdit->setEchoMode(mode);
    }
}

void MasterKeyDialog::onPasswordChanged()
{
    updateOkButton();
}

void MasterKeyDialog::onOkClicked()
{
    QString password = m_passwordEdit->text();

    // Check if password is empty
    if (password.isEmpty()) {
        QMessageBox::warning(this, tr("Empty Password"),
                           tr("The password cannot be empty.\n"
                              "Please enter a password."));
        m_passwordEdit->setFocus();
        return;
    }

    // For CreateNew mode, check if passwords match
    if (m_mode == CreateNew) {
        QString confirmPassword = m_confirmPasswordEdit->text();

        if (password != confirmPassword) {
            QMessageBox::warning(this, tr("Password Mismatch"),
                               tr("The passwords do not match.\n"
                                  "Please enter the same password in both fields."));
            m_confirmPasswordEdit->clear();
            m_confirmPasswordEdit->setFocus();
            return;
        }

        // Warn about short passwords
        if (password.length() < 8) {
            QMessageBox::StandardButton result = QMessageBox::question(
                this,
                tr("Weak Password"),
                tr("The password is shorter than 8 characters.\n"
                   "For better security, consider using a longer password.\n\n"
                   "Do you want to use this password anyway?"),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No
            );

            if (result != QMessageBox::Yes) {
                m_passwordEdit->setFocus();
                return;
            }
        }
    }

    accept();
}

void MasterKeyDialog::updateOkButton()
{
    bool enableOk = false;

    if (m_mode == CreateNew) {
        // Both fields must have text
        bool hasPassword = !m_passwordEdit->text().isEmpty();
        bool hasConfirm = m_confirmPasswordEdit && !m_confirmPasswordEdit->text().isEmpty();
        enableOk = hasPassword && hasConfirm;

        // Show warning if passwords don't match
        if (m_warningLabel && hasPassword && hasConfirm) {
            bool match = m_passwordEdit->text() == m_confirmPasswordEdit->text();
            m_warningLabel->setText(match ? "" : tr("Passwords do not match"));
            m_warningLabel->setVisible(!match);
        }
    } else {
        // Only password field must have text
        enableOk = !m_passwordEdit->text().isEmpty();
    }

    m_okButton->setEnabled(enableOk);
}
