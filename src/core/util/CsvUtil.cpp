/*
  Qt KeePass - CSV Utility Implementation
*/

#include "CsvUtil.h"
#include "../PwManager.h"
#include "../PwStructs.h"
#include "PwUtil.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QStringConverter>
#include <cstring>

bool CsvUtil::exportToCSV(const QString& filePath,
                          PwManager* pwManager,
                          const CsvExportOptions& options,
                          QString* errorMsg)
{
    if (!pwManager) {
        if (errorMsg) *errorMsg = "Invalid database manager";
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMsg) *errorMsg = QString("Cannot open file for writing: %1").arg(file.errorString());
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // Write header row
    QStringList headers;
    if (options.includeGroup) headers << "Group";
    if (options.includeTitle) headers << "Account";
    if (options.includeUsername) headers << "Login Name";
    if (options.includePassword) headers << "Password";
    if (options.includeUrl) headers << "Web Site";
    if (options.includeNotes) headers << "Comments";
    if (options.includeUuid) headers << "UUID";
    if (options.includeCreationTime) headers << "Creation Time";
    if (options.includeLastModTime) headers << "Last Modification";
    if (options.includeLastAccessTime) headers << "Last Access";
    if (options.includeExpireTime) headers << "Expires";

    // Write quoted header
    out << "\"" << headers.join("\",\"") << "\"" << "\n";

    // Export all entries
    quint32 numEntries = pwManager->getNumberOfEntries();
    for (quint32 i = 0; i < numEntries; ++i) {
        PW_ENTRY* entry = pwManager->getEntry(i);
        if (!entry) continue;

        // Get group name
        QString groupName;
        if (options.includeGroup) {
            PW_GROUP* group = pwManager->getGroupById(entry->uGroupId);
            if (group && group->pszGroupName) {
                groupName = QString::fromUtf8(group->pszGroupName);
            }
        }

        // Unlock password for export
        pwManager->unlockEntryPassword(entry);

        // Build field list
        QStringList fields;
        if (options.includeGroup) {
            fields << escapeCsvField(groupName);
        }
        if (options.includeTitle) {
            fields << escapeCsvField(QString::fromUtf8(entry->pszTitle ? entry->pszTitle : ""));
        }
        if (options.includeUsername) {
            fields << escapeCsvField(QString::fromUtf8(entry->pszUserName ? entry->pszUserName : ""));
        }
        if (options.includePassword) {
            fields << escapeCsvField(QString::fromUtf8(entry->pszPassword ? entry->pszPassword : ""));
        }
        if (options.includeUrl) {
            fields << escapeCsvField(QString::fromUtf8(entry->pszURL ? entry->pszURL : ""));
        }
        if (options.includeNotes) {
            fields << escapeCsvField(QString::fromUtf8(entry->pszAdditional ? entry->pszAdditional : ""));
        }
        if (options.includeUuid) {
            QString uuid;
            for (int j = 0; j < 16; ++j) {
                uuid += QString("%1").arg(entry->uuid[j], 2, 16, QChar('0')).toUpper();
            }
            fields << escapeCsvField(uuid);
        }
        if (options.includeCreationTime) {
            QDateTime dt = PwUtil::pwTimeToDateTime(&entry->tCreation);
            fields << escapeCsvField(dt.toString(Qt::ISODate));
        }
        if (options.includeLastModTime) {
            QDateTime dt = PwUtil::pwTimeToDateTime(&entry->tLastMod);
            fields << escapeCsvField(dt.toString(Qt::ISODate));
        }
        if (options.includeLastAccessTime) {
            QDateTime dt = PwUtil::pwTimeToDateTime(&entry->tLastAccess);
            fields << escapeCsvField(dt.toString(Qt::ISODate));
        }
        if (options.includeExpireTime) {
            QDateTime dt = PwUtil::pwTimeToDateTime(&entry->tExpire);
            fields << escapeCsvField(dt.toString(Qt::ISODate));
        }

        // Lock password again
        pwManager->lockEntryPassword(entry);

        // Write quoted row
        out << "\"" << fields.join("\",\"") << "\"" << "\n";
    }

    file.close();
    return true;
}

bool CsvUtil::importFromCSV(const QString& filePath,
                            PwManager* pwManager,
                            const CsvImportOptions& options,
                            int* entriesImported,
                            QString* errorMsg)
{
    if (!pwManager) {
        if (errorMsg) *errorMsg = "Invalid database manager";
        return false;
    }

    if (options.targetGroupId == 0 || options.targetGroupId == 0xFFFFFFFF) {
        if (errorMsg) *errorMsg = "Invalid target group";
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMsg) *errorMsg = QString("Cannot open file for reading: %1").arg(file.errorString());
        return false;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    int imported = 0;
    int lineNumber = 0;

    while (!in.atEnd()) {
        QString line = in.readLine();
        lineNumber++;

        // Skip header (first line) or empty lines
        if (lineNumber == 1 || line.trimmed().isEmpty()) {
            continue;
        }

        // Parse CSV line
        QStringList fields = parseCsvLine(line);
        if (fields.isEmpty()) {
            continue;
        }

        // Extract fields based on column mapping
        QString title = (options.titleColumn >= 0 && options.titleColumn < fields.size())
                       ? fields[options.titleColumn] : "";
        QString username = (options.usernameColumn >= 0 && options.usernameColumn < fields.size())
                          ? fields[options.usernameColumn] : "";
        QString password = (options.passwordColumn >= 0 && options.passwordColumn < fields.size())
                          ? fields[options.passwordColumn] : "";
        QString url = (options.urlColumn >= 0 && options.urlColumn < fields.size())
                     ? fields[options.urlColumn] : "";
        QString notes = (options.notesColumn >= 0 && options.notesColumn < fields.size())
                       ? fields[options.notesColumn] : "";

        // Create entry template
        PW_ENTRY entryTemplate;
        std::memset(&entryTemplate, 0, sizeof(PW_ENTRY));

        // Set timestamps
        QDateTime now = QDateTime::currentDateTime();
        PwUtil::dateTimeToPwTime(now, &entryTemplate.tCreation);
        PwUtil::dateTimeToPwTime(now, &entryTemplate.tLastMod);
        PwUtil::dateTimeToPwTime(now, &entryTemplate.tLastAccess);
        PwManager::getNeverExpireTime(&entryTemplate.tExpire);

        // Set strings (must allocate memory)
        QByteArray titleUtf8 = title.toUtf8();
        entryTemplate.pszTitle = new char[titleUtf8.size() + 1];
        std::strcpy(entryTemplate.pszTitle, titleUtf8.constData());

        QByteArray usernameUtf8 = username.toUtf8();
        entryTemplate.pszUserName = new char[usernameUtf8.size() + 1];
        std::strcpy(entryTemplate.pszUserName, usernameUtf8.constData());

        QByteArray passwordUtf8 = password.toUtf8();
        entryTemplate.pszPassword = new char[passwordUtf8.size() + 1];
        std::strcpy(entryTemplate.pszPassword, passwordUtf8.constData());
        entryTemplate.uPasswordLen = passwordUtf8.size();

        QByteArray urlUtf8 = url.toUtf8();
        entryTemplate.pszURL = new char[urlUtf8.size() + 1];
        std::strcpy(entryTemplate.pszURL, urlUtf8.constData());

        QByteArray notesUtf8 = notes.toUtf8();
        entryTemplate.pszAdditional = new char[notesUtf8.size() + 1];
        std::strcpy(entryTemplate.pszAdditional, notesUtf8.constData());

        // Set empty binary data
        entryTemplate.pszBinaryDesc = new char[1];
        entryTemplate.pszBinaryDesc[0] = '\0';
        entryTemplate.pBinaryData = nullptr;
        entryTemplate.uBinaryDataLen = 0;

        // Set other properties
        entryTemplate.uGroupId = options.targetGroupId;
        entryTemplate.uImageId = 0;  // Default icon

        // Add entry to database
        bool success = pwManager->addEntry(&entryTemplate);

        // Clean up allocated memory
        delete[] entryTemplate.pszTitle;
        delete[] entryTemplate.pszUserName;
        delete[] entryTemplate.pszPassword;
        delete[] entryTemplate.pszURL;
        delete[] entryTemplate.pszAdditional;
        delete[] entryTemplate.pszBinaryDesc;

        if (success) {
            imported++;
        }
    }

    file.close();

    if (entriesImported) {
        *entriesImported = imported;
    }

    return true;
}

QStringList CsvUtil::parseCsvLine(const QString& line)
{
    QStringList fields;
    QString field;
    bool inQuotes = false;
    int len = line.length();

    for (int i = 0; i < len; ++i) {
        QChar c = line[i];

        if (c == '"') {
            if (inQuotes && i + 1 < len && line[i + 1] == '"') {
                // Escaped quote ("")
                field += '"';
                ++i; // Skip next quote
            } else {
                // Toggle quote state
                inQuotes = !inQuotes;
            }
        } else if (c == ',' && !inQuotes) {
            // Field separator (outside quotes)
            fields << field;
            field.clear();
        } else {
            field += c;
        }
    }

    // Add last field
    fields << field;

    return fields;
}

QString CsvUtil::escapeCsvField(const QString& field)
{
    // If field contains comma, quote, or newline, wrap in quotes
    // Also escape internal quotes by doubling them
    if (field.contains(',') || field.contains('"') || field.contains('\n') || field.contains('\r')) {
        QString escaped = field;
        escaped.replace("\"", "\"\""); // Escape quotes by doubling
        return escaped;
    }

    return field;
}
