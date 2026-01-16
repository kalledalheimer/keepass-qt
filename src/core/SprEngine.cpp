/*
  Qt KeePass - String Placeholder Replacement Engine

  Reference: MFC WinGUI/Util/SprEngine/SprEngine.cpp
*/

#include "SprEngine.h"
#include "PwManager.h"
#include "PwStructs.h"
#include <QCoreApplication>
#include <QDir>
#include <QRegularExpression>

SprEngine::SprEngine()
{
}

QString SprEngine::compile(const QString& text,
                           PW_ENTRY* entry,
                           PwManager* database,
                           const SprContentFlags& flags)
{
    QMap<QString, QString> refCache;
    return compileInternal(text, entry, database, flags, 0, refCache);
}

QString SprEngine::compileInternal(const QString& text,
                                    PW_ENTRY* entry,
                                    PwManager* database,
                                    const SprContentFlags& flags,
                                    int recursionLevel,
                                    QMap<QString, QString>& refCache)
{
    // Prevent infinite recursion
    if (recursionLevel >= MaxRecursionDepth) {
        return QString();
    }

    if (text.isEmpty()) {
        return text;
    }

    QString result;
    int pos = 0;

    while (pos < text.length()) {
        // Look for placeholder start
        int placeholderStart = text.indexOf('{', pos);

        if (placeholderStart == -1) {
            // No more placeholders, add remaining text
            result += text.mid(pos);
            break;
        }

        // Add text before placeholder
        if (placeholderStart > pos) {
            result += text.mid(pos, placeholderStart - pos);
        }

        // Find placeholder end
        int placeholderEnd = text.indexOf('}', placeholderStart);
        if (placeholderEnd == -1) {
            // Unclosed placeholder, add rest as-is
            result += text.mid(placeholderStart);
            break;
        }

        // Extract placeholder content (without braces)
        QString placeholder = text.mid(placeholderStart + 1, placeholderEnd - placeholderStart - 1);

        // Resolve placeholder
        QString resolved = resolvePlaceholder(placeholder, entry, database, flags,
                                              recursionLevel, refCache);

        // If resolution returned something, use it; otherwise keep original
        if (!resolved.isNull()) {
            result += resolved;
        } else {
            // Keep original placeholder if not recognized
            result += text.mid(placeholderStart, placeholderEnd - placeholderStart + 1);
        }

        pos = placeholderEnd + 1;
    }

    return result;
}

QString SprEngine::resolvePlaceholder(const QString& placeholder,
                                       PW_ENTRY* entry,
                                       PwManager* database,
                                       const SprContentFlags& flags,
                                       int recursionLevel,
                                       QMap<QString, QString>& refCache)
{
    QString name = placeholder.trimmed().toUpper();

    // Entry field placeholders
    if (name == "USERNAME" || name == "USER") {
        return resolveEntryField("USERNAME", entry, database);
    }
    if (name == "PASSWORD" || name == "PASS" || name == "PWD") {
        return resolveEntryField("PASSWORD", entry, database);
    }
    if (name == "PASSWORD_ENC") {
        // Encrypted password placeholder - not typically used in Qt version
        return resolveEntryField("PASSWORD", entry, database);
    }
    if (name == "TITLE") {
        return resolveEntryField("TITLE", entry, database);
    }
    if (name == "URL") {
        return resolveEntryField("URL", entry, database);
    }
    if (name == "NOTES") {
        return resolveEntryField("NOTES", entry, database);
    }

    // DateTime placeholders
    if (name.startsWith("DT_")) {
        return resolveDateTime(name);
    }

    // Special placeholders
    if (name == "CLEARFIELD") {
        return clearFieldSequence();
    }
    if (name == "APPDIR") {
        return QCoreApplication::applicationDirPath();
    }

    // Field reference: REF:Field@SearchType:Value
    if (name.startsWith("REF:")) {
        return resolveFieldReference(placeholder.mid(4), database, flags,
                                     recursionLevel, refCache);
    }

    // Custom string field: S:FieldName
    // Note: KDB v1.x doesn't support custom string fields, but we handle the syntax
    if (name.startsWith("S:")) {
        // Not supported in KDB v1.x, return empty
        return QString();
    }

    // Not recognized - return null to indicate placeholder should be kept
    return QString();
}

QString SprEngine::resolveEntryField(const QString& fieldName,
                                      PW_ENTRY* entry,
                                      PwManager* database)
{
    if (entry == nullptr) {
        return QString();
    }

    if (fieldName == "USERNAME") {
        if (entry->pszUserName != nullptr && entry->pszUserName[0] != '\0') {
            return QString::fromUtf8(entry->pszUserName);
        }
        return QString();
    }

    if (fieldName == "PASSWORD") {
        if (entry->pszPassword != nullptr && database != nullptr) {
            // Unlock password to access it
            database->unlockEntryPassword(entry);
            QString password = QString::fromUtf8(entry->pszPassword);
            database->lockEntryPassword(entry);
            return password;
        }
        return QString();
    }

    if (fieldName == "TITLE") {
        if (entry->pszTitle != nullptr && entry->pszTitle[0] != '\0') {
            return QString::fromUtf8(entry->pszTitle);
        }
        return QString();
    }

    if (fieldName == "URL") {
        if (entry->pszURL != nullptr && entry->pszURL[0] != '\0') {
            return QString::fromUtf8(entry->pszURL);
        }
        return QString();
    }

    if (fieldName == "NOTES") {
        if (entry->pszAdditional != nullptr && entry->pszAdditional[0] != '\0') {
            QString notes = QString::fromUtf8(entry->pszAdditional);
            return removeMetadata(notes);
        }
        return QString();
    }

    return QString();
}

QString SprEngine::resolveDateTime(const QString& placeholder)
{
    // Get current time
    QDateTime now = QDateTime::currentDateTime();
    QDateTime utcNow = QDateTime::currentDateTimeUtc();

    // Local time placeholders
    if (placeholder == "DT_SIMPLE") {
        return now.toString("yyyyMMddhhmmss");
    }
    if (placeholder == "DT_YEAR") {
        return now.toString("yyyy");
    }
    if (placeholder == "DT_MONTH") {
        return now.toString("MM");
    }
    if (placeholder == "DT_DAY") {
        return now.toString("dd");
    }
    if (placeholder == "DT_HOUR") {
        return now.toString("hh");
    }
    if (placeholder == "DT_MINUTE") {
        return now.toString("mm");
    }
    if (placeholder == "DT_SECOND") {
        return now.toString("ss");
    }

    // UTC time placeholders
    if (placeholder == "DT_UTC_SIMPLE") {
        return utcNow.toString("yyyyMMddhhmmss");
    }
    if (placeholder == "DT_UTC_YEAR") {
        return utcNow.toString("yyyy");
    }
    if (placeholder == "DT_UTC_MONTH") {
        return utcNow.toString("MM");
    }
    if (placeholder == "DT_UTC_DAY") {
        return utcNow.toString("dd");
    }
    if (placeholder == "DT_UTC_HOUR") {
        return utcNow.toString("hh");
    }
    if (placeholder == "DT_UTC_MINUTE") {
        return utcNow.toString("mm");
    }
    if (placeholder == "DT_UTC_SECOND") {
        return utcNow.toString("ss");
    }

    return QString();
}

QString SprEngine::resolveFieldReference(const QString& refSpec,
                                          PwManager* database,
                                          const SprContentFlags& flags,
                                          int recursionLevel,
                                          QMap<QString, QString>& refCache)
{
    if (database == nullptr) {
        return QString();
    }

    // Check cache first
    QString cacheKey = "REF:" + refSpec.toUpper();
    if (refCache.contains(cacheKey)) {
        return refCache.value(cacheKey);
    }

    // Parse the reference specification
    QChar targetField;
    QChar searchType;
    QString searchValue;

    if (!parseFieldReference(refSpec, targetField, searchType, searchValue)) {
        return QString();
    }

    // Find the target entry
    PW_ENTRY* targetEntry = findEntryByField(database, searchType, searchValue);
    if (targetEntry == nullptr) {
        // Cache empty result to prevent repeated lookups
        refCache.insert(cacheKey, QString());
        return QString();
    }

    // Get the requested field
    QString result = getEntryField(targetEntry, database, targetField);

    // Recursively resolve any placeholders in the result
    if (!result.isEmpty() && result.contains('{')) {
        result = compileInternal(result, targetEntry, database, flags,
                                 recursionLevel + 1, refCache);
    }

    // Cache the result
    refCache.insert(cacheKey, result);

    return result;
}

bool SprEngine::parseFieldReference(const QString& refSpec,
                                     QChar& targetField,
                                     QChar& searchType,
                                     QString& searchValue)
{
    // Format: Field@SearchType:Value
    // Example: U@T:MyTitle (get Username from entry with Title "MyTitle")
    // Example: P@I:550e8400-e29b-41d4-a716-446655440000 (get Password by UUID)

    // Find @ separator
    int atPos = refSpec.indexOf('@');
    if (atPos < 1) {
        return false;
    }

    // Target field (before @)
    QString targetStr = refSpec.left(atPos).trimmed().toUpper();
    if (targetStr.isEmpty()) {
        return false;
    }
    targetField = targetStr.at(0);

    // Find : separator
    int colonPos = refSpec.indexOf(':', atPos);
    if (colonPos < atPos + 2) {
        return false;
    }

    // Search type (between @ and :)
    QString searchTypeStr = refSpec.mid(atPos + 1, colonPos - atPos - 1).trimmed().toUpper();
    if (searchTypeStr.isEmpty()) {
        return false;
    }
    searchType = searchTypeStr.at(0);

    // Search value (after :)
    searchValue = refSpec.mid(colonPos + 1).trimmed();

    // Validate field codes
    // T=Title, U=Username, A=URL, P=Password, N=Notes, I=UUID
    QString validFields = "TUAPNI";
    if (!validFields.contains(targetField) || !validFields.contains(searchType)) {
        return false;
    }

    return !searchValue.isEmpty();
}

PW_ENTRY* SprEngine::findEntryByField(PwManager* database,
                                       QChar searchType,
                                       const QString& searchValue)
{
    if (database == nullptr) {
        return nullptr;
    }

    quint32 entryCount = database->getNumberOfEntries();

    for (quint32 i = 0; i < entryCount; ++i) {
        PW_ENTRY* entry = database->getEntry(i);
        if (entry == nullptr) {
            continue;
        }

        QString fieldValue;

        switch (searchType.toLatin1()) {
            case 'T':  // Title
                if (entry->pszTitle != nullptr) {
                    fieldValue = QString::fromUtf8(entry->pszTitle);
                }
                break;
            case 'U':  // Username
                if (entry->pszUserName != nullptr) {
                    fieldValue = QString::fromUtf8(entry->pszUserName);
                }
                break;
            case 'A':  // URL
                if (entry->pszURL != nullptr) {
                    fieldValue = QString::fromUtf8(entry->pszURL);
                }
                break;
            case 'P':  // Password
                if (entry->pszPassword != nullptr) {
                    database->unlockEntryPassword(entry);
                    fieldValue = QString::fromUtf8(entry->pszPassword);
                    database->lockEntryPassword(entry);
                }
                break;
            case 'N':  // Notes
                if (entry->pszAdditional != nullptr) {
                    fieldValue = QString::fromUtf8(entry->pszAdditional);
                }
                break;
            case 'I':  // UUID
                {
                    // Convert UUID bytes to string for comparison
                    QString uuidStr;
                    for (int j = 0; j < 16; ++j) {
                        uuidStr += QString("%1").arg(entry->uuid[j], 2, 16, QChar('0'));
                    }
                    fieldValue = uuidStr;
                }
                break;
            default:
                continue;
        }

        // Case-insensitive comparison
        if (fieldValue.compare(searchValue, Qt::CaseInsensitive) == 0) {
            return entry;
        }
    }

    return nullptr;
}

QString SprEngine::getEntryField(PW_ENTRY* entry,
                                  PwManager* database,
                                  QChar fieldType)
{
    if (entry == nullptr) {
        return QString();
    }

    switch (fieldType.toLatin1()) {
        case 'T':  // Title
            if (entry->pszTitle != nullptr) {
                return QString::fromUtf8(entry->pszTitle);
            }
            break;
        case 'U':  // Username
            if (entry->pszUserName != nullptr) {
                return QString::fromUtf8(entry->pszUserName);
            }
            break;
        case 'A':  // URL
            if (entry->pszURL != nullptr) {
                return QString::fromUtf8(entry->pszURL);
            }
            break;
        case 'P':  // Password
            if (entry->pszPassword != nullptr && database != nullptr) {
                database->unlockEntryPassword(entry);
                QString password = QString::fromUtf8(entry->pszPassword);
                database->lockEntryPassword(entry);
                return password;
            }
            break;
        case 'N':  // Notes
            if (entry->pszAdditional != nullptr) {
                return removeMetadata(QString::fromUtf8(entry->pszAdditional));
            }
            break;
        case 'I':  // UUID
            {
                QString uuidStr;
                for (int i = 0; i < 16; ++i) {
                    uuidStr += QString("%1").arg(entry->uuid[i], 2, 16, QChar('0'));
                }
                return uuidStr;
            }
    }

    return QString();
}

QString SprEngine::removeMetadata(const QString& notes)
{
    // Remove auto-type configuration metadata from notes
    // Lines starting with "Auto-Type:" or "Auto-Type-Window:" are metadata
    QStringList lines = notes.split('\n');
    QStringList filtered;

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (!trimmed.startsWith("Auto-Type:", Qt::CaseInsensitive) &&
            !trimmed.startsWith("Auto-Type-Window:", Qt::CaseInsensitive)) {
            filtered.append(line);
        }
    }

    return filtered.join('\n').trimmed();
}

QString SprEngine::clearFieldSequence()
{
    // CLEARFIELD sequence: delay, select all, delete
    // Matches MFC: {DELAY 150}{HOME}(+{END}){BKSP}{DELAY 150}
    // Simplified version that works cross-platform
    return "{DELAY 150}{HOME}+{END}{BKSP}{DELAY 150}";
}

QString SprEngine::transformContent(const QString& content, const SprContentFlags& flags)
{
    QString result = content;

    if (flags.escapeForAutoType) {
        result = encodeForAutoType(result);
    }

    if (flags.escapeForCommandLine) {
        result = escapeForCommandLine(result);
    }

    return result;
}

QString SprEngine::encodeForAutoType(const QString& text)
{
    // Encode special characters for auto-type
    // These characters have special meaning in auto-type sequences
    QString result = text;

    // Replace special characters with their escape sequences
    result.replace('+', "{PLUS}");
    result.replace('^', "{CARET}");
    result.replace('%', "{PERCENT}");
    result.replace('~', "{TILDE}");

    // Braces need special handling - double them to escape
    // But we need to be careful not to escape already-valid placeholders
    // For now, we don't escape braces as they're typically intentional

    return result;
}

QString SprEngine::escapeForCommandLine(const QString& text)
{
    // Escape for command line execution
    QString result = text;

    // Escape double quotes by tripling them (Windows convention)
    result.replace("\"", "\"\"\"");

    // Wrap in quotes if contains spaces or special characters
    if (result.contains(' ') || result.contains('&') || result.contains('|') ||
        result.contains('<') || result.contains('>') || result.contains('^')) {
        result = "\"" + result + "\"";
    }

    return result;
}
