/*
  Qt KeePass - TAN Wizard Dialog Implementation
  Reference: MFC/MFC-KeePass/WinGUI/TanWizardDlg.cpp
*/

#include "TanWizardDialog.h"
#include "../core/platform/PwSettings.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>

TanWizardDialog::TanWizardDialog(const QString& groupName, QWidget* parent)
    : QDialog(parent)
    , m_groupName(groupName)
    , m_useNumbering(false)
    , m_startNumber(0)
{
    setupUI();
    setWindowTitle(tr("TAN Wizard"));
    setMinimumWidth(550);
    setMinimumHeight(400);
}

void TanWizardDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Header
    QLabel* headerLabel = new QLabel(
        tr("Using this wizard you can easily add TAN entries."), this);
    headerLabel->setWordWrap(true);
    headerLabel->setStyleSheet("font-weight: bold; font-size: 11px; padding: 8px;");
    mainLayout->addWidget(headerLabel);

    // Group name display
    if (!m_groupName.isEmpty()) {
        QLabel* groupLabel = new QLabel(
            tr("TANs will be added to group: <b>%1</b>").arg(m_groupName), this);
        groupLabel->setWordWrap(true);
        groupLabel->setStyleSheet("padding: 4px; color: #555;");
        mainLayout->addWidget(groupLabel);
    }

    // === TAN Input Section ===
    QGroupBox* inputGroup = new QGroupBox(tr("TANs to Add"), this);
    QVBoxLayout* inputLayout = new QVBoxLayout(inputGroup);

    QLabel* inputHelp = new QLabel(
        tr("Enter or paste your TANs below (one per line or separated by spaces/commas):"),
        this);
    inputHelp->setWordWrap(true);
    inputHelp->setStyleSheet("font-size: 10px; color: #666;");

    m_tansEdit = new QPlainTextEdit(this);
    m_tansEdit->setPlaceholderText(tr("Paste TANs here..."));
    m_tansEdit->setMinimumHeight(150);

    inputLayout->addWidget(inputHelp);
    inputLayout->addWidget(m_tansEdit);

    mainLayout->addWidget(inputGroup);

    // === Character Set Section ===
    QGroupBox* charsGroup = new QGroupBox(tr("Acceptable Characters"), this);
    QVBoxLayout* charsLayout = new QVBoxLayout(charsGroup);

    QLabel* charsHelp = new QLabel(
        tr("Only characters listed below will be extracted as TANs:"),
        this);
    charsHelp->setWordWrap(true);
    charsHelp->setStyleSheet("font-size: 10px; color: #666;");

    m_charsEdit = new QLineEdit(this);

    // Load from settings or use default
    QString savedChars = PwSettings::instance().getTanChars();
    if (savedChars.isEmpty()) {
        savedChars = TAN_DEFAULT_CHARS;
    }
    m_charsEdit->setText(savedChars);

    charsLayout->addWidget(charsHelp);
    charsLayout->addWidget(m_charsEdit);

    mainLayout->addWidget(charsGroup);

    // === Numbering Section ===
    QGroupBox* numberingGroup = new QGroupBox(tr("Sequential Numbering"), this);
    QVBoxLayout* numberingLayout = new QVBoxLayout(numberingGroup);

    m_numberingCheck = new QCheckBox(tr("Assign sequential numbers to TANs"), this);
    m_numberingCheck->setChecked(false);

    QHBoxLayout* startLayout = new QHBoxLayout();
    QLabel* startLabel = new QLabel(tr("Starting number:"), this);
    m_startNumberSpin = new QSpinBox(this);
    m_startNumberSpin->setRange(0, 999999);
    m_startNumberSpin->setValue(0);
    m_startNumberSpin->setEnabled(false);
    m_startNumberSpin->setMinimumWidth(100);

    startLayout->addWidget(startLabel);
    startLayout->addWidget(m_startNumberSpin);
    startLayout->addStretch();

    QLabel* numberingHelp = new QLabel(
        tr("If enabled, each TAN will be assigned a sequential number (stored in Username field) "
           "to help you track which TAN to use next."),
        this);
    numberingHelp->setWordWrap(true);
    numberingHelp->setStyleSheet("font-size: 10px; color: #666;");

    numberingLayout->addWidget(m_numberingCheck);
    numberingLayout->addLayout(startLayout);
    numberingLayout->addWidget(numberingHelp);

    mainLayout->addWidget(numberingGroup);

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
    connect(m_numberingCheck, &QCheckBox::toggled, this, &TanWizardDialog::onNumberingToggled);
    connect(m_okButton, &QPushButton::clicked, this, &TanWizardDialog::onAccept);
    connect(m_cancelButton, &QPushButton::clicked, this, &TanWizardDialog::reject);
}

void TanWizardDialog::onNumberingToggled(bool checked)
{
    m_startNumberSpin->setEnabled(checked);
}

void TanWizardDialog::onAccept()
{
    // Save character set to settings
    PwSettings::instance().setTanChars(m_charsEdit->text());

    // Parse TANs from input
    parseTans();

    // Store numbering settings
    m_useNumbering = m_numberingCheck->isChecked();
    m_startNumber = m_startNumberSpin->value();

    accept();
}

void TanWizardDialog::parseTans()
{
    // Reference: MFC/MFC-KeePass/WinGUI/PwSafeDlg.cpp OnExtrasTanWizard
    m_tanList.clear();

    QString tansText = m_tansEdit->toPlainText();
    QString acceptableChars = m_charsEdit->text();

    if (tansText.isEmpty() || acceptableChars.isEmpty()) {
        return;
    }

    // Find a separator character that's not in the input
    // (MFC uses this technique to simplify parsing)
    QChar separator = '!';
    while (tansText.contains(separator)) {
        separator = QChar(separator.unicode() + 1);
    }

    // Append terminating character
    tansText += separator;

    // Parse TANs by extracting sequences of acceptable characters
    QString currentTan;
    bool inValidSequence = false;

    for (const QChar& ch : tansText) {
        bool isAcceptable = acceptableChars.contains(ch);

        if (isAcceptable && !inValidSequence) {
            // Start of a new TAN
            currentTan = ch;
            inValidSequence = true;
        }
        else if (isAcceptable && inValidSequence) {
            // Continue building current TAN
            currentTan += ch;
        }
        else if (!isAcceptable && inValidSequence) {
            // End of current TAN
            if (!currentTan.isEmpty()) {
                m_tanList.append(currentTan);
            }
            currentTan.clear();
            inValidSequence = false;
        }
    }
}
