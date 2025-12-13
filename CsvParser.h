// CSV parsing utilities for Comptine
#pragma once

#include <QString>
#include <QStringList>
#include <QtMath>

namespace CsvParser {

// Parse amount string handling French format (e.g., "-5 428,69 €" or "-5428.69")
inline double parseAmount(const QString& str) {
  QString cleaned = str.trimmed();
  cleaned.remove(' ');            // Remove regular spaces (thousand separators)
  cleaned.remove(QChar(0xA0));    // Remove non-breaking space (U+00A0)
  cleaned.remove(QChar(0x202F));  // Remove narrow no-break space (U+202F) - used by French banks
  cleaned.remove(QChar(0x20AC));  // Remove Euro symbol (€)
  cleaned.remove('"');            // Remove quotes

  // Handle plus sign for positive amounts (e.g., "+45,00")
  bool isPositive = cleaned.startsWith('+');
  if (isPositive) {
    cleaned = cleaned.mid(1);
  }

  cleaned.replace(',', '.');  // French decimal to standard

  double value = cleaned.toDouble();
  return isPositive ? qAbs(value) : value;
}

// Parse CSV line respecting quoted fields
inline QStringList parseCsvLine(const QString& line, QChar delimiter) {
  QStringList fields;
  QString field;
  bool inQuotes = false;

  for (int i = 0; i < line.length(); i++) {
    QChar c = line[i];

    if (c == '"') {
      if (inQuotes && i + 1 < line.length() && line[i + 1] == '"') {
        // Escaped quote
        field += '"';
        i++;
      } else {
        inQuotes = !inQuotes;
      }
    } else if (c == delimiter && !inQuotes) {
      fields.append(field);
      field.clear();
    } else {
      field += c;
    }
  }
  fields.append(field);  // Add last field

  return fields;
}

// Structure to hold detected CSV column indices
struct CsvFieldIndices {
  int date = -1;
  int description = -1;
  int category = -1;  // Last matching category column (most specific)
  int budgetDate = -1;
  int debit = -1;
  int credit = -1;
  int amount = -1;

  bool isValid() const {
    return date >= 0 && description >= 0 && (debit >= 0 || credit >= 0 || amount >= 0);
  }
};

// Safe field access - returns empty string if index is out of bounds
inline QString getField(const QStringList& fields, int index) {
  if (index >= 0 && index < fields.size()) {
    return fields[index].trimmed();
  }
  return QString();
}

// Normalize header for comparison (lowercase, remove accents)
inline QString normalizeHeader(const QString& header) {
  QString h = header.trimmed().toLower();
  // Remove common French accents
  h.replace(QString::fromUtf8("é"), "e");
  h.replace(QString::fromUtf8("è"), "e");
  h.replace(QString::fromUtf8("ê"), "e");
  h.replace(QString::fromUtf8("à"), "a");
  h.replace(QString::fromUtf8("â"), "a");
  h.replace(QString::fromUtf8("ô"), "o");
  h.replace(QString::fromUtf8("î"), "i");
  h.replace(QString::fromUtf8("ï"), "i");
  h.replace(QString::fromUtf8("ù"), "u");
  h.replace(QString::fromUtf8("û"), "u");
  h.replace(QString::fromUtf8("ç"), "c");
  return h;
}

// Parse header row to detect column indices
// For fields that can appear multiple times (category), the LAST match wins
inline CsvFieldIndices parseHeader(const QStringList& headerFields) {
  CsvFieldIndices indices;

  for (int i = 0; i < headerFields.size(); i++) {
    QString h = normalizeHeader(headerFields[i]);

    // Date column (first match wins)
    if (indices.date < 0 && (h == "date" || h == "date de comptabilisation")) {
      indices.date = i;
    }
    // Budget date column (first match wins)
    else if (indices.budgetDate < 0 && (h == "date budget" || h == "budget date")) {
      indices.budgetDate = i;
    }
    // Description column (first match wins)
    else if (indices.description < 0 && (h == "libelle simplifie" || h == "libelle" || h == "description" || h == "label" || h == "operation")) {
      indices.description = i;
    }
    // Category column (last match wins - keep updating)
    else if (h == "sous categorie ce" || h == "sous categorie" || h == "sub-category" || h == "subcategory" || h == "categorie ce" || h == "categorie" || h == "category") {
      indices.category = i;
    }
    // Debit column (first match wins)
    else if (indices.debit < 0 && (h == "debit")) {
      indices.debit = i;
    }
    // Credit column (first match wins)
    else if (indices.credit < 0 && (h == "credit")) {
      indices.credit = i;
    }
    // Single amount column (first match wins)
    else if (indices.amount < 0 && (h == "montant" || h == "amount")) {
      indices.amount = i;
    }
  }

  return indices;
}

// Check if line is empty (only delimiters and whitespace)
inline bool isEmptyLine(const QString& line, QChar delimiter) {
  QString stripped = line;
  stripped.remove(delimiter);
  stripped.remove('"');
  return stripped.trimmed().isEmpty();
}

}  // namespace CsvParser
