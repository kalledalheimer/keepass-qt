/*
  Qt KeePass - CSV Import Dialog

  Dialog for importing entries from CSV format.

  MFC Reference: WinGUI/ImportOptionsDlg.h (simplified)
*/

#ifndef CSVIMPORTDIALOG_H
#define CSVIMPORTDIALOG_H

#include <QDialog>
#include "../core/util/CsvUtil.h"

class QComboBox;
class QLabel;
class QPushButton;
class PwManager;

class CsvImportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CsvImportDialog(PwManager *pwManager, QWidget *parent = nullptr);
    ~CsvImportDialog();

    // Get selected import options
    CsvImportOptions getImportOptions() const;

private:
    void setupUi();
    void populateGroups();

    PwManager *m_pwManager;
    QComboBox *m_comboGroup;
    QLabel *m_infoLabel;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;
};

#endif // CSVIMPORTDIALOG_H
