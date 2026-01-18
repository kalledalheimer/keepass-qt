/*
  Qt KeePass - Entropy Collector Dialog
  Reference: MFC/MFC-KeePass/WinGUI/GetRandomDlg.h

  Collects random entropy from user mouse movements and keyboard input
  to strengthen the random number generator.
*/

#ifndef ENTROPYCOLLECTORDIALOG_H
#define ENTROPYCOLLECTORDIALOG_H

#include <QDialog>
#include <QByteArray>
#include <QPoint>
#include <QVector>

class QProgressBar;
class QLineEdit;
class QPushButton;
class QLabel;

/// Entropy Collector Dialog
/// Collects randomness from mouse movements and keyboard input
class EntropyCollectorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EntropyCollectorDialog(QWidget* parent = nullptr);

    /// Get the collected entropy (32 bytes of SHA-256 hash)
    QByteArray collectedEntropy() const { return m_entropy; }

protected:
    void mouseMoveEvent(QMouseEvent* event) override;

private slots:
    void onStartMouseCollection();
    void onAccept();
    void onRandomTextChanged();

private:
    void setupUi();
    void finalizeEntropy();

    // UI components
    QLabel* m_instructionLabel;
    QLabel* m_mouseAreaLabel;
    QProgressBar* m_progressBar;
    QLineEdit* m_randomTextEdit;
    QPushButton* m_startButton;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;

    // Collection state
    bool m_collecting;
    QVector<QPoint> m_mousePoints;
    static const int MaxMousePoints = 100;

    // Result
    QByteArray m_entropy;
};

#endif // ENTROPYCOLLECTORDIALOG_H
