/*
  Qt KeePass - Database Settings Dialog Implementation
  Reference: MFC/MFC-KeePass/WinGUI/DbSettingsDlg.cpp
*/

#include "DatabaseSettingsDialog.h"
#include "../core/PwManager.h"
#include "../core/crypto/KeyTransform.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QSlider>
#include <QCheckBox>
#include <QLabel>
#include <QMessageBox>
#include <QProgressDialog>
#include <QApplication>
#include <QColor>
#include <cmath>

DatabaseSettingsDialog::DatabaseSettingsDialog(PwManager* pwManager, QWidget* parent)
    : QDialog(parent)
    , m_pwManager(pwManager)
    , m_algorithm(0)
    , m_keyRounds(600000)
    , m_color(0xFFFFFFFF)  // DWORD_MAX = no custom color
{
    setupUI();
    setWindowTitle(tr("Database Settings"));
    setMinimumWidth(550);
}

void DatabaseSettingsDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // === Encryption Algorithm Section ===
    QGroupBox* algoGroup = new QGroupBox(tr("Encryption Algorithm"), this);
    QVBoxLayout* algoLayout = new QVBoxLayout(algoGroup);

    m_algorithmCombo = new QComboBox(this);
    m_algorithmCombo->addItem(tr("Advanced Encryption Standard (AES) (128-bit block cipher using 256-bit key)"));
    m_algorithmCombo->addItem(tr("Twofish (128-bit block cipher using 256-bit key)"));
    m_algorithmCombo->setCurrentIndex(0);
    algoLayout->addWidget(m_algorithmCombo);

    mainLayout->addWidget(algoGroup);

    // === Key Transformation Section ===
    QGroupBox* keyGroup = new QGroupBox(tr("Key Transformation Rounds"), this);
    QVBoxLayout* keyLayout = new QVBoxLayout(keyGroup);

    QHBoxLayout* roundsLayout = new QHBoxLayout();
    QLabel* roundsLabel = new QLabel(tr("Number of rounds:"), this);

    m_roundsSpin = new QSpinBox(this);
    m_roundsSpin->setRange(1, 2147483646);  // Max: 0xFFFFFFFE
    m_roundsSpin->setValue(600000);
    m_roundsSpin->setMinimumWidth(150);

    m_calculateButton = new QPushButton(tr("Calculate"), this);
    m_calculateButton->setToolTip(tr("Benchmark to determine optimal number of rounds"));

    roundsLayout->addWidget(roundsLabel);
    roundsLayout->addWidget(m_roundsSpin);
    roundsLayout->addWidget(m_calculateButton);
    roundsLayout->addStretch();

    QLabel* roundsHelp = new QLabel(
        tr("The database content is encrypted using the master key. Before it "
           "is used for encryption, the master key is transformed multiple times. "
           "The higher the number of rounds, the longer it takes to crack the "
           "database using brute-force methods (recommended: at least 600,000 rounds)."),
        this);
    roundsHelp->setWordWrap(true);
    roundsHelp->setStyleSheet("color: #666; font-size: 10px;");

    keyLayout->addLayout(roundsLayout);
    keyLayout->addWidget(roundsHelp);

    mainLayout->addWidget(keyGroup);

    // === Default Username Section ===
    QGroupBox* userGroup = new QGroupBox(tr("Default Username"), this);
    QVBoxLayout* userLayout = new QVBoxLayout(userGroup);

    QLabel* userLabel = new QLabel(tr("Default user name for new entries:"), this);
    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText(tr("Leave empty for no default"));

    userLayout->addWidget(userLabel);
    userLayout->addWidget(m_usernameEdit);

    mainLayout->addWidget(userGroup);

    // === Database Color Section ===
    QGroupBox* colorGroup = new QGroupBox(tr("Database Color"), this);
    QVBoxLayout* colorLayout = new QVBoxLayout(colorGroup);

    m_customColorCheck = new QCheckBox(tr("Use custom color"), this);

    QHBoxLayout* sliderLayout = new QHBoxLayout();
    QLabel* colorLabel = new QLabel(tr("Hue:"), this);

    m_colorSlider = new QSlider(Qt::Horizontal, this);
    m_colorSlider->setRange(0, 360);
    m_colorSlider->setValue(240);  // Blue
    m_colorSlider->setEnabled(false);

    m_colorPreview = new QLabel(this);
    m_colorPreview->setFixedSize(100, 30);
    m_colorPreview->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_colorPreview->setAutoFillBackground(true);

    sliderLayout->addWidget(colorLabel);
    sliderLayout->addWidget(m_colorSlider, 1);
    sliderLayout->addWidget(m_colorPreview);

    colorLayout->addWidget(m_customColorCheck);
    colorLayout->addLayout(sliderLayout);

    mainLayout->addWidget(colorGroup);

    // === Buttons ===
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_okButton = new QPushButton(tr("OK"), this);
    m_cancelButton = new QPushButton(tr("Cancel"), this);

    m_okButton->setDefault(true);

    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    // Connect signals
    connect(m_calculateButton, &QPushButton::clicked, this, &DatabaseSettingsDialog::onCalculateRounds);
    connect(m_customColorCheck, &QCheckBox::toggled, this, &DatabaseSettingsDialog::onCustomColorToggled);
    connect(m_colorSlider, &QSlider::valueChanged, this, &DatabaseSettingsDialog::onColorSliderChanged);
    connect(m_okButton, &QPushButton::clicked, this, &DatabaseSettingsDialog::onAccept);
    connect(m_cancelButton, &QPushButton::clicked, this, &DatabaseSettingsDialog::reject);

    // Initial state
    updateColorPreview();
}

void DatabaseSettingsDialog::setEncryptionAlgorithm(int algorithm)
{
    m_algorithm = algorithm;
    if (m_algorithmCombo) {
        m_algorithmCombo->setCurrentIndex(algorithm);
    }
}

void DatabaseSettingsDialog::setKeyTransformRounds(quint32 rounds)
{
    m_keyRounds = rounds;
    if (m_roundsSpin) {
        m_roundsSpin->setValue(rounds);
    }
}

void DatabaseSettingsDialog::setDefaultUsername(const QString& username)
{
    m_defaultUsername = username;
    if (m_usernameEdit) {
        m_usernameEdit->setText(username);
    }
}

void DatabaseSettingsDialog::setDatabaseColor(quint32 color)
{
    m_color = color;

    if (color == 0xFFFFFFFF) {
        // No custom color
        m_customColorCheck->setChecked(false);
        m_colorSlider->setValue(240);  // Blue
    }
    else {
        // Custom color
        m_customColorCheck->setChecked(true);

        // Convert RGB to HSV and set slider
        QColor qcolor(
            (color >> 16) & 0xFF,  // R
            (color >> 8) & 0xFF,   // G
            color & 0xFF           // B
        );
        float hue = rgbToHue(qcolor);
        m_colorSlider->setValue(static_cast<int>(hue));
    }

    updateColorPreview();
    enableControls();
}

void DatabaseSettingsDialog::onCalculateRounds()
{
    // Reference: MFC OnBtnCalcRounds
    QProgressDialog progress(tr("Benchmarking key transformation..."),
                            tr("Cancel"), 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setValue(0);
    QApplication::processEvents();

    // Run benchmark (1 second)
    quint64 rounds = KeyTransform::benchmark(1000);

    // Clamp to valid range
    if (rounds > 0xFFFFFFFE) {
        rounds = 0xFFFFFFFE;
    }

    m_roundsSpin->setValue(static_cast<quint32>(rounds));

    progress.setValue(1);
}

void DatabaseSettingsDialog::onCustomColorToggled(bool checked)
{
    enableControls();
    updateColorPreview();
}

void DatabaseSettingsDialog::onColorSliderChanged(int value)
{
    updateColorPreview();
}

void DatabaseSettingsDialog::onAccept()
{
    // Get values from UI
    m_algorithm = m_algorithmCombo->currentIndex();
    m_keyRounds = m_roundsSpin->value();
    m_defaultUsername = m_usernameEdit->text();

    // Get color
    if (m_customColorCheck->isChecked()) {
        float hue = static_cast<float>(m_colorSlider->value());
        QColor color = hsvToRgb(hue, 1.0f, 1.0f);

        // Convert QColor to DWORD (0x00RRGGBB)
        m_color = (color.red() << 16) | (color.green() << 8) | color.blue();
    }
    else {
        m_color = 0xFFFFFFFF;  // No custom color
    }

    accept();
}

void DatabaseSettingsDialog::updateColorPreview()
{
    if (!m_colorPreview) return;

    QColor color;
    if (m_customColorCheck->isChecked()) {
        float hue = static_cast<float>(m_colorSlider->value());
        color = hsvToRgb(hue, 1.0f, 1.0f);
    }
    else {
        color = QColor(0, 0, 255);  // Default blue
    }

    QPalette pal = m_colorPreview->palette();
    pal.setColor(QPalette::Window, color);
    m_colorPreview->setPalette(pal);
}

void DatabaseSettingsDialog::enableControls()
{
    bool customColor = m_customColorCheck->isChecked();
    m_colorSlider->setEnabled(customColor);
}

QColor DatabaseSettingsDialog::hsvToRgb(float h, float s, float v)
{
    // Reference: MFC NewGUI_ColorFromHsv
    // H: 0-360, S: 0-1, V: 0-1

    float c = v * s;
    float x = c * (1.0f - std::fabs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;

    float r, g, b;

    if (h < 60.0f) {
        r = c; g = x; b = 0;
    }
    else if (h < 120.0f) {
        r = x; g = c; b = 0;
    }
    else if (h < 180.0f) {
        r = 0; g = c; b = x;
    }
    else if (h < 240.0f) {
        r = 0; g = x; b = c;
    }
    else if (h < 300.0f) {
        r = x; g = 0; b = c;
    }
    else {
        r = c; g = 0; b = x;
    }

    return QColor(
        static_cast<int>((r + m) * 255.0f),
        static_cast<int>((g + m) * 255.0f),
        static_cast<int>((b + m) * 255.0f)
    );
}

float DatabaseSettingsDialog::rgbToHue(const QColor& color)
{
    // Reference: MFC NewGUI_GetHue
    float r = color.red() / 255.0f;
    float g = color.green() / 255.0f;
    float b = color.blue() / 255.0f;

    float cmax = std::max({r, g, b});
    float cmin = std::min({r, g, b});
    float delta = cmax - cmin;

    if (delta == 0.0f) {
        return 0.0f;
    }

    float hue;
    if (cmax == r) {
        hue = 60.0f * std::fmod((g - b) / delta, 6.0f);
    }
    else if (cmax == g) {
        hue = 60.0f * ((b - r) / delta + 2.0f);
    }
    else {
        hue = 60.0f * ((r - g) / delta + 4.0f);
    }

    if (hue < 0.0f) {
        hue += 360.0f;
    }

    return hue;
}
