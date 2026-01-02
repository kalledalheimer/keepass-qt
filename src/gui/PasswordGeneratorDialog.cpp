/*
  Qt KeePass - Password Generator Dialog Implementation
  Reference: MFC/MFC-KeePass/WinGUI/PwGeneratorExDlg.cpp
*/

#include "PasswordGeneratorDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QSlider>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QMessageBox>

PasswordGeneratorDialog::PasswordGeneratorDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUI();
    setWindowTitle(tr("Password Generator"));

    // Set default settings
    setSettings(PasswordGenerator::getDefaultSettings());

    // Generate initial password
    onGenerate();
}

void PasswordGeneratorDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // === Password Length Section ===
    QGroupBox* lengthGroup = new QGroupBox(tr("Password Length"), this);
    QHBoxLayout* lengthLayout = new QHBoxLayout(lengthGroup);

    m_lengthSpin = new QSpinBox(this);
    m_lengthSpin->setRange(1, 999);
    m_lengthSpin->setValue(20);
    m_lengthSpin->setMinimumWidth(80);

    m_lengthSlider = new QSlider(Qt::Horizontal, this);
    m_lengthSlider->setRange(1, 128);
    m_lengthSlider->setValue(20);
    m_lengthSlider->setTickPosition(QSlider::TicksBelow);
    m_lengthSlider->setTickInterval(16);

    lengthLayout->addWidget(m_lengthSpin);
    lengthLayout->addWidget(m_lengthSlider);

    mainLayout->addWidget(lengthGroup);

    // === Character Sets Section ===
    QGroupBox* charSetGroup = new QGroupBox(tr("Character Sets"), this);
    QGridLayout* charSetLayout = new QGridLayout(charSetGroup);

    m_upperCaseCheck = new QCheckBox(tr("A-Z (Uppercase)"), this);
    m_lowerCaseCheck = new QCheckBox(tr("a-z (Lowercase)"), this);
    m_digitsCheck = new QCheckBox(tr("0-9 (Digits)"), this);
    m_minusCheck = new QCheckBox(tr("Minus (-)"), this);
    m_underlineCheck = new QCheckBox(tr("Underline (_)"), this);
    m_spaceCheck = new QCheckBox(tr("Space ( )"), this);
    m_specialCheck = new QCheckBox(tr("Special (!\"#$%&...)"), this);
    m_bracketsCheck = new QCheckBox(tr("Brackets ([]{}()<>)"), this);

    charSetLayout->addWidget(m_upperCaseCheck, 0, 0);
    charSetLayout->addWidget(m_lowerCaseCheck, 0, 1);
    charSetLayout->addWidget(m_digitsCheck, 1, 0);
    charSetLayout->addWidget(m_minusCheck, 1, 1);
    charSetLayout->addWidget(m_underlineCheck, 2, 0);
    charSetLayout->addWidget(m_spaceCheck, 2, 1);
    charSetLayout->addWidget(m_specialCheck, 3, 0);
    charSetLayout->addWidget(m_bracketsCheck, 3, 1);

    mainLayout->addWidget(charSetGroup);

    // === Custom Character Set ===
    QGroupBox* customGroup = new QGroupBox(tr("Custom Character Set (optional)"), this);
    QVBoxLayout* customLayout = new QVBoxLayout(customGroup);

    m_customCharSetEdit = new QLineEdit(this);
    m_customCharSetEdit->setPlaceholderText(tr("Leave empty to use checkboxes above..."));

    QLabel* customHint = new QLabel(tr("Custom character set overrides checkboxes if specified"), this);
    customHint->setStyleSheet("color: #666; font-size: 10px;");

    customLayout->addWidget(m_customCharSetEdit);
    customLayout->addWidget(customHint);

    mainLayout->addWidget(customGroup);

    // === Advanced Options ===
    QGroupBox* advancedGroup = new QGroupBox(tr("Advanced Options"), this);
    QVBoxLayout* advancedLayout = new QVBoxLayout(advancedGroup);

    m_excludeLookAlikeCheck = new QCheckBox(tr("Exclude look-alike characters (O, 0, I, l, 1, |)"), this);

    m_noRepeatCheck = new QCheckBox(tr("No repeated characters (each character used at most once)"), this);

    QHBoxLayout* excludeLayout = new QHBoxLayout();
    QLabel* excludeLabel = new QLabel(tr("Also exclude:"), this);
    m_excludeCharsEdit = new QLineEdit(this);
    m_excludeCharsEdit->setPlaceholderText(tr("Characters to exclude..."));
    excludeLayout->addWidget(excludeLabel);
    excludeLayout->addWidget(m_excludeCharsEdit);

    advancedLayout->addWidget(m_excludeLookAlikeCheck);
    advancedLayout->addWidget(m_noRepeatCheck);
    advancedLayout->addLayout(excludeLayout);

    mainLayout->addWidget(advancedGroup);

    // === Character Set Preview ===
    m_charSetSizeLabel = new QLabel(tr("Character set size: 62"), this);
    m_charSetSizeLabel->setStyleSheet("color: #666; font-size: 10px;");
    mainLayout->addWidget(m_charSetSizeLabel);

    // === Generated Password Section ===
    QGroupBox* passwordGroup = new QGroupBox(tr("Generated Password"), this);
    QVBoxLayout* passwordLayout = new QVBoxLayout(passwordGroup);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setReadOnly(true);
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    m_showPasswordCheck = new QCheckBox(tr("Show password"), this);

    passwordLayout->addWidget(m_passwordEdit);
    passwordLayout->addWidget(m_showPasswordCheck);

    mainLayout->addWidget(passwordGroup);

    // === Password Quality ===
    QGroupBox* qualityGroup = new QGroupBox(tr("Password Quality"), this);
    QVBoxLayout* qualityLayout = new QVBoxLayout(qualityGroup);

    m_qualityBar = new QProgressBar(this);
    m_qualityBar->setRange(0, 100);
    m_qualityBar->setTextVisible(true);
    m_qualityBar->setFormat("%p% (%v/100)");

    m_qualityLabel = new QLabel(tr("Medium"), this);
    m_qualityLabel->setAlignment(Qt::AlignCenter);

    qualityLayout->addWidget(m_qualityBar);
    qualityLayout->addWidget(m_qualityLabel);

    mainLayout->addWidget(qualityGroup);

    // === Buttons ===
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_generateButton = new QPushButton(tr("Generate"), this);
    m_okButton = new QPushButton(tr("OK"), this);
    m_cancelButton = new QPushButton(tr("Cancel"), this);

    m_okButton->setDefault(true);

    buttonLayout->addWidget(m_generateButton);
    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    // Connect signals
    connect(m_lengthSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PasswordGeneratorDialog::onLengthChanged);
    connect(m_lengthSlider, &QSlider::valueChanged,
            m_lengthSpin, &QSpinBox::setValue);

    connect(m_upperCaseCheck, &QCheckBox::toggled, this, &PasswordGeneratorDialog::onCharSetChanged);
    connect(m_lowerCaseCheck, &QCheckBox::toggled, this, &PasswordGeneratorDialog::onCharSetChanged);
    connect(m_digitsCheck, &QCheckBox::toggled, this, &PasswordGeneratorDialog::onCharSetChanged);
    connect(m_minusCheck, &QCheckBox::toggled, this, &PasswordGeneratorDialog::onCharSetChanged);
    connect(m_underlineCheck, &QCheckBox::toggled, this, &PasswordGeneratorDialog::onCharSetChanged);
    connect(m_spaceCheck, &QCheckBox::toggled, this, &PasswordGeneratorDialog::onCharSetChanged);
    connect(m_specialCheck, &QCheckBox::toggled, this, &PasswordGeneratorDialog::onCharSetChanged);
    connect(m_bracketsCheck, &QCheckBox::toggled, this, &PasswordGeneratorDialog::onCharSetChanged);

    connect(m_customCharSetEdit, &QLineEdit::textChanged, this, &PasswordGeneratorDialog::onCharSetChanged);
    connect(m_excludeLookAlikeCheck, &QCheckBox::toggled, this, &PasswordGeneratorDialog::onCharSetChanged);
    connect(m_noRepeatCheck, &QCheckBox::toggled, this, &PasswordGeneratorDialog::onCharSetChanged);
    connect(m_excludeCharsEdit, &QLineEdit::textChanged, this, &PasswordGeneratorDialog::onCharSetChanged);

    connect(m_showPasswordCheck, &QCheckBox::toggled, this, &PasswordGeneratorDialog::onShowPasswordToggled);
    connect(m_passwordEdit, &QLineEdit::textChanged, this, &PasswordGeneratorDialog::onPasswordTextChanged);

    connect(m_generateButton, &QPushButton::clicked, this, &PasswordGeneratorDialog::onGenerate);
    connect(m_okButton, &QPushButton::clicked, this, &PasswordGeneratorDialog::onAccept);
    connect(m_cancelButton, &QPushButton::clicked, this, &PasswordGeneratorDialog::onCancel);

    setMinimumWidth(550);
}

PasswordGeneratorSettings PasswordGeneratorDialog::settings() const
{
    PasswordGeneratorSettings s;

    s.length = m_lengthSpin->value();

    s.includeUpperCase = m_upperCaseCheck->isChecked();
    s.includeLowerCase = m_lowerCaseCheck->isChecked();
    s.includeDigits = m_digitsCheck->isChecked();
    s.includeMinus = m_minusCheck->isChecked();
    s.includeUnderline = m_underlineCheck->isChecked();
    s.includeSpace = m_spaceCheck->isChecked();
    s.includeSpecial = m_specialCheck->isChecked();
    s.includeBrackets = m_bracketsCheck->isChecked();

    s.customCharSet = m_customCharSetEdit->text();

    s.excludeLookAlike = m_excludeLookAlikeCheck->isChecked();
    s.noRepeatChars = m_noRepeatCheck->isChecked();
    s.excludeChars = m_excludeCharsEdit->text();

    return s;
}

void PasswordGeneratorDialog::setSettings(const PasswordGeneratorSettings& s)
{
    m_lengthSpin->setValue(s.length);
    m_lengthSlider->setValue(qMin(static_cast<int>(s.length), 128));

    m_upperCaseCheck->setChecked(s.includeUpperCase);
    m_lowerCaseCheck->setChecked(s.includeLowerCase);
    m_digitsCheck->setChecked(s.includeDigits);
    m_minusCheck->setChecked(s.includeMinus);
    m_underlineCheck->setChecked(s.includeUnderline);
    m_spaceCheck->setChecked(s.includeSpace);
    m_specialCheck->setChecked(s.includeSpecial);
    m_bracketsCheck->setChecked(s.includeBrackets);

    m_customCharSetEdit->setText(s.customCharSet);

    m_excludeLookAlikeCheck->setChecked(s.excludeLookAlike);
    m_noRepeatCheck->setChecked(s.noRepeatChars);
    m_excludeCharsEdit->setText(s.excludeChars);

    updateCharSetPreview();
}

void PasswordGeneratorDialog::onGenerate()
{
    PasswordGeneratorSettings s = settings();

    QString error;
    QString password = PasswordGenerator::generate(s, &error);

    if (password.isEmpty()) {
        QMessageBox::warning(this, tr("Password Generator"),
                           tr("Failed to generate password: %1").arg(error.isEmpty() ? tr("Unknown error") : error));
        return;
    }

    m_generatedPassword = password;
    m_passwordEdit->setText(password);
    updatePasswordQuality();
}

void PasswordGeneratorDialog::onAccept()
{
    if (m_generatedPassword.isEmpty()) {
        QMessageBox::warning(this, tr("Password Generator"),
                           tr("Please generate a password first"));
        return;
    }

    accept();
}

void PasswordGeneratorDialog::onCancel()
{
    reject();
}

void PasswordGeneratorDialog::onLengthChanged(int value)
{
    // Sync slider (clamped to max 128 for UI)
    m_lengthSlider->setValue(qMin(value, 128));

    // Update character set preview (affects no-repeat validation)
    updateCharSetPreview();
}

void PasswordGeneratorDialog::onCharSetChanged()
{
    updateCharSetPreview();
}

void PasswordGeneratorDialog::onShowPasswordToggled(bool checked)
{
    m_passwordEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
}

void PasswordGeneratorDialog::onPasswordTextChanged()
{
    updatePasswordQuality();
}

void PasswordGeneratorDialog::onAdvancedOptions()
{
    // Reserved for future advanced options dialog
}

void PasswordGeneratorDialog::updatePasswordQuality()
{
    QString password = m_passwordEdit->text();
    if (password.isEmpty()) {
        m_qualityBar->setValue(0);
        m_qualityLabel->setText(tr("No password"));
        m_qualityBar->setStyleSheet("");
        return;
    }

    quint32 quality = PasswordGenerator::calculateQuality(password);
    m_qualityBar->setValue(quality);

    // Update quality label and color
    QString qualityText;
    QString color;

    if (quality < 33) {
        qualityText = tr("Weak");
        color = "#d32f2f";  // Red
    } else if (quality < 66) {
        qualityText = tr("Medium");
        color = "#f57c00";  // Orange
    } else if (quality < 90) {
        qualityText = tr("Strong");
        color = "#388e3c";  // Green
    } else {
        qualityText = tr("Very Strong");
        color = "#1976d2";  // Blue
    }

    m_qualityLabel->setText(qualityText);
    m_qualityBar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; }").arg(color));
}

void PasswordGeneratorDialog::updateCharSetPreview()
{
    PasswordGeneratorSettings s = settings();

    QString error;
    if (!s.isValid(&error)) {
        m_charSetSizeLabel->setText(tr("Error: %1").arg(error));
        m_charSetSizeLabel->setStyleSheet("color: #d32f2f; font-size: 10px;");
        return;
    }

    QString charSet = s.buildCharSet();

    // Apply exclusions to get final size
    if (s.excludeLookAlike) {
        for (const QChar& ch : PwCharSets::ConfusingChars) {
            charSet.remove(ch);
        }
    }

    if (!s.excludeChars.isEmpty()) {
        for (const QChar& ch : s.excludeChars) {
            charSet.remove(ch);
        }
    }

    // Remove duplicates
    QSet<QChar> unique;
    for (const QChar& ch : charSet) {
        unique.insert(ch);
    }

    m_charSetSizeLabel->setText(tr("Character set size: %1").arg(unique.size()));
    m_charSetSizeLabel->setStyleSheet("color: #666; font-size: 10px;");
}
