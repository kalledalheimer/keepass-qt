/*
  KeePass Password Safe - Qt Port
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef UPDATEINFODIALOG_H
#define UPDATEINFODIALOG_H

#include <QDialog>
#include <QList>

class QLabel;
class QTableWidget;
class QPushButton;
struct ComponentInfo;

/// Dialog displaying update check results
/// Reference: MFC UpdateInfoDlg.cpp
class UpdateInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UpdateInfoDialog(QWidget* parent = nullptr);

    /// Initialize with component list and status message
    void setUpdateInfo(const QList<ComponentInfo>& components,
                       const QString& statusMessage,
                       bool hasError);

private slots:
    void onVisitWebsite();

private:
    void setupUi();

    QLabel* m_statusLabel;
    QTableWidget* m_componentTable;
    QPushButton* m_visitWebsiteButton;
    QPushButton* m_closeButton;
};

#endif // UPDATEINFODIALOG_H
