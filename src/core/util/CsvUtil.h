/*
  Qt KeePass - CSV Utility

  Utilities for exporting and importing CSV files.
  Supports the KeePass CSV format with quoted fields.

  MFC Reference: KeePassLibCpp/DataExchange/PwExport.cpp, PwImport.cpp
*/

#ifndef CSVUTIL_H
#define CSVUTIL_H

#include <QString>
#include <QStringList>
#include <QList>

class PwManager;

/**
 * CSV Export Options
 * Controls which fields to include in the export
 */
struct CsvExportOptions
{
    bool includeGroup;
    bool includeTitle;
    bool includeUsername;
    bool includePassword;
    bool includeUrl;
    bool includeNotes;
    bool includeUuid;
    bool includeCreationTime;
    bool includeLastModTime;
    bool includeLastAccessTime;
    bool includeExpireTime;

    CsvExportOptions()
        : includeGroup(false)
        , includeTitle(true)
        , includeUsername(true)
        , includePassword(true)
        , includeUrl(true)
        , includeNotes(true)
        , includeUuid(false)
        , includeCreationTime(false)
        , includeLastModTime(false)
        , includeLastAccessTime(false)
        , includeExpireTime(false)
    {}
};

/**
 * CSV Import Options
 * Maps CSV columns to entry fields
 */
struct CsvImportOptions
{
    int titleColumn;       // -1 = not mapped
    int usernameColumn;    // -1 = not mapped
    int passwordColumn;    // -1 = not mapped
    int urlColumn;         // -1 = not mapped
    int notesColumn;       // -1 = not mapped
    quint32 targetGroupId; // Group to import into

    CsvImportOptions()
        : titleColumn(0)
        , usernameColumn(1)
        , passwordColumn(2)
        , urlColumn(3)
        , notesColumn(4)
        , targetGroupId(0)
    {}
};

/**
 * CSV Utility Class
 * Handles CSV export and import for KeePass databases
 */
class CsvUtil
{
public:
    /**
     * Export entries to CSV file
     * @param filePath Output file path
     * @param pwManager Database manager
     * @param options Export options
     * @param errorMsg Error message (output)
     * @return true on success, false on failure
     */
    static bool exportToCSV(const QString& filePath,
                           PwManager* pwManager,
                           const CsvExportOptions& options,
                           QString* errorMsg = nullptr);

    /**
     * Import entries from CSV file
     * @param filePath Input file path
     * @param pwManager Database manager
     * @param options Import options
     * @param entriesImported Number of entries imported (output)
     * @param errorMsg Error message (output)
     * @return true on success, false on failure
     */
    static bool importFromCSV(const QString& filePath,
                             PwManager* pwManager,
                             const CsvImportOptions& options,
                             int* entriesImported = nullptr,
                             QString* errorMsg = nullptr);

    /**
     * Parse CSV line into fields
     * Handles quoted fields and embedded commas
     */
    static QStringList parseCsvLine(const QString& line);

    /**
     * Escape CSV field (add quotes if needed, escape internal quotes)
     */
    static QString escapeCsvField(const QString& field);

private:
    CsvUtil(); // Static class, no instances
};

#endif // CSVUTIL_H
