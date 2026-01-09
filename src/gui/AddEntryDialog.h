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
    enum Mode : quint8 {
        AddMode,
        EditMode
    };

    // Constructor
    // For AddMode: idValue is the selectedGroupId
    // For EditMode: idValue is the entryIndex
    explicit AddEntryDialog(PwManager *pwManager, Mode mode, quint32 idValue = 0, QWidget *parent = nullptr);

    ~AddEntryDialog() override = default;

    // Get mode
    [[nodiscard]] Mode getMode() const { return m_mode; }
    [[nodiscard]] quint32 getEntryIndex() const { return m_entryIndex; }

    // Getters for dialog results
    [[nodiscard]] QString getTitle() const;
    [[nodiscard]] QString getUsername() const;
    [[nodiscard]] QString getPassword() const;
    [[nodiscard]] QString getUrl() const;
    [[nodiscard]] QString getNotes() const;
    [[nodiscard]] quint32 getGroupId() const;
    [[nodiscard]] quint32 getIconId() const;
    [[nodiscard]] PW_TIME getExpirationTime() const;
    [[nodiscard]] bool hasExpiration() const;

    // Attachment getters
    [[nodiscard]] QString getAttachmentPath() const { return m_attachmentPath; }
    [[nodiscard]] bool isAttachmentModified() const { return m_attachmentModified; }

    // Auto-type getters
    [[nodiscard]] QString getAutoTypeSequence() const;
    [[nodiscard]] QString getAutoTypeWindow() const;

private slots:
    void onPasswordChanged();
    void onShowPasswordToggled(bool checked);
    void onExpiresToggled(bool checked);
    void validateAndAccept();

    // Attachment slots
    void onSetAttachment();
    void onSaveAttachment();
    void onRemoveAttachment();

    // Auto-type slots
    void onInsertDefaultSequence();
    void onSelectTargetWindow();

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

    // Auto-type UI
    QLineEdit *m_autoTypeSequenceEdit;
    QLineEdit *m_autoTypeWindowEdit;
    QPushButton *m_insertDefaultSeqButton;
    QPushButton *m_selectWindowButton;

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
