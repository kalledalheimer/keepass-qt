/*
  KeePass Password Safe - Qt Port
  Unit Tests for PwManager

  These tests verify the core database functionality including:
  - Database creation
  - Opening KDB v1.x files
  - Entry and group management
  - Encryption/decryption
*/

#include <QtTest/QtTest>
#include <QTemporaryFile>
#include <QFile>
#include <QDir>
#include "../src/core/PwManager.h"
#include "../src/core/PwStructs.h"
#include "../src/core/util/Random.h"
#include "../src/core/util/PwUtil.h"

class TestPwManager : public QObject
{
    Q_OBJECT

private slots:
    // Initialize before all tests
    void initTestCase();

    // Test cases
    void testConstructor();
    void testNewDatabase();
    void testSetMasterKey();
    void testAddGroup();
    void testAddEntry();
    void testSaveAndOpenEmptyDatabase();
    void testSaveAndOpenDatabaseWithData();
    void testPasswordEncryption();
    void testInvalidFileOperations();
    void testKDBXDetection();

    // Cleanup after all tests
    void cleanupTestCase();

private:
    QString m_testDataDir;
    PwManager* createTestManager();
    void verifyDatabaseIntegrity(PwManager* mgr, int expectedGroups, int expectedEntries);
};

void TestPwManager::initTestCase()
{
    qDebug() << "Starting PwManager tests...";
    m_testDataDir = QDir::currentPath() + "/tests/data";

    // Ensure test data directory exists
    QDir().mkpath(m_testDataDir);
}

void TestPwManager::cleanupTestCase()
{
    qDebug() << "PwManager tests completed.";
}

PwManager* TestPwManager::createTestManager()
{
    PwManager* mgr = new PwManager();
    mgr->initPrimaryInstance();
    return mgr;
}

void TestPwManager::verifyDatabaseIntegrity(PwManager* mgr, int expectedGroups, int expectedEntries)
{
    QVERIFY(mgr != nullptr);
    QCOMPARE((int)mgr->getNumberOfGroups(), expectedGroups);
    QCOMPARE((int)mgr->getNumberOfEntries(), expectedEntries);
}

void TestPwManager::testConstructor()
{
    PwManager* mgr = createTestManager();
    QVERIFY(mgr != nullptr);

    // New manager should have no groups/entries
    QCOMPARE((int)mgr->getNumberOfGroups(), 0);
    QCOMPARE((int)mgr->getNumberOfEntries(), 0);

    delete mgr;
}

void TestPwManager::testNewDatabase()
{
    PwManager* mgr = createTestManager();
    mgr->newDatabase();

    // After newDatabase(), should have 0 groups and 0 entries (empty state)
    QVERIFY(mgr->getNumberOfGroups() == 0);
    QVERIFY(mgr->getNumberOfEntries() == 0);

    delete mgr;
}

void TestPwManager::testSetMasterKey()
{
    PwManager* mgr = createTestManager();
    mgr->newDatabase();

    // Set a master password
    QString masterPassword = "TestPassword123!";
    int result = mgr->setMasterKey(masterPassword, false, QString(), false, QString());

    QCOMPARE(result, PWE_SUCCESS);

    delete mgr;
}

void TestPwManager::testAddGroup()
{
    PwManager* mgr = createTestManager();
    mgr->newDatabase();

    // Create a test group
    PW_GROUP group;
    std::memset(&group, 0, sizeof(PW_GROUP));

    QString groupName = "Test Group";
    QByteArray groupNameUtf8 = groupName.toUtf8();
    group.pszGroupName = new char[groupNameUtf8.length() + 1];
    std::strcpy(group.pszGroupName, groupNameUtf8.constData());

    group.uGroupId = 1;
    group.uImageId = 0;
    group.usLevel = 0;

    // Set creation time
    PwUtil::getCurrentTime(&group.tCreation);
    group.tLastMod = group.tCreation;
    group.tLastAccess = group.tCreation;
    PwUtil::getNeverExpireTime(&group.tExpire);

    bool added = mgr->addGroup(&group);
    QVERIFY(added);
    QCOMPARE((int)mgr->getNumberOfGroups(), 1);

    // Verify group was added
    PW_GROUP* retrievedGroup = mgr->getGroup(0);
    QVERIFY(retrievedGroup != nullptr);
    QCOMPARE(QString(retrievedGroup->pszGroupName), groupName);

    delete[] group.pszGroupName;
    delete mgr;
}

void TestPwManager::testAddEntry()
{
    PwManager* mgr = createTestManager();
    mgr->newDatabase();

    // First add a group
    PW_GROUP group;
    std::memset(&group, 0, sizeof(PW_GROUP));
    QString groupName = "Test Group";
    QByteArray groupNameUtf8 = groupName.toUtf8();
    group.pszGroupName = new char[groupNameUtf8.length() + 1];
    std::strcpy(group.pszGroupName, groupNameUtf8.constData());
    group.uGroupId = 1;
    group.uImageId = 0;
    group.usLevel = 0;
    PwUtil::getCurrentTime(&group.tCreation);
    group.tLastMod = group.tCreation;
    group.tLastAccess = group.tCreation;
    PwUtil::getNeverExpireTime(&group.tExpire);

    mgr->addGroup(&group);

    // Now add an entry
    PW_ENTRY entry;
    std::memset(&entry, 0, sizeof(PW_ENTRY));

    entry.uGroupId = 1;
    entry.uImageId = 0;

    QString title = "Test Entry";
    QString username = "testuser";
    QString password = "testpass123";
    QString url = "https://example.com";
    QString notes = "Test notes";

    QByteArray titleUtf8 = title.toUtf8();
    QByteArray usernameUtf8 = username.toUtf8();
    QByteArray passwordUtf8 = password.toUtf8();
    QByteArray urlUtf8 = url.toUtf8();
    QByteArray notesUtf8 = notes.toUtf8();

    entry.pszTitle = new char[titleUtf8.length() + 1];
    entry.pszUserName = new char[usernameUtf8.length() + 1];
    entry.pszPassword = new char[passwordUtf8.length() + 1];
    entry.pszURL = new char[urlUtf8.length() + 1];
    entry.pszAdditional = new char[notesUtf8.length() + 1];
    entry.pszBinaryDesc = new char[1];

    std::strcpy(entry.pszTitle, titleUtf8.constData());
    std::strcpy(entry.pszUserName, usernameUtf8.constData());
    std::strcpy(entry.pszPassword, passwordUtf8.constData());
    std::strcpy(entry.pszURL, urlUtf8.constData());
    std::strcpy(entry.pszAdditional, notesUtf8.constData());
    entry.pszBinaryDesc[0] = '\0';

    entry.uPasswordLen = passwordUtf8.length();
    entry.pBinaryData = nullptr;
    entry.uBinaryDataLen = 0;

    // Generate UUID
    Random::fillBuffer(entry.uuid, 16);

    PwUtil::getCurrentTime(&entry.tCreation);
    entry.tLastMod = entry.tCreation;
    entry.tLastAccess = entry.tCreation;
    PwUtil::getNeverExpireTime(&entry.tExpire);

    bool added = mgr->addEntry(&entry);
    QVERIFY(added);
    QCOMPARE((int)mgr->getNumberOfEntries(), 1);

    // Verify entry was added
    PW_ENTRY* retrievedEntry = mgr->getEntry(0);
    QVERIFY(retrievedEntry != nullptr);
    QCOMPARE(QString(retrievedEntry->pszTitle), title);
    QCOMPARE(QString(retrievedEntry->pszUserName), username);

    // Cleanup
    delete[] group.pszGroupName;
    delete[] entry.pszTitle;
    delete[] entry.pszUserName;
    delete[] entry.pszPassword;
    delete[] entry.pszURL;
    delete[] entry.pszAdditional;
    delete[] entry.pszBinaryDesc;
    delete mgr;
}

void TestPwManager::testSaveAndOpenEmptyDatabase()
{
    QString testFile = m_testDataDir + "/test_empty.kdb";

    // Remove old test file if it exists
    QFile::remove(testFile);

    // Create and save an empty database
    PwManager* mgr1 = createTestManager();
    mgr1->newDatabase();

    QString masterPassword = "TestPassword123!";
    mgr1->setMasterKey(masterPassword, false, QString(), false, QString());

    int saveResult = mgr1->saveDatabase(testFile);
    QCOMPARE(saveResult, PWE_SUCCESS);

    // Verify file was created
    QVERIFY(QFile::exists(testFile));

    delete mgr1;

    // Open the database
    PwManager* mgr2 = createTestManager();
    mgr2->setMasterKey(masterPassword, false, QString(), false, QString());

    int openResult = mgr2->openDatabase(testFile);
    QCOMPARE(openResult, PWE_SUCCESS);

    // Verify empty database
    verifyDatabaseIntegrity(mgr2, 0, 0);

    delete mgr2;
    QFile::remove(testFile);
}

void TestPwManager::testSaveAndOpenDatabaseWithData()
{
    QString testFile = m_testDataDir + "/test_with_data.kdb";
    QFile::remove(testFile);

    // Create database with data
    PwManager* mgr1 = createTestManager();
    mgr1->newDatabase();

    QString masterPassword = "TestPassword456!";
    mgr1->setMasterKey(masterPassword, false, QString(), false, QString());

    // Add a group
    PW_GROUP group;
    std::memset(&group, 0, sizeof(PW_GROUP));
    QString groupName = "My Group";
    QByteArray groupNameUtf8 = groupName.toUtf8();
    group.pszGroupName = new char[groupNameUtf8.length() + 1];
    std::strcpy(group.pszGroupName, groupNameUtf8.constData());
    group.uGroupId = 1;
    group.uImageId = 0;
    group.usLevel = 0;
    PwUtil::getCurrentTime(&group.tCreation);
    group.tLastMod = group.tCreation;
    group.tLastAccess = group.tCreation;
    PwUtil::getNeverExpireTime(&group.tExpire);
    mgr1->addGroup(&group);

    // Add an entry
    PW_ENTRY entry;
    std::memset(&entry, 0, sizeof(PW_ENTRY));
    entry.uGroupId = 1;
    entry.uImageId = 0;

    QString title = "My Entry";
    QString username = "myuser";
    QString password = "mypass";
    QString url = "https://test.com";
    QString notes = "My notes";

    QByteArray titleUtf8 = title.toUtf8();
    QByteArray usernameUtf8 = username.toUtf8();
    QByteArray passwordUtf8 = password.toUtf8();
    QByteArray urlUtf8 = url.toUtf8();
    QByteArray notesUtf8 = notes.toUtf8();

    entry.pszTitle = new char[titleUtf8.length() + 1];
    entry.pszUserName = new char[usernameUtf8.length() + 1];
    entry.pszPassword = new char[passwordUtf8.length() + 1];
    entry.pszURL = new char[urlUtf8.length() + 1];
    entry.pszAdditional = new char[notesUtf8.length() + 1];
    entry.pszBinaryDesc = new char[1];

    std::strcpy(entry.pszTitle, titleUtf8.constData());
    std::strcpy(entry.pszUserName, usernameUtf8.constData());
    std::strcpy(entry.pszPassword, passwordUtf8.constData());
    std::strcpy(entry.pszURL, urlUtf8.constData());
    std::strcpy(entry.pszAdditional, notesUtf8.constData());
    entry.pszBinaryDesc[0] = '\0';

    entry.uPasswordLen = passwordUtf8.length();
    entry.pBinaryData = nullptr;
    entry.uBinaryDataLen = 0;

    Random::fillBuffer(entry.uuid, 16);
    PwUtil::getCurrentTime(&entry.tCreation);
    entry.tLastMod = entry.tCreation;
    entry.tLastAccess = entry.tCreation;
    PwUtil::getNeverExpireTime(&entry.tExpire);

    mgr1->addEntry(&entry);

    // Save database
    int saveResult = mgr1->saveDatabase(testFile);
    QCOMPARE(saveResult, PWE_SUCCESS);
    QVERIFY(QFile::exists(testFile));

    // Cleanup first manager
    delete[] group.pszGroupName;
    delete[] entry.pszTitle;
    delete[] entry.pszUserName;
    delete[] entry.pszPassword;
    delete[] entry.pszURL;
    delete[] entry.pszAdditional;
    delete[] entry.pszBinaryDesc;
    delete mgr1;

    // Open and verify
    PwManager* mgr2 = createTestManager();
    mgr2->setMasterKey(masterPassword, false, QString(), false, QString());

    int openResult = mgr2->openDatabase(testFile);
    QCOMPARE(openResult, PWE_SUCCESS);

    // Verify data
    verifyDatabaseIntegrity(mgr2, 1, 1);

    PW_GROUP* retrievedGroup = mgr2->getGroup(0);
    QVERIFY(retrievedGroup != nullptr);
    QCOMPARE(QString(retrievedGroup->pszGroupName), groupName);

    PW_ENTRY* retrievedEntry = mgr2->getEntry(0);
    QVERIFY(retrievedEntry != nullptr);
    QCOMPARE(QString(retrievedEntry->pszTitle), title);
    QCOMPARE(QString(retrievedEntry->pszUserName), username);
    QCOMPARE(QString(retrievedEntry->pszURL), url);
    QCOMPARE(QString(retrievedEntry->pszAdditional), notes);

    delete mgr2;
    QFile::remove(testFile);
}

void TestPwManager::testPasswordEncryption()
{
    PwManager* mgr = createTestManager();
    mgr->newDatabase();

    // Add entry with password
    PW_ENTRY entry;
    std::memset(&entry, 0, sizeof(PW_ENTRY));
    entry.uGroupId = 1;  // Must be non-zero and not DWORD_MAX
    entry.uImageId = 0;

    QString password = "SecretPassword123!";
    QByteArray passwordUtf8 = password.toUtf8();
    entry.pszPassword = new char[passwordUtf8.length() + 1];
    std::strcpy(entry.pszPassword, passwordUtf8.constData());
    entry.uPasswordLen = passwordUtf8.length();

    // Initialize other required fields
    entry.pszTitle = new char[10];
    std::strcpy(entry.pszTitle, "Test");
    entry.pszUserName = new char[10];
    std::strcpy(entry.pszUserName, "user");
    entry.pszURL = new char[1];
    entry.pszURL[0] = '\0';
    entry.pszAdditional = new char[1];
    entry.pszAdditional[0] = '\0';
    entry.pszBinaryDesc = new char[1];
    entry.pszBinaryDesc[0] = '\0';
    entry.pBinaryData = nullptr;
    entry.uBinaryDataLen = 0;

    Random::fillBuffer(entry.uuid, 16);
    PwUtil::getCurrentTime(&entry.tCreation);
    entry.tLastMod = entry.tCreation;
    entry.tLastAccess = entry.tCreation;
    PwUtil::getNeverExpireTime(&entry.tExpire);

    mgr->addEntry(&entry);

    PW_ENTRY* storedEntry = mgr->getEntry(0);
    QVERIFY(storedEntry != nullptr);

    // Password is already locked by setEntry(), so it should be encrypted
    // We need to unlock it first to see the original
    mgr->unlockEntryPassword(storedEntry);
    QString decryptedPassword(storedEntry->pszPassword);
    QCOMPARE(decryptedPassword, password);

    // Now lock it again
    mgr->lockEntryPassword(storedEntry);

    // Encrypted password should differ from original
    QString encryptedPassword(storedEntry->pszPassword);
    QVERIFY(encryptedPassword != password);

    // Unlock again should restore original
    mgr->unlockEntryPassword(storedEntry);
    QString decryptedPassword2(storedEntry->pszPassword);
    QCOMPARE(decryptedPassword2, password);

    // Cleanup
    delete[] entry.pszTitle;
    delete[] entry.pszUserName;
    delete[] entry.pszPassword;
    delete[] entry.pszURL;
    delete[] entry.pszAdditional;
    delete[] entry.pszBinaryDesc;
    delete mgr;
}

void TestPwManager::testInvalidFileOperations()
{
    PwManager* mgr = createTestManager();

    // Try to open non-existent file
    int result = mgr->openDatabase("/nonexistent/path/file.kdb");
    QVERIFY(result != PWE_SUCCESS);
    QCOMPARE(result, PWE_NOFILEACCESS_READ);

    // Try to open empty path
    result = mgr->openDatabase(QString());
    QCOMPARE(result, PWE_INVALID_PARAM);

    delete mgr;
}

void TestPwManager::testKDBXDetection()
{
    // Create a fake KDBX file (KeePass 2.x format)
    QString testFile = m_testDataDir + "/test_fake_kdbx.kdbx";
    QFile file(testFile);

    if (file.open(QIODevice::WriteOnly)) {
        QDataStream stream(&file);
        stream.setByteOrder(QDataStream::LittleEndian);

        // Write KDBX pre-release signature
        stream << (quint32)PWM_DBSIG_1_KDBX_P;
        stream << (quint32)PWM_DBSIG_2_KDBX_P;

        // Fill rest with zeros
        for (int i = 0; i < 116; ++i) {
            stream << (quint8)0;
        }

        file.close();
    }

    // Try to open KDBX file - should be rejected
    PwManager* mgr = createTestManager();
    int result = mgr->openDatabase(testFile);

    QVERIFY(result != PWE_SUCCESS);
    QCOMPARE(result, PWE_UNSUPPORTED_KDBX);

    delete mgr;
    QFile::remove(testFile);
}

QTEST_MAIN(TestPwManager)
#include "test_pwmanager.moc"
