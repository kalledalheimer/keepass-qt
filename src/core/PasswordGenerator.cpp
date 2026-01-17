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
    if (mode == PasswordGeneratorMode::Pattern) {
        // Pattern mode validation
        if (pattern.isEmpty()) {
            if (error) *error = "Pattern is empty";
            return false;
        }
        // Pattern syntax will be validated during generation
        return true;
    }

    // Character set mode validation
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

    // Use pattern-based generation if in pattern mode
    if (settings.mode == PasswordGeneratorMode::Pattern) {
        return generateFromPattern(settings, error);
    }

    // Build character set for standard mode
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

QString PasswordGenerator::getCharSetForIdentifier(QChar identifier)
{
    // Reference: MFC PwCharSet::AddCharSet
    // Pattern character identifiers:
    // a = lowercase + digits           A = lowercase + uppercase + digits
    // U = uppercase + digits           c = lowercase consonants
    // C = lowercase + uppercase consonants  z = uppercase consonants
    // d = digits                        h = lowercase hex
    // H = uppercase hex                 l = lowercase
    // L = lowercase + uppercase         u = uppercase
    // p = punctuation                   b = brackets
    // s = special ASCII                 S = all printable
    // v = lowercase vowels              V = lowercase + uppercase vowels
    // Z = uppercase vowels

    switch (identifier.unicode()) {
    case 'a':
        return PwCharSets::LowerCase + PwCharSets::Digits;
    case 'A':
        return PwCharSets::LowerCase + PwCharSets::UpperCase + PwCharSets::Digits;
    case 'U':
        return PwCharSets::UpperCase + PwCharSets::Digits;
    case 'c':
        return PwCharSets::LowerConsonants;
    case 'C':
        return PwCharSets::LowerConsonants + PwCharSets::UpperConsonants;
    case 'z':
        return PwCharSets::UpperConsonants;
    case 'd':
        return PwCharSets::Digits;
    case 'h':
        return PwCharSets::LowerHex;
    case 'H':
        return PwCharSets::UpperHex;
    case 'l':
        return PwCharSets::LowerCase;
    case 'L':
        return PwCharSets::LowerCase + PwCharSets::UpperCase;
    case 'u':
        return PwCharSets::UpperCase;
    case 'p':
        return PwCharSets::Punctuation;
    case 'b':
        return PwCharSets::Brackets;
    case 's':
        return PwCharSets::Special;
    case 'S':
        return PwCharSets::UpperCase + PwCharSets::LowerCase +
               PwCharSets::Digits + PwCharSets::Special;
    case 'v':
        return PwCharSets::LowerVowels;
    case 'V':
        return PwCharSets::LowerVowels + PwCharSets::UpperVowels;
    case 'Z':
        return PwCharSets::UpperVowels;
    default:
        return QString();  // Unknown identifier
    }
}

void PasswordGenerator::shuffleString(QString& str)
{
    // Fisher-Yates shuffle
    for (int i = str.length() - 1; i > 0; --i) {
        int j = Random::generateUInt32() % (i + 1);
        QChar temp = str[i];
        str[i] = str[j];
        str[j] = temp;
    }
}

QString PasswordGenerator::generateFromPattern(const PasswordGeneratorSettings& settings, QString* error)
{
    // Reference: MFC PatternBasedGenerator.cpp PbgGenerate
    // Pattern syntax:
    // - Character identifiers (a, A, d, l, L, u, s, etc.) generate random char from that set
    // - \X = literal character X (escape)
    // - [abc] = custom char set (choose from a, b, c)
    // - [a^d] = char set 'a' excluding 'd' (^ toggles exclusion mode)
    // - X{n} = repeat character set X n times

    QString result;
    const QString& pattern = settings.pattern;
    int pos = 0;
    QSet<QChar> usedChars;  // For no-repeat mode

    while (pos < pattern.length()) {
        QChar ch = pattern[pos];
        QString charSet;

        if (ch == '\\') {
            // Escape sequence - use literal next character
            ++pos;
            if (pos >= pattern.length()) {
                if (error != nullptr) {
                    *error = "Invalid pattern: escape at end of pattern";
                }
                return QString();
            }
            charSet = QString(pattern[pos]);
            ++pos;
        }
        else if (ch == '[') {
            // Custom character set
            ++pos;
            bool exclude = false;

            while (pos < pattern.length() && pattern[pos] != ']') {
                QChar setCh = pattern[pos];

                if (setCh == '\\') {
                    // Escaped character in custom set
                    ++pos;
                    if (pos >= pattern.length()) {
                        if (error != nullptr) {
                            *error = "Invalid pattern: escape at end of custom character set";
                        }
                        return QString();
                    }
                    if (exclude) {
                        charSet.remove(pattern[pos]);
                    } else {
                        charSet += pattern[pos];
                    }
                }
                else if (setCh == '^') {
                    // Toggle exclusion mode
                    exclude = true;
                }
                else {
                    // Character set identifier or literal
                    QString subSet = getCharSetForIdentifier(setCh);
                    if (subSet.isEmpty()) {
                        // Not a known identifier, use as literal
                        subSet = QString(setCh);
                    }

                    if (exclude) {
                        for (const QChar& c : subSet) {
                            charSet.remove(c);
                        }
                    } else {
                        charSet += subSet;
                    }
                }
                ++pos;
            }

            if (pos >= pattern.length()) {
                if (error != nullptr) {
                    *error = "Invalid pattern: unclosed '[' bracket";
                }
                return QString();
            }
            ++pos;  // Skip closing ']'
        }
        else {
            // Character set identifier
            charSet = getCharSetForIdentifier(ch);
            if (charSet.isEmpty()) {
                if (error != nullptr) {
                    *error = QString("Invalid pattern: unknown character '%1'").arg(ch);
                }
                return QString();
            }
            ++pos;
        }

        // Check for repeat count {n}
        int repeatCount = 1;
        if (pos < pattern.length() && pattern[pos] == '{') {
            ++pos;
            QString countStr;
            while (pos < pattern.length() && pattern[pos].isDigit()) {
                countStr += pattern[pos];
                ++pos;
            }

            if (pos >= pattern.length() || pattern[pos] != '}' || countStr.isEmpty()) {
                if (error != nullptr) {
                    *error = "Invalid pattern: malformed repeat count";
                }
                return QString();
            }
            ++pos;  // Skip closing '}'

            bool ok = false;
            repeatCount = countStr.toInt(&ok);
            if (!ok || repeatCount < 0) {
                if (error != nullptr) {
                    *error = "Invalid pattern: invalid repeat count";
                }
                return QString();
            }
        }

        // Generate characters
        charSet = removeDuplicates(charSet);

        // Apply exclusions if in no-repeat mode
        if (settings.noRepeatChars) {
            for (const QChar& used : usedChars) {
                charSet.remove(used);
            }
        }

        if (charSet.isEmpty()) {
            if (error != nullptr) {
                *error = "Character set is empty (too few characters for no-repeat mode)";
            }
            return QString();
        }

        for (int i = 0; i < repeatCount; ++i) {
            QString currentCharSet = charSet;

            // For no-repeat, we need to update the available chars after each generation
            if (settings.noRepeatChars && i > 0) {
                for (const QChar& used : usedChars) {
                    currentCharSet.remove(used);
                }
                if (currentCharSet.isEmpty()) {
                    if (error != nullptr) {
                        *error = "Not enough unique characters for no-repeat mode";
                    }
                    return QString();
                }
            }

            int index = Random::generateUInt32() % currentCharSet.length();
            QChar generated = currentCharSet[index];
            result += generated;

            if (settings.noRepeatChars) {
                usedChars.insert(generated);
            }
        }
    }

    // Apply permutation if requested
    if (settings.patternPermute) {
        shuffleString(result);
    }

    return result;
}
