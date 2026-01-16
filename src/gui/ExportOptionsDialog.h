/*
  Qt KeePass - Export Options Dialog

  Allows user to select which fields to include in export.
  Reference: MFC GetExportOptions dialog
*/

#ifndef EXPORTOPTIONSDIALOG_H
#define EXPORTOPTIONSDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QDialogButtonBox>
#include "../core/io/PwExport.h"

class ExportOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportOptionsDialog(quint32 exportFormat, QWidget *parent = nullptr);

    [[nodiscard]] quint32 getSelectedFields() const;
    void setSelectedFields(quint32 fields);

private:
    void setupUi();
    void loadDefaultsForFormat();

    // Export format
    quint32 m_exportFormat;

    // Field checkboxes
    QCheckBox *m_cbGroup;
    QCheckBox *m_cbGroupTree;
    QCheckBox *m_cbTitle;
    QCheckBox *m_cbUsername;
    QCheckBox *m_cbPassword;
    QCheckBox *m_cbURL;
    QCheckBox *m_cbNotes;
    QCheckBox *m_cbUUID;
    QCheckBox *m_cbIcon;
    QCheckBox *m_cbCreation;
    QCheckBox *m_cbLastAccess;
    QCheckBox *m_cbLastMod;
    QCheckBox *m_cbExpire;
    QCheckBox *m_cbAttachDesc;
    QCheckBox *m_cbAttachment;

    // Buttons
    QDialogButtonBox *m_buttonBox;
};

#endif // EXPORTOPTIONSDIALOG_H
