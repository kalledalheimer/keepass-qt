/*
  KeePass Password Safe - Qt Port
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/**
 * @file UpdateChecker.h
 * @brief Network-based application update checker
 *
 * Provides functionality to check for available updates by downloading and
 * parsing a version information file from a remote server.
 *
 * @see UpdateInfoDialog for the UI component
 * @see MFC UpdateCheckEx.cpp for original implementation reference
 */

#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>

/**
 * @brief Status of a component's version check
 *
 * Indicates the relationship between the installed version and the
 * latest available version from the update server.
 */
enum class UpdateStatus : quint8 {
    Unknown,        ///< Version info not available from server
    UpToDate,       ///< Current version matches latest available
    UpdateAvailable,///< Newer version available for download
    PreRelease      ///< Running pre-release (newer than published version)
};

/**
 * @brief Information about a component's version and update status
 *
 * Stores version information for a single component (main application or plugin).
 * Versions are stored as 64-bit packed integers for efficient comparison.
 *
 * @note Version format: (major << 48) | (minor << 32) | (build << 16) | revision
 */
struct ComponentInfo {
    QString name;                   ///< Component name (e.g., "KeePass", plugin name)
    quint64 installedVersion = 0;   ///< Currently installed version (packed format)
    quint64 availableVersion = 0;   ///< Latest available version from server
    UpdateStatus status = UpdateStatus::Unknown;  ///< Version comparison status
    QString statusText;             ///< Human-readable status message
    int statusIcon = 44;            ///< Icon index for status display in UI

    /**
     * @brief Format a packed version number as a string
     * @param version Packed version number (64-bit)
     * @return Formatted string (e.g., "1.43.0.0", "2.0", "1.2.3")
     *
     * Trailing zero components are omitted for cleaner display.
     */
    [[nodiscard]] static QString formatVersion(quint64 version);

    /**
     * @brief Parse a version string to packed 64-bit format
     * @param versionStr Version string (e.g., "1.43.0.0")
     * @return Packed version number, or 0 if parsing fails
     */
    [[nodiscard]] static quint64 parseVersion(const QString& versionStr);
};

/**
 * @brief Network-based application update checker
 *
 * Downloads and parses a version information file from a remote server,
 * compares installed versions with available versions, and reports results
 * asynchronously via Qt signals.
 *
 * @par Usage Example:
 * @code
 * UpdateChecker* checker = new UpdateChecker(this);
 * connect(checker, &UpdateChecker::checkCompleted, this, &MyClass::onUpdateCheckDone);
 * connect(checker, &UpdateChecker::checkFailed, this, &MyClass::onUpdateCheckError);
 * checker->checkForUpdates();
 * @endcode
 *
 * @par Version File Format:
 * The remote version file should contain one component per line:
 * @code
 * ComponentName#Major.Minor.Build.Revision
 * KeePass#1.43.0.0
 * MyPlugin#2.1.0.0
 * @endcode
 *
 * @see UpdateInfoDialog for the UI dialog that displays results
 * @see ComponentInfo for version information structure
 * @note Reference: MFC UpdateCheckEx.cpp
 */
class UpdateChecker : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Construct an update checker
     * @param parent Parent QObject for memory management
     */
    explicit UpdateChecker(QObject* parent = nullptr);

    /** @brief Destructor */
    ~UpdateChecker() override;

    /**
     * @brief Start checking for updates asynchronously
     *
     * Initiates an HTTP request to download the version file.
     * Results are delivered via checkCompleted() or checkFailed() signals.
     *
     * @note Network timeout is 5 seconds
     * @see checkCompleted()
     * @see checkFailed()
     */
    void checkForUpdates();

    /**
     * @brief Get list of checked components
     * @return List of components with version information
     * @note Only valid after checkCompleted() signal
     */
    [[nodiscard]] const QList<ComponentInfo>& components() const { return m_components; }

    /**
     * @brief Get status message describing check results
     * @return Human-readable status (e.g., "No updates available", "1 update is available!")
     * @note Only valid after checkCompleted() or checkFailed() signal
     */
    [[nodiscard]] const QString& statusMessage() const { return m_statusMessage; }

    /**
     * @brief Get number of components with available updates
     * @return Count of components that have newer versions available
     * @note Only valid after checkCompleted() signal
     */
    [[nodiscard]] int updateCount() const { return m_updateCount; }

    /**
     * @brief Check if an error occurred during update check
     * @return true if check failed due to network or parsing error
     */
    [[nodiscard]] bool hasError() const { return m_hasError; }

    /** @brief URL of the remote version information file */
    static constexpr const char* VERSION_URL = "https://www.dalheimer.de/update/version1x.txt";

    /** @brief URL of the download page for updates */
    static constexpr const char* DOWNLOAD_URL = "https://www.dalheimer.de/keepass-qt/download.html";

    /**
     * @brief Current application version as packed 64-bit integer
     *
     * Version 1.43.0.0 = 0x0001002B00000000
     * @note Format: (major << 48) | (minor << 32) | (build << 16) | revision
     */
    static constexpr quint64 CURRENT_VERSION = 0x0001002B00000000ULL; // 1.43.0.0

signals:
    /**
     * @brief Emitted when update check completes successfully
     *
     * After this signal, call components(), statusMessage(), and updateCount()
     * to retrieve results.
     */
    void checkCompleted();

    /**
     * @brief Emitted when update check fails
     * @param errorMessage Description of the error (e.g., "Connection failed")
     */
    void checkFailed(const QString& errorMessage);

private slots:
    void onNetworkReply(QNetworkReply* reply);

private:
    void getInstalledComponents();
    void parseVersionFile(const QByteArray& data);
    void compareVersions();
    void copyAvailableVersions(const QList<ComponentInfo>& available);

    QNetworkAccessManager* m_networkManager;
    QList<ComponentInfo> m_components;
    QString m_statusMessage;
    int m_updateCount = 0;
    bool m_hasError = false;
};

#endif // UPDATECHECKER_H
