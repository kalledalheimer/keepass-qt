/*
  Qt KeePass - Add Entry Dialog

  Dialog for creating new password entries.
  Matches MFC KeePass AddEntryDlg functionality.
*/

#ifndef ADDENTRYDIALOG_H
#define ADDENTRYDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QPushButton>
#include "../core/PwStructs.h"

// Forward declarations
class PwManager;

class AddEntryDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        AddMode,
        EditMode
    };

    // Constructor
    // For AddMode: idValue is the selectedGroupId
    // For EditMode: idValue is the entryIndex
    explicit AddEntryDialog(PwManager *pwManager, Mode mode, quint32 idValue = 0, QWidget *parent = nullptr);

    ~AddEntryDialog() override = default;

    // Get mode
    Mode getMode() const { return m_mode; }
    quint32 getEntryIndex() const { return m_entryIndex; }

    // Getters for dialog results
    QString getTitle() const;
    QString getUsername() const;
    QString getPassword() const;
    QString getUrl() const;
    QString getNotes() const;
    quint32 getGroupId() const;
    quint32 getIconId() const;
    PW_TIME getExpirationTime() const;
    bool hasExpiration() const;

    // Attachment getters
    QString getAttachmentPath() const { return m_attachmentPath; }
    bool isAttachmentModified() const { return m_attachmentModified; }

private slots:
    void onPasswordChanged();
    void onShowPasswordToggled(bool checked);
    void onExpiresToggled(bool checked);
    void validateAndAccept();

    // Attachment slots
    void onSetAttachment();
    void onSaveAttachment();
    void onRemoveAttachment();

private:
    void setupUi();
    bool validateInput();
    void populateGroupCombo();
    void populateFromEntry(PW_ENTRY *entry);
    QString generateRandomPassword();
    void updateAttachmentControls();

    // UI components
    QComboBox *m_groupCombo;
    QSpinBox *m_iconIdSpin;
    QLineEdit *m_titleEdit;
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QLineEdit *m_repeatPasswordEdit;
    QCheckBox *m_showPasswordCheck;
    QLineEdit *m_urlEdit;
    QTextEdit *m_notesEdit;
    QCheckBox *m_expiresCheck;
    QDateTimeEdit *m_expirationDateTime;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;

    // Attachment UI
    QLineEdit *m_attachmentEdit;
    QPushButton *m_setAttachmentButton;
    QPushButton *m_saveAttachmentButton;
    QPushButton *m_removeAttachmentButton;

    // Data
    PwManager *m_pwManager;
    Mode m_mode;
    quint32 m_selectedGroupId;  // For Add mode
    quint32 m_entryIndex;       // For Edit mode

    // Attachment state
    QString m_attachmentPath;      // Path to file when setting new attachment
    QString m_originalAttachment;  // Original attachment description (for edit mode)
    bool m_attachmentModified;     // Track if attachment was changed
};

#endif // ADDENTRYDIALOG_H
