/*
  Qt KeePass - Password Generator Dialog
  Reference: MFC/MFC-KeePass/WinGUI/PwGeneratorExDlg.h
*/

#ifndef PASSWORDGENERATORDIALOG_H
#define PASSWORDGENERATORDIALOG_H

#include <QDialog>
#include "../core/PasswordGenerator.h"

class QSpinBox;
class QSlider;
class QLineEdit;
class QCheckBox;
class QPushButton;
class QProgressBar;
class QLabel;

/// Password Generator Dialog
class PasswordGeneratorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PasswordGeneratorDialog(QWidget* parent = nullptr);

    /// Get the generated password
    QString generatedPassword() const { return m_generatedPassword; }

    /// Get current settings
    PasswordGeneratorSettings settings() const;

    /// Set settings (for restoring from saved state)
    void setSettings(const PasswordGeneratorSettings& settings);

private slots:
    void onGenerate();
    void onAccept();
    void onCancel();
    void onLengthChanged(int value);
    void onCharSetChanged();
    void onShowPasswordToggled(bool checked);
    void onPasswordTextChanged();
    void onAdvancedOptions();

private:
    void setupUI();
    void updatePasswordQuality();
    void updateCharSetPreview();

    // UI Components
    QSpinBox* m_lengthSpin;
    QSlider* m_lengthSlider;

    // Character set checkboxes
    QCheckBox* m_upperCaseCheck;
    QCheckBox* m_lowerCaseCheck;
    QCheckBox* m_digitsCheck;
    QCheckBox* m_minusCheck;
    QCheckBox* m_underlineCheck;
    QCheckBox* m_spaceCheck;
    QCheckBox* m_specialCheck;
    QCheckBox* m_bracketsCheck;

    // Custom character set
    QLineEdit* m_customCharSetEdit;

    // Advanced options
    QCheckBox* m_excludeLookAlikeCheck;
    QCheckBox* m_noRepeatCheck;
    QLineEdit* m_excludeCharsEdit;

    // Generated password
    QLineEdit* m_passwordEdit;
    QCheckBox* m_showPasswordCheck;

    // Quality indicator
    QProgressBar* m_qualityBar;
    QLabel* m_qualityLabel;
    QLabel* m_charSetSizeLabel;

    // Buttons
    QPushButton* m_generateButton;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;

    // State
    QString m_generatedPassword;
};

#endif // PASSWORDGENERATORDIALOG_H
