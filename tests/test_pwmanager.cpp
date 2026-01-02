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
#include "../src/core/PasswordGenerator.h"

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
    void testDeleteEntry();
    void testDeleteGroup();
    void testBackupEntry();
    void testGetGroupId();
    void testFind();
    void testFindAll();
    void testFindExcludeBackups();
    void testFindExcludeExpired();

    // Password Generator tests
    void testPasswordGeneratorBasic();
    void testPasswordGeneratorCharSets();
    void testPasswordGeneratorExclusions();
    void testPasswordGeneratorNoRepeat();
    void testPasswordGeneratorEntropy();
    void testPasswordGeneratorQuality();
    void testPasswordGeneratorSettingsValidation();

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

    // Create and save a database with one empty group (minimal valid database)
    PwManager* mgr1 = createTestManager();
    mgr1->newDatabase();

    QString masterPassword = "TestPassword123!";
    mgr1->setMasterKey(masterPassword, false, QString(), false, QString());

    // Add a default group (KeePass databases require at least one group)
    PW_GROUP group;
    std::memset(&group, 0, sizeof(PW_GROUP));
    group.uGroupId = 1;
    group.pszGroupName = new char[8];
    std::strcpy(group.pszGroupName, "General");
    group.uImageId = 1;
    PwManager::getNeverExpireTime(&group.tExpire);
    PwManager::getNeverExpireTime(&group.tCreation);
    PwManager::getNeverExpireTime(&group.tLastMod);
    PwManager::getNeverExpireTime(&group.tLastAccess);
    mgr1->addGroup(&group);
    delete[] group.pszGroupName;

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

    // Verify database has one group, no entries
    verifyDatabaseIntegrity(mgr2, 1, 0);

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

void TestPwManager::testDeleteEntry()
{
    PwManager* mgr = createTestManager();
    mgr->newDatabase();
    mgr->setMasterKey("test", false, "", true, "");

    // Add a group
    PW_GROUP group;
    std::memset(&group, 0, sizeof(PW_GROUP));
    group.pszGroupName = const_cast<char*>("Test Group");
    group.uGroupId = 1;
    PwUtil::dateTimeToPwTime(QDateTime::currentDateTime(), &group.tCreation);
    group.tLastAccess = group.tCreation;
    group.tLastMod = group.tCreation;
    PwManager::getNeverExpireTime(&group.tExpire);
    group.uImageId = 1;
    QVERIFY(mgr->addGroup(&group));

    // Add multiple entries
    for (int i = 0; i < 3; ++i) {
        PW_ENTRY entry;
        std::memset(&entry, 0, sizeof(PW_ENTRY));
        entry.uGroupId = 1;
        entry.uImageId = 0;
        entry.pszTitle = const_cast<char*>(QString("Entry %1").arg(i).toUtf8().constData());
        entry.pszUserName = const_cast<char*>("user");
        entry.pszPassword = const_cast<char*>("pass");
        entry.pszURL = const_cast<char*>("");
        entry.pszAdditional = const_cast<char*>("");
        entry.pszBinaryDesc = const_cast<char*>("");
        entry.pBinaryData = nullptr;
        entry.uBinaryDataLen = 0;
        Random::fillBuffer(entry.uuid, 16);
        PwUtil::dateTimeToPwTime(QDateTime::currentDateTime(), &entry.tCreation);
        entry.tLastAccess = entry.tCreation;
        entry.tLastMod = entry.tCreation;
        PwManager::getNeverExpireTime(&entry.tExpire);

        QVERIFY(mgr->addEntry(&entry));
    }

    QCOMPARE((int)mgr->getNumberOfEntries(), 3);

    // Delete middle entry (index 1)
    QVERIFY(mgr->deleteEntry(1));
    QCOMPARE((int)mgr->getNumberOfEntries(), 2);

    // Verify entries shifted correctly
    PW_ENTRY* remainingEntry1 = mgr->getEntry(0);
    QVERIFY(remainingEntry1 != nullptr);

    PW_ENTRY* remainingEntry2 = mgr->getEntry(1);
    QVERIFY(remainingEntry2 != nullptr);

    // Delete first entry (index 0)
    QVERIFY(mgr->deleteEntry(0));
    QCOMPARE((int)mgr->getNumberOfEntries(), 1);

    // Delete last entry
    QVERIFY(mgr->deleteEntry(0));
    QCOMPARE((int)mgr->getNumberOfEntries(), 0);

    // Try to delete non-existent entry
    QVERIFY(!mgr->deleteEntry(0));

    delete mgr;
}

void TestPwManager::testDeleteGroup()
{
    PwManager* mgr = createTestManager();
    mgr->newDatabase();
    mgr->setMasterKey("test", false, "", true, "");

    // Add groups
    for (int i = 0; i < 3; ++i) {
        PW_GROUP group;
        std::memset(&group, 0, sizeof(PW_GROUP));
        group.pszGroupName = const_cast<char*>(QString("Group %1").arg(i).toUtf8().constData());
        group.uGroupId = i + 1;
        PwUtil::dateTimeToPwTime(QDateTime::currentDateTime(), &group.tCreation);
        group.tLastAccess = group.tCreation;
        group.tLastMod = group.tCreation;
        PwManager::getNeverExpireTime(&group.tExpire);
        group.uImageId = 1;
        QVERIFY(mgr->addGroup(&group));
    }

    QCOMPARE((int)mgr->getNumberOfGroups(), 3);

    // Add entry to group 2
    PW_ENTRY entry;
    std::memset(&entry, 0, sizeof(PW_ENTRY));
    entry.uGroupId = 2;
    entry.uImageId = 0;
    entry.pszTitle = const_cast<char*>("Test Entry");
    entry.pszUserName = const_cast<char*>("user");
    entry.pszPassword = const_cast<char*>("pass");
    entry.pszURL = const_cast<char*>("");
    entry.pszAdditional = const_cast<char*>("");
    entry.pszBinaryDesc = const_cast<char*>("");
    entry.pBinaryData = nullptr;
    entry.uBinaryDataLen = 0;
    Random::fillBuffer(entry.uuid, 16);
    PwUtil::dateTimeToPwTime(QDateTime::currentDateTime(), &entry.tCreation);
    entry.tLastAccess = entry.tCreation;
    entry.tLastMod = entry.tCreation;
    PwManager::getNeverExpireTime(&entry.tExpire);
    QVERIFY(mgr->addEntry(&entry));

    QCOMPARE((int)mgr->getNumberOfEntries(), 1);

    // Delete group 2 without backup
    QVERIFY(mgr->deleteGroupById(2, false));
    QCOMPARE((int)mgr->getNumberOfGroups(), 2);
    QCOMPARE((int)mgr->getNumberOfEntries(), 0);  // Entry should be deleted too

    // Delete group 1 (empty group)
    QVERIFY(mgr->deleteGroupById(1, false));
    QCOMPARE((int)mgr->getNumberOfGroups(), 1);

    delete mgr;
}

void TestPwManager::testBackupEntry()
{
    PwManager* mgr = createTestManager();
    mgr->newDatabase();
    mgr->setMasterKey("test", false, "", true, "");

    // Add a group
    PW_GROUP group;
    std::memset(&group, 0, sizeof(PW_GROUP));
    group.pszGroupName = const_cast<char*>("Test Group");
    group.uGroupId = 1;
    PwUtil::dateTimeToPwTime(QDateTime::currentDateTime(), &group.tCreation);
    group.tLastAccess = group.tCreation;
    group.tLastMod = group.tCreation;
    PwManager::getNeverExpireTime(&group.tExpire);
    group.uImageId = 1;
    QVERIFY(mgr->addGroup(&group));

    // Add an entry
    PW_ENTRY entry;
    std::memset(&entry, 0, sizeof(PW_ENTRY));
    entry.uGroupId = 1;
    entry.uImageId = 0;
    entry.pszTitle = const_cast<char*>("Original Entry");
    entry.pszUserName = const_cast<char*>("user");
    entry.pszPassword = const_cast<char*>("password123");
    entry.pszURL = const_cast<char*>("http://example.com");
    entry.pszAdditional = const_cast<char*>("notes");
    entry.pszBinaryDesc = const_cast<char*>("");
    entry.pBinaryData = nullptr;
    entry.uBinaryDataLen = 0;
    Random::fillBuffer(entry.uuid, 16);
    PwUtil::dateTimeToPwTime(QDateTime::currentDateTime(), &entry.tCreation);
    entry.tLastAccess = entry.tCreation;
    entry.tLastMod = entry.tCreation;
    PwManager::getNeverExpireTime(&entry.tExpire);
    QVERIFY(mgr->addEntry(&entry));

    QCOMPARE((int)mgr->getNumberOfGroups(), 1);
    QCOMPARE((int)mgr->getNumberOfEntries(), 1);

    // Backup the entry
    PW_ENTRY* originalEntry = mgr->getEntry(0);
    bool groupCreated = false;
    QVERIFY(mgr->backupEntry(originalEntry, &groupCreated));

    // Should have created "Backup" group
    QVERIFY(groupCreated);
    QCOMPARE((int)mgr->getNumberOfGroups(), 2);
    QCOMPARE((int)mgr->getNumberOfEntries(), 2);

    // Verify backup group was created
    quint32 backupGroupId = mgr->getGroupId("Backup");
    QVERIFY(backupGroupId != 0xFFFFFFFF);

    // Backup entry again (group already exists)
    groupCreated = false;
    QVERIFY(mgr->backupEntry(originalEntry, &groupCreated));
    QVERIFY(!groupCreated);  // Group already existed
    QCOMPARE((int)mgr->getNumberOfGroups(), 2);
    QCOMPARE((int)mgr->getNumberOfEntries(), 3);  // Now 3 entries (1 original + 2 backups)

    delete mgr;
}

void TestPwManager::testGetGroupId()
{
    PwManager* mgr = createTestManager();
    mgr->newDatabase();
    mgr->setMasterKey("test", false, "", true, "");

    // Add groups with different names
    for (int i = 0; i < 3; ++i) {
        PW_GROUP group;
        std::memset(&group, 0, sizeof(PW_GROUP));
        QByteArray nameBytes = QString("Group %1").arg(i).toUtf8();
        group.pszGroupName = new char[nameBytes.size() + 1];
        std::strcpy(group.pszGroupName, nameBytes.constData());
        group.uGroupId = (i + 1) * 100;  // Use distinct IDs
        PwUtil::dateTimeToPwTime(QDateTime::currentDateTime(), &group.tCreation);
        group.tLastAccess = group.tCreation;
        group.tLastMod = group.tCreation;
        PwManager::getNeverExpireTime(&group.tExpire);
        group.uImageId = 1;
        QVERIFY(mgr->addGroup(&group));
        delete[] group.pszGroupName;
    }

    // Test case-insensitive search
    QCOMPARE(mgr->getGroupId("group 0"), (quint32)100);
    QCOMPARE(mgr->getGroupId("GROUP 1"), (quint32)200);
    QCOMPARE(mgr->getGroupId("GrOuP 2"), (quint32)300);

    // Test exact match
    QCOMPARE(mgr->getGroupId("Group 0"), (quint32)100);

    // Test non-existent group
    QCOMPARE(mgr->getGroupId("Non-existent"), (quint32)0xFFFFFFFF);

    // Test empty string
    QCOMPARE(mgr->getGroupId(""), (quint32)0xFFFFFFFF);

    // Test getGroupIdByIndex
    QCOMPARE(mgr->getGroupIdByIndex(0), (quint32)100);
    QCOMPARE(mgr->getGroupIdByIndex(1), (quint32)200);
    QCOMPARE(mgr->getGroupIdByIndex(2), (quint32)300);
    QCOMPARE(mgr->getGroupIdByIndex(999), (quint32)0xFFFFFFFF);

    delete mgr;
}

void TestPwManager::testFind()
{
    PwManager* mgr = createTestManager();
    mgr->newDatabase();
    mgr->setMasterKey("test", false, "", true, "");

    // Add a test group
    PW_GROUP group;
    std::memset(&group, 0, sizeof(PW_GROUP));
    group.pszGroupName = const_cast<char*>("Internet");
    group.uGroupId = 1;
    PwUtil::dateTimeToPwTime(QDateTime::currentDateTime(), &group.tCreation);
    group.tLastAccess = group.tCreation;
    group.tLastMod = group.tCreation;
    PwManager::getNeverExpireTime(&group.tExpire);
    group.uImageId = 1;
    QVERIFY(mgr->addGroup(&group));

    // Add test entries
    PW_ENTRY entry1;
    std::memset(&entry1, 0, sizeof(PW_ENTRY));
    entry1.uGroupId = 1;
    entry1.uImageId = 0;
    entry1.pszTitle = const_cast<char*>("Gmail");
    entry1.pszUserName = const_cast<char*>("user@gmail.com");
    entry1.pszPassword = const_cast<char*>("SecretPass123");
    entry1.pszURL = const_cast<char*>("https://mail.google.com");
    entry1.pszAdditional = const_cast<char*>("My email account");
    entry1.pszBinaryDesc = const_cast<char*>("");
    entry1.pBinaryData = nullptr;
    entry1.uBinaryDataLen = 0;
    Random::fillBuffer(entry1.uuid, 16);
    PwUtil::dateTimeToPwTime(QDateTime::currentDateTime(), &entry1.tCreation);
    entry1.tLastAccess = entry1.tCreation;
    entry1.tLastMod = entry1.tCreation;
    PwManager::getNeverExpireTime(&entry1.tExpire);
    QVERIFY(mgr->addEntry(&entry1));

    PW_ENTRY entry2;
    std::memset(&entry2, 0, sizeof(PW_ENTRY));
    entry2.uGroupId = 1;
    entry2.uImageId = 0;
    entry2.pszTitle = const_cast<char*>("GitHub");
    entry2.pszUserName = const_cast<char*>("developer");
    entry2.pszPassword = const_cast<char*>("CodePass456");
    entry2.pszURL = const_cast<char*>("https://github.com");
    entry2.pszAdditional = const_cast<char*>("Development repository");
    entry2.pszBinaryDesc = const_cast<char*>("");
    entry2.pBinaryData = nullptr;
    entry2.uBinaryDataLen = 0;
    Random::fillBuffer(entry2.uuid, 16);
    PwUtil::dateTimeToPwTime(QDateTime::currentDateTime(), &entry2.tCreation);
    entry2.tLastAccess = entry2.tCreation;
    entry2.tLastMod = entry2.tCreation;
    PwManager::getNeverExpireTime(&entry2.tExpire);
    QVERIFY(mgr->addEntry(&entry2));

    PW_ENTRY entry3;
    std::memset(&entry3, 0, sizeof(PW_ENTRY));
    entry3.uGroupId = 1;
    entry3.uImageId = 0;
    entry3.pszTitle = const_cast<char*>("Banking");
    entry3.pszUserName = const_cast<char*>("john.doe");
    entry3.pszPassword = const_cast<char*>("BankPass789");
    entry3.pszURL = const_cast<char*>("https://bank.example.com");
    entry3.pszAdditional = const_cast<char*>("Online banking");
    entry3.pszBinaryDesc = const_cast<char*>("");
    entry3.pBinaryData = nullptr;
    entry3.uBinaryDataLen = 0;
    Random::fillBuffer(entry3.uuid, 16);
    PwUtil::dateTimeToPwTime(QDateTime::currentDateTime(), &entry3.tCreation);
    entry3.tLastAccess = entry3.tCreation;
    entry3.tLastMod = entry3.tCreation;
    PwManager::getNeverExpireTime(&entry3.tExpire);
    QVERIFY(mgr->addEntry(&entry3));

    QCOMPARE((int)mgr->getNumberOfEntries(), 3);

    // Test 1: Search by title (case-insensitive)
    QString error;
    quint32 result = mgr->find("gmail", false, PWMF_TITLE, 0, 0xFFFFFFFF, &error);
    QVERIFY(result != 0xFFFFFFFF);
    QCOMPARE(result, (quint32)0);  // First entry

    // Test 2: Search by title (case-sensitive, should fail)
    result = mgr->find("gmail", true, PWMF_TITLE, 0, 0xFFFFFFFF, &error);
    QVERIFY(result == 0xFFFFFFFF);  // Not found

    // Test 3: Search by title (case-sensitive, exact match)
    result = mgr->find("Gmail", true, PWMF_TITLE, 0, 0xFFFFFFFF, &error);
    QVERIFY(result != 0xFFFFFFFF);
    QCOMPARE(result, (quint32)0);

    // Test 4: Search by username
    result = mgr->find("developer", false, PWMF_USER, 0, 0xFFFFFFFF, &error);
    QVERIFY(result != 0xFFFFFFFF);
    QCOMPARE(result, (quint32)1);  // Second entry

    // Test 5: Search by URL
    result = mgr->find("github", false, PWMF_URL, 0, 0xFFFFFFFF, &error);
    QVERIFY(result != 0xFFFFFFFF);
    QCOMPARE(result, (quint32)1);

    // Test 6: Search in notes
    result = mgr->find("email", false, PWMF_ADDITIONAL, 0, 0xFFFFFFFF, &error);
    QVERIFY(result != 0xFFFFFFFF);
    QCOMPARE(result, (quint32)0);

    // Test 7: Search in multiple fields
    result = mgr->find("banking", false, PWMF_TITLE | PWMF_ADDITIONAL, 0, 0xFFFFFFFF, &error);
    QVERIFY(result != 0xFFFFFFFF);
    QCOMPARE(result, (quint32)2);

    // Test 8: Search with start index
    result = mgr->find("http", false, PWMF_URL, 1, 0xFFFFFFFF, &error);
    QVERIFY(result != 0xFFFFFFFF);
    QCOMPARE(result, (quint32)1);  // Should find GitHub (skips Gmail at index 0)

    // Test 9: Search not found
    result = mgr->find("nonexistent", false, PWMF_TITLE, 0, 0xFFFFFFFF, &error);
    QVERIFY(result == 0xFFFFFFFF);

    // Test 10: Test regex search (if supported)
    result = mgr->find("G.*l", false, PWMF_TITLE | PWMS_REGEX, 0, 0xFFFFFFFF, &error);
    QVERIFY(result != 0xFFFFFFFF);
    QCOMPARE(result, (quint32)0);  // Matches "Gmail"

    // Test 11: Test findEx (wrapper)
    result = mgr->findEx("github", false, PWMF_URL, 0, &error);
    QVERIFY(result != 0xFFFFFFFF);
    QCOMPARE(result, (quint32)1);

    // Test 12: Empty search string
    result = mgr->find("", false, PWMF_TITLE, 0, 0xFFFFFFFF, &error);
    QVERIFY(result == 0xFFFFFFFF);
    QVERIFY(!error.isEmpty());  // Should have error message

    delete mgr;
}

void TestPwManager::testFindAll()
{
    PwManager* mgr = createTestManager();
    mgr->newDatabase();
    mgr->setMasterKey("test", false, "", true, "");

    // Add a test group
    PW_GROUP group;
    std::memset(&group, 0, sizeof(PW_GROUP));
    group.pszGroupName = const_cast<char*>("Internet");
    group.uGroupId = 1;
    PwUtil::dateTimeToPwTime(QDateTime::currentDateTime(), &group.tCreation);
    group.tLastAccess = group.tCreation;
    group.tLastMod = group.tCreation;
    PwManager::getNeverExpireTime(&group.tExpire);
    group.uImageId = 1;
    QVERIFY(mgr->addGroup(&group));

    // Add multiple test entries with "test" in different fields
    for (int i = 0; i < 5; ++i) {
        PW_ENTRY entry;
        std::memset(&entry, 0, sizeof(PW_ENTRY));
        entry.uGroupId = 1;
        entry.uImageId = 0;

        // Mix of titles
        if (i % 2 == 0) {
            entry.pszTitle = const_cast<char*>("Test Entry");
        } else {
            entry.pszTitle = const_cast<char*>("Other Entry");
        }

        entry.pszUserName = const_cast<char*>("user");
        entry.pszPassword = const_cast<char*>("password");
        entry.pszURL = const_cast<char*>("http://example.com");

        if (i == 2) {
            entry.pszAdditional = const_cast<char*>("Testing notes");
        } else {
            entry.pszAdditional = const_cast<char*>("notes");
        }

        entry.pszBinaryDesc = const_cast<char*>("");
        entry.pBinaryData = nullptr;
        entry.uBinaryDataLen = 0;
        Random::fillBuffer(entry.uuid, 16);
        PwUtil::dateTimeToPwTime(QDateTime::currentDateTime(), &entry.tCreation);
        entry.tLastAccess = entry.tCreation;
        entry.tLastMod = entry.tCreation;
        PwManager::getNeverExpireTime(&entry.tExpire);
        QVERIFY(mgr->addEntry(&entry));
    }

    QCOMPARE((int)mgr->getNumberOfEntries(), 5);

    // Test findAll: should find all entries with "test" in title or notes
    QString error;
    QList<quint32> results = mgr->findAll("test", false, PWMF_TITLE | PWMF_ADDITIONAL,
                                          false, false, &error);

    // Should find: entries 0, 2, 4 (with "Test Entry" title)
    // Entry 2 also has "Testing notes" but appears only once in results
    QCOMPARE(results.count(), 3);  // Entries at indices 0, 2, 4
    QVERIFY(results.contains(0));
    QVERIFY(results.contains(2));
    QVERIFY(results.contains(4));

    // Test findAll with regex
    results = mgr->findAll("Entry$", false, PWMF_TITLE | PWMS_REGEX,
                          false, false, &error);
    QCOMPARE(results.count(), 5);  // All entries end with "Entry"

    delete mgr;
}

void TestPwManager::testFindExcludeBackups()
{
    PwManager* mgr = createTestManager();
    mgr->newDatabase();
    mgr->setMasterKey("test", false, "", true, "");

    // Add normal group
    PW_GROUP group;
    std::memset(&group, 0, sizeof(PW_GROUP));
    group.pszGroupName = const_cast<char*>("Internet");
    group.uGroupId = 1;
    PwUtil::dateTimeToPwTime(QDateTime::currentDateTime(), &group.tCreation);
    group.tLastAccess = group.tCreation;
    group.tLastMod = group.tCreation;
    PwManager::getNeverExpireTime(&group.tExpire);
    group.uImageId = 1;
    QVERIFY(mgr->addGroup(&group));

    // Add Backup group
    PW_GROUP backupGroup;
    std::memset(&backupGroup, 0, sizeof(PW_GROUP));
    backupGroup.pszGroupName = const_cast<char*>("Backup");
    backupGroup.uGroupId = 2;
    PwUtil::dateTimeToPwTime(QDateTime::currentDateTime(), &backupGroup.tCreation);
    backupGroup.tLastAccess = backupGroup.tCreation;
    backupGroup.tLastMod = backupGroup.tCreation;
    PwManager::getNeverExpireTime(&backupGroup.tExpire);
    backupGroup.uImageId = 4;
    QVERIFY(mgr->addGroup(&backupGroup));

    // Add entry in normal group
    PW_ENTRY entry1;
    std::memset(&entry1, 0, sizeof(PW_ENTRY));
    entry1.uGroupId = 1;
    entry1.uImageId = 0;
    entry1.pszTitle = const_cast<char*>("Test Entry");
    entry1.pszUserName = const_cast<char*>("user");
    entry1.pszPassword = const_cast<char*>("password");
    entry1.pszURL = const_cast<char*>("http://example.com");
    entry1.pszAdditional = const_cast<char*>("notes");
    entry1.pszBinaryDesc = const_cast<char*>("");
    entry1.pBinaryData = nullptr;
    entry1.uBinaryDataLen = 0;
    Random::fillBuffer(entry1.uuid, 16);
    PwUtil::dateTimeToPwTime(QDateTime::currentDateTime(), &entry1.tCreation);
    entry1.tLastAccess = entry1.tCreation;
    entry1.tLastMod = entry1.tCreation;
    PwManager::getNeverExpireTime(&entry1.tExpire);
    QVERIFY(mgr->addEntry(&entry1));

    // Add entry in Backup group
    PW_ENTRY entry2;
    std::memset(&entry2, 0, sizeof(PW_ENTRY));
    entry2.uGroupId = 2;
    entry2.uImageId = 0;
    entry2.pszTitle = const_cast<char*>("Test Backup");
    entry2.pszUserName = const_cast<char*>("user");
    entry2.pszPassword = const_cast<char*>("password");
    entry2.pszURL = const_cast<char*>("http://example.com");
    entry2.pszAdditional = const_cast<char*>("notes");
    entry2.pszBinaryDesc = const_cast<char*>("");
    entry2.pBinaryData = nullptr;
    entry2.uBinaryDataLen = 0;
    Random::fillBuffer(entry2.uuid, 16);
    PwUtil::dateTimeToPwTime(QDateTime::currentDateTime(), &entry2.tCreation);
    entry2.tLastAccess = entry2.tCreation;
    entry2.tLastMod = entry2.tCreation;
    PwManager::getNeverExpireTime(&entry2.tExpire);
    QVERIFY(mgr->addEntry(&entry2));

    QString error;

    // Search WITHOUT excluding backups - should find both
    QList<quint32> results = mgr->findAll("test", false, PWMF_TITLE,
                                          false, false, &error);
    QCOMPARE(results.count(), 2);

    // Search WITH excluding backups - should find only the non-backup entry
    results = mgr->findAll("test", false, PWMF_TITLE,
                          true, false, &error);
    QCOMPARE(results.count(), 1);
    QCOMPARE(results.at(0), (quint32)0);  // First entry only

    delete mgr;
}

void TestPwManager::testFindExcludeExpired()
{
    PwManager* mgr = createTestManager();
    mgr->newDatabase();
    mgr->setMasterKey("test", false, "", true, "");

    // Add a test group
    PW_GROUP group;
    std::memset(&group, 0, sizeof(PW_GROUP));
    group.pszGroupName = const_cast<char*>("Internet");
    group.uGroupId = 1;
    PwUtil::dateTimeToPwTime(QDateTime::currentDateTime(), &group.tCreation);
    group.tLastAccess = group.tCreation;
    group.tLastMod = group.tCreation;
    PwManager::getNeverExpireTime(&group.tExpire);
    group.uImageId = 1;
    QVERIFY(mgr->addGroup(&group));

    // Add non-expired entry
    PW_ENTRY entry1;
    std::memset(&entry1, 0, sizeof(PW_ENTRY));
    entry1.uGroupId = 1;
    entry1.uImageId = 0;
    entry1.pszTitle = const_cast<char*>("Test Entry");
    entry1.pszUserName = const_cast<char*>("user");
    entry1.pszPassword = const_cast<char*>("password");
    entry1.pszURL = const_cast<char*>("http://example.com");
    entry1.pszAdditional = const_cast<char*>("notes");
    entry1.pszBinaryDesc = const_cast<char*>("");
    entry1.pBinaryData = nullptr;
    entry1.uBinaryDataLen = 0;
    Random::fillBuffer(entry1.uuid, 16);
    PwUtil::dateTimeToPwTime(QDateTime::currentDateTime(), &entry1.tCreation);
    entry1.tLastAccess = entry1.tCreation;
    entry1.tLastMod = entry1.tCreation;
    PwManager::getNeverExpireTime(&entry1.tExpire);  // Never expires
    QVERIFY(mgr->addEntry(&entry1));

    // Add expired entry (expiry date in the past)
    PW_ENTRY entry2;
    std::memset(&entry2, 0, sizeof(PW_ENTRY));
    entry2.uGroupId = 1;
    entry2.uImageId = 0;
    entry2.pszTitle = const_cast<char*>("Test Expired");
    entry2.pszUserName = const_cast<char*>("user");
    entry2.pszPassword = const_cast<char*>("password");
    entry2.pszURL = const_cast<char*>("http://example.com");
    entry2.pszAdditional = const_cast<char*>("notes");
    entry2.pszBinaryDesc = const_cast<char*>("");
    entry2.pBinaryData = nullptr;
    entry2.uBinaryDataLen = 0;
    Random::fillBuffer(entry2.uuid, 16);
    PwUtil::dateTimeToPwTime(QDateTime::currentDateTime(), &entry2.tCreation);
    entry2.tLastAccess = entry2.tCreation;
    entry2.tLastMod = entry2.tCreation;
    // Set expiry to yesterday
    entry2.tExpire.shYear = 2020;
    entry2.tExpire.btMonth = 1;
    entry2.tExpire.btDay = 1;
    entry2.tExpire.btHour = 0;
    entry2.tExpire.btMinute = 0;
    entry2.tExpire.btSecond = 0;
    QVERIFY(mgr->addEntry(&entry2));

    QString error;

    // Search WITHOUT excluding expired - should find both
    QList<quint32> results = mgr->findAll("test", false, PWMF_TITLE,
                                          false, false, &error);
    QCOMPARE(results.count(), 2);

    // Search WITH excluding expired - should find only non-expired entry
    results = mgr->findAll("test", false, PWMF_TITLE,
                          false, true, &error);
    QCOMPARE(results.count(), 1);
    QCOMPARE(results.at(0), (quint32)0);  // First entry only (non-expired)

    delete mgr;
}

//==============================================================================
// Password Generator Tests
//==============================================================================

void TestPwManager::testPasswordGeneratorBasic()
{
    PasswordGeneratorSettings settings = PasswordGenerator::getDefaultSettings();
    QCOMPARE(settings.length, (quint32)20);
    QVERIFY(settings.includeUpperCase);
    QVERIFY(settings.includeLowerCase);
    QVERIFY(settings.includeDigits);

    QString error;
    QString password = PasswordGenerator::generate(settings, &error);

    QVERIFY(!password.isEmpty());
    QVERIFY(error.isEmpty());
    QCOMPARE(password.length(), 20);

    // Verify password contains expected character types
    bool hasUpper = false, hasLower = false, hasDigit = false;
    for (const QChar& ch : password) {
        if (ch.isUpper()) hasUpper = true;
        if (ch.isLower()) hasLower = true;
        if (ch.isDigit()) hasDigit = true;
    }
    QVERIFY(hasUpper || hasLower || hasDigit);  // At least one type
}

void TestPwManager::testPasswordGeneratorCharSets()
{
    // Test uppercase only
    PasswordGeneratorSettings settings;
    settings.length = 10;
    settings.includeUpperCase = true;
    settings.includeLowerCase = false;
    settings.includeDigits = false;

    QString error;
    QString password = PasswordGenerator::generate(settings, &error);
    QVERIFY(!password.isEmpty());
    QCOMPARE(password.length(), 10);

    for (const QChar& ch : password) {
        QVERIFY(ch.isUpper());
    }

    // Test lowercase only
    settings.includeUpperCase = false;
    settings.includeLowerCase = true;
    password = PasswordGenerator::generate(settings, &error);
    QVERIFY(!password.isEmpty());

    for (const QChar& ch : password) {
        QVERIFY(ch.isLower());
    }

    // Test digits only
    settings.includeLowerCase = false;
    settings.includeDigits = true;
    password = PasswordGenerator::generate(settings, &error);
    QVERIFY(!password.isEmpty());

    for (const QChar& ch : password) {
        QVERIFY(ch.isDigit());
    }

    // Test special characters
    settings.includeDigits = false;
    settings.includeSpecial = true;
    password = PasswordGenerator::generate(settings, &error);
    QVERIFY(!password.isEmpty());

    // Test custom character set
    settings = PasswordGeneratorSettings();
    settings.length = 10;
    settings.customCharSet = "ABC123";
    password = PasswordGenerator::generate(settings, &error);
    QVERIFY(!password.isEmpty());
    QCOMPARE(password.length(), 10);

    for (const QChar& ch : password) {
        QVERIFY(settings.customCharSet.contains(ch));
    }
}

void TestPwManager::testPasswordGeneratorExclusions()
{
    // Test exclude look-alike characters (O0Il1|)
    PasswordGeneratorSettings settings = PasswordGenerator::getDefaultSettings();
    settings.excludeLookAlike = true;

    QString error;
    QString password = PasswordGenerator::generate(settings, &error);
    QVERIFY(!password.isEmpty());

    // Should not contain confusing characters
    QString confusing = "O0Il1|";
    for (const QChar& ch : password) {
        QVERIFY(!confusing.contains(ch));
    }

    // Test custom exclusions (case-sensitive)
    settings = PasswordGenerator::getDefaultSettings();
    settings.excludeChars = "aeiou";  // Exclude lowercase vowels

    password = PasswordGenerator::generate(settings, &error);
    QVERIFY(!password.isEmpty());

    for (const QChar& ch : password) {
        QVERIFY(!settings.excludeChars.contains(ch));  // Exact match, case-sensitive
    }

    // Test combined exclusions
    settings.excludeLookAlike = true;
    settings.excludeChars = "xyz";

    password = PasswordGenerator::generate(settings, &error);
    QVERIFY(!password.isEmpty());

    QString allExcluded = confusing + settings.excludeChars;
    for (const QChar& ch : password) {
        QVERIFY(!allExcluded.contains(ch));  // Case-sensitive
    }
}

void TestPwManager::testPasswordGeneratorNoRepeat()
{
    // Test no repeat characters
    PasswordGeneratorSettings settings;
    settings.length = 10;
    settings.includeUpperCase = true;
    settings.includeLowerCase = true;
    settings.includeDigits = true;
    settings.noRepeatChars = true;

    QString error;
    QString password = PasswordGenerator::generate(settings, &error);
    QVERIFY(!password.isEmpty());
    QCOMPARE(password.length(), 10);

    // Verify all characters are unique
    QSet<QChar> uniqueChars;
    for (const QChar& ch : password) {
        uniqueChars.insert(ch);
    }
    QCOMPARE(uniqueChars.size(), 10);

    // Test that length > charset size with no-repeat should fail
    // (can't have unique characters if charset is smaller than length)
    settings.customCharSet = "ABC";
    settings.length = 5;
    settings.noRepeatChars = true;

    password = PasswordGenerator::generate(settings, &error);
    QVERIFY(password.isEmpty());  // Should fail
    QVERIFY(!error.isEmpty());  // Should have error message
    QVERIFY(error.contains("only 3 characters") || error.contains("charset"));

    // But it should work when length <= charset size
    settings.length = 3;
    error.clear();
    password = PasswordGenerator::generate(settings, &error);
    QVERIFY(!password.isEmpty());
    QVERIFY(error.isEmpty());
    QCOMPARE(password.length(), 3);

    // All 3 characters should be unique
    uniqueChars.clear();
    for (const QChar& ch : password) {
        uniqueChars.insert(ch);
    }
    QCOMPARE(uniqueChars.size(), 3);
}

void TestPwManager::testPasswordGeneratorEntropy()
{
    // Test entropy calculation
    // Entropy = length * log2(charSetSize)

    // charset size 10, length 10 -> entropy = 10 * log2(10) ≈ 33.22 bits
    double entropy = PasswordGenerator::calculateEntropy(10, 10);
    QVERIFY(entropy > 33.0 && entropy < 34.0);

    // charset size 62 (A-Za-z0-9), length 20 -> entropy = 20 * log2(62) ≈ 119.3 bits
    entropy = PasswordGenerator::calculateEntropy(62, 20);
    QVERIFY(entropy > 119.0 && entropy < 120.0);

    // charset size 95 (all printable), length 16 -> entropy = 16 * log2(95) ≈ 105.4 bits
    entropy = PasswordGenerator::calculateEntropy(95, 16);
    QVERIFY(entropy > 105.0 && entropy < 106.0);

    // Edge cases
    QCOMPARE(PasswordGenerator::calculateEntropy(0, 10), 0.0);
    QCOMPARE(PasswordGenerator::calculateEntropy(10, 0), 0.0);
}

void TestPwManager::testPasswordGeneratorQuality()
{
    // Test quality calculation (0-100 scale)

    // Weak password (< 33)
    QString weakPassword = "abc";  // ~3 chars, low entropy
    quint32 quality = PasswordGenerator::calculateQuality(weakPassword);
    QVERIFY(quality < 33);

    // Medium password (33-66)
    // Need ~12+ chars with good variety for medium
    QString mediumPassword = "Hello12345678";  // 13 chars
    quality = PasswordGenerator::calculateQuality(mediumPassword);
    QVERIFY(quality >= 33 && quality < 90);

    // Strong password (66-90)
    QString strongPassword = "Tr0ub4dor&3SecurePass";  // 20 chars, mixed case, digits, special
    quality = PasswordGenerator::calculateQuality(strongPassword);
    QVERIFY(quality >= 66);

    // Very strong password (90-100)
    QString veryStrongPassword = "CorrectHorseBatteryStaple1234567890";  // 35 chars
    quality = PasswordGenerator::calculateQuality(veryStrongPassword);
    QVERIFY(quality >= 90);

    // Empty password
    quality = PasswordGenerator::calculateQuality("");
    QCOMPARE(quality, (quint32)0);

    // Long random password should be very high quality
    PasswordGeneratorSettings settings = PasswordGenerator::getDefaultSettings();
    settings.length = 32;
    QString error;
    QString password = PasswordGenerator::generate(settings, &error);
    quality = PasswordGenerator::calculateQuality(password);
    QVERIFY(quality >= 90);
}

void TestPwManager::testPasswordGeneratorSettingsValidation()
{
    PasswordGeneratorSettings settings;
    QString error;

    // Valid default settings
    settings = PasswordGenerator::getDefaultSettings();
    QVERIFY(settings.isValid(&error));
    QVERIFY(error.isEmpty());

    // Test empty character set (all checkboxes off, no custom)
    settings.includeUpperCase = false;
    settings.includeLowerCase = false;
    settings.includeDigits = false;
    settings.customCharSet = "";
    QVERIFY(!settings.isValid(&error));
    QVERIFY(!error.isEmpty());
    QVERIFY(error.contains("character set") || error.contains("empty"));

    // Test invalid length (0)
    settings = PasswordGenerator::getDefaultSettings();
    settings.length = 0;
    QVERIFY(!settings.isValid(&error));
    QVERIFY(!error.isEmpty());

    // Test no repeat with length > charset size
    settings = PasswordGeneratorSettings();
    settings.customCharSet = "AB";
    settings.length = 10;
    settings.noRepeatChars = true;
    // Should be invalid (can't have 10 unique chars from charset of 2)
    QVERIFY(!settings.isValid(&error));
    QVERIFY(!error.isEmpty());

    // Should be valid when length <= charset size
    settings.length = 2;
    error.clear();
    QVERIFY(settings.isValid(&error));
    QVERIFY(error.isEmpty());

    // Test valid custom charset
    settings = PasswordGeneratorSettings();
    settings.customCharSet = "XYZ123";
    settings.length = 5;
    QVERIFY(settings.isValid(&error));
    QVERIFY(error.isEmpty());

    // Verify buildCharSet works
    QString charSet = settings.buildCharSet();
    QCOMPARE(charSet, QString("XYZ123"));

    // Test buildCharSet with checkboxes
    settings = PasswordGeneratorSettings();
    settings.includeUpperCase = true;
    settings.includeLowerCase = false;
    settings.includeDigits = true;
    charSet = settings.buildCharSet();
    QVERIFY(charSet.contains('A'));
    QVERIFY(charSet.contains('Z'));
    QVERIFY(charSet.contains('0'));
    QVERIFY(charSet.contains('9'));
    QVERIFY(!charSet.contains('a'));
}

QTEST_MAIN(TestPwManager)
#include "test_pwmanager.moc"
