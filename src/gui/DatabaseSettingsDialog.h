/*
  Qt KeePass - Database Settings Dialog
  Reference: MFC/MFC-KeePass/WinGUI/DbSettingsDlg.h
*/

#ifndef DATABASESETTINGSDIALOG_H
#define DATABASESETTINGSDIALOG_H

#include <QDialog>

class QComboBox;
class QLineEdit;
class QSpinBox;
class QPushButton;
class QSlider;
class QCheckBox;
class QLabel;
class PwManager;

/// Database Settings Dialog
class DatabaseSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DatabaseSettingsDialog(PwManager* pwManager, QWidget* parent = nullptr);

    /// Set database file path (for info display)
    void setFilePath(const QString& filePath);

    /// Get selected encryption algorithm (0 = AES, 1 = Twofish)
    int encryptionAlgorithm() const { return m_algorithm; }

    /// Get number of key transformation rounds
    quint32 keyTransformRounds() const { return m_keyRounds; }

    /// Get default username
    QString defaultUsername() const { return m_defaultUsername; }

    /// Get database color (DWORD_MAX if no custom color)
    quint32 databaseColor() const { return m_color; }

    /// Set initial values
    void setEncryptionAlgorithm(int algorithm);
    void setKeyTransformRounds(quint32 rounds);
    void setDefaultUsername(const QString& username);
    void setDatabaseColor(quint32 color);

private slots:
    void onCalculateRounds();
    void onCustomColorToggled(bool checked);
    void onColorSliderChanged(int value);
    void onAccept();

private:
    void setupUI();
    void updateColorPreview();
    void enableControls();
    QColor hsvToRgb(float h, float s, float v);
    float rgbToHue(const QColor& color);

    // UI Components - Info section
    QLabel* m_filePathLabel;
    QLabel* m_fileSizeLabel;
    QLabel* m_groupCountLabel;
    QLabel* m_entryCountLabel;

    // UI Components - Settings
    QComboBox* m_algorithmCombo;
    QSpinBox* m_roundsSpin;
    QPushButton* m_calculateButton;
    QLineEdit* m_usernameEdit;
    QCheckBox* m_customColorCheck;
    QSlider* m_colorSlider;
    QLabel* m_colorPreview;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;

    // Data
    PwManager* m_pwManager;
    QString m_filePath;
    int m_algorithm;
    quint32 m_keyRounds;
    QString m_defaultUsername;
    quint32 m_color;  // DWORD_MAX = no custom color
};

#endif // DATABASESETTINGSDIALOG_H
