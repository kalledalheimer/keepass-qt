/*
  Qt KeePass - Password Generator
  Reference: MFC/MFC-KeePass/KeePassLibCpp/PasswordGenerator/PasswordGenerator.h
*/

#ifndef PASSWORDGENERATOR_H
#define PASSWORDGENERATOR_H

#include <QString>
#include <QList>

/// Character set definitions (matching MFC PDCS_* constants)
namespace PwCharSets {
    const QString UpperCase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const QString LowerCase = "abcdefghijklmnopqrstuvwxyz";
    const QString Digits = "0123456789";
    const QString Minus = "-";
    const QString Underline = "_";
    const QString Space = " ";
    const QString Special = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
    const QString Brackets = "[]{}()<>";
    const QString ConfusingChars = "O0Il1|";  // Characters that look similar
    const QString Punctuation = ",.;:";
    const QString LowerHex = "0123456789abcdef";
    const QString UpperHex = "0123456789ABCDEF";
    const QString LowerVowels = "aeiou";
    const QString UpperVowels = "AEIOU";
    const QString LowerConsonants = "bcdfghjklmnpqrstvwxyz";
    const QString UpperConsonants = "BCDFGHJKLMNPQRSTVWXYZ";
}

/// Password generation mode
enum class PasswordGeneratorMode : quint8 {
    CharacterSet,  // Traditional character-set based generation
    Pattern        // Pattern-based generation (e.g., "dddd-LLLL-ssss")
};

/// Password generator settings
struct PasswordGeneratorSettings
{
    // Generation mode
    PasswordGeneratorMode mode = PasswordGeneratorMode::CharacterSet;

    // Length (used for CharacterSet mode)
    quint32 length = 20;

    // Character sets (checkbox flags, used for CharacterSet mode)
    bool includeUpperCase = true;
    bool includeLowerCase = true;
    bool includeDigits = true;
    bool includeMinus = false;
    bool includeUnderline = false;
    bool includeSpace = false;
    bool includeSpecial = false;
    bool includeBrackets = false;

    // Custom character set (overrides checkboxes if not empty)
    QString customCharSet;

    // Advanced options
    bool excludeLookAlike = false;  // Exclude confusing characters (O0Il1|)
    bool noRepeatChars = false;     // Each character used at most once
    QString excludeChars;           // Additional characters to exclude

    // Pattern-based settings
    QString pattern;          // Pattern string (e.g., "dddd-LLLL-ssss")
    bool patternPermute = false;  // Shuffle generated password after pattern generation

    // Build the effective character set based on settings
    QString buildCharSet() const;

    // Check if settings are valid
    bool isValid(QString* error = nullptr) const;
};

/// Password generator class
class PasswordGenerator
{
public:
    /// Generate a password using the given settings
    /// Returns empty string on error, sets error message if provided
    static QString generate(const PasswordGeneratorSettings& settings, QString* error = nullptr);

    /// Calculate password entropy (bits) for a given character set size and length
    static double calculateEntropy(quint32 charSetSize, quint32 length);

    /// Calculate password quality/strength (0-100 scale)
    /// Based on entropy: 0-40 bits = weak, 40-80 = medium, 80+ = strong
    static quint32 calculateQuality(const QString& password);

    /// Get the default settings (matching MFC defaults)
    static PasswordGeneratorSettings getDefaultSettings();

private:
    /// Remove characters from a character set
    static QString removeChars(const QString& charSet, const QString& charsToRemove);

    /// Check if character set has duplicates
    static bool hasDuplicates(const QString& charSet);

    /// Remove duplicate characters from a string
    static QString removeDuplicates(const QString& str);

    /// Generate password from pattern
    static QString generateFromPattern(const PasswordGeneratorSettings& settings, QString* error);

    /// Get character set for a pattern character identifier
    static QString getCharSetForIdentifier(QChar identifier);

    /// Shuffle a string randomly (for pattern permutation)
    static void shuffleString(QString& str);
};

#endif // PASSWORDGENERATOR_H
