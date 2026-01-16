/*
  Qt KeePass - Mass Modify Dialog

  Allows batch modification of multiple selected entries.
  User can selectively modify: Group, Icon, Expiration, and delete attachments.

  Reference: MFC WinGUI/EntryPropertiesDlg.h
*/

#ifndef MASSMODIFYDIALOG_H
#define MASSMODIFYDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QDateTimeEdit>
#include <QRadioButton>
#include <QLabel>
#include <QList>
#include "../core/PwStructs.h"

// Forward declarations
class PwManager;

class MassModifyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MassModifyDialog(PwManager *pwManager, const QList<quint32>& entryIndices,
                              QWidget *parent = nullptr);
    ~MassModifyDialog() override = default;

    // Get results
    [[nodiscard]] bool modifyGroup() const { return m_modifyGroup; }
    [[nodiscard]] bool modifyIcon() const { return m_modifyIcon; }
    [[nodiscard]] bool modifyExpiration() const { return m_modifyExpiration; }
    [[nodiscard]] bool deleteAttachments() const { return m_deleteAttachments; }

    [[nodiscard]] quint32 getGroupId() const { return m_newGroupId; }
    [[nodiscard]] quint32 getIconId() const { return m_newIconId; }
    [[nodiscard]] bool hasExpiration() const { return m_hasExpiration; }
    [[nodiscard]] PW_TIME getExpirationTime() const { return m_expirationTime; }

private slots:
    void onModifyGroupToggled(bool checked);
    void onModifyIconToggled(bool checked);
    void onModifyExpirationToggled(bool checked);
    void onSelectIcon();
    void onExpires1Week();
    void onExpires2Weeks();
    void onExpires1Month();
    void onExpires3Months();
    void onExpires6Months();
    void onExpires12Months();
    void onExpiresNow();
    void onNoExpiration(bool checked);
    void validateAndAccept();

private:
    void setupUi();
    void populateGroupCombo();
    void setExpireDays(int days);
    void updateExpirationControls();

    // UI components
    QCheckBox *m_checkModifyGroup;
    QComboBox *m_groupCombo;

    QCheckBox *m_checkModifyIcon;
    QPushButton *m_selectIconButton;
    QLabel *m_iconLabel;

    QCheckBox *m_checkModifyExpiration;
    QRadioButton *m_radioExpires;
    QRadioButton *m_radioNoExpiration;
    QDateTimeEdit *m_expirationDateTime;
    QPushButton *m_btn1Week;
    QPushButton *m_btn2Weeks;
    QPushButton *m_btn1Month;
    QPushButton *m_btn3Months;
    QPushButton *m_btn6Months;
    QPushButton *m_btn12Months;
    QPushButton *m_btnNow;

    QCheckBox *m_checkDeleteAttachments;

    QPushButton *m_okButton;
    QPushButton *m_cancelButton;

    // Data
    PwManager *m_pwManager;
    QList<quint32> m_entryIndices;

    // Results
    bool m_modifyGroup;
    bool m_modifyIcon;
    bool m_modifyExpiration;
    bool m_deleteAttachments;

    quint32 m_newGroupId;
    quint32 m_newIconId;
    bool m_hasExpiration;
    PW_TIME m_expirationTime;
};

#endif // MASSMODIFYDIALOG_H
