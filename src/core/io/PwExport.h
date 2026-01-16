/*
  Qt KeePass - Password Export

  Exports password database to various formats (HTML, XML, TXT, CSV).
  Matches MFC CPwExport functionality for format compatibility.
*/

#ifndef PWEXPORT_H
#define PWEXPORT_H

#include <QString>
#include <QFile>
#include "../PwManager.h"
#include "../PwStructs.h"

// Export format constants (matching MFC PWEXP_* defines)
constexpr quint32 PWEXP_NULL = 0;      // No format
constexpr quint32 PWEXP_TXT = 1;       // Plain text
constexpr quint32 PWEXP_HTML = 2;      // HTML table
constexpr quint32 PWEXP_XML = 3;       // XML structured
constexpr quint32 PWEXP_CSV = 4;       // CSV delimited (already implemented via CsvExportDialog)
constexpr quint32 PWEXP_KEEPASS = 5;   // KeePass database format (native save)
constexpr quint32 PWEXP_LAST = 6;      // Boundary constant

// Export field flags (matching MFC PWMF_* defines)
// Note: These are separate from PwFieldFlags which are used for Find
namespace PwExportFlags {
    constexpr quint32 GROUP         = 0x00000001;  // Group name
    constexpr quint32 GROUPTREE     = 0x00000002;  // Group tree path
    constexpr quint32 TITLE         = 0x00000004;  // Entry title
    constexpr quint32 USERNAME      = 0x00000008;  // Username
    constexpr quint32 PASSWORD      = 0x00000010;  // Password
    constexpr quint32 URL           = 0x00000020;  // URL
    constexpr quint32 NOTES         = 0x00000040;  // Notes
    constexpr quint32 UUID          = 0x00000080;  // UUID
    constexpr quint32 IMAGEID       = 0x00000100;  // Icon/Image ID
    constexpr quint32 CREATION      = 0x00000200;  // Creation time
    constexpr quint32 LASTACCESS    = 0x00000400;  // Last access time
    constexpr quint32 LASTMOD       = 0x00000800;  // Last modification time
    constexpr quint32 EXPIRE        = 0x00001000;  // Expiration time
    constexpr quint32 ATTACHMENT    = 0x00002000;  // Binary attachment
    constexpr quint32 ATTACHDESC    = 0x00004000;  // Attachment description

    // Default field selections per format
    constexpr quint32 DEFAULT_TXT  = GROUP | TITLE | USERNAME |
                                     PASSWORD | URL | NOTES;

    constexpr quint32 DEFAULT_HTML = GROUP | TITLE | USERNAME |
                                     PASSWORD | URL | NOTES;

    constexpr quint32 DEFAULT_XML  = GROUP | GROUPTREE | TITLE |
                                     USERNAME | PASSWORD | URL |
                                     NOTES | UUID | IMAGEID |
                                     CREATION | LASTACCESS |
                                     LASTMOD | EXPIRE |
                                     ATTACHMENT | ATTACHDESC;
}

class PwExport
{
public:
    // Export entire database
    static bool exportDatabase(PwManager *manager, const QString &filePath,
                               quint32 format, quint32 fieldFlags);

    // Export specific group (and optionally its children)
    static bool exportGroup(PwManager *manager, quint32 groupId,
                           const QString &filePath, quint32 format,
                           quint32 fieldFlags, bool includeSubgroups = true);

private:
    // Format-specific export methods
    static bool exportToTxt(PwManager *manager, QFile &file,
                           const QList<const PW_ENTRY*> &entries,
                           quint32 fieldFlags);

    static bool exportToHtml(PwManager *manager, QFile &file,
                            const QList<const PW_ENTRY*> &entries,
                            quint32 fieldFlags);

    static bool exportToXml(PwManager *manager, QFile &file,
                           const QList<const PW_ENTRY*> &entries,
                           quint32 fieldFlags);

    // Helper methods
    static void writeUtf8Bom(QFile &file);
    static QString encodeXml(const QString &text);
    static QString encodeHtml(const QString &text);
    static QString formatTime(const PW_TIME &time);
    static QString formatTimeISO8601(const PW_TIME &time);
    static QString getGroupName(PwManager *manager, quint32 groupId);
    static QString getGroupTreePath(PwManager *manager, quint32 groupId);
    static QList<const PW_ENTRY*> getEntriesForGroup(PwManager *manager,
                                                      quint32 groupId,
                                                      bool includeSubgroups);
};

#endif // PWEXPORT_H
