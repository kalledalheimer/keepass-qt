/*
  Qt KeePass - Languages Dialog Implementation
*/

#include "LanguagesDialog.h"
#include "TranslationManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>

LanguagesDialog::LanguagesDialog(QWidget *parent)
    : QDialog(parent)
    , m_languageList(nullptr)
    , m_infoLabel(nullptr)
    , m_okButton(nullptr)
    , m_cancelButton(nullptr)
{
    setupUi();
    populateLanguages();
}

QString LanguagesDialog::selectedLanguage() const
{
    return m_selectedLanguage;
}

void LanguagesDialog::setupUi()
{
    setWindowTitle(tr("Select Language"));
    setMinimumSize(400, 350);
    resize(450, 400);

    auto *mainLayout = new QVBoxLayout(this);

    // Info label
    auto *headerLabel = new QLabel(tr("Available Languages:"), this);
    mainLayout->addWidget(headerLabel);

    // Language list
    m_languageList = new QListWidget(this);
    m_languageList->setAlternatingRowColors(true);
    m_languageList->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout->addWidget(m_languageList);

    // Info label for selected language
    m_infoLabel = new QLabel(this);
    m_infoLabel->setWordWrap(true);
    m_infoLabel->setStyleSheet("QLabel { color: gray; }");
    mainLayout->addWidget(m_infoLabel);

    // Note about restart
    auto *noteLabel = new QLabel(
        tr("<b>Note:</b> Some changes may require restarting the application."),
        this
    );
    noteLabel->setWordWrap(true);
    mainLayout->addWidget(noteLabel);

    // Buttons
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_okButton = new QPushButton(tr("&OK"), this);
    m_okButton->setDefault(true);
    m_okButton->setEnabled(false);
    buttonLayout->addWidget(m_okButton);

    m_cancelButton = new QPushButton(tr("&Cancel"), this);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    // Connections
    connect(m_languageList, &QListWidget::itemSelectionChanged,
            this, &LanguagesDialog::onLanguageSelectionChanged);
    connect(m_languageList, &QListWidget::itemDoubleClicked,
            this, &LanguagesDialog::onLanguageDoubleClicked);
    connect(m_okButton, &QPushButton::clicked,
            this, &LanguagesDialog::onOkClicked);
    connect(m_cancelButton, &QPushButton::clicked,
            this, &QDialog::reject);
}

void LanguagesDialog::populateLanguages()
{
    TranslationManager &tm = TranslationManager::instance();
    QString currentLang = tm.currentLanguage();

    QList<LanguageInfo> languages = tm.availableLanguages();

    for (const LanguageInfo &lang : languages) {
        // Format: "Native Name (English Name)"
        QString displayText;
        if (lang.name == lang.englishName) {
            displayText = lang.name;
        } else {
            displayText = QString("%1 (%2)").arg(lang.name, lang.englishName);
        }

        auto *item = new QListWidgetItem(displayText, m_languageList);
        item->setData(Qt::UserRole, lang.code);

        // Mark current language
        if (lang.code == currentLang) {
            item->setSelected(true);
            m_languageList->setCurrentItem(item);
        }
    }
}

void LanguagesDialog::onLanguageSelectionChanged()
{
    QListWidgetItem *item = m_languageList->currentItem();
    if (item == nullptr) {
        m_okButton->setEnabled(false);
        m_infoLabel->clear();
        return;
    }

    QString langCode = item->data(Qt::UserRole).toString();
    TranslationManager &tm = TranslationManager::instance();
    LanguageInfo info = tm.languageInfo(langCode);

    // Update info label
    QString infoText;
    if (!info.translatorName.isEmpty()) {
        infoText = tr("Translator: %1").arg(info.translatorName);
    }
    if (!info.filePath.isEmpty()) {
        if (!infoText.isEmpty()) {
            infoText += "\n";
        }
        infoText += tr("File: %1").arg(info.filePath);
    }
    m_infoLabel->setText(infoText);

    // Enable OK if selection changed from current
    m_okButton->setEnabled(langCode != tm.currentLanguage());
}

void LanguagesDialog::onLanguageDoubleClicked(QListWidgetItem *item)
{
    if (item != nullptr) {
        onOkClicked();
    }
}

void LanguagesDialog::onOkClicked()
{
    QListWidgetItem *item = m_languageList->currentItem();
    if (item == nullptr) {
        return;
    }

    m_selectedLanguage = item->data(Qt::UserRole).toString();
    accept();
}
