/*
  KeePass Password Safe - Qt Port
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "UpdateChecker.h"

#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>

// ============================================================================
// ComponentInfo Implementation
// ============================================================================

QString ComponentInfo::formatVersion(quint64 version)
{
    if (version == 0) {
        return QStringLiteral("?");
    }

    quint16 major = static_cast<quint16>((version >> 48) & 0xFFFF);
    quint16 minor = static_cast<quint16>((version >> 32) & 0xFFFF);
    quint16 build = static_cast<quint16>((version >> 16) & 0xFFFF);
    quint16 revision = static_cast<quint16>(version & 0xFFFF);

    // Omit trailing zeros for cleaner display
    if (revision == 0 && build == 0) {
        return QStringLiteral("%1.%2").arg(major).arg(minor);
    }
    if (revision == 0) {
        return QStringLiteral("%1.%2.%3").arg(major).arg(minor).arg(build);
    }
    return QStringLiteral("%1.%2.%3.%4").arg(major).arg(minor).arg(build).arg(revision);
}

quint64 ComponentInfo::parseVersion(const QString& versionStr)
{
    QStringList parts = versionStr.split(QLatin1Char('.'));
    if (parts.size() < 2) {
        return 0;
    }

    quint64 major = parts.value(0).toULongLong();
    quint64 minor = parts.value(1).toULongLong();
    quint64 build = parts.value(2, QStringLiteral("0")).toULongLong();
    quint64 revision = parts.value(3, QStringLiteral("0")).toULongLong();

    return (major << 48) | (minor << 32) | (build << 16) | revision;
}

// ============================================================================
// UpdateChecker Implementation
// ============================================================================

UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &UpdateChecker::onNetworkReply);
}

UpdateChecker::~UpdateChecker() = default;

void UpdateChecker::checkForUpdates()
{
    // Reset state
    m_hasError = false;
    m_updateCount = 0;
    m_statusMessage.clear();

    // Get list of installed components
    getInstalledComponents();

    // Create network request with appropriate headers
    QNetworkRequest request(QUrl(QString::fromUtf8(VERSION_URL)));
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("Qt-KeePass/%1")
                          .arg(ComponentInfo::formatVersion(CURRENT_VERSION)));

    // Set timeout (5 seconds)
    request.setTransferTimeout(5000);

    // Start async download
    m_networkManager->get(request);
}

void UpdateChecker::onNetworkReply(QNetworkReply* reply)
{
    // Ensure reply is cleaned up
    reply->deleteLater();

    // Check for network errors
    if (reply->error() != QNetworkReply::NoError) {
        m_hasError = true;
        m_statusMessage = tr("Connection failed: %1").arg(reply->errorString());
        emit checkFailed(m_statusMessage);
        return;
    }

    // Read response data
    QByteArray data = reply->readAll();
    if (data.isEmpty()) {
        m_hasError = true;
        m_statusMessage = tr("Empty response from server.");
        emit checkFailed(m_statusMessage);
        return;
    }

    // Parse the version file
    parseVersionFile(data);

    // Compare versions
    compareVersions();

    // Emit completion signal
    emit checkCompleted();
}

void UpdateChecker::getInstalledComponents()
{
    m_components.clear();

    // Add main application
    ComponentInfo kp;
    kp.name = QStringLiteral("KeePass");
    kp.installedVersion = CURRENT_VERSION;
    kp.status = UpdateStatus::Unknown;
    kp.statusText = tr("Checking...");
    kp.statusIcon = 44;  // Question mark icon

    m_components.append(kp);

    // TODO: Add plugins when plugin system provides version info
    // The MFC version also checks plugin versions
}

void UpdateChecker::parseVersionFile(const QByteArray& data)
{
    // Version file format (UTF-8):
    // ComponentName#Major.Minor.Build.Revision
    // Lines starting with # are reserved/comments

    QList<ComponentInfo> available;

    QString content = QString::fromUtf8(data);
    QStringList lines = content.split(QRegularExpression(QStringLiteral("[\r\n]+")),
                                       Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        // Skip empty lines and comments
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }

        // Parse: ComponentName#Version
        int hashIndex = line.indexOf(QLatin1Char('#'));
        if (hashIndex < 1) {
            continue;
        }

        ComponentInfo info;
        info.name = line.left(hashIndex).trimmed();
        QString versionStr = line.mid(hashIndex + 1).trimmed();
        info.availableVersion = ComponentInfo::parseVersion(versionStr);

        if (!info.name.isEmpty() && info.availableVersion > 0) {
            available.append(info);
        }
    }

    // Copy available versions to our installed components
    copyAvailableVersions(available);
}

void UpdateChecker::copyAvailableVersions(const QList<ComponentInfo>& available)
{
    for (int i = 0; i < m_components.size(); ++i) {
        for (const ComponentInfo& av : available) {
            if (av.name.compare(m_components[i].name, Qt::CaseInsensitive) == 0) {
                m_components[i].availableVersion = av.availableVersion;
                break;
            }
        }
    }
}

void UpdateChecker::compareVersions()
{
    m_updateCount = 0;

    for (int i = 0; i < m_components.size(); ++i) {
        ComponentInfo& c = m_components[i];

        quint64 installed = c.installedVersion;
        quint64 available = c.availableVersion;

        if (available == 0) {
            // No version info available
            c.status = UpdateStatus::Unknown;
            c.statusText = tr("Unknown");
            c.statusIcon = 44;  // Question mark
        } else if (installed == available) {
            // Up to date
            c.status = UpdateStatus::UpToDate;
            c.statusText = tr("Up to date");
            c.statusIcon = 53;  // Checkmark
        } else if (installed < available) {
            // Update available
            c.status = UpdateStatus::UpdateAvailable;
            c.statusText = tr("Update available");
            c.statusIcon = 61;  // Exclamation/update icon
            ++m_updateCount;
        } else {
            // Pre-release (installed > available)
            c.status = UpdateStatus::PreRelease;
            c.statusText = tr("Pre-release version");
            c.statusIcon = 39;  // Beta/dev icon
        }
    }

    // Generate status message
    if (m_updateCount == 0) {
        m_statusMessage = tr("No updates available.");
    } else if (m_updateCount == 1) {
        m_statusMessage = tr("1 update is available!");
    } else {
        m_statusMessage = tr("%1 updates are available!").arg(m_updateCount);
    }
}
