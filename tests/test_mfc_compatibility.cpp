/*
  Qt KeePass - MFC Compatibility Test

  Tests that Qt port can successfully open databases created by MFC KeePass.
  This validates bidirectional compatibility: Qt → MFC (validated manually)
  and MFC → Qt (validated here).
*/

#include <QtTest/QtTest>
#include "../src/core/PwManager.h"
#include "../src/core/PwStructs.h"

class TestMfcCompatibility : public QObject
{
    Q_OBJECT

private slots:
    void testOpenMfcGeneratedFile();
};

void TestMfcCompatibility::testOpenMfcGeneratedFile()
{
    // Test file created by MFC KeePass on Windows
    QString testFile = "tests/mfc-reference.kdb";
    QString password = "MFCTestPass123";

    PwManager mgr;

    // Set master key before opening
    mgr.setMasterKey(password, false, QString(), false, QString());

    // Open the MFC-generated database
    int result = mgr.openDatabase(testFile);

    // Verify it opens successfully
    QCOMPARE(result, PWE_SUCCESS);

    // Verify expected content based on VALIDATION.md instructions:
    // - 1 group: "MFC Test Group"
    // - 1 entry:
    //   - Title: "MFC Generated Entry"
    //   - Username: "mfcuser"
    //   - Password: "MFCPassword123"
    //   - URL: "https://mfc.test"
    //   - Notes: "This was created by MFC KeePass"

    quint32 groupCount = mgr.getNumberOfGroups();
    quint32 entryCount = mgr.getNumberOfEntries();

    // Should have at least 1 group and 1 entry
    // (May have additional backup/system groups depending on MFC version)
    QVERIFY(groupCount >= 1);
    QVERIFY(entryCount >= 1);

    // Find the test group
    bool foundGroup = false;
    for (quint32 i = 0; i < groupCount; i++) {
        PW_GROUP* group = mgr.getGroup(i);
        if (group && QString::fromUtf8(group->pszGroupName) == "MFC Test Group") {
            foundGroup = true;
            qInfo() << "✓ Found group:" << group->pszGroupName;
            break;
        }
    }
    QVERIFY2(foundGroup, "Expected group 'MFC Test Group' not found");

    // Find the test entry
    bool foundEntry = false;
    PW_ENTRY* testEntry = nullptr;

    for (quint32 i = 0; i < entryCount; i++) {
        PW_ENTRY* entry = mgr.getEntry(i);
        if (entry && QString::fromUtf8(entry->pszTitle) == "MFC Generated Entry") {
            foundEntry = true;
            testEntry = entry;
            break;
        }
    }
    QVERIFY2(foundEntry, "Expected entry 'MFC Generated Entry' not found");
    QVERIFY(testEntry != nullptr);

    // Verify entry fields
    mgr.unlockEntryPassword(testEntry);

    QString title = QString::fromUtf8(testEntry->pszTitle);
    QString username = QString::fromUtf8(testEntry->pszUserName);
    QString password_field = QString::fromUtf8(testEntry->pszPassword);
    QString url = QString::fromUtf8(testEntry->pszURL);
    QString notes = QString::fromUtf8(testEntry->pszAdditional);

    mgr.lockEntryPassword(testEntry);

    QCOMPARE(title, QString("MFC Generated Entry"));
    QCOMPARE(username, QString("mfcuser"));
    QCOMPARE(password_field, QString("MFCPassword123"));
    QCOMPARE(url, QString("https://mfc.test"));
    QCOMPARE(notes, QString("This was created by MFC KeePass."));

    qInfo() << "✓ All fields match expected values:";
    qInfo() << "  - Title:" << title;
    qInfo() << "  - Username:" << username;
    qInfo() << "  - Password:" << password_field;
    qInfo() << "  - URL:" << url;
    qInfo() << "  - Notes:" << notes;

    // Success! Qt can read MFC-generated files correctly
    qInfo() << "\n✅ MFC → Qt compatibility verified!";
    qInfo() << "   Qt port successfully opens and reads MFC KeePass databases";
}

QTEST_MAIN(TestMfcCompatibility)
#include "test_mfc_compatibility.moc"
