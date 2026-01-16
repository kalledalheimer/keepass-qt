/*
  Qt KeePass - Password Export Implementation

  Reference: MFC KeePassLibCpp/DataExchange/PwExport.cpp
*/

#include "PwExport.h"
#include "../util/PwUtil.h"
#include "../SysDefEx.h"
#include <QTextStream>
#include <QStringConverter>
#include <QDateTime>

// Export entire database
bool PwExport::exportDatabase(PwManager *manager, const QString &filePath,
                              quint32 format, quint32 fieldFlags)
{
    if (manager == nullptr) return false;

    // Get all entries
    QList<const PW_ENTRY*> entries;
    for (quint32 i = 0; i < manager->getNumberOfEntries(); ++i) {
        const PW_ENTRY* entry = manager->getEntry(i);
        if (entry != nullptr) {
            entries.append(entry);
        }
    }

    // Open file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    // Write UTF-8 BOM
    writeUtf8Bom(file);

    // Export based on format
    bool success = false;
    switch (format) {
        case PWEXP_TXT:
            success = exportToTxt(manager, file, entries, fieldFlags);
            break;
        case PWEXP_HTML:
            success = exportToHtml(manager, file, entries, fieldFlags);
            break;
        case PWEXP_XML:
            success = exportToXml(manager, file, entries, fieldFlags);
            break;
        default:
            break;
    }

    file.close();
    return success;
}

// Export specific group
bool PwExport::exportGroup(PwManager *manager, quint32 groupId,
                          const QString &filePath, quint32 format,
                          quint32 fieldFlags, bool includeSubgroups)
{
    if (manager == nullptr) return false;

    // Get entries for this group
    QList<const PW_ENTRY*> entries = getEntriesForGroup(manager, groupId, includeSubgroups);

    // Open file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    // Write UTF-8 BOM
    writeUtf8Bom(file);

    // Export based on format
    bool success = false;
    switch (format) {
        case PWEXP_TXT:
            success = exportToTxt(manager, file, entries, fieldFlags);
            break;
        case PWEXP_HTML:
            success = exportToHtml(manager, file, entries, fieldFlags);
            break;
        case PWEXP_XML:
            success = exportToXml(manager, file, entries, fieldFlags);
            break;
        default:
            break;
    }

    file.close();
    return success;
}

// Export to plain text format
bool PwExport::exportToTxt(PwManager *manager, QFile &file,
                           const QList<const PW_ENTRY*> &entries,
                           quint32 fieldFlags)
{
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // Unlock all passwords for export
    manager->unlockEntryPassword(nullptr);

    for (const PW_ENTRY* entry : entries) {
        if (entry == nullptr) continue;

        // Title in square brackets (always included)
        out << "[" << QString::fromUtf8(entry->pszTitle) << "]\n";

        // Group
        if ((fieldFlags & PwExportFlags::GROUP) != 0) {
            QString groupName = getGroupName(manager, entry->uGroupId);
            out << "Group: " << groupName << "\n";
        }

        // Group Tree
        if ((fieldFlags & PwExportFlags::GROUPTREE) != 0) {
            QString groupTree = getGroupTreePath(manager, entry->uGroupId);
            out << "Group Tree: " << groupTree << "\n";
        }

        // User Name
        if ((fieldFlags & PwExportFlags::USERNAME) != 0) {
            out << "User Name: " << QString::fromUtf8(entry->pszUserName) << "\n";
        }

        // Password
        if ((fieldFlags & PwExportFlags::PASSWORD) != 0) {
            out << "Password: " << QString::fromUtf8(entry->pszPassword) << "\n";
        }

        // URL
        if ((fieldFlags & PwExportFlags::URL) != 0) {
            out << "URL: " << QString::fromUtf8(entry->pszURL) << "\n";
        }

        // Notes
        if ((fieldFlags & PwExportFlags::NOTES) != 0) {
            out << "Notes:\n" << QString::fromUtf8(entry->pszAdditional) << "\n";
        }

        // UUID
        if ((fieldFlags & PwExportFlags::UUID) != 0) {
            QString uuid = PwUtil::uuidToString(entry->uuid);
            out << "UUID: " << uuid << "\n";
        }

        // Icon
        if ((fieldFlags & PwExportFlags::IMAGEID) != 0) {
            out << "Icon: " << entry->uImageId << "\n";
        }

        // Creation Time
        if ((fieldFlags & PwExportFlags::CREATION) != 0) {
            QString timeStr = formatTime(entry->tCreation);
            out << "Creation Time: " << timeStr << "\n";
        }

        // Last Access
        if ((fieldFlags & PwExportFlags::LASTACCESS) != 0) {
            QString timeStr = formatTime(entry->tLastAccess);
            out << "Last Access: " << timeStr << "\n";
        }

        // Last Modification
        if ((fieldFlags & PwExportFlags::LASTMOD) != 0) {
            QString timeStr = formatTime(entry->tLastMod);
            out << "Last Modification: " << timeStr << "\n";
        }

        // Expires
        if ((fieldFlags & PwExportFlags::EXPIRE) != 0) {
            QString timeStr = formatTime(entry->tExpire);
            out << "Expires: " << timeStr << "\n";
        }

        // Attachment Description
        if ((fieldFlags & PwExportFlags::ATTACHDESC) != 0 && entry->pszBinaryDesc != nullptr) {
            out << "Attachment Description: " << QString::fromUtf8(entry->pszBinaryDesc) << "\n";
        }

        // Attachment (Base64)
        if ((fieldFlags & PwExportFlags::ATTACHMENT) != 0 && entry->pBinaryData != nullptr && entry->uBinaryDataLen > 0) {
            QByteArray binaryData(reinterpret_cast<const char*>(entry->pBinaryData),
                                 static_cast<int>(entry->uBinaryDataLen));
            QString base64 = binaryData.toBase64();
            out << "Attachment: " << base64 << "\n";
        }

        // Blank line between entries
        out << "\n";
    }

    // Lock passwords again
    manager->lockEntryPassword(nullptr);

    return true;
}

// Export to HTML format
bool PwExport::exportToHtml(PwManager *manager, QFile &file,
                            const QList<const PW_ENTRY*> &entries,
                            quint32 fieldFlags)
{
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // Unlock all passwords for export
    manager->unlockEntryPassword(nullptr);

    // Write HTML header with CSS styling (matching MFC)
    out << "<!DOCTYPE html>\n";
    out << "<html>\n";
    out << "<head>\n";
    out << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n";
    out << "<meta name=\"GENERATOR\" content=\"Qt KeePass\">\n";
    out << "<style type=\"text/css\">\n";
    out << "<!--\n";
    out << "body, table, th, td, p, input {\n";
    out << "  font-family: Tahoma, Arial, 'Microsoft Sans Serif', 'Noto Sans', Verdana, 'DejaVu Sans', sans-serif;\n";
    out << "  font-size: 10pt;\n";
    out << "}\n";
    out << "table {\n";
    out << "  border-collapse: collapse;\n";
    out << "  width: 100%;\n";
    out << "  hyphens: auto;\n";
    out << "}\n";
    out << "th, td {\n";
    out << "  border: 1px solid #000000;\n";
    out << "  padding: 4px;\n";
    out << "  text-align: left;\n";
    out << "  vertical-align: top;\n";
    out << "}\n";
    out << "th {\n";
    out << "  background-color: #D0D0D0;\n";
    out << "  font-weight: bold;\n";
    out << "}\n";
    out << ".f_password {\n";
    out << "  font-family: 'Courier New', Courier, monospace;\n";
    out << "}\n";
    out << "//-->\n";
    out << "</style>\n";
    out << "<title>Password List</title>\n";
    out << "</head>\n";
    out << "<body>\n\n";

    // Table header
    out << "<table>\n";
    out << "<thead>\n<tr>\n";

    // Column headers based on field flags
    if ((fieldFlags & PwExportFlags::GROUP) != 0) out << "<th>Group</th>\n";
    if ((fieldFlags & PwExportFlags::GROUPTREE) != 0) out << "<th>Group Tree</th>\n";
    if ((fieldFlags & PwExportFlags::TITLE) != 0) out << "<th>Title</th>\n";
    if ((fieldFlags & PwExportFlags::USERNAME) != 0) out << "<th>User Name</th>\n";
    if ((fieldFlags & PwExportFlags::PASSWORD) != 0) out << "<th>Password</th>\n";
    if ((fieldFlags & PwExportFlags::URL) != 0) out << "<th>URL</th>\n";
    if ((fieldFlags & PwExportFlags::NOTES) != 0) out << "<th>Notes</th>\n";
    if ((fieldFlags & PwExportFlags::UUID) != 0) out << "<th>UUID</th>\n";
    if ((fieldFlags & PwExportFlags::IMAGEID) != 0) out << "<th>Icon</th>\n";
    if ((fieldFlags & PwExportFlags::CREATION) != 0) out << "<th>Creation Time</th>\n";
    if ((fieldFlags & PwExportFlags::LASTACCESS) != 0) out << "<th>Last Access</th>\n";
    if ((fieldFlags & PwExportFlags::LASTMOD) != 0) out << "<th>Last Modification</th>\n";
    if ((fieldFlags & PwExportFlags::EXPIRE) != 0) out << "<th>Expires</th>\n";
    if ((fieldFlags & PwExportFlags::ATTACHDESC) != 0) out << "<th>Attachment Desc</th>\n";
    if ((fieldFlags & PwExportFlags::ATTACHMENT) != 0) out << "<th>Attachment</th>\n";

    out << "</tr>\n</thead>\n";

    // Table body
    out << "<tbody>\n";

    for (const PW_ENTRY* entry : entries) {
        if (entry == nullptr) continue;

        out << "<tr>\n";

        // Group
        if ((fieldFlags & PwExportFlags::GROUP) != 0) {
            QString groupName = getGroupName(manager, entry->uGroupId);
            out << "<td>" << encodeHtml(groupName) << "</td>\n";
        }

        // Group Tree
        if ((fieldFlags & PwExportFlags::GROUPTREE) != 0) {
            QString groupTree = getGroupTreePath(manager, entry->uGroupId);
            out << "<td>" << encodeHtml(groupTree) << "</td>\n";
        }

        // Title
        if ((fieldFlags & PwExportFlags::TITLE) != 0) {
            QString title = QString::fromUtf8(entry->pszTitle);
            out << "<td>" << encodeHtml(title) << "</td>\n";
        }

        // User Name
        if ((fieldFlags & PwExportFlags::USERNAME) != 0) {
            QString username = QString::fromUtf8(entry->pszUserName);
            out << "<td>" << encodeHtml(username) << "</td>\n";
        }

        // Password (with monospace styling)
        if ((fieldFlags & PwExportFlags::PASSWORD) != 0) {
            QString password = QString::fromUtf8(entry->pszPassword);
            out << "<td><span class=\"f_password\">" << encodeHtml(password) << "</span></td>\n";
        }

        // URL (as clickable link)
        if ((fieldFlags & PwExportFlags::URL) != 0) {
            QString url = QString::fromUtf8(entry->pszURL);
            if (!url.isEmpty() && (url.contains("://") || url.startsWith("www."))) {
                out << "<td><a href=\"" << encodeHtml(url) << "\">" << encodeHtml(url) << "</a></td>\n";
            } else {
                out << "<td>" << encodeHtml(url) << "</td>\n";
            }
        }

        // Notes (preserve line breaks)
        if ((fieldFlags & PwExportFlags::NOTES) != 0) {
            QString notes = QString::fromUtf8(entry->pszAdditional);
            notes = encodeHtml(notes);
            notes.replace("\n", "<br>\n");
            out << "<td>" << notes << "</td>\n";
        }

        // UUID
        if ((fieldFlags & PwExportFlags::UUID) != 0) {
            QString uuid = PwUtil::uuidToString(entry->uuid);
            out << "<td>" << uuid << "</td>\n";
        }

        // Icon
        if ((fieldFlags & PwExportFlags::IMAGEID) != 0) {
            out << "<td>" << entry->uImageId << "</td>\n";
        }

        // Creation Time
        if ((fieldFlags & PwExportFlags::CREATION) != 0) {
            QString timeStr = formatTime(entry->tCreation);
            out << "<td>" << encodeHtml(timeStr) << "</td>\n";
        }

        // Last Access
        if ((fieldFlags & PwExportFlags::LASTACCESS) != 0) {
            QString timeStr = formatTime(entry->tLastAccess);
            out << "<td>" << encodeHtml(timeStr) << "</td>\n";
        }

        // Last Modification
        if ((fieldFlags & PwExportFlags::LASTMOD) != 0) {
            QString timeStr = formatTime(entry->tLastMod);
            out << "<td>" << encodeHtml(timeStr) << "</td>\n";
        }

        // Expires
        if ((fieldFlags & PwExportFlags::EXPIRE) != 0) {
            QString timeStr = formatTime(entry->tExpire);
            out << "<td>" << encodeHtml(timeStr) << "</td>\n";
        }

        // Attachment Description
        if ((fieldFlags & PwExportFlags::ATTACHDESC) != 0) {
            QString desc = (entry->pszBinaryDesc != nullptr) ?
                          QString::fromUtf8(entry->pszBinaryDesc) : QString();
            out << "<td>" << encodeHtml(desc) << "</td>\n";
        }

        // Attachment (Base64)
        if ((fieldFlags & PwExportFlags::ATTACHMENT) != 0) {
            if (entry->pBinaryData != nullptr && entry->uBinaryDataLen > 0) {
                QByteArray binaryData(reinterpret_cast<const char*>(entry->pBinaryData),
                                     static_cast<int>(entry->uBinaryDataLen));
                QString base64 = binaryData.toBase64();
                out << "<td>" << base64 << "</td>\n";
            } else {
                out << "<td>&nbsp;</td>\n";
            }
        }

        out << "</tr>\n";
    }

    out << "</tbody>\n";
    out << "</table>\n\n";

    // HTML footer
    out << "</body>\n";
    out << "</html>\n";

    // Lock passwords again
    manager->lockEntryPassword(nullptr);

    return true;
}

// Export to XML format
bool PwExport::exportToXml(PwManager *manager, QFile &file,
                           const QList<const PW_ENTRY*> &entries,
                           quint32 fieldFlags)
{
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // Unlock all passwords for export
    manager->unlockEntryPassword(nullptr);

    // XML declaration
    out << "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>\n";
    out << "<pwlist>\n";

    for (const PW_ENTRY* entry : entries) {
        if (entry == nullptr) continue;

        out << "\t<pwentry>\n";

        // Group
        if ((fieldFlags & PwExportFlags::GROUP) != 0) {
            QString groupName = getGroupName(manager, entry->uGroupId);

            // Include tree path as attribute if flag is set
            if ((fieldFlags & PwExportFlags::GROUPTREE) != 0) {
                QString groupTree = getGroupTreePath(manager, entry->uGroupId);
                out << "\t\t<group tree=\"" << encodeXml(groupTree) << "\">";
            } else {
                out << "\t\t<group>";
            }

            out << encodeXml(groupName) << "</group>\n";
        } else if ((fieldFlags & PwExportFlags::GROUPTREE) != 0) {
            // Group tree without group name
            QString groupTree = getGroupTreePath(manager, entry->uGroupId);
            out << "\t\t<grouptree>" << encodeXml(groupTree) << "</grouptree>\n";
        }

        // Title
        if ((fieldFlags & PwExportFlags::TITLE) != 0) {
            QString title = QString::fromUtf8(entry->pszTitle);
            out << "\t\t<title>" << encodeXml(title) << "</title>\n";
        }

        // User Name
        if ((fieldFlags & PwExportFlags::USERNAME) != 0) {
            QString username = QString::fromUtf8(entry->pszUserName);
            out << "\t\t<username>" << encodeXml(username) << "</username>\n";
        }

        // Password
        if ((fieldFlags & PwExportFlags::PASSWORD) != 0) {
            QString password = QString::fromUtf8(entry->pszPassword);
            out << "\t\t<password>" << encodeXml(password) << "</password>\n";
        }

        // URL
        if ((fieldFlags & PwExportFlags::URL) != 0) {
            QString url = QString::fromUtf8(entry->pszURL);
            out << "\t\t<url>" << encodeXml(url) << "</url>\n";
        }

        // Notes
        if ((fieldFlags & PwExportFlags::NOTES) != 0) {
            QString notes = QString::fromUtf8(entry->pszAdditional);
            out << "\t\t<notes>" << encodeXml(notes) << "</notes>\n";
        }

        // UUID
        if ((fieldFlags & PwExportFlags::UUID) != 0) {
            QString uuid = PwUtil::uuidToString(entry->uuid);
            out << "\t\t<uuid>" << uuid << "</uuid>\n";
        }

        // Icon/Image ID
        if ((fieldFlags & PwExportFlags::IMAGEID) != 0) {
            out << "\t\t<image>" << entry->uImageId << "</image>\n";
        }

        // Creation Time
        if ((fieldFlags & PwExportFlags::CREATION) != 0) {
            QString timeStr = formatTimeISO8601(entry->tCreation);
            out << "\t\t<creationtime>" << timeStr << "</creationtime>\n";
        }

        // Last Modification Time
        if ((fieldFlags & PwExportFlags::LASTMOD) != 0) {
            QString timeStr = formatTimeISO8601(entry->tLastMod);
            out << "\t\t<lastmodtime>" << timeStr << "</lastmodtime>\n";
        }

        // Last Access Time
        if ((fieldFlags & PwExportFlags::LASTACCESS) != 0) {
            QString timeStr = formatTimeISO8601(entry->tLastAccess);
            out << "\t\t<lastaccesstime>" << timeStr << "</lastaccesstime>\n";
        }

        // Expiration Time (with expires attribute)
        if ((fieldFlags & PwExportFlags::EXPIRE) != 0) {
            QString timeStr = formatTimeISO8601(entry->tExpire);
            // Check if entry expires (year 2999 means never)
            bool expires = (entry->tExpire.shYear < 2999);
            out << "\t\t<expiretime expires=\"" << (expires ? "true" : "false") << "\">";
            out << timeStr << "</expiretime>\n";
        }

        // Attachment Description
        if ((fieldFlags & PwExportFlags::ATTACHDESC) != 0 && entry->pszBinaryDesc != nullptr) {
            QString desc = QString::fromUtf8(entry->pszBinaryDesc);
            out << "\t\t<attachdesc>" << encodeXml(desc) << "</attachdesc>\n";
        }

        // Attachment (Base64)
        if ((fieldFlags & PwExportFlags::ATTACHMENT) != 0 && entry->pBinaryData != nullptr && entry->uBinaryDataLen > 0) {
            QByteArray binaryData(reinterpret_cast<const char*>(entry->pBinaryData),
                                 static_cast<int>(entry->uBinaryDataLen));
            QString base64 = binaryData.toBase64();
            out << "\t\t<attachment>" << base64 << "</attachment>\n";
        }

        out << "\t</pwentry>\n";
    }

    out << "</pwlist>\n";

    // Lock passwords again
    manager->lockEntryPassword(nullptr);

    return true;
}

// Helper: Write UTF-8 BOM
void PwExport::writeUtf8Bom(QFile &file)
{
    // UTF-8 BOM: 0xEF, 0xBB, 0xBF
    const unsigned char bom[3] = { 0xEF, 0xBB, 0xBF };
    file.write(reinterpret_cast<const char*>(bom), 3);
}

// Helper: Encode XML entities
QString PwExport::encodeXml(const QString &text)
{
    QString result = text;
    result.replace("&", "&amp;");
    result.replace("<", "&lt;");
    result.replace(">", "&gt;");
    result.replace("\"", "&quot;");
    result.replace("'", "&apos;");
    return result;
}

// Helper: Encode HTML entities
QString PwExport::encodeHtml(const QString &text)
{
    QString result = text;
    result.replace("&", "&amp;");
    result.replace("<", "&lt;");
    result.replace(">", "&gt;");
    result.replace("\"", "&quot;");
    return result;
}

// Helper: Format time for display
QString PwExport::formatTime(const PW_TIME &time)
{
    // Format: YYYY-MM-DD HH:MM:SS
    return QString::asprintf("%04d-%02d-%02d %02d:%02d:%02d",
                            time.shYear, time.btMonth, time.btDay,
                            time.btHour, time.btMinute, time.btSecond);
}

// Helper: Format time in ISO 8601 format for XML
QString PwExport::formatTimeISO8601(const PW_TIME &time)
{
    // Format: YYYY-MM-DDTHH:MM:SS
    return QString::asprintf("%04d-%02d-%02dT%02d:%02d:%02d",
                            time.shYear, time.btMonth, time.btDay,
                            time.btHour, time.btMinute, time.btSecond);
}

// Helper: Get group name
QString PwExport::getGroupName(PwManager *manager, quint32 groupId)
{
    for (quint32 i = 0; i < manager->getNumberOfGroups(); ++i) {
        const PW_GROUP* group = manager->getGroup(i);
        if ((group != nullptr) && (group->uGroupId == groupId)) {
            return QString::fromUtf8(group->pszGroupName);
        }
    }
    return QString();
}

// Helper: Get group tree path
QString PwExport::getGroupTreePath(PwManager *manager, quint32 groupId)
{
    // Build path from root to this group using backslash separators (MFC style)
    QStringList pathParts;

    quint32 currentId = groupId;
    while ((currentId != 0) && (currentId != 0xFFFFFFFF)) {
        const PW_GROUP* group = nullptr;

        // Find the group
        for (quint32 i = 0; i < manager->getNumberOfGroups(); ++i) {
            const PW_GROUP* g = manager->getGroup(i);
            if ((g != nullptr) && (g->uGroupId == currentId)) {
                group = g;
                break;
            }
        }

        if (group == nullptr) break;

        // Add to path
        pathParts.prepend(QString::fromUtf8(group->pszGroupName));

        // Find parent group (group with level one less, appearing before this one)
        if (group->usLevel == 0) break;

        quint32 parentId = 0;
        for (quint32 i = 0; i < manager->getNumberOfGroups(); ++i) {
            const PW_GROUP* g = manager->getGroup(i);
            if (g == nullptr) continue;
            if (g->uGroupId == currentId) break;  // We've reached our current group
            if (g->usLevel == group->usLevel - 1) {
                parentId = g->uGroupId;  // This is the parent
            }
        }

        currentId = parentId;
    }

    return pathParts.join("\\");
}

// Helper: Get entries for a group (and optionally its subgroups)
QList<const PW_ENTRY*> PwExport::getEntriesForGroup(PwManager *manager,
                                                     quint32 groupId,
                                                     bool includeSubgroups)
{
    QList<const PW_ENTRY*> result;

    if (!includeSubgroups) {
        // Just this group
        for (quint32 i = 0; i < manager->getNumberOfEntries(); ++i) {
            const PW_ENTRY* entry = manager->getEntry(i);
            if ((entry != nullptr) && (entry->uGroupId == groupId)) {
                result.append(entry);
            }
        }
    } else {
        // This group and all its subgroups
        // First, find the group's level
        const PW_GROUP* selectedGroup = nullptr;
        for (quint32 i = 0; i < manager->getNumberOfGroups(); ++i) {
            const PW_GROUP* group = manager->getGroup(i);
            if ((group != nullptr) && (group->uGroupId == groupId)) {
                selectedGroup = group;
                break;
            }
        }

        if (selectedGroup != nullptr) {
            // Build list of group IDs to include
            QList<quint32> groupIds;
            groupIds.append(groupId);

            // Find subgroups (groups with higher level that come after selected group)
            bool foundSelected = false;
            for (quint32 i = 0; i < manager->getNumberOfGroups(); ++i) {
                const PW_GROUP* group = manager->getGroup(i);
                if (group == nullptr) continue;

                if (group->uGroupId == groupId) {
                    foundSelected = true;
                    continue;
                }

                if (foundSelected) {
                    // If level is higher, it's a subgroup
                    if (group->usLevel > selectedGroup->usLevel) {
                        groupIds.append(group->uGroupId);
                    } else {
                        // Different branch, stop
                        break;
                    }
                }
            }

            // Get all entries from these groups
            for (quint32 i = 0; i < manager->getNumberOfEntries(); ++i) {
                const PW_ENTRY* entry = manager->getEntry(i);
                if ((entry != nullptr) && groupIds.contains(entry->uGroupId)) {
                    result.append(entry);
                }
            }
        }
    }

    return result;
}
