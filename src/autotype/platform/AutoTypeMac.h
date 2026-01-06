/*
  Qt KeePass - macOS Auto-Type Implementation

  Uses Core Graphics CGEvent APIs for keyboard simulation.

  macOS requires accessibility permissions to send keyboard events.
  Users must grant permission in System Preferences > Security & Privacy >
  Privacy > Accessibility.

  Reference: MFC KeePassLibCpp/Util/AppUtil.cpp (CSendKeysEx)
*/

#ifndef AUTOTYPEMAC_H
#define AUTOTYPEMAC_H

#include "AutoTypePlatform.h"

#ifdef Q_OS_MAC

class AutoTypeMac : public AutoTypePlatform
{
public:
    AutoTypeMac();
    ~AutoTypeMac() override;

    bool performAutoType(const QList<AutoTypeAction>& actions,
                        int defaultDelay = 10) override;

    void releaseModifiers() override;

    bool isAvailable() const override;

    QString lastError() const override;

private:
    void sendKeyPress(AutoTypeKey key);
    void sendKeyDown(AutoTypeKey key);
    void sendKeyUp(AutoTypeKey key);
    void sendText(const QString& text);
    void delay(int ms);

    // Convert AutoTypeKey to CGKeyCode
    quint16 keyCodeForAutoTypeKey(AutoTypeKey key) const;

    // Convert Unicode character to CGKeyCode
    quint16 keyCodeForCharacter(QChar ch, bool& needsShift) const;

    QString m_lastError;
};

#endif // Q_OS_MAC

#endif // AUTOTYPEMAC_H
