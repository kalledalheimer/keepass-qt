/*
  KeePass Password Safe - Qt Port
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include "core/PwManager.h"
#include "core/util/Random.h"
#include "core/crypto/SHA256.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("KeePass");
    app.setApplicationVersion(PWM_VERSION_STR);
    app.setOrganizationName("KeePass");
    app.setOrganizationDomain("keepass.info");

    // Test basic functionality
    qDebug() << "Qt-KeePass" << PWM_VERSION_STR;
    qDebug() << "Testing core library...";

    // Test random number generation
    QByteArray randomData = Random::generateBytes(16);
    qDebug() << "Generated 16 random bytes:" << randomData.toHex();

    // Test SHA-256 hashing
    QString testString = "Hello, KeePass!";
    QByteArray hash = SHA256::hash(testString.toUtf8());
    qDebug() << "SHA-256 of" << testString << ":" << hash.toHex();

    // Test PwManager initialization
    PwManager pwManager;
    qDebug() << "PwManager created successfully";
    qDebug() << "Default key rounds:" << pwManager.getKeyEncRounds();
    qDebug() << "Algorithm:" << (pwManager.getAlgorithm() == ALGO_AES ? "AES-256" : "Twofish");

    // Create a new database
    pwManager.newDatabase();
    qDebug() << "New database initialized";
    qDebug() << "Number of entries:" << pwManager.getNumberOfEntries();
    qDebug() << "Number of groups:" << pwManager.getNumberOfGroups();

    // Show info dialog
    QString message = QString(
        "Qt-KeePass v%1\n\n"
        "Migration from MFC to Qt - Phase 1 Complete\n\n"
        "✓ Project structure\n"
        "✓ Data structures (KDB format)\n"
        "✓ Cryptography (AES, Twofish, SHA-256)\n"
        "✓ OpenSSL key transformation\n"
        "✓ Cross-platform memory protection\n"
        "✓ PwManager core (basic)\n\n"
        "Next: Implement OpenDatabase/SaveDatabase\n"
        "for full KDB v1.x compatibility"
    ).arg(PWM_VERSION_STR);

    QMessageBox::information(nullptr, "Qt-KeePass", message);

    return 0;
}
