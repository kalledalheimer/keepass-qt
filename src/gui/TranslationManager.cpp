/*
  Qt KeePass - Translation Manager Implementation
*/

#include "TranslationManager.h"
#include <QApplication>
#include <QCoreApplication>
#include <QTranslator>
#include <QDir>
#include <QDirIterator>
#include <QLibraryInfo>
#include <QDebug>

TranslationManager& TranslationManager::instance()
{
    static TranslationManager instance;
    return instance;
}

TranslationManager::TranslationManager()
    : QObject(nullptr)
    , m_app(nullptr)
    , m_appTranslator(nullptr)
    , m_qtTranslator(nullptr)
    , m_currentLanguage("en")
    , m_initialized(false)
{
}

TranslationManager::~TranslationManager()
{
    removeCurrentTranslation();
}

void TranslationManager::initialize(QApplication *app)
{
    if (m_initialized) {
        return;
    }

    m_app = app;
    m_initialized = true;

    // Determine translations path
    // Check multiple locations in order of preference
    QStringList searchPaths;

#ifdef Q_OS_MAC
    // macOS: Check inside app bundle first
    searchPaths << QCoreApplication::applicationDirPath() + "/../Resources/translations";
#endif
    // Check relative to executable
    searchPaths << QCoreApplication::applicationDirPath() + "/translations";
    // Check in source tree (for development)
    searchPaths << QCoreApplication::applicationDirPath() + "/../translations";
    // Check standard install location
    searchPaths << QCoreApplication::applicationDirPath() + "/../share/keepass/translations";

    for (const QString &path : searchPaths) {
        QDir dir(path);
        if (dir.exists()) {
            m_translationsPath = dir.absolutePath();
            break;
        }
    }

    // If no translations directory found, use app dir
    if (m_translationsPath.isEmpty()) {
        m_translationsPath = QCoreApplication::applicationDirPath() + "/translations";
        QDir().mkpath(m_translationsPath);
    }

    // Add built-in English as fallback
    LanguageInfo english;
    english.code = "en";
    english.name = "English";
    english.englishName = "English";
    english.translatorName = "KeePass Team";
    english.filePath = "";  // Built-in, no file needed
    m_languages["en"] = english;

    // Scan for available translations
    scanTranslations();

    qDebug() << "TranslationManager initialized";
    qDebug() << "  Translations path:" << m_translationsPath;
    qDebug() << "  Available languages:" << m_languages.keys();
}

void TranslationManager::scanTranslations()
{
    QDir dir(m_translationsPath);
    if (!dir.exists()) {
        return;
    }

    // Look for keepass_*.qm files
    QStringList filters;
    filters << "keepass_*.qm";
    dir.setNameFilters(filters);

    for (const QString &fileName : dir.entryList(QDir::Files)) {
        // Extract language code from filename (keepass_XX.qm -> XX)
        QString baseName = fileName;
        baseName.remove("keepass_");
        baseName.remove(".qm");

        if (baseName.isEmpty() || baseName == "en") {
            continue;  // Skip empty or English (already added)
        }

        // Create language info
        LanguageInfo info;
        info.code = baseName;
        info.filePath = dir.absoluteFilePath(fileName);

        // Get locale info for native name
        QLocale locale(baseName);
        info.name = locale.nativeLanguageName();
        info.englishName = QLocale::languageToString(locale.language());

        // If native name is empty, use English name
        if (info.name.isEmpty()) {
            info.name = info.englishName;
        }

        // Capitalize first letter
        if (!info.name.isEmpty()) {
            info.name[0] = info.name[0].toUpper();
        }

        m_languages[baseName] = info;
    }
}

QList<LanguageInfo> TranslationManager::availableLanguages() const
{
    QList<LanguageInfo> result;
    for (auto it = m_languages.constBegin(); it != m_languages.constEnd(); ++it) {
        result.append(it.value());
    }

    // Sort by English name
    std::sort(result.begin(), result.end(), [](const LanguageInfo &a, const LanguageInfo &b) {
        return a.englishName.toLower() < b.englishName.toLower();
    });

    return result;
}

QString TranslationManager::currentLanguage() const
{
    return m_currentLanguage;
}

LanguageInfo TranslationManager::languageInfo(const QString &code) const
{
    return m_languages.value(code);
}

bool TranslationManager::isLanguageAvailable(const QString &code) const
{
    return m_languages.contains(code);
}

QString TranslationManager::translationsPath() const
{
    return m_translationsPath;
}

bool TranslationManager::setLanguage(const QString &languageCode)
{
    if (!m_initialized || m_app == nullptr) {
        qWarning() << "TranslationManager not initialized";
        return false;
    }

    // Check if language is available
    if (!m_languages.contains(languageCode)) {
        qWarning() << "Language not available:" << languageCode;
        return false;
    }

    // Skip if already using this language
    if (m_currentLanguage == languageCode) {
        return true;
    }

    // Remove current translation
    removeCurrentTranslation();

    // Load new translation (unless it's English - the base language)
    if (languageCode != "en") {
        if (!loadTranslation(languageCode)) {
            // Fall back to English
            m_currentLanguage = "en";
            emit languageChanged(m_currentLanguage);
            return false;
        }
    }

    m_currentLanguage = languageCode;
    qDebug() << "Language changed to:" << languageCode;

    emit languageChanged(m_currentLanguage);
    return true;
}

bool TranslationManager::setSystemLanguage()
{
    QLocale systemLocale = QLocale::system();
    QString languageCode = systemLocale.name().left(2);  // Get first 2 chars (e.g., "en" from "en_US")

    // Check if this language is available
    if (m_languages.contains(languageCode)) {
        return setLanguage(languageCode);
    }

    // Fall back to English
    return setLanguage("en");
}

bool TranslationManager::loadTranslation(const QString &languageCode)
{
    const LanguageInfo &info = m_languages[languageCode];

    // Load application translation
    m_appTranslator = new QTranslator(this);
    if (!info.filePath.isEmpty() && m_appTranslator->load(info.filePath)) {
        m_app->installTranslator(m_appTranslator);
        qDebug() << "Loaded app translation:" << info.filePath;
    } else {
        delete m_appTranslator;
        m_appTranslator = nullptr;
        qWarning() << "Failed to load app translation:" << info.filePath;
        return false;
    }

    // Load Qt built-in translation
    m_qtTranslator = new QTranslator(this);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QString qtTransPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
#else
    QString qtTransPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#endif

    if (m_qtTranslator->load("qt_" + languageCode, qtTransPath)) {
        m_app->installTranslator(m_qtTranslator);
        qDebug() << "Loaded Qt translation for:" << languageCode;
    } else {
        // Qt translation not found - not critical
        delete m_qtTranslator;
        m_qtTranslator = nullptr;
    }

    return true;
}

void TranslationManager::removeCurrentTranslation()
{
    if (m_appTranslator != nullptr) {
        if (m_app != nullptr) {
            m_app->removeTranslator(m_appTranslator);
        }
        delete m_appTranslator;
        m_appTranslator = nullptr;
    }

    if (m_qtTranslator != nullptr) {
        if (m_app != nullptr) {
            m_app->removeTranslator(m_qtTranslator);
        }
        delete m_qtTranslator;
        m_qtTranslator = nullptr;
    }
}
