/*
  KeePass Password Safe - Qt Port
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "UpdateInfoDialog.h"
#include "IconManager.h"
#include "core/UpdateChecker.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>

UpdateInfoDialog::UpdateInfoDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUi();
}

void UpdateInfoDialog::setupUi()
{
    setWindowTitle(tr("Check for Updates"));
    setMinimumSize(500, 300);
    resize(550, 350);

    auto* mainLayout = new QVBoxLayout(this);

    // Status label at the top
    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    QFont statusFont = m_statusLabel->font();
    statusFont.setBold(true);
    m_statusLabel->setFont(statusFont);
    mainLayout->addWidget(m_statusLabel);

    // Component table
    m_componentTable = new QTableWidget(this);
    m_componentTable->setColumnCount(4);
    m_componentTable->setHorizontalHeaderLabels({
        tr("Component"),
        tr("Installed"),
        tr("Available"),
        tr("Status")
    });

    // Configure table appearance
    m_componentTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_componentTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_componentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_componentTable->setAlternatingRowColors(true);
    m_componentTable->verticalHeader()->setVisible(false);

    // Set column widths
    QHeaderView* header = m_componentTable->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    mainLayout->addWidget(m_componentTable);

    // Info label
    auto* infoLabel = new QLabel(tr("Click 'Visit Website' to download the latest version."), this);
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);

    // Button row
    auto* buttonLayout = new QHBoxLayout();

    m_visitWebsiteButton = new QPushButton(tr("&Visit Website"), this);
    m_visitWebsiteButton->setIcon(IconManager::instance().getEntryIcon(1));  // Internet icon
    connect(m_visitWebsiteButton, &QPushButton::clicked, this, &UpdateInfoDialog::onVisitWebsite);
    buttonLayout->addWidget(m_visitWebsiteButton);

    buttonLayout->addStretch();

    m_closeButton = new QPushButton(tr("&Close"), this);
    m_closeButton->setDefault(true);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_closeButton);

    mainLayout->addLayout(buttonLayout);
}

void UpdateInfoDialog::setUpdateInfo(const QList<ComponentInfo>& components,
                                      const QString& statusMessage,
                                      bool hasError)
{
    // Set status message with appropriate styling
    if (hasError) {
        m_statusLabel->setStyleSheet(QStringLiteral("color: #c62828;"));  // Red for errors
    } else if (statusMessage.contains(tr("available"))) {
        m_statusLabel->setStyleSheet(QStringLiteral("color: #2e7d32;"));  // Green for updates
    } else {
        m_statusLabel->setStyleSheet(QString());  // Default color
    }
    m_statusLabel->setText(statusMessage);

    // Populate component table
    m_componentTable->setRowCount(components.size());

    for (int i = 0; i < components.size(); ++i) {
        const ComponentInfo& c = components[i];

        // Component name with icon
        auto* nameItem = new QTableWidgetItem(c.name);
        nameItem->setIcon(IconManager::instance().getEntryIcon(c.statusIcon));
        m_componentTable->setItem(i, 0, nameItem);

        // Installed version
        auto* installedItem = new QTableWidgetItem(ComponentInfo::formatVersion(c.installedVersion));
        installedItem->setTextAlignment(Qt::AlignCenter);
        m_componentTable->setItem(i, 1, installedItem);

        // Available version
        auto* availableItem = new QTableWidgetItem(ComponentInfo::formatVersion(c.availableVersion));
        availableItem->setTextAlignment(Qt::AlignCenter);
        m_componentTable->setItem(i, 2, availableItem);

        // Status text
        auto* statusItem = new QTableWidgetItem(c.statusText);

        // Color-code status
        switch (c.status) {
        case UpdateStatus::UpToDate:
            statusItem->setForeground(QColor(0x2e, 0x7d, 0x32));  // Green
            break;
        case UpdateStatus::UpdateAvailable:
            statusItem->setForeground(QColor(0x15, 0x65, 0xc0));  // Blue
            statusItem->setFont([statusItem]() {
                QFont f = statusItem->font();
                f.setBold(true);
                return f;
            }());
            break;
        case UpdateStatus::PreRelease:
            statusItem->setForeground(QColor(0xf5, 0x7c, 0x00));  // Orange
            break;
        case UpdateStatus::Unknown:
        default:
            statusItem->setForeground(QColor(0x75, 0x75, 0x75));  // Gray
            break;
        }

        m_componentTable->setItem(i, 3, statusItem);
    }

    // Resize rows to content
    m_componentTable->resizeRowsToContents();
}

void UpdateInfoDialog::onVisitWebsite()
{
    QDesktopServices::openUrl(QUrl(QString::fromUtf8(UpdateChecker::DOWNLOAD_URL)));
}
