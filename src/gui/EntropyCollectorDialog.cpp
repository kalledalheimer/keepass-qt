/*
  Qt KeePass - Entropy Collector Dialog Implementation
  Reference: MFC/MFC-KeePass/WinGUI/GetRandomDlg.cpp
*/

#include "EntropyCollectorDialog.h"
#include "../core/util/Random.h"
#include "../core/crypto/SHA256.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QProgressBar>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMouseEvent>
#include <QMessageBox>
#include <QFrame>

EntropyCollectorDialog::EntropyCollectorDialog(QWidget* parent)
    : QDialog(parent)
    , m_collecting(false)
{
    setupUi();
    setWindowTitle(tr("Collect Random Data"));
    setMouseTracking(true);

    // Pre-fill with system random to ensure some entropy even if user provides minimal input
    m_entropy = Random::generateBytes(32);
}

void EntropyCollectorDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Instructions
    m_instructionLabel = new QLabel(
        tr("Move the mouse within the box below and/or type random text "
           "to generate additional entropy for key generation."), this);
    m_instructionLabel->setWordWrap(true);
    mainLayout->addWidget(m_instructionLabel);

    // Mouse collection area
    QGroupBox* mouseGroup = new QGroupBox(tr("Mouse Input"), this);
    QVBoxLayout* mouseLayout = new QVBoxLayout(mouseGroup);

    m_mouseAreaLabel = new QLabel(this);
    m_mouseAreaLabel->setMinimumSize(300, 150);
    m_mouseAreaLabel->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    m_mouseAreaLabel->setAlignment(Qt::AlignCenter);
    m_mouseAreaLabel->setText(tr("Click 'Start' then move mouse here"));
    m_mouseAreaLabel->setStyleSheet(
        "QLabel { background-color: #f5f5f5; border: 2px solid #ccc; }");
    m_mouseAreaLabel->setMouseTracking(true);
    mouseLayout->addWidget(m_mouseAreaLabel);

    QHBoxLayout* progressLayout = new QHBoxLayout();
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, MaxMousePoints);
    m_progressBar->setValue(0);
    m_progressBar->setFormat(tr("%v / %m points"));

    m_startButton = new QPushButton(tr("Start"), this);
    progressLayout->addWidget(m_progressBar, 1);
    progressLayout->addWidget(m_startButton);
    mouseLayout->addLayout(progressLayout);

    mainLayout->addWidget(mouseGroup);

    // Keyboard input area
    QGroupBox* keyboardGroup = new QGroupBox(tr("Keyboard Input (optional)"), this);
    QVBoxLayout* keyboardLayout = new QVBoxLayout(keyboardGroup);

    m_randomTextEdit = new QLineEdit(this);
    m_randomTextEdit->setPlaceholderText(tr("Type some random characters..."));
    keyboardLayout->addWidget(m_randomTextEdit);

    QLabel* keyboardHint = new QLabel(
        tr("Any text you type here will be mixed into the entropy pool."), this);
    keyboardHint->setStyleSheet("color: #666; font-size: 10px;");
    keyboardLayout->addWidget(keyboardHint);

    mainLayout->addWidget(keyboardGroup);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_okButton = new QPushButton(tr("OK"), this);
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    m_okButton->setDefault(true);

    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    // Connections
    connect(m_startButton, &QPushButton::clicked, this, &EntropyCollectorDialog::onStartMouseCollection);
    connect(m_okButton, &QPushButton::clicked, this, &EntropyCollectorDialog::onAccept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_randomTextEdit, &QLineEdit::textChanged, this, &EntropyCollectorDialog::onRandomTextChanged);

    setMinimumWidth(400);
}

void EntropyCollectorDialog::onStartMouseCollection()
{
    m_collecting = true;
    m_mousePoints.clear();
    m_progressBar->setValue(0);
    m_startButton->setEnabled(false);
    m_mouseAreaLabel->setText(tr("Move mouse here..."));
    m_mouseAreaLabel->setStyleSheet(
        "QLabel { background-color: #e3f2fd; border: 2px solid #2196f3; }");
}

void EntropyCollectorDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_collecting) {
        // Check if mouse is within the collection area
        QPoint localPos = m_mouseAreaLabel->mapFromGlobal(event->globalPosition().toPoint());
        QRect areaRect = m_mouseAreaLabel->rect();

        if (areaRect.contains(localPos)) {
            if (m_mousePoints.size() < MaxMousePoints) {
                // Only collect some points (randomize collection)
                if ((Random::generateUInt32() % 5) == 0) {
                    m_mousePoints.append(event->globalPosition().toPoint());
                    m_progressBar->setValue(m_mousePoints.size());

                    // Add immediate entropy from the point
                    QPoint pt = event->globalPosition().toPoint();
                    Random::addEntropy(&pt, sizeof(QPoint));
                }
            }

            if (m_mousePoints.size() >= MaxMousePoints) {
                // Done collecting
                m_collecting = false;
                m_startButton->setEnabled(true);
                m_mouseAreaLabel->setText(tr("Collection complete!"));
                m_mouseAreaLabel->setStyleSheet(
                    "QLabel { background-color: #e8f5e9; border: 2px solid #4caf50; }");
            }
        }
    }

    QDialog::mouseMoveEvent(event);
}

void EntropyCollectorDialog::onRandomTextChanged()
{
    // Add keyboard entropy immediately
    QString text = m_randomTextEdit->text();
    if (!text.isEmpty()) {
        QByteArray textBytes = text.toUtf8();
        Random::addEntropy(textBytes.constData(), textBytes.size());
    }
}

void EntropyCollectorDialog::onAccept()
{
    // Check if user provided any input
    if (m_mousePoints.isEmpty() && m_randomTextEdit->text().isEmpty()) {
        QMessageBox::warning(this, tr("No Input"),
            tr("Please provide some random input using the mouse and/or keyboard "
               "before closing this dialog."));
        return;
    }

    finalizeEntropy();
    accept();
}

void EntropyCollectorDialog::finalizeEntropy()
{
    // Combine all entropy sources using SHA-256
    // Reference: MFC GetRandomDlg::OnOK

    SHA256::Context sha256;

    // Start with pre-seeded entropy
    sha256.update(reinterpret_cast<const unsigned char*>(m_entropy.constData()),
                  static_cast<unsigned long>(m_entropy.size()));

    // Add mouse points
    if (!m_mousePoints.isEmpty()) {
        for (const QPoint& pt : m_mousePoints) {
            sha256.update(reinterpret_cast<const unsigned char*>(&pt), sizeof(QPoint));
        }
    }

    // Add keyboard text
    QString text = m_randomTextEdit->text();
    if (!text.isEmpty()) {
        QByteArray textBytes = text.toUtf8();
        sha256.update(reinterpret_cast<const unsigned char*>(textBytes.constData()),
                      static_cast<unsigned long>(textBytes.size()));
    }

    // Finalize and store result
    unsigned char hash[32];
    sha256.finalize(hash);
    m_entropy = QByteArray(reinterpret_cast<const char*>(hash), 32);

    // Also add to the global random pool
    Random::addEntropy(hash, 32);
}
