/*
  Qt KeePass - Password Generator Implementation
  Reference: MFC/MFC-KeePass/KeePassLibCpp/PasswordGenerator/PasswordGenerator.cpp
*/

#include "PasswordGenerator.h"
#include "util/Random.h"
#include <QSet>
#include <QtMath>

QString PasswordGeneratorSettings::buildCharSet() const
{
    // If custom character set is specified, use it
    if (!customCharSet.isEmpty()) {
        return customCharSet;
    }

    // Otherwise, build from checkboxes
    QString result;

    if (includeUpperCase) result += PwCharSets::UpperCase;
    if (includeLowerCase) result += PwCharSets::LowerCase;
    if (includeDigits) result += PwCharSets::Digits;
    if (includeMinus) result += PwCharSets::Minus;
    if (includeUnderline) result += PwCharSets::Underline;
    if (includeSpace) result += PwCharSets::Space;
    if (includeSpecial) result += PwCharSets::Special;
    if (includeBrackets) result += PwCharSets::Brackets;

    return result;
}

bool PasswordGeneratorSettings::isValid(QString* error) const
{
    if (length == 0 || length > 30000) {
        if (error) *error = "Password length must be between 1 and 30000";
        return false;
    }

    QString charSet = buildCharSet();
    if (charSet.isEmpty()) {
        if (error) *error = "Character set is empty - select at least one character type";
        return false;
    }

    // If no repeat is enabled, character set must have enough characters
    if (noRepeatChars && charSet.length() < static_cast<int>(length)) {
        if (error) *error = QString("Character set has only %1 characters but password length is %2 (no repeat enabled)")
                            .arg(charSet.length()).arg(length);
        return false;
    }

    return true;
}

QString PasswordGenerator::generate(const PasswordGeneratorSettings& settings, QString* error)
{
    // Validate settings
    if (!settings.isValid(error)) {
        return QString();
    }

    // Build character set
    QString charSet = settings.buildCharSet();

    // Apply exclusions
    if (settings.excludeLookAlike) {
        charSet = removeChars(charSet, PwCharSets::ConfusingChars);
    }

    if (!settings.excludeChars.isEmpty()) {
        charSet = removeChars(charSet, settings.excludeChars);
    }

    // Remove duplicates from character set
    charSet = removeDuplicates(charSet);

    // Final validation
    if (charSet.isEmpty()) {
        if (error) *error = "Character set is empty after applying exclusions";
        return QString();
    }

    if (settings.noRepeatChars && charSet.length() < static_cast<int>(settings.length)) {
        if (error) *error = QString("Character set has only %1 characters but password length is %2")
                            .arg(charSet.length()).arg(settings.length);
        return QString();
    }

    // Generate password
    QString password;
    password.reserve(settings.length);

    QSet<QChar> usedChars;  // For no-repeat mode

    for (quint32 i = 0; i < settings.length; ++i) {
        QChar ch;
        int attempts = 0;
        const int maxAttempts = 1000;

        do {
            // Get random index in character set
            quint32 index = Random::generateUInt32() % charSet.length();
            ch = charSet.at(index);

            // If no repeat is enabled, check if we've used this character
            if (!settings.noRepeatChars || !usedChars.contains(ch)) {
                break;
            }

            ++attempts;
        } while (attempts < maxAttempts);

        if (attempts >= maxAttempts) {
            if (error) *error = "Failed to generate password (too many collisions in no-repeat mode)";
            return QString();
        }

        password.append(ch);
        if (settings.noRepeatChars) {
            usedChars.insert(ch);
        }
    }

    return password;
}

double PasswordGenerator::calculateEntropy(quint32 charSetSize, quint32 length)
{
    if (charSetSize == 0 || length == 0) {
        return 0.0;
    }

    // Entropy = log2(charSetSize^length) = length * log2(charSetSize)
    return static_cast<double>(length) * qLn(charSetSize) / qLn(2.0);
}

quint32 PasswordGenerator::calculateQuality(const QString& password)
{
    if (password.isEmpty()) {
        return 0;
    }

    // Count unique characters to estimate character set size
    QSet<QChar> uniqueChars;
    for (const QChar& ch : password) {
        uniqueChars.insert(ch);
    }

    // Estimate character set size based on what's used
    quint32 estimatedCharSetSize = uniqueChars.size();

    // If we have a very small unique set but long password, boost the estimate
    // (user might be using limited chars from a larger set)
    if (estimatedCharSetSize < 10) {
        estimatedCharSetSize = 10;
    }

    // Calculate entropy in bits
    double entropy = calculateEntropy(estimatedCharSetSize, password.length());

    // Map entropy to 0-100 quality scale
    // 0-40 bits = 0-33 (weak)
    // 40-80 bits = 33-66 (medium)
    // 80-128 bits = 66-100 (strong)
    // 128+ bits = 100 (very strong)

    quint32 quality;
    if (entropy < 40.0) {
        quality = static_cast<quint32>((entropy / 40.0) * 33.0);
    } else if (entropy < 80.0) {
        quality = 33 + static_cast<quint32>(((entropy - 40.0) / 40.0) * 33.0);
    } else if (entropy < 128.0) {
        quality = 66 + static_cast<quint32>(((entropy - 80.0) / 48.0) * 34.0);
    } else {
        quality = 100;
    }

    return qMin(quality, static_cast<quint32>(100));
}

PasswordGeneratorSettings PasswordGenerator::getDefaultSettings()
{
    // Reference: MFC PwgGetDefaultProfile - 20 chars, A-Z, a-z, 0-9
    PasswordGeneratorSettings settings;
    settings.length = 20;
    settings.includeUpperCase = true;
    settings.includeLowerCase = true;
    settings.includeDigits = true;
    settings.includeMinus = false;
    settings.includeUnderline = false;
    settings.includeSpace = false;
    settings.includeSpecial = false;
    settings.includeBrackets = false;
    settings.excludeLookAlike = false;
    settings.noRepeatChars = false;
    settings.customCharSet.clear();
    settings.excludeChars.clear();

    return settings;
}

QString PasswordGenerator::removeChars(const QString& charSet, const QString& charsToRemove)
{
    QString result;
    result.reserve(charSet.length());

    for (const QChar& ch : charSet) {
        if (!charsToRemove.contains(ch)) {
            result.append(ch);
        }
    }

    return result;
}

bool PasswordGenerator::hasDuplicates(const QString& charSet)
{
    QSet<QChar> seen;
    for (const QChar& ch : charSet) {
        if (seen.contains(ch)) {
            return true;
        }
        seen.insert(ch);
    }
    return false;
}

QString PasswordGenerator::removeDuplicates(const QString& str)
{
    QString result;
    QSet<QChar> seen;

    for (const QChar& ch : str) {
        if (!seen.contains(ch)) {
            result.append(ch);
            seen.insert(ch);
        }
    }

    return result;
}
