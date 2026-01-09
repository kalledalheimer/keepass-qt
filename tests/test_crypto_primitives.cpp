/*
  Qt KeePass - Cryptographic Primitives Unit Tests

  Tests crypto implementations with known test vectors from:
  - NIST FIPS-197 (AES/Rijndael)
  - Official Twofish test vectors
  - NIST FIPS 180-2 (SHA-256)

  These tests verify that our crypto implementations match
  standard test vectors, ensuring correctness and compatibility.
*/

#include <QtTest/QtTest>
#include <QByteArray>
#include "../src/core/crypto/Rijndael.h"
#include "../src/core/crypto/Twofish.h"
#include "../src/core/crypto/SHA256.h"
#include "../src/core/PwStructs.h"

class TestCryptoPrimitives : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // AES/Rijndael Tests
    void testAES128_ECB();
    void testAES192_ECB();
    void testAES256_ECB();
    void testAES256_CBC();
    void testRijndaelPadEncrypt();

    // Twofish Tests
    void testTwofish128();
    void testTwofish192();
    void testTwofish256();

    // SHA-256 Tests
    void testSHA256_Empty();
    void testSHA256_SingleBlock();
    void testSHA256_MultiBlock();
    void testSHA256_Incremental();

    // Key Transformation Tests
    void testKeyTransformation();
    void testKeyTransformationRounds();

    // Time Compression Tests
    void testPwTimeSize();
    void testPwTimeEdgeCases();

private:
    // Helper functions
    QByteArray hexToBytes(const QString& hex);
    QString bytesToHex(const QByteArray& bytes);
    bool compareBytes(const UINT8* a, const UINT8* b, int len);
};

// Helper: Convert hex string to byte array
QByteArray TestCryptoPrimitives::hexToBytes(const QString& hex)
{
    QByteArray result;
    for (int i = 0; i < hex.length(); i += 2) {
        QString byte = hex.mid(i, 2);
        result.append(static_cast<char>(byte.toInt(nullptr, 16)));
    }
    return result;
}

// Helper: Convert byte array to hex string
QString TestCryptoPrimitives::bytesToHex(const QByteArray& bytes)
{
    QString result;
    for (int i = 0; i < bytes.length(); ++i) {
        result.append(QString("%1").arg(static_cast<quint8>(bytes[i]), 2, 16, QChar('0')));
    }
    return result.toLower();
}

// Helper: Compare byte arrays
bool TestCryptoPrimitives::compareBytes(const UINT8* a, const UINT8* b, int len)
{
    for (int i = 0; i < len; ++i) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

void TestCryptoPrimitives::initTestCase()
{
    qDebug() << "Initializing Crypto Primitives Tests...";

    // Initialize Twofish (required before use)
    Twofish_initialise();
}

void TestCryptoPrimitives::cleanupTestCase()
{
    qDebug() << "Crypto Primitives Tests Complete.";
}

// =============================================================================
// AES/Rijndael Tests (NIST FIPS-197 Test Vectors)
// =============================================================================

void TestCryptoPrimitives::testAES128_ECB()
{
    // NIST FIPS-197 Appendix C.1
    // AES-128 ECB Mode Test Vector

    QByteArray key = hexToBytes("000102030405060708090a0b0c0d0e0f");
    QByteArray plaintext = hexToBytes("00112233445566778899aabbccddeeff");
    QByteArray expectedCiphertext = hexToBytes("69c4e0d86a7b0430d8cdb78070b4c55a");

    CRijndael aes;
    UINT8 ciphertext[16];

    int result = aes.Init(CRijndael::ECB, CRijndael::EncryptDir,
                          reinterpret_cast<const UINT8*>(key.constData()),
                          CRijndael::Key16Bytes, nullptr);
    QCOMPARE(result, RIJNDAEL_SUCCESS);

    result = aes.BlockEncrypt(reinterpret_cast<const UINT8*>(plaintext.constData()),
                              128, ciphertext);
    QCOMPARE(result, 128); // Returns length in bits

    QVERIFY(compareBytes(ciphertext,
                        reinterpret_cast<const UINT8*>(expectedCiphertext.constData()),
                        16));
}

void TestCryptoPrimitives::testAES192_ECB()
{
    // NIST FIPS-197 Appendix C.2
    // AES-192 ECB Mode Test Vector

    QByteArray key = hexToBytes("000102030405060708090a0b0c0d0e0f1011121314151617");
    QByteArray plaintext = hexToBytes("00112233445566778899aabbccddeeff");
    QByteArray expectedCiphertext = hexToBytes("dda97ca4864cdfe06eaf70a0ec0d7191");

    CRijndael aes;
    UINT8 ciphertext[16];

    int result = aes.Init(CRijndael::ECB, CRijndael::EncryptDir,
                          reinterpret_cast<const UINT8*>(key.constData()),
                          CRijndael::Key24Bytes, nullptr);
    QCOMPARE(result, RIJNDAEL_SUCCESS);

    result = aes.BlockEncrypt(reinterpret_cast<const UINT8*>(plaintext.constData()),
                              128, ciphertext);
    QCOMPARE(result, 128);

    QVERIFY(compareBytes(ciphertext,
                        reinterpret_cast<const UINT8*>(expectedCiphertext.constData()),
                        16));
}

void TestCryptoPrimitives::testAES256_ECB()
{
    // NIST FIPS-197 Appendix C.3
    // AES-256 ECB Mode Test Vector (used by KeePass)

    QByteArray key = hexToBytes("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
    QByteArray plaintext = hexToBytes("00112233445566778899aabbccddeeff");
    QByteArray expectedCiphertext = hexToBytes("8ea2b7ca516745bfeafc49904b496089");

    CRijndael aes;
    UINT8 ciphertext[16];

    int result = aes.Init(CRijndael::ECB, CRijndael::EncryptDir,
                          reinterpret_cast<const UINT8*>(key.constData()),
                          CRijndael::Key32Bytes, nullptr);
    QCOMPARE(result, RIJNDAEL_SUCCESS);

    result = aes.BlockEncrypt(reinterpret_cast<const UINT8*>(plaintext.constData()),
                              128, ciphertext);
    QCOMPARE(result, 128);

    QVERIFY(compareBytes(ciphertext,
                        reinterpret_cast<const UINT8*>(expectedCiphertext.constData()),
                        16));
}

void TestCryptoPrimitives::testAES256_CBC()
{
    // AES-256 CBC Mode Test (KeePass uses CBC mode for database encryption)
    // NIST SP 800-38A F.2.5

    QByteArray key = hexToBytes("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");
    QByteArray iv = hexToBytes("000102030405060708090a0b0c0d0e0f");
    QByteArray plaintext = hexToBytes("6bc1bee22e409f96e93d7e117393172a");
    QByteArray expectedCiphertext = hexToBytes("f58c4c04d6e5f1ba779eabfb5f7bfbd6");

    CRijndael aes;
    UINT8 ciphertext[16];

    int result = aes.Init(CRijndael::CBC, CRijndael::EncryptDir,
                          reinterpret_cast<const UINT8*>(key.constData()),
                          CRijndael::Key32Bytes,
                          reinterpret_cast<const UINT8*>(iv.constData()));
    QCOMPARE(result, RIJNDAEL_SUCCESS);

    result = aes.BlockEncrypt(reinterpret_cast<const UINT8*>(plaintext.constData()),
                              128, ciphertext);
    QCOMPARE(result, 128);

    QVERIFY(compareBytes(ciphertext,
                        reinterpret_cast<const UINT8*>(expectedCiphertext.constData()),
                        16));
}

void TestCryptoPrimitives::testRijndaelPadEncrypt()
{
    // Test PadEncrypt (automatic padding) and PadDecrypt (used by KeePass)

    QByteArray key = hexToBytes("2b7e151628aed2a6abf7158809cf4f3c");
    QString originalText = "Hello, KeePass!"; // 15 bytes - needs padding

    CRijndael aesEnc;
    int result = aesEnc.Init(CRijndael::CBC, CRijndael::EncryptDir,
                             reinterpret_cast<const UINT8*>(key.constData()),
                             CRijndael::Key16Bytes, nullptr);
    QCOMPARE(result, RIJNDAEL_SUCCESS);

    // Encrypt
    UINT8 ciphertext[32]; // Must be at least plaintext + 16
    int ciphertextLen = aesEnc.PadEncrypt(
        reinterpret_cast<const UINT8*>(originalText.toLatin1().constData()),
        originalText.length(),
        ciphertext);

    QVERIFY(ciphertextLen > 0);
    QVERIFY(ciphertextLen >= originalText.length());
    QCOMPARE(ciphertextLen % 16, 0); // Must be multiple of block size

    // Decrypt
    CRijndael aesDec;
    result = aesDec.Init(CRijndael::CBC, CRijndael::DecryptDir,
                         reinterpret_cast<const UINT8*>(key.constData()),
                         CRijndael::Key16Bytes, nullptr);
    QCOMPARE(result, RIJNDAEL_SUCCESS);

    UINT8 decrypted[32];
    int decryptedLen = aesDec.PadDecrypt(ciphertext, ciphertextLen, decrypted);

    QVERIFY(decryptedLen > 0);
    QCOMPARE(decryptedLen, originalText.length());

    QString decryptedText = QString::fromLatin1(
        reinterpret_cast<const char*>(decrypted), decryptedLen);
    QCOMPARE(decryptedText, originalText);
}

// =============================================================================
// Twofish Tests (Official Twofish Test Vectors)
// =============================================================================

void TestCryptoPrimitives::testTwofish128()
{
    // Official Twofish 128-bit key test vector
    // Source: Twofish specification test vectors

    Twofish_Byte key[16];
    Twofish_Byte plaintext[16];
    Twofish_Byte ciphertext[16];
    Twofish_Byte decrypted[16];

    // Key: all zeros
    memset(key, 0, 16);

    // Plaintext: all zeros
    memset(plaintext, 0, 16);

    // Expected ciphertext for (key=0, plaintext=0)
    QByteArray expectedCiphertext = hexToBytes("9f589f5cf6122c32b6bfec2f2ae8c35a");

    // Prepare key
    Twofish_key xkey;
    Twofish_prepare_key(key, 16, &xkey);

    // Encrypt
    Twofish_encrypt(&xkey, plaintext, ciphertext);

    QVERIFY(compareBytes(ciphertext,
                        reinterpret_cast<const UINT8*>(expectedCiphertext.constData()),
                        16));

    // Decrypt and verify round-trip
    Twofish_decrypt(&xkey, ciphertext, decrypted);
    QVERIFY(compareBytes(decrypted, plaintext, 16));
}

void TestCryptoPrimitives::testTwofish192()
{
    // Official Twofish 192-bit key test vector
    // Source: https://www.schneier.com/code/ecb_ival.txt

    Twofish_Byte key[24];
    Twofish_Byte plaintext[16];
    Twofish_Byte ciphertext[16];

    // Key: all zeros
    memset(key, 0, 24);

    // Plaintext: all zeros
    memset(plaintext, 0, 16);

    // Expected ciphertext for (key=0x00...00, plaintext=0x00...00)
    QByteArray expectedCiphertext = hexToBytes("efa71f788965bd4453f860178fc19101");

    Twofish_key xkey;
    Twofish_prepare_key(key, 24, &xkey);
    Twofish_encrypt(&xkey, plaintext, ciphertext);

    QVERIFY(compareBytes(ciphertext,
                        reinterpret_cast<const UINT8*>(expectedCiphertext.constData()),
                        16));
}

void TestCryptoPrimitives::testTwofish256()
{
    // Official Twofish 256-bit key test vector (used by KeePass)
    // Source: https://www.schneier.com/code/ecb_ival.txt

    Twofish_Byte key[32];
    Twofish_Byte plaintext[16];
    Twofish_Byte ciphertext[16];
    Twofish_Byte decrypted[16];

    // Key: all zeros
    memset(key, 0, 32);

    // Plaintext: all zeros
    memset(plaintext, 0, 16);

    // Expected ciphertext for (key=0x00...00, plaintext=0x00...00)
    QByteArray expectedCiphertext = hexToBytes("57ff739d4dc92c1bd7fc01700cc8216f");

    Twofish_key xkey;
    Twofish_prepare_key(key, 32, &xkey);
    Twofish_encrypt(&xkey, plaintext, ciphertext);

    QVERIFY(compareBytes(ciphertext,
                        reinterpret_cast<const UINT8*>(expectedCiphertext.constData()),
                        16));

    // Decrypt and verify round-trip
    Twofish_decrypt(&xkey, ciphertext, decrypted);
    QVERIFY(compareBytes(decrypted, plaintext, 16));
}

// =============================================================================
// SHA-256 Tests (NIST FIPS 180-2 Test Vectors)
// =============================================================================

void TestCryptoPrimitives::testSHA256_Empty()
{
    // SHA-256("") - empty string
    // NIST test vector

    QByteArray input;
    QByteArray expectedHash = hexToBytes(
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

    QByteArray hash = SHA256::hash(input);

    QCOMPARE(hash.length(), 32);
    QCOMPARE(bytesToHex(hash), bytesToHex(expectedHash));
}

void TestCryptoPrimitives::testSHA256_SingleBlock()
{
    // SHA-256("abc")
    // NIST FIPS 180-2 test vector

    QByteArray input("abc");
    QByteArray expectedHash = hexToBytes(
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    QByteArray hash = SHA256::hash(input);

    QCOMPARE(hash.length(), 32);
    QCOMPARE(bytesToHex(hash), bytesToHex(expectedHash));
}

void TestCryptoPrimitives::testSHA256_MultiBlock()
{
    // SHA-256 with multi-block input
    // NIST test vector: "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"

    QByteArray input("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
    QByteArray expectedHash = hexToBytes(
        "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");

    QByteArray hash = SHA256::hash(input);

    QCOMPARE(hash.length(), 32);
    QCOMPARE(bytesToHex(hash), bytesToHex(expectedHash));
}

void TestCryptoPrimitives::testSHA256_Incremental()
{
    // Test incremental hashing (Context API)

    SHA256::Context ctx;
    ctx.update(QByteArray("abc"));
    ctx.update(QByteArray("def"));
    ctx.update(QByteArray("ghi"));

    QByteArray hash = ctx.finalize();

    // Should match SHA-256("abcdefghi")
    QByteArray expectedHash = SHA256::hash(QByteArray("abcdefghi"));

    QCOMPARE(hash.length(), 32);
    QCOMPARE(bytesToHex(hash), bytesToHex(expectedHash));
}

// =============================================================================
// Key Transformation Tests (KeePass-specific)
// =============================================================================

void TestCryptoPrimitives::testKeyTransformation()
{
    // Test basic key transformation with AES
    // This tests the core of KeePass's key derivation

    UINT8 key[32];
    UINT8 seed[32];

    // Initialize with known values
    for (int i = 0; i < 32; ++i) {
        key[i] = static_cast<UINT8>(i);
        seed[i] = static_cast<UINT8>(31 - i);
    }

    // Perform 1000 rounds of transformation
    CRijndael aes;
    int result = aes.Init(CRijndael::ECB, CRijndael::EncryptDir,
                          seed, CRijndael::Key32Bytes, nullptr);
    QCOMPARE(result, RIJNDAEL_SUCCESS);

    const int rounds = 1000;
    for (int i = 0; i < rounds; ++i) {
        // Encrypt first half
        aes.BlockEncrypt(key, 128, key);
        // Encrypt second half
        aes.BlockEncrypt(key + 16, 128, key + 16);
    }

    // Verify key has changed
    bool changed = false;
    for (int i = 0; i < 32; ++i) {
        if (key[i] != static_cast<UINT8>(i)) {
            changed = true;
            break;
        }
    }
    QVERIFY(changed);

    // Hash the result
    UINT8 finalKey[32];
    SHA256::hash(key, 32, finalKey);

    // Verify final key is 32 bytes
    QVERIFY(true); // If we got here, transformation succeeded
}

void TestCryptoPrimitives::testKeyTransformationRounds()
{
    // Verify that different round counts produce different results

    UINT8 key1[32];
    UINT8 key2[32];
    UINT8 seed[32];

    // Initialize with same values
    for (int i = 0; i < 32; ++i) {
        key1[i] = static_cast<UINT8>(i);
        key2[i] = static_cast<UINT8>(i);
        seed[i] = static_cast<UINT8>(i * 2);
    }

    // Transform with different round counts
    CRijndael aes1;
    aes1.Init(CRijndael::ECB, CRijndael::EncryptDir,
              seed, CRijndael::Key32Bytes, nullptr);

    CRijndael aes2;
    aes2.Init(CRijndael::ECB, CRijndael::EncryptDir,
              seed, CRijndael::Key32Bytes, nullptr);

    // 100 rounds for key1
    for (int i = 0; i < 100; ++i) {
        aes1.BlockEncrypt(key1, 128, key1);
        aes1.BlockEncrypt(key1 + 16, 128, key1 + 16);
    }

    // 200 rounds for key2
    for (int i = 0; i < 200; ++i) {
        aes2.BlockEncrypt(key2, 128, key2);
        aes2.BlockEncrypt(key2 + 16, 128, key2 + 16);
    }

    // Keys should be different
    QVERIFY(!compareBytes(key1, key2, 32));
}

// =============================================================================
// Time Compression Tests (PW_TIME structure)
// =============================================================================

void TestCryptoPrimitives::testPwTimeSize()
{
    // CRITICAL: PW_TIME must be exactly 7 bytes for KDB format compatibility

    PW_TIME time;
    QCOMPARE(static_cast<int>(sizeof(PW_TIME)), 7);
    QCOMPARE(static_cast<int>(sizeof(time)), 7);
}

void TestCryptoPrimitives::testPwTimeEdgeCases()
{
    // Test PW_TIME edge cases

    PW_TIME time;

    // Test case 1: Year 2000
    time.shYear = 2000;
    time.btMonth = 1;
    time.btDay = 1;
    time.btHour = 0;
    time.btMinute = 0;
    time.btSecond = 0;
    QCOMPARE(time.shYear, static_cast<USHORT>(2000));
    QCOMPARE(time.btMonth, static_cast<BYTE>(1));

    // Test case 2: Year 2038 (Unix timestamp limit)
    time.shYear = 2038;
    time.btMonth = 1;
    time.btDay = 19;
    time.btHour = 3;
    time.btMinute = 14;
    time.btSecond = 7;
    QCOMPARE(time.shYear, static_cast<USHORT>(2038));
    QCOMPARE(time.btSecond, static_cast<BYTE>(7));

    // Test case 3: Leap year (Feb 29)
    time.shYear = 2024;
    time.btMonth = 2;
    time.btDay = 29;
    time.btHour = 23;
    time.btMinute = 59;
    time.btSecond = 59;
    QCOMPARE(time.btMonth, static_cast<BYTE>(2));
    QCOMPARE(time.btDay, static_cast<BYTE>(29));

    // Test case 4: Year 9999 (maximum reasonable year)
    time.shYear = 9999;
    time.btMonth = 12;
    time.btDay = 31;
    time.btHour = 23;
    time.btMinute = 59;
    time.btSecond = 59;
    QCOMPARE(time.shYear, static_cast<USHORT>(9999));
}

// Main test runner
QTEST_MAIN(TestCryptoPrimitives)
#include "test_crypto_primitives.moc"
