/*
  Qt KeePass - Find Dialog
  Reference: MFC/MFC-KeePass/WinGUI/FindInDbDlg.h
*/

#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>

class QLineEdit;
class QCheckBox;
class QPushButton;
class PwManager;

/// Find/Search dialog for searching entries in the database
class FindDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindDialog(PwManager* pwManager, QWidget* parent = nullptr);

    /// Get the search string entered by user
    QString searchString() const;

    /// Get search flags based on selected checkboxes (PWMF_* flags)
    quint32 searchFlags() const;

    /// Check if case sensitive search is enabled
    bool isCaseSensitive() const;

    /// Check if regular expression search is enabled
    bool isRegexEnabled() const;

    /// Check if backups should be excluded
    bool excludeBackups() const;

    /// Check if expired entries should be excluded
    bool excludeExpired() const;

private slots:
    void onOK();
    void onCancel();

private:
    void setupUI();

    // Widgets
    QLineEdit* m_searchEdit;

    // Field checkboxes
    QCheckBox* m_titleCheck;
    QCheckBox* m_usernameCheck;
    QCheckBox* m_urlCheck;
    QCheckBox* m_passwordCheck;
    QCheckBox* m_notesCheck;
    QCheckBox* m_uuidCheck;
    QCheckBox* m_groupNameCheck;

    // Option checkboxes
    QCheckBox* m_caseSensitiveCheck;
    QCheckBox* m_regexCheck;
    QCheckBox* m_excludeBackupsCheck;
    QCheckBox* m_excludeExpiredCheck;

    QPushButton* m_okButton;
    QPushButton* m_cancelButton;

    PwManager* m_pwManager;
};

#endif // FINDDIALOG_H
