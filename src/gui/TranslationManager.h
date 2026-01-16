/*
  Qt KeePass - Translation Manager

  Manages application translations and language switching.
  Uses Qt's i18n framework (QTranslator).
*/

#ifndef TRANSLATIONMANAGER_H
#define TRANSLATIONMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QLocale>
#include <QMap>

class QTranslator;
class QApplication;

// Language information structure
struct LanguageInfo {
    QString code;           // ISO 639-1 code (e.g., "en", "de", "fr")
    QString name;           // Native name (e.g., "Deutsch", "Français")
    QString englishName;    // English name (e.g., "German", "French")
    QString translatorName; // Translator credit
    QString filePath;       // Path to .qm file
};

class TranslationManager : public QObject
{
    Q_OBJECT

public:
    // Singleton access
    static TranslationManager& instance();

    // Initialize with application instance
    void initialize(QApplication *app);

    // Get list of available languages
    QList<LanguageInfo> availableLanguages() const;

    // Get current language code
    QString currentLanguage() const;

    // Get language info by code
    LanguageInfo languageInfo(const QString &code) const;

    // Check if a language is available
    bool isLanguageAvailable(const QString &code) const;

    // Get translations directory path
    QString translationsPath() const;

public slots:
    // Switch to a different language (returns true on success)
    bool setLanguage(const QString &languageCode);

    // Switch to system default language
    bool setSystemLanguage();

signals:
    // Emitted when language changes (for UI refresh)
    void languageChanged(const QString &newLanguageCode);

private:
    // Private constructor for singleton
    TranslationManager();
    ~TranslationManager();

    // Disable copy
    TranslationManager(const TranslationManager&) = delete;
    TranslationManager& operator=(const TranslationManager&) = delete;

    // Scan for available translation files
    void scanTranslations();

    // Load a translation file
    bool loadTranslation(const QString &languageCode);

    // Remove current translation
    void removeCurrentTranslation();

    // Members
    QApplication *m_app;
    QTranslator *m_appTranslator;       // Application strings
    QTranslator *m_qtTranslator;        // Qt built-in strings
    QString m_currentLanguage;
    QMap<QString, LanguageInfo> m_languages;
    QString m_translationsPath;
    bool m_initialized;
};

#endif // TRANSLATIONMANAGER_H
