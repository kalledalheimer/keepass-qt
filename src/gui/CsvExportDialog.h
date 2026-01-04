/*
  Qt KeePass - CSV Export Dialog

  Dialog for selecting fields to export to CSV format.

  MFC Reference: WinGUI/ExportOptionsDlg.h (simplified)
*/

#ifndef CSVEXPORTDIALOG_H
#define CSVEXPORTDIALOG_H

#include <QDialog>
#include "../core/util/CsvUtil.h"

class QCheckBox;
class QPushButton;

class CsvExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CsvExportDialog(QWidget *parent = nullptr);
    ~CsvExportDialog();

    // Get selected export options
    CsvExportOptions getExportOptions() const;

private:
    void setupUi();

    // Field selection checkboxes
    QCheckBox *m_checkGroup;
    QCheckBox *m_checkTitle;
    QCheckBox *m_checkUsername;
    QCheckBox *m_checkPassword;
    QCheckBox *m_checkUrl;
    QCheckBox *m_checkNotes;
    QCheckBox *m_checkUuid;
    QCheckBox *m_checkCreationTime;
    QCheckBox *m_checkLastModTime;
    QCheckBox *m_checkLastAccessTime;
    QCheckBox *m_checkExpireTime;

    // Buttons
    QPushButton *m_btnSelectAll;
    QPushButton *m_btnDeselectAll;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;

private slots:
    void onSelectAll();
    void onDeselectAll();
};

#endif // CSVEXPORTDIALOG_H
