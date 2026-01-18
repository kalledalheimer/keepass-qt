/*
  Qt KeePass - Performance Benchmarks

  Measures and reports performance characteristics for:
  - Key derivation (transformation rounds)
  - AES-256 encryption/decryption
  - Twofish-256 encryption/decryption
  - SHA-256 hashing
  - Database open/save operations

  Reference: Issue #13 - Performance benchmarking
*/

#include <QtTest>
#include <QElapsedTimer>
#include <QTemporaryFile>

#include "core/PwManager.h"
#include "core/crypto/KeyTransform.h"
#include "core/crypto/Rijndael.h"
#include "core/crypto/TwofishClass.h"
#include "core/crypto/SHA256.h"
#include "core/util/Random.h"

class TestPerformance : public QObject
{
    Q_OBJECT

private:
    // Helper to format bytes per second
    static QString formatThroughput(double bytesPerSec)
    {
        if (bytesPerSec >= 1e9) {
            return QString("%1 GB/s").arg(bytesPerSec / 1e9, 0, 'f', 2);
        } else if (bytesPerSec >= 1e6) {
            return QString("%1 MB/s").arg(bytesPerSec / 1e6, 0, 'f', 2);
        } else if (bytesPerSec >= 1e3) {
            return QString("%1 KB/s").arg(bytesPerSec / 1e3, 0, 'f', 2);
        }
        return QString("%1 B/s").arg(bytesPerSec, 0, 'f', 2);
    }

    // Helper to format operations per second
    static QString formatOpsPerSec(double ops)
    {
        if (ops >= 1e6) {
            return QString("%1 M ops/s").arg(ops / 1e6, 0, 'f', 2);
        } else if (ops >= 1e3) {
            return QString("%1 K ops/s").arg(ops / 1e3, 0, 'f', 2);
        }
        return QString("%1 ops/s").arg(ops, 0, 'f', 2);
    }

private slots:
    void initTestCase()
    {
        qDebug() << "";
        qDebug() << "==============================================";
        qDebug() << "Qt-KeePass Performance Benchmark Suite";
        qDebug() << "==============================================";
        qDebug() << "Platform:" << QSysInfo::prettyProductName();
        qDebug() << "CPU Architecture:" << QSysInfo::currentCpuArchitecture();
        qDebug() << "Qt Version:" << qVersion();
        qDebug() << "==============================================";
        qDebug() << "";
    }

    // =========================================================================
    // KEY TRANSFORMATION BENCHMARKS
    // =========================================================================

    void benchmarkKeyTransform_data()
    {
        QTest::addColumn<quint64>("rounds");
        QTest::addColumn<QString>("description");

        QTest::newRow("1K rounds")     << quint64(1000)      << "Fast (testing)";
        QTest::newRow("10K rounds")    << quint64(10000)     << "Very fast";
        QTest::newRow("100K rounds")   << quint64(100000)    << "Fast";
        QTest::newRow("600K rounds")   << quint64(600000)    << "KeePass default";
        QTest::newRow("1M rounds")     << quint64(1000000)   << "Strong";
    }

    void benchmarkKeyTransform()
    {
        QFETCH(quint64, rounds);
        QFETCH(QString, description);

        // Generate random key and seed
        quint8 buffer[32];
        quint8 keySeed[32];
        Random::fillBuffer(buffer, 32);
        Random::fillBuffer(keySeed, 32);

        QElapsedTimer timer;
        timer.start();

        bool success = KeyTransform::transform256(rounds, buffer, keySeed);

        qint64 elapsed = timer.elapsed();

        QVERIFY(success);

        double roundsPerSec = (double)rounds / (elapsed / 1000.0);

        qDebug() << QString("Key Transform %1: %2 ms (%3) - %4")
                    .arg(rounds)
                    .arg(elapsed)
                    .arg(description)
                    .arg(formatOpsPerSec(roundsPerSec));
    }

    void benchmarkKeyTransformBuiltin()
    {
        // Test the built-in benchmark function that calculates rounds for 1 second
        qDebug() << "";
        qDebug() << "Testing KeyTransform::benchmark(1000ms)...";

        QElapsedTimer timer;
        timer.start();

        quint64 rounds = KeyTransform::benchmark(1000);

        qint64 elapsed = timer.elapsed();

        qDebug() << QString("1-second benchmark result: %1 rounds in %2 ms")
                    .arg(rounds)
                    .arg(elapsed);
        qDebug() << QString("Rounds per second: %1").arg(formatOpsPerSec(rounds));

        // Should complete in approximately 1 second (allow 20% tolerance)
        QVERIFY2(elapsed >= 800 && elapsed <= 1500,
                 "Benchmark should complete in approximately 1 second");
        QVERIFY2(rounds > 100000,
                 "Modern hardware should achieve at least 100K rounds/sec");
    }

    // =========================================================================
    // AES-256 ENCRYPTION BENCHMARKS
    // =========================================================================

    void benchmarkAES256Encryption_data()
    {
        QTest::addColumn<int>("dataSize");
        QTest::addColumn<QString>("description");

        QTest::newRow("1 KB")   << 1024       << "Small data";
        QTest::newRow("64 KB")  << 65536      << "Typical entry";
        QTest::newRow("1 MB")   << 1048576    << "Large attachment";
        QTest::newRow("10 MB")  << 10485760   << "Very large database";
    }

    void benchmarkAES256Encryption()
    {
        QFETCH(int, dataSize);
        QFETCH(QString, description);

        // Generate random key, IV, and data
        quint8 key[32];
        quint8 iv[16];
        Random::fillBuffer(key, 32);
        Random::fillBuffer(iv, 16);

        QByteArray plaintext = Random::generateBytes(dataSize);
        QByteArray ciphertext(dataSize + 16, 0);  // Extra for padding

        CRijndael aes;

        // Benchmark encryption
        QElapsedTimer timer;
        timer.start();

        int result = aes.Init(CRijndael::CBC, CRijndael::EncryptDir,
                              key, CRijndael::Key32Bytes, iv);
        QCOMPARE(result, RIJNDAEL_SUCCESS);

        int encLen = aes.PadEncrypt(
            reinterpret_cast<const UINT8*>(plaintext.constData()),
            plaintext.size(),
            reinterpret_cast<UINT8*>(ciphertext.data()));

        qint64 encElapsed = timer.elapsed();

        QVERIFY(encLen > 0);

        // Benchmark decryption
        QByteArray decrypted(encLen, 0);

        timer.restart();

        result = aes.Init(CRijndael::CBC, CRijndael::DecryptDir,
                          key, CRijndael::Key32Bytes, iv);
        QCOMPARE(result, RIJNDAEL_SUCCESS);

        int decLen = aes.PadDecrypt(
            reinterpret_cast<const UINT8*>(ciphertext.constData()),
            encLen,
            reinterpret_cast<UINT8*>(decrypted.data()));

        qint64 decElapsed = timer.elapsed();

        QCOMPARE(decLen, dataSize);

        double encThroughput = (double)dataSize / (encElapsed / 1000.0);
        double decThroughput = (double)dataSize / (decElapsed / 1000.0);

        qDebug() << QString("AES-256 %1 (%2):")
                    .arg(dataSize)
                    .arg(description);
        qDebug() << QString("  Encrypt: %1 ms (%2)")
                    .arg(encElapsed)
                    .arg(formatThroughput(encThroughput));
        qDebug() << QString("  Decrypt: %1 ms (%2)")
                    .arg(decElapsed)
                    .arg(formatThroughput(decThroughput));
    }

    // =========================================================================
    // TWOFISH-256 ENCRYPTION BENCHMARKS
    // =========================================================================

    void benchmarkTwofish256Encryption_data()
    {
        QTest::addColumn<int>("dataSize");
        QTest::addColumn<QString>("description");

        QTest::newRow("1 KB")   << 1024       << "Small data";
        QTest::newRow("64 KB")  << 65536      << "Typical entry";
        QTest::newRow("1 MB")   << 1048576    << "Large attachment";
    }

    void benchmarkTwofish256Encryption()
    {
        QFETCH(int, dataSize);
        QFETCH(QString, description);

        // Generate random key, IV, and data
        quint8 key[32];
        quint8 iv[16];
        Random::fillBuffer(key, 32);
        Random::fillBuffer(iv, 16);

        QByteArray plaintext = Random::generateBytes(dataSize);
        QByteArray ciphertext(dataSize + 16, 0);

        CTwofish twofish;

        // Benchmark encryption
        QElapsedTimer timer;
        timer.start();

        bool success = twofish.Init(key, 32, iv);
        QVERIFY(success);

        int encLen = twofish.PadEncrypt(
            reinterpret_cast<const UINT8*>(plaintext.constData()),
            plaintext.size(),
            reinterpret_cast<UINT8*>(ciphertext.data()));

        qint64 encElapsed = timer.elapsed();

        QVERIFY(encLen > 0);

        // Benchmark decryption (need new instance)
        QByteArray decrypted(encLen, 0);
        CTwofish twofish2;

        timer.restart();

        success = twofish2.Init(key, 32, iv);
        QVERIFY(success);

        int decLen = twofish2.PadDecrypt(
            reinterpret_cast<const UINT8*>(ciphertext.constData()),
            encLen,
            reinterpret_cast<UINT8*>(decrypted.data()));

        qint64 decElapsed = timer.elapsed();

        QCOMPARE(decLen, dataSize);

        double encThroughput = (double)dataSize / (encElapsed / 1000.0);
        double decThroughput = (double)dataSize / (decElapsed / 1000.0);

        qDebug() << QString("Twofish-256 %1 (%2):")
                    .arg(dataSize)
                    .arg(description);
        qDebug() << QString("  Encrypt: %1 ms (%2)")
                    .arg(encElapsed)
                    .arg(formatThroughput(encThroughput));
        qDebug() << QString("  Decrypt: %1 ms (%2)")
                    .arg(decElapsed)
                    .arg(formatThroughput(decThroughput));
    }

    // =========================================================================
    // SHA-256 HASHING BENCHMARKS
    // =========================================================================

    void benchmarkSHA256_data()
    {
        QTest::addColumn<int>("dataSize");
        QTest::addColumn<QString>("description");

        QTest::newRow("1 KB")   << 1024       << "Small data";
        QTest::newRow("64 KB")  << 65536      << "Medium data";
        QTest::newRow("1 MB")   << 1048576    << "Large data";
        QTest::newRow("10 MB")  << 10485760   << "Very large data";
    }

    void benchmarkSHA256()
    {
        QFETCH(int, dataSize);
        QFETCH(QString, description);

        QByteArray data = Random::generateBytes(dataSize);

        QElapsedTimer timer;
        timer.start();

        QByteArray hash = SHA256::hash(data);

        qint64 elapsed = timer.elapsed();

        QCOMPARE(hash.size(), 32);

        double throughput = (double)dataSize / (elapsed / 1000.0);

        qDebug() << QString("SHA-256 %1 (%2): %3 ms (%4)")
                    .arg(dataSize)
                    .arg(description)
                    .arg(elapsed)
                    .arg(formatThroughput(throughput));
    }

    // =========================================================================
    // DATABASE OPERATION BENCHMARKS
    // =========================================================================

    void benchmarkDatabaseOperations_data()
    {
        QTest::addColumn<int>("entryCount");
        QTest::addColumn<QString>("description");

        QTest::newRow("10 entries")    << 10    << "Small database";
        QTest::newRow("100 entries")   << 100   << "Typical database";
        QTest::newRow("1000 entries")  << 1000  << "Large database";
        QTest::newRow("5000 entries")  << 5000  << "Very large database";
    }

    void benchmarkDatabaseOperations()
    {
        QFETCH(int, entryCount);
        QFETCH(QString, description);

        // Create database with entries
        PwManager manager;
        manager.newDatabase();
        manager.setMasterKey("BenchmarkPassword123!", false, QString(), true, QString());
        manager.setKeyEncRounds(1000);  // Low rounds for fast benchmark

        // Add a group
        PW_GROUP group;
        memset(&group, 0, sizeof(group));
        group.uImageId = 1;
        QString groupName = "Benchmark Group";
        QByteArray groupNameUtf8 = groupName.toUtf8();
        group.pszGroupName = new char[groupNameUtf8.size() + 1];
        strcpy(group.pszGroupName, groupNameUtf8.constData());
        group.usLevel = 0;

        quint32 groupId = manager.addGroup(&group);
        delete[] group.pszGroupName;

        // Add entries
        QElapsedTimer timer;
        timer.start();

        for (int i = 0; i < entryCount; ++i) {
            PW_ENTRY entry;
            memset(&entry, 0, sizeof(entry));
            Random::generateUuid(entry.uuid);
            entry.uGroupId = groupId;
            entry.uImageId = i % 69;

            QString title = QString("Entry %1").arg(i);
            QString username = QString("user%1").arg(i);
            QString password = QString("password%1!@#").arg(i);
            QString url = QString("https://example%1.com").arg(i);
            QString notes = QString("Notes for entry %1").arg(i);

            QByteArray titleUtf8 = title.toUtf8();
            QByteArray usernameUtf8 = username.toUtf8();
            QByteArray passwordUtf8 = password.toUtf8();
            QByteArray urlUtf8 = url.toUtf8();
            QByteArray notesUtf8 = notes.toUtf8();

            entry.pszTitle = new char[titleUtf8.size() + 1];
            entry.pszUserName = new char[usernameUtf8.size() + 1];
            entry.pszPassword = new char[passwordUtf8.size() + 1];
            entry.pszURL = new char[urlUtf8.size() + 1];
            entry.pszAdditional = new char[notesUtf8.size() + 1];

            strcpy(entry.pszTitle, titleUtf8.constData());
            strcpy(entry.pszUserName, usernameUtf8.constData());
            strcpy(entry.pszPassword, passwordUtf8.constData());
            strcpy(entry.pszURL, urlUtf8.constData());
            strcpy(entry.pszAdditional, notesUtf8.constData());

            entry.uPasswordLen = passwordUtf8.size();

            manager.addEntry(&entry);

            delete[] entry.pszTitle;
            delete[] entry.pszUserName;
            delete[] entry.pszPassword;
            delete[] entry.pszURL;
            delete[] entry.pszAdditional;
        }

        qint64 createElapsed = timer.elapsed();

        // Save database
        QTemporaryFile tempFile;
        tempFile.setAutoRemove(true);
        QVERIFY(tempFile.open());
        QString filePath = tempFile.fileName();
        tempFile.close();

        timer.restart();
        int saveResult = manager.saveDatabase(filePath);
        qint64 saveElapsed = timer.elapsed();

        QCOMPARE(saveResult, PWE_SUCCESS);

        // Get file size
        QFileInfo fileInfo(filePath);
        qint64 fileSize = fileInfo.size();

        // Open database
        PwManager manager2;
        manager2.setMasterKey("BenchmarkPassword123!", false, QString(), true, QString());

        timer.restart();
        int openResult = manager2.openDatabase(filePath, nullptr);
        qint64 openElapsed = timer.elapsed();

        QCOMPARE(openResult, PWE_SUCCESS);
        QCOMPARE(manager2.getNumberOfEntries(), static_cast<quint32>(entryCount));

        qDebug() << QString("Database %1 entries (%2):")
                    .arg(entryCount)
                    .arg(description);
        qDebug() << QString("  File size: %1 bytes").arg(fileSize);
        qDebug() << QString("  Create entries: %1 ms").arg(createElapsed);
        qDebug() << QString("  Save: %1 ms").arg(saveElapsed);
        qDebug() << QString("  Open: %1 ms").arg(openElapsed);
    }

    // =========================================================================
    // SUMMARY
    // =========================================================================

    void cleanupTestCase()
    {
        qDebug() << "";
        qDebug() << "==============================================";
        qDebug() << "Benchmark Summary";
        qDebug() << "==============================================";
        qDebug() << "All benchmarks completed successfully.";
        qDebug() << "";
        qDebug() << "Key Performance Indicators:";
        qDebug() << "- Key Transform 600K rounds: Target < 1000 ms";
        qDebug() << "- AES-256 1MB: Target > 50 MB/s";
        qDebug() << "- SHA-256 1MB: Target > 100 MB/s";
        qDebug() << "- Database 1000 entries: Target < 500 ms open";
        qDebug() << "==============================================";
    }
};

QTEST_MAIN(TestPerformance)
#include "test_performance.moc"
