/*
  KeePass Password Safe - Qt Port
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>

/// Status of a component's version check
enum class UpdateStatus : quint8 {
    Unknown,        ///< Version info not available
    UpToDate,       ///< Current version matches latest
    UpdateAvailable,///< Newer version available
    PreRelease      ///< Running pre-release version
};

/// Information about a component's version
struct ComponentInfo {
    QString name;                   ///< Component name (e.g., "KeePass")
    quint64 installedVersion = 0;   ///< Currently installed version
    quint64 availableVersion = 0;   ///< Latest available version
    UpdateStatus status = UpdateStatus::Unknown;
    QString statusText;             ///< Human-readable status
    int statusIcon = 44;            ///< Icon index for status display

    /// Format version as string (e.g., "1.43.0.0")
    [[nodiscard]] static QString formatVersion(quint64 version);

    /// Parse version string to 64-bit format
    [[nodiscard]] static quint64 parseVersion(const QString& versionStr);
};

/// Checks for application updates via network
/// Reference: MFC UpdateCheckEx.cpp
class UpdateChecker : public QObject
{
    Q_OBJECT

public:
    explicit UpdateChecker(QObject* parent = nullptr);
    ~UpdateChecker() override;

    /// Start checking for updates (async)
    void checkForUpdates();

    /// Get list of checked components after check completes
    [[nodiscard]] const QList<ComponentInfo>& components() const { return m_components; }

    /// Get status message after check completes
    [[nodiscard]] const QString& statusMessage() const { return m_statusMessage; }

    /// Get number of available updates
    [[nodiscard]] int updateCount() const { return m_updateCount; }

    /// Check if an error occurred
    [[nodiscard]] bool hasError() const { return m_hasError; }

    /// URL for version information file
    static constexpr const char* VERSION_URL = "https://www.dalheimer.de/update/version1x.txt";

    /// URL for download page
    static constexpr const char* DOWNLOAD_URL = "https://www.dalheimer.de/keepass-qt/download.html";

    /// Current application version as 64-bit packed integer
    /// Format: (major << 48) | (minor << 32) | (build << 16) | revision
    static constexpr quint64 CURRENT_VERSION = 0x0001002B00000000ULL; // 1.43.0.0

signals:
    /// Emitted when update check completes
    void checkCompleted();

    /// Emitted if an error occurs during check
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
