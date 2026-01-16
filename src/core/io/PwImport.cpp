/*
  Qt KeePass - Password Import Implementation

  Reference: MFC KeePassLibCpp/DataExchange/PwImport.cpp
*/

#include "PwImport.h"
#include "../util/PwUtil.h"
#include "../util/Random.h"
#include <QFile>
#include <QTextStream>
#include <QStringConverter>
#include <QDateTime>
#include <QDebug>

// CodeWallet separators
static const QString CW_SEPARATOR_OLD = "----------------------------------------";
static const QString CW_SEPARATOR_NEW = "*---------------------------------------------------";

// Import from file based on format
bool PwImport::importFromFile(PwManager *manager, const QString &filePath,
                              quint32 format, QString *errorMsg)
{
    if (manager == nullptr) {
        if (errorMsg != nullptr) *errorMsg = "Invalid manager";
        return false;
    }

    switch (format) {
        case PWIMP_CWALLET:
            return importCodeWallet(manager, filePath, errorMsg);
        case PWIMP_PWSAFE:
            return importPwSafe(manager, filePath, errorMsg);
        default:
            if (errorMsg != nullptr) *errorMsg = "Unknown import format";
            return false;
    }
}

// Merge from another KeePass database
bool PwImport::mergeDatabase(PwManager *targetManager,
                            const QString &sourceFile,
                            const QString &masterPassword,
                            KdbMergeMode mergeMode,
                            QString *errorMsg)
{
    if (targetManager == nullptr) {
        if (errorMsg != nullptr) *errorMsg = "Invalid target manager";
        return false;
    }

    // Create a temporary manager for the source database
    PwManager sourceManager;

    // Set master key and open source database
    int keyResult = sourceManager.setMasterKey(masterPassword, true, "", false, "");
    if (keyResult != static_cast<int>(PwError::SUCCESS)) {
        if (errorMsg != nullptr) *errorMsg = "Failed to set master key for source database";
        return false;
    }

    int openResult = sourceManager.openDatabase(sourceFile, nullptr);
    if (openResult != static_cast<int>(PwError::SUCCESS)) {
        if (errorMsg != nullptr) {
            *errorMsg = QString("Failed to open source database: error %1")
                       .arg(openResult);
        }
        return false;
    }

    bool createNewUUIDs = (mergeMode == KdbMergeMode::CREATE_NEW_UUIDS);
    bool compareTimes = (mergeMode == KdbMergeMode::OVERWRITE_IF_NEWER);

    // Merge groups first
    for (quint32 i = 0; i < sourceManager.getNumberOfGroups(); ++i) {
        const PW_GROUP* srcGroup = sourceManager.getGroup(i);
        if (srcGroup == nullptr) continue;

        if (createNewUUIDs) {
            // Create new group with new ID
            PW_GROUP newGroup = *srcGroup;
            newGroup.uGroupId = Random::generateUInt32();

            // Copy group name
            if (srcGroup->pszGroupName != nullptr) {
                newGroup.pszGroupName = strdup(srcGroup->pszGroupName);
            }

            targetManager->addGroup(&newGroup);
        } else {
            // Look for existing group by ID
            bool found = false;
            for (quint32 j = 0; j < targetManager->getNumberOfGroups(); ++j) {
                const PW_GROUP* tgtGroup = targetManager->getGroup(j);
                if (tgtGroup != nullptr && tgtGroup->uGroupId == srcGroup->uGroupId) {
                    found = true;

                    if (compareTimes) {
                        // Only replace if source is newer
                        if (PwUtil::compareTime(&srcGroup->tLastMod, &tgtGroup->tLastMod) > 0) {
                            PW_GROUP updatedGroup = *srcGroup;
                            if (srcGroup->pszGroupName != nullptr) {
                                updatedGroup.pszGroupName = strdup(srcGroup->pszGroupName);
                            }
                            targetManager->setGroup(j, &updatedGroup);
                        }
                    } else {
                        // Overwrite unconditionally
                        PW_GROUP updatedGroup = *srcGroup;
                        if (srcGroup->pszGroupName != nullptr) {
                            updatedGroup.pszGroupName = strdup(srcGroup->pszGroupName);
                        }
                        targetManager->setGroup(j, &updatedGroup);
                    }
                    break;
                }
            }

            if (!found) {
                // Add new group
                PW_GROUP newGroup = *srcGroup;
                if (srcGroup->pszGroupName != nullptr) {
                    newGroup.pszGroupName = strdup(srcGroup->pszGroupName);
                }
                targetManager->addGroup(&newGroup);
            }
        }
    }

    // Merge entries
    sourceManager.unlockEntryPassword(nullptr);  // Unlock all passwords for reading

    for (quint32 i = 0; i < sourceManager.getNumberOfEntries(); ++i) {
        const PW_ENTRY* srcEntry = sourceManager.getEntry(i);
        if (srcEntry == nullptr) continue;

        if (createNewUUIDs) {
            // Create new entry with new UUID
            PW_ENTRY newEntry;
            memset(&newEntry, 0, sizeof(PW_ENTRY));

            // Generate new UUID
            Random::generateUuid(newEntry.uuid);

            // Copy all fields
            newEntry.uGroupId = srcEntry->uGroupId;
            newEntry.uImageId = srcEntry->uImageId;
            newEntry.tCreation = srcEntry->tCreation;
            newEntry.tLastMod = srcEntry->tLastMod;
            newEntry.tLastAccess = srcEntry->tLastAccess;
            newEntry.tExpire = srcEntry->tExpire;

            if (srcEntry->pszTitle != nullptr) {
                newEntry.pszTitle = strdup(srcEntry->pszTitle);
            }
            if (srcEntry->pszUserName != nullptr) {
                newEntry.pszUserName = strdup(srcEntry->pszUserName);
            }
            if (srcEntry->pszURL != nullptr) {
                newEntry.pszURL = strdup(srcEntry->pszURL);
            }
            if (srcEntry->pszAdditional != nullptr) {
                newEntry.pszAdditional = strdup(srcEntry->pszAdditional);
            }
            if (srcEntry->pszPassword != nullptr) {
                newEntry.uPasswordLen = srcEntry->uPasswordLen;
                newEntry.pszPassword = static_cast<char*>(malloc(newEntry.uPasswordLen + 1));
                memcpy(newEntry.pszPassword, srcEntry->pszPassword, newEntry.uPasswordLen);
                newEntry.pszPassword[newEntry.uPasswordLen] = '\0';
            }

            targetManager->addEntry(&newEntry);
        } else {
            // Look for existing entry by UUID
            bool found = false;
            for (quint32 j = 0; j < targetManager->getNumberOfEntries(); ++j) {
                const PW_ENTRY* tgtEntry = targetManager->getEntry(j);
                if (tgtEntry != nullptr &&
                    memcmp(tgtEntry->uuid, srcEntry->uuid, 16) == 0) {
                    found = true;

                    bool shouldReplace = !compareTimes ||
                        (PwUtil::compareTime(&srcEntry->tLastMod, &tgtEntry->tLastMod) > 0);

                    if (shouldReplace) {
                        // Replace entry
                        PW_ENTRY updatedEntry;
                        memset(&updatedEntry, 0, sizeof(PW_ENTRY));
                        memcpy(updatedEntry.uuid, srcEntry->uuid, 16);

                        updatedEntry.uGroupId = srcEntry->uGroupId;
                        updatedEntry.uImageId = srcEntry->uImageId;
                        updatedEntry.tCreation = srcEntry->tCreation;
                        updatedEntry.tLastMod = srcEntry->tLastMod;
                        updatedEntry.tLastAccess = srcEntry->tLastAccess;
                        updatedEntry.tExpire = srcEntry->tExpire;

                        if (srcEntry->pszTitle != nullptr) {
                            updatedEntry.pszTitle = strdup(srcEntry->pszTitle);
                        }
                        if (srcEntry->pszUserName != nullptr) {
                            updatedEntry.pszUserName = strdup(srcEntry->pszUserName);
                        }
                        if (srcEntry->pszURL != nullptr) {
                            updatedEntry.pszURL = strdup(srcEntry->pszURL);
                        }
                        if (srcEntry->pszAdditional != nullptr) {
                            updatedEntry.pszAdditional = strdup(srcEntry->pszAdditional);
                        }
                        if (srcEntry->pszPassword != nullptr) {
                            updatedEntry.uPasswordLen = srcEntry->uPasswordLen;
                            updatedEntry.pszPassword = static_cast<char*>(malloc(updatedEntry.uPasswordLen + 1));
                            memcpy(updatedEntry.pszPassword, srcEntry->pszPassword, updatedEntry.uPasswordLen);
                            updatedEntry.pszPassword[updatedEntry.uPasswordLen] = '\0';
                        }

                        targetManager->setEntry(j, &updatedEntry);
                    }
                    break;
                }
            }

            if (!found) {
                // Add new entry
                PW_ENTRY newEntry;
                memset(&newEntry, 0, sizeof(PW_ENTRY));
                memcpy(newEntry.uuid, srcEntry->uuid, 16);

                newEntry.uGroupId = srcEntry->uGroupId;
                newEntry.uImageId = srcEntry->uImageId;
                newEntry.tCreation = srcEntry->tCreation;
                newEntry.tLastMod = srcEntry->tLastMod;
                newEntry.tLastAccess = srcEntry->tLastAccess;
                newEntry.tExpire = srcEntry->tExpire;

                if (srcEntry->pszTitle != nullptr) {
                    newEntry.pszTitle = strdup(srcEntry->pszTitle);
                }
                if (srcEntry->pszUserName != nullptr) {
                    newEntry.pszUserName = strdup(srcEntry->pszUserName);
                }
                if (srcEntry->pszURL != nullptr) {
                    newEntry.pszURL = strdup(srcEntry->pszURL);
                }
                if (srcEntry->pszAdditional != nullptr) {
                    newEntry.pszAdditional = strdup(srcEntry->pszAdditional);
                }
                if (srcEntry->pszPassword != nullptr) {
                    newEntry.uPasswordLen = srcEntry->uPasswordLen;
                    newEntry.pszPassword = static_cast<char*>(malloc(newEntry.uPasswordLen + 1));
                    memcpy(newEntry.pszPassword, srcEntry->pszPassword, newEntry.uPasswordLen);
                    newEntry.pszPassword[newEntry.uPasswordLen] = '\0';
                }

                targetManager->addEntry(&newEntry);
            }
        }
    }

    sourceManager.lockEntryPassword(nullptr);

    return true;
}

// Import from CodeWallet format
bool PwImport::importCodeWallet(PwManager *manager, const QString &filePath,
                                QString *errorMsg)
{
    QStringList lines = readLines(filePath, errorMsg);
    if (lines.isEmpty()) {
        return false;
    }

    // Current entry data
    QString title, userName, password, url, notes;
    QString groupName = "Imported";
    bool inNotes = false;
    int importedCount = 0;

    auto saveCurrentEntry = [&]() {
        if (title.isEmpty() && userName.isEmpty() && password.isEmpty()) {
            return;  // Nothing to save
        }

        // Find or create group
        quint32 groupId = findOrCreateGroup(manager, groupName);

        // Create entry
        PW_ENTRY entry;
        memset(&entry, 0, sizeof(PW_ENTRY));

        Random::generateUuid(entry.uuid);
        entry.uGroupId = groupId;
        entry.uImageId = getPreferredIcon(groupName);

        // Set current time for timestamps
        QDateTime now = QDateTime::currentDateTime();
        entry.tCreation.shYear = static_cast<quint16>(now.date().year());
        entry.tCreation.btMonth = static_cast<quint8>(now.date().month());
        entry.tCreation.btDay = static_cast<quint8>(now.date().day());
        entry.tCreation.btHour = static_cast<quint8>(now.time().hour());
        entry.tCreation.btMinute = static_cast<quint8>(now.time().minute());
        entry.tCreation.btSecond = static_cast<quint8>(now.time().second());
        entry.tLastMod = entry.tCreation;
        entry.tLastAccess = entry.tCreation;

        // Set expiration to "never"
        entry.tExpire.shYear = 2999;
        entry.tExpire.btMonth = 12;
        entry.tExpire.btDay = 28;
        entry.tExpire.btHour = 23;
        entry.tExpire.btMinute = 59;
        entry.tExpire.btSecond = 59;

        // Copy strings
        if (!title.isEmpty()) {
            entry.pszTitle = strdup(title.toUtf8().constData());
        } else {
            entry.pszTitle = strdup("Untitled");
        }
        if (!userName.isEmpty()) {
            entry.pszUserName = strdup(userName.toUtf8().constData());
        }
        if (!url.isEmpty()) {
            entry.pszURL = strdup(url.toUtf8().constData());
        }
        if (!notes.isEmpty()) {
            entry.pszAdditional = strdup(notes.toUtf8().constData());
        }
        if (!password.isEmpty()) {
            QByteArray pwBytes = password.toUtf8();
            entry.uPasswordLen = static_cast<quint32>(pwBytes.length());
            entry.pszPassword = static_cast<char*>(malloc(entry.uPasswordLen + 1));
            memcpy(entry.pszPassword, pwBytes.constData(), entry.uPasswordLen);
            entry.pszPassword[entry.uPasswordLen] = '\0';
        }

        manager->addEntry(&entry);
        importedCount++;

        // Reset for next entry
        title.clear();
        userName.clear();
        password.clear();
        url.clear();
        notes.clear();
        inNotes = false;
    };

    // Parse lines
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();

        // Check for entry separator
        if (trimmed == CW_SEPARATOR_OLD || trimmed.startsWith(CW_SEPARATOR_NEW)) {
            saveCurrentEntry();
            continue;
        }

        // Check for title (in square brackets)
        if (trimmed.startsWith('[') && trimmed.endsWith(']')) {
            saveCurrentEntry();
            title = trimmed.mid(1, trimmed.length() - 2);
            continue;
        }

        // Check for known field prefixes
        QString value;

        // Category/Folder -> Group
        if (isFieldPrefix(trimmed, {"Category:", "Folder:"}, &value)) {
            groupName = value;
            // Remove trailing parentheses content if present
            int parenIdx = groupName.indexOf('(');
            if (parenIdx > 0) {
                groupName = groupName.left(parenIdx).trimmed();
            }
            continue;
        }

        // Username variants
        if (isFieldPrefix(trimmed, {"User ID:", "Login:", "Access #:", "System:",
                                    "Content:", "Date:", "Issuer:", "Number:",
                                    "Network:", "Ftp login:"}, &value)) {
            if (userName.isEmpty()) {
                userName = value;
            }
            continue;
        }

        // URL variants
        if (isFieldPrefix(trimmed, {"URL:", "Web site:", "Registered e-mail:"}, &value)) {
            url = value;
            continue;
        }

        // Password/PIN
        if (isFieldPrefix(trimmed, {"Password:", "PIN:"}, &value)) {
            password = value;
            continue;
        }

        // Notes marker
        if (trimmed.startsWith("Notes:")) {
            inNotes = true;
            QString noteContent = trimmed.mid(6).trimmed();
            if (!noteContent.isEmpty()) {
                notes = noteContent;
            }
            continue;
        }

        // If in notes mode or unrecognized line, append to notes
        if (inNotes || (!trimmed.isEmpty() && !title.isEmpty())) {
            if (!notes.isEmpty()) {
                notes += "\n";
            }
            notes += line;
        }
    }

    // Save last entry
    saveCurrentEntry();

    if (errorMsg != nullptr) {
        *errorMsg = QString("Imported %1 entries").arg(importedCount);
    }

    return importedCount > 0;
}

// Import from Password Safe format
bool PwImport::importPwSafe(PwManager *manager, const QString &filePath,
                           QString *errorMsg)
{
    QStringList lines = readLines(filePath, errorMsg);
    if (lines.isEmpty()) {
        return false;
    }

    int importedCount = 0;

    for (const QString &line : lines) {
        if (line.trimmed().isEmpty()) continue;

        // Split by tabs
        QStringList fields = line.split('\t');
        if (fields.size() < 3) continue;  // Need at least group.title, username, password

        // Extract fields
        QString groupTitle = fields[0];
        QString userName = fields.size() > 1 ? fields[1] : QString();
        QString password = fields.size() > 2 ? fields[2] : QString();
        QString notes;

        // Notes field may be quoted
        if (fields.size() > 3) {
            notes = fields[3];
            // Remove surrounding quotes
            if (notes.startsWith('"') && notes.endsWith('"')) {
                notes = notes.mid(1, notes.length() - 2);
            }
        }

        // Split group.title intelligently
        QString groupName, title;
        splitPwSafeTitle(groupTitle, groupName, title);

        // Find or create group
        quint32 groupId = findOrCreateGroup(manager, groupName);

        // Create entry
        PW_ENTRY entry;
        memset(&entry, 0, sizeof(PW_ENTRY));

        Random::generateUuid(entry.uuid);
        entry.uGroupId = groupId;
        entry.uImageId = getPreferredIcon(groupName);

        // Set timestamps
        QDateTime now = QDateTime::currentDateTime();
        entry.tCreation.shYear = static_cast<quint16>(now.date().year());
        entry.tCreation.btMonth = static_cast<quint8>(now.date().month());
        entry.tCreation.btDay = static_cast<quint8>(now.date().day());
        entry.tCreation.btHour = static_cast<quint8>(now.time().hour());
        entry.tCreation.btMinute = static_cast<quint8>(now.time().minute());
        entry.tCreation.btSecond = static_cast<quint8>(now.time().second());
        entry.tLastMod = entry.tCreation;
        entry.tLastAccess = entry.tCreation;

        entry.tExpire.shYear = 2999;
        entry.tExpire.btMonth = 12;
        entry.tExpire.btDay = 28;
        entry.tExpire.btHour = 23;
        entry.tExpire.btMinute = 59;
        entry.tExpire.btSecond = 59;

        // Copy strings
        entry.pszTitle = strdup(title.toUtf8().constData());
        if (!userName.isEmpty()) {
            entry.pszUserName = strdup(userName.toUtf8().constData());
        }
        if (!notes.isEmpty()) {
            entry.pszAdditional = strdup(notes.toUtf8().constData());
        }
        if (!password.isEmpty()) {
            QByteArray pwBytes = password.toUtf8();
            entry.uPasswordLen = static_cast<quint32>(pwBytes.length());
            entry.pszPassword = static_cast<char*>(malloc(entry.uPasswordLen + 1));
            memcpy(entry.pszPassword, pwBytes.constData(), entry.uPasswordLen);
            entry.pszPassword[entry.uPasswordLen] = '\0';
        }

        manager->addEntry(&entry);
        importedCount++;
    }

    if (errorMsg != nullptr) {
        *errorMsg = QString("Imported %1 entries").arg(importedCount);
    }

    return importedCount > 0;
}

// Detect file encoding from BOM
QString PwImport::detectEncoding(const QByteArray &data)
{
    if (data.size() >= 3) {
        // UTF-8 BOM
        if (static_cast<unsigned char>(data[0]) == 0xEF &&
            static_cast<unsigned char>(data[1]) == 0xBB &&
            static_cast<unsigned char>(data[2]) == 0xBF) {
            return "UTF-8";
        }
    }
    if (data.size() >= 2) {
        // Unicode LE BOM
        if (static_cast<unsigned char>(data[0]) == 0xFF &&
            static_cast<unsigned char>(data[1]) == 0xFE) {
            return "UTF-16LE";
        }
        // Unicode BE BOM
        if (static_cast<unsigned char>(data[0]) == 0xFE &&
            static_cast<unsigned char>(data[1]) == 0xFF) {
            return "UTF-16BE";
        }
    }
    return "UTF-8";  // Default to UTF-8
}

// Read lines from file with encoding detection
QStringList PwImport::readLines(const QString &filePath, QString *errorMsg)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMsg != nullptr) {
            *errorMsg = QString("Cannot open file: %1").arg(filePath);
        }
        return QStringList();
    }

    QByteArray data = file.readAll();
    file.close();

    QString encoding = detectEncoding(data);

    // Skip BOM if present
    int offset = 0;
    if (encoding == "UTF-8" && data.size() >= 3 &&
        static_cast<unsigned char>(data[0]) == 0xEF) {
        offset = 3;
    } else if (encoding.startsWith("UTF-16") && data.size() >= 2) {
        offset = 2;
    }

    QString content;
    if (encoding == "UTF-8") {
        content = QString::fromUtf8(data.mid(offset));
    } else if (encoding == "UTF-16LE") {
        content = QString::fromUtf16(reinterpret_cast<const char16_t*>(data.constData() + offset),
                                    (data.size() - offset) / 2);
    } else if (encoding == "UTF-16BE") {
        // Swap bytes for BE
        QByteArray swapped;
        for (int i = offset; i + 1 < data.size(); i += 2) {
            swapped.append(data[i + 1]);
            swapped.append(data[i]);
        }
        content = QString::fromUtf16(reinterpret_cast<const char16_t*>(swapped.constData()),
                                    swapped.size() / 2);
    } else {
        content = QString::fromUtf8(data.mid(offset));
    }

    return content.split('\n');
}

// Find or create a group by name
quint32 PwImport::findOrCreateGroup(PwManager *manager, const QString &groupName)
{
    QString name = groupName.trimmed();
    if (name.isEmpty()) {
        name = "Imported";
    }

    // Look for existing group
    for (quint32 i = 0; i < manager->getNumberOfGroups(); ++i) {
        const PW_GROUP* group = manager->getGroup(i);
        if (group != nullptr && group->pszGroupName != nullptr) {
            if (QString::fromUtf8(group->pszGroupName).compare(name, Qt::CaseInsensitive) == 0) {
                return group->uGroupId;
            }
        }
    }

    // Create new group
    PW_GROUP newGroup;
    memset(&newGroup, 0, sizeof(PW_GROUP));

    newGroup.uGroupId = Random::generateUInt32();
    newGroup.uImageId = getPreferredIcon(name);
    newGroup.usLevel = 0;
    newGroup.pszGroupName = strdup(name.toUtf8().constData());

    // Set timestamps
    QDateTime now = QDateTime::currentDateTime();
    newGroup.tCreation.shYear = static_cast<quint16>(now.date().year());
    newGroup.tCreation.btMonth = static_cast<quint8>(now.date().month());
    newGroup.tCreation.btDay = static_cast<quint8>(now.date().day());
    newGroup.tCreation.btHour = static_cast<quint8>(now.time().hour());
    newGroup.tCreation.btMinute = static_cast<quint8>(now.time().minute());
    newGroup.tCreation.btSecond = static_cast<quint8>(now.time().second());
    newGroup.tLastMod = newGroup.tCreation;
    newGroup.tLastAccess = newGroup.tCreation;

    newGroup.tExpire.shYear = 2999;
    newGroup.tExpire.btMonth = 12;
    newGroup.tExpire.btDay = 28;

    manager->addGroup(&newGroup);
    return newGroup.uGroupId;
}

// Get preferred icon based on group name keywords
quint32 PwImport::getPreferredIcon(const QString &groupName)
{
    QString lower = groupName.toLower();

    if (lower.contains("windows")) return 38;
    if (lower.contains("network")) return 3;
    if (lower.contains("internet") || lower.contains("web")) return 1;
    if (lower.contains("email") || lower.contains("mail")) return 19;
    if (lower.contains("bank") || lower.contains("finance")) return 37;
    if (lower.contains("explorer") || lower.contains("browser")) return 9;

    return 48;  // Default folder icon
}

// Extract field value from line with prefix
QString PwImport::extractField(const QString &line, const QString &prefix)
{
    if (line.startsWith(prefix, Qt::CaseInsensitive)) {
        return line.mid(prefix.length()).trimmed();
    }
    return {};
}

// Check if line matches any of the given prefixes
bool PwImport::isFieldPrefix(const QString &line, const QStringList &prefixes,
                            QString *value)
{
    for (const QString &prefix : prefixes) {
        if (line.startsWith(prefix, Qt::CaseInsensitive)) {
            if (value != nullptr) {
                *value = line.mid(prefix.length()).trimmed();
            }
            return true;
        }
    }
    return false;
}

// Split Password Safe combined group.title field
void PwImport::splitPwSafeTitle(const QString &combined, QString &group, QString &title)
{
    // Password Safe uses dots as separators, but dots can also appear in URLs
    // We need to be smart about splitting

    int lastDot = combined.lastIndexOf('.');
    if (lastDot == -1) {
        // No dot found
        group = "Imported from Password Safe";
        title = combined;
        return;
    }

    // Check if the suffix is a domain extension
    QString suffix = combined.mid(lastDot);
    if (isDomainExtension(suffix)) {
        // This is likely a URL, look for an earlier dot
        int earlierDot = combined.lastIndexOf('.', lastDot - 1);
        if (earlierDot == -1) {
            group = "Imported from Password Safe";
            title = combined;
        } else {
            group = combined.left(earlierDot);
            title = combined.mid(earlierDot + 1);
        }
    } else {
        // Standard split at last dot
        group = combined.left(lastDot);
        title = combined.mid(lastDot + 1);
    }

    // Clean up
    if (group.isEmpty()) {
        group = "Imported from Password Safe";
    }
    if (title.isEmpty()) {
        title = combined;
    }
}

// Check if extension is a domain extension
bool PwImport::isDomainExtension(const QString &ext)
{
    static const QStringList domainExts = {
        ".com", ".org", ".edu", ".net", ".gov", ".mil",
        ".uk", ".de", ".ch", ".at", ".it", ".fr", ".es",
        ".au", ".ca", ".nl", ".be", ".jp", ".cn", ".ru",
        ".co.uk", ".com.au", ".co.nz"
    };

    QString lower = ext.toLower();
    for (const QString &domain : domainExts) {
        if (lower == domain || lower.endsWith(domain)) {
            return true;
        }
    }
    return false;
}
