/*
  KeePass Qt Validation Suite Generator
  Generates test databases for MFC KeePass compatibility validation
*/

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QString>
#include <QTextStream>
#include <cstring>
#include "../../src/core/PwManager.h"
#include "../../src/core/PwStructs.h"
#include "../../src/core/util/PwUtil.h"
#include "../../src/core/util/Random.h"

class ValidationSuiteGenerator {
public:
    ValidationSuiteGenerator(const QString& outputDir)
        : m_outputDir(outputDir)
    {
        QDir dir;
        if (!dir.exists(outputDir)) {
            dir.mkpath(outputDir);
        }
    }

    bool generateAllDatabases() {
        bool success = true;

        qInfo() << "Generating validation test suite...";
        qInfo() << "Output directory:" << m_outputDir;
        qInfo() << "";

        success &= generateEmptyDatabase();
        success &= generateSimpleAES();
        success &= generateSimpleTwofish();
        success &= generateComplexAES();
        success &= generateUnicode();
        success &= generateAttachment();
        success &= generateLowRounds();
        success &= generateHighRounds();

        if (success) {
            qInfo() << "\n✅ All 8 test databases generated successfully!";
            qInfo() << "Package location:" << m_outputDir;
        } else {
            qCritical() << "\n❌ Some databases failed to generate";
        }

        return success;
    }

private:
    QString m_outputDir;

    bool saveDatabase(PwManager& mgr, const QString& filename, const QString& password) {
        QString fullPath = m_outputDir + "/" + filename;

        mgr.setMasterKey(password, false, QString(), false, QString());
        int result = mgr.saveDatabase(fullPath);

        if (result == PWE_SUCCESS) {
            qInfo() << "✓" << filename << "created";
            return true;
        } else {
            qCritical() << "✗ Failed to create" << filename << "- Error:" << result;
            return false;
        }
    }

    PW_GROUP createGroup(quint32 id, const QString& name, quint32 imageId = 1) {
        PW_GROUP group;
        std::memset(&group, 0, sizeof(PW_GROUP));

        group.uGroupId = id;
        QByteArray nameUtf8 = name.toUtf8();
        group.pszGroupName = new char[nameUtf8.length() + 1];
        std::strcpy(group.pszGroupName, nameUtf8.constData());
        group.uImageId = imageId;

        PwUtil::getNeverExpireTime(&group.tExpire);
        PwUtil::getNeverExpireTime(&group.tCreation);
        PwUtil::getNeverExpireTime(&group.tLastMod);
        PwUtil::getNeverExpireTime(&group.tLastAccess);

        return group;
    }

    PW_ENTRY createEntry(quint32 groupId, const QString& title, const QString& username,
                         const QString& password, const QString& url, const QString& notes) {
        PW_ENTRY entry;
        std::memset(&entry, 0, sizeof(PW_ENTRY));

        entry.uGroupId = groupId;
        entry.uImageId = 0;

        // Allocate and copy strings
        auto allocString = [](const QString& str) -> char* {
            QByteArray utf8 = str.toUtf8();
            char* result = new char[utf8.length() + 1];
            std::strcpy(result, utf8.constData());
            return result;
        };

        entry.pszTitle = allocString(title);
        entry.pszUserName = allocString(username);
        entry.pszPassword = allocString(password);
        entry.pszURL = allocString(url);
        entry.pszAdditional = allocString(notes);
        entry.uPasswordLen = password.toUtf8().length();

        PwUtil::getNeverExpireTime(&entry.tExpire);
        PwUtil::getNeverExpireTime(&entry.tCreation);
        PwUtil::getNeverExpireTime(&entry.tLastMod);
        PwUtil::getNeverExpireTime(&entry.tLastAccess);

        // Generate UUID
        quint8 uuid[16];
        Random::fillBuffer(uuid, 16);
        std::memcpy(entry.uuid, uuid, 16);

        return entry;
    }

    bool generateEmptyDatabase() {
        qInfo() << "Generating test-empty-aes.kdb...";

        PwManager mgr;
        mgr.newDatabase();
        mgr.setAlgorithm(ALGO_AES);
        mgr.setKeyEncRounds(600000);

        // Add one empty group (KDB format requires at least one group)
        PW_GROUP group = createGroup(1, "Backup");
        mgr.addGroup(&group);
        delete[] group.pszGroupName;

        return saveDatabase(mgr, "test-empty-aes.kdb", "EmptyPass123");
    }

    bool generateSimpleAES() {
        qInfo() << "Generating test-simple-aes.kdb...";

        PwManager mgr;
        mgr.newDatabase();
        mgr.setAlgorithm(ALGO_AES);
        mgr.setKeyEncRounds(600000);

        // Add one group
        PW_GROUP group = createGroup(1, "General");
        mgr.addGroup(&group);
        delete[] group.pszGroupName;

        // Add one entry
        PW_ENTRY entry = createEntry(1, "Sample Entry", "testuser", "TestPass456",
                                     "https://example.com", "This is a test note");
        mgr.addEntry(&entry);

        // Clean up
        delete[] entry.pszTitle;
        delete[] entry.pszUserName;
        delete[] entry.pszPassword;
        delete[] entry.pszURL;
        delete[] entry.pszAdditional;

        return saveDatabase(mgr, "test-simple-aes.kdb", "SimplePass123");
    }

    bool generateSimpleTwofish() {
        qInfo() << "Generating test-simple-twofish.kdb...";

        PwManager mgr;
        mgr.newDatabase();
        mgr.setAlgorithm(ALGO_TWOFISH);
        mgr.setKeyEncRounds(600000);

        // Add one group
        PW_GROUP group = createGroup(1, "General");
        mgr.addGroup(&group);
        delete[] group.pszGroupName;

        // Add one entry
        PW_ENTRY entry = createEntry(1, "Twofish Test", "tfuser", "TwoFish789",
                                     "https://twofish.test", "Testing Twofish encryption");
        mgr.addEntry(&entry);

        // Clean up
        delete[] entry.pszTitle;
        delete[] entry.pszUserName;
        delete[] entry.pszPassword;
        delete[] entry.pszURL;
        delete[] entry.pszAdditional;

        return saveDatabase(mgr, "test-simple-twofish.kdb", "TwofishPass123");
    }

    bool generateComplexAES() {
        qInfo() << "Generating test-complex-aes.kdb...";

        PwManager mgr;
        mgr.newDatabase();
        mgr.setAlgorithm(ALGO_AES);
        mgr.setKeyEncRounds(600000);

        // Add multiple groups
        PW_GROUP g1 = createGroup(1, "Personal", 1);
        mgr.addGroup(&g1);
        delete[] g1.pszGroupName;

        PW_GROUP g2 = createGroup(2, "Work", 2);
        mgr.addGroup(&g2);
        delete[] g2.pszGroupName;

        PW_GROUP g3 = createGroup(3, "Banking", 3);
        mgr.addGroup(&g3);
        delete[] g3.pszGroupName;

        // Add entries to different groups
        const struct {
            quint32 groupId;
            QString title;
            QString user;
            QString pass;
            QString url;
        } entries[] = {
            {1, "Email Account", "user@email.com", "EmailPass123", "https://mail.example.com"},
            {1, "Social Media", "social_user", "Social456", "https://social.example.com"},
            {1, "Cloud Storage", "cloud_user", "CloudPass789", "https://storage.example.com"},
            {2, "Corporate Email", "john.doe@company.com", "WorkPass111", "https://mail.company.com"},
            {2, "Project Management", "jdoe", "Project222", "https://pm.company.com"},
            {2, "VPN Access", "john.doe", "VPN333", "https://vpn.company.com"},
            {3, "Main Bank", "customer12345", "Bank444", "https://bank.example.com"},
            {3, "Credit Card", "ccuser", "Card555", "https://creditcard.example.com"},
            {3, "Investment", "investor", "Invest666", "https://invest.example.com"},
            {3, "Savings", "saver", "Save777", "https://savings.example.com"}
        };

        for (const auto& e : entries) {
            PW_ENTRY entry = createEntry(e.groupId, e.title, e.user, e.pass, e.url,
                                        "Notes for " + e.title);
            mgr.addEntry(&entry);
            delete[] entry.pszTitle;
            delete[] entry.pszUserName;
            delete[] entry.pszPassword;
            delete[] entry.pszURL;
            delete[] entry.pszAdditional;
        }

        return saveDatabase(mgr, "test-complex-aes.kdb", "ComplexPass123");
    }

    bool generateUnicode() {
        qInfo() << "Generating test-unicode.kdb...";

        PwManager mgr;
        mgr.newDatabase();
        mgr.setAlgorithm(ALGO_AES);
        mgr.setKeyEncRounds(600000);

        // Add group with Unicode name
        PW_GROUP group = createGroup(1, "国际化测试 🌍");
        mgr.addGroup(&group);
        delete[] group.pszGroupName;

        // Add entries with various Unicode characters
        const struct {
            QString title;
            QString user;
            QString pass;
            QString notes;
        } entries[] = {
            {"中文标题", "用户名", "密码123", "这是中文备注"},
            {"العربية", "مستخدم", "كلمة السر", "ملاحظات عربية"},
            {"日本語", "ユーザー", "パスワード", "日本語のノート"},
            {"Русский", "пользователь", "пароль123", "Русские заметки"},
            {"Emoji Test 😀", "user🎉", "pass🔒", "Notes with emoji 🚀✨🌈"}
        };

        for (const auto& e : entries) {
            PW_ENTRY entry = createEntry(1, e.title, e.user, e.pass,
                                        "https://unicode.test", e.notes);
            mgr.addEntry(&entry);
            delete[] entry.pszTitle;
            delete[] entry.pszUserName;
            delete[] entry.pszPassword;
            delete[] entry.pszURL;
            delete[] entry.pszAdditional;
        }

        return saveDatabase(mgr, "test-unicode.kdb", "UnicodePass123");
    }

    bool generateAttachment() {
        qInfo() << "Generating test-attachment.kdb...";

        PwManager mgr;
        mgr.newDatabase();
        mgr.setAlgorithm(ALGO_AES);
        mgr.setKeyEncRounds(600000);

        // Add group
        PW_GROUP group = createGroup(1, "Attachments");
        mgr.addGroup(&group);
        delete[] group.pszGroupName;

        // Create entry with binary attachment
        PW_ENTRY entry = createEntry(1, "File with Attachment", "fileuser", "FilePass123",
                                     "https://files.example.com", "Entry with 100KB binary attachment");

        // Create 100KB binary data
        const quint32 attachmentSize = 100 * 1024;  // 100 KB
        quint8* attachmentData = new quint8[attachmentSize];
        for (quint32 i = 0; i < attachmentSize; i++) {
            attachmentData[i] = static_cast<quint8>(i % 256);
        }

        entry.pBinaryData = attachmentData;
        entry.uBinaryDataLen = attachmentSize;
        entry.pszBinaryDesc = new char[20];
        std::strcpy(entry.pszBinaryDesc, "test-file.bin");

        mgr.addEntry(&entry);

        // Clean up (PwManager takes ownership of binary data)
        delete[] entry.pszTitle;
        delete[] entry.pszUserName;
        delete[] entry.pszPassword;
        delete[] entry.pszURL;
        delete[] entry.pszAdditional;
        // Note: pBinaryData and pszBinaryDesc are now owned by PwManager

        return saveDatabase(mgr, "test-attachment.kdb", "AttachPass123");
    }

    bool generateLowRounds() {
        qInfo() << "Generating test-lowrounds.kdb...";

        PwManager mgr;
        mgr.newDatabase();
        mgr.setAlgorithm(ALGO_AES);
        mgr.setKeyEncRounds(1000);  // Very low for fast encryption

        // Add simple content
        PW_GROUP group = createGroup(1, "General");
        mgr.addGroup(&group);
        delete[] group.pszGroupName;

        PW_ENTRY entry = createEntry(1, "Low Rounds Test", "lowuser", "LowPass123",
                                     "https://low.test", "Testing with 1,000 key rounds");
        mgr.addEntry(&entry);
        delete[] entry.pszTitle;
        delete[] entry.pszUserName;
        delete[] entry.pszPassword;
        delete[] entry.pszURL;
        delete[] entry.pszAdditional;

        return saveDatabase(mgr, "test-lowrounds.kdb", "LowRoundsPass123");
    }

    bool generateHighRounds() {
        qInfo() << "Generating test-highrounds.kdb...";

        qInfo() << "⚠️  Warning: This will take several minutes (10M key transformation rounds)";

        PwManager mgr;
        mgr.newDatabase();
        mgr.setAlgorithm(ALGO_AES);
        mgr.setKeyEncRounds(10000000);  // 10 million rounds - very slow!

        // Add simple content
        PW_GROUP group = createGroup(1, "General");
        mgr.addGroup(&group);
        delete[] group.pszGroupName;

        PW_ENTRY entry = createEntry(1, "High Rounds Test", "highuser", "HighPass123",
                                     "https://high.test", "Testing with 10M key rounds");
        mgr.addEntry(&entry);
        delete[] entry.pszTitle;
        delete[] entry.pszUserName;
        delete[] entry.pszPassword;
        delete[] entry.pszURL;
        delete[] entry.pszAdditional;

        qInfo() << "Starting key transformation (this may take 2-5 minutes)...";
        bool result = saveDatabase(mgr, "test-highrounds.kdb", "HighRoundsPass123");

        if (result) {
            qInfo() << "High rounds test completed successfully";
        }

        return result;
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString outputDir = argc > 1 ? argv[1] : "tests/validation-package";

    qInfo() << "═══════════════════════════════════════════════════════";
    qInfo() << "  KeePass Qt → MFC Validation Suite Generator";
    qInfo() << "═══════════════════════════════════════════════════════";
    qInfo() << "";

    ValidationSuiteGenerator generator(outputDir);
    bool success = generator.generateAllDatabases();

    return success ? 0 : 1;
}
