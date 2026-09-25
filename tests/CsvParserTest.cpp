// Unit tests for CSV parsing functions
#include <QTest>

#include "services/CsvParser.h"

using namespace CsvParser;
using namespace Qt::StringLiterals;

class CsvParserTest : public QObject {
  Q_OBJECT

private slots:
  // parseAmount tests
  void parseAmount_SimpleNegative() { QCOMPARE(parseAmount(u"-52,30"_s), -52.30); }

  void parseAmount_SimplePositive() { QCOMPARE(parseAmount(u"+45,00"_s), 45.00); }

  void parseAmount_PositiveNoSign() {
    QCOMPARE(parseAmount(u"2500,00"_s), 2500.00);
  }

  void parseAmount_FrenchFormatWithEuro() {
    // From a.csv: "-5 428,69 €"_L1
    QCOMPARE(parseAmount(u"-5 428,69 €"_s), -5428.69);
  }

  void parseAmount_FrenchFormatWithEuroQuoted() {
    // From a.csv (quoted): "\"-5 428,69 €\""_L1
    QCOMPARE(parseAmount(u"\"-5 428,69 €\""_s), -5428.69);
  }

  void parseAmount_SmallNegative() {
    QCOMPARE(parseAmount(u"-379,99 €"_s), -379.99);
  }

  void parseAmount_PositiveCredit() {
    // From example_import.csv credit column: "+2500,00"_L1
    QCOMPARE(parseAmount(u"+2500,00"_s), 2500.00);
  }

  void parseAmount_NonBreakingSpace() {
    // Amount with non-breaking space (0xA0)
    QString amountWithNbsp = u"-5"_s + QChar(0xA0) + u"428,69"_s;
    QCOMPARE(parseAmount(amountWithNbsp), -5428.69);
  }

  void parseAmount_NarrowNoBreakSpace() {
    // Amount with narrow no-break space (U+202F) - used by French banks
    QString amountWithNnbsp =
        u"-5"_s + QChar(0x202F) + u"428,69 €"_s;
    QCOMPARE(parseAmount(amountWithNnbsp), -5428.69);
  }

  void parseAmount_NarrowNoBreakSpaceQuoted() {
    // Exact bytes from a.csv: "-5 428,69 €"_L1 with U+202F (narrow no-break space)
    // UTF-8: 22 2d 35 e2 80 af 34 32 38 2c 36 39 20 e2 82 ac 22
    QByteArray rawBytes;
    rawBytes.append("\"-5"_ba);
    rawBytes.append("\xe2\x80\xaf"_ba);  // U+202F narrow no-break space
    rawBytes.append("428,69 "_ba);
    rawBytes.append("\xe2\x82\xac"_ba);  // € Euro sign
    rawBytes.append("\""_ba);
    QString amountStr = QString::fromUtf8(rawBytes);
    QCOMPARE(parseAmount(amountStr), -5428.69);
  }

  void parseAmount_Empty() { QCOMPARE(parseAmount(u""_s), 0.0); }

  void parseAmount_Whitespace() { QCOMPARE(parseAmount(u"   "_s), 0.0); }

  // parseCsvLine tests
  void parseCsvLine_Semicolon() {
    QString line =
        u"28/11/2025;CARREFOUR MARKET;CB CARREFOUR MARKE FACT 261125;;;Carte "_s
        u"bancaire;Alimentation;Hyper/supermarche;-52,30;;26/11/2025;28/11/"_s
        u"2025;0"_s;
    QStringList fields = parseCsvLine(line, ';');
    QCOMPARE(fields.size(), 13);
    QCOMPARE(fields[0], u"28/11/2025"_s);
    QCOMPARE(fields[1], u"CARREFOUR MARKET"_s);
    QCOMPARE(fields[8], u"-52,30"_s);
  }

  void parseCsvLine_Comma() {
    QString line = u"26/11/2022,M NICK LARSONO,VIR SEPA M NICK LARSONO"_s;
    QStringList fields = parseCsvLine(line, ',');
    QCOMPARE(fields.size(), 3);
    QCOMPARE(fields[0], u"26/11/2022"_s);
    QCOMPARE(fields[1], u"M NICK LARSONO"_s);
  }

  void parseCsvLine_QuotedField() {
    // From a.csv: quoted amounts like "-5 428,69 €"_L1
    QString line = u"26/11/2022,M NICK LARSONO,VIR SEPA,\"-5 428,69 €\""_s;
    QStringList fields = parseCsvLine(line, ',');
    QCOMPARE(fields.size(), 4);
    QCOMPARE(fields[3], u"-5 428,69 €"_s);
  }

  void parseCsvLine_QuotedFieldWithComma() {
    QString line = u"date,\"label, with comma\",amount"_s;
    QStringList fields = parseCsvLine(line, ',');
    QCOMPARE(fields.size(), 3);
    QCOMPARE(fields[1], u"label, with comma"_s);
  }

  void parseCsvLine_EscapedQuote() {
    QString line = u"date,\"label with \"\"quotes\"\"\",amount"_s;
    QStringList fields = parseCsvLine(line, ',');
    QCOMPARE(fields.size(), 3);
    QCOMPARE(fields[1], u"label with \"quotes\""_s);
  }

  void parseCsvLine_EmptyFields() {
    QString line = u"date;;;amount"_s;
    QStringList fields = parseCsvLine(line, ';');
    QCOMPARE(fields.size(), 4);
    QCOMPARE(fields[1], u""_s);
    QCOMPARE(fields[2], u""_s);
  }

  // parseHeader tests
  void parseHeader_SemicolonFormat() {
    // Header from example_import.csv
    // Last category column wins: "Sous categorie"_L1 at index 7
    QString header =
        u"Date de comptabilisation;Libelle simplifie;Libelle "_s
        u"operation;Reference;Informations complementaires;Type "_s
        u"operation;Categorie;Sous categorie;Debit;Credit;Date "_s
        u"operation;Date de valeur;Pointage operation"_s;
    QStringList fields = parseCsvLine(header, ';');
    CsvFieldIndices idx = parseHeader(fields);

    QCOMPARE(idx.date, 0);
    QCOMPARE(idx.label, 1);
    QCOMPARE(idx.category, 7);  // Last match: "Sous categorie"_L1
    QCOMPARE(idx.debit, 8);
    QCOMPARE(idx.credit, 9);
    QVERIFY(idx.isValid());
  }

  void parseHeader_CommaFormat() {
    // Header from a.csv (with accents)
    // Last category column wins: "Sous catégorie CE"_L1 at index 7
    QString header =
        u"Date,Libellé simplifié,Libellé,Réference,Informations "_s
        u"complémentaires,Type opération,Catégorie CE,Sous "_s
        u"catégorie CE,Débit,Crédit,Date opération,Date de "_s
        u"valeur,Pointage opération,Montant,Solde"_s;
    QStringList fields = parseCsvLine(header, ',');
    CsvFieldIndices idx = parseHeader(fields);

    QCOMPARE(idx.date, 0);
    QCOMPARE(idx.label, 1);
    QCOMPARE(idx.category, 7);  // Last match: "Sous catégorie CE"_L1
    QCOMPARE(idx.debit, 8);
    QCOMPARE(idx.credit, 9);
    QCOMPARE(idx.amount, 13);
    QVERIFY(idx.isValid());
  }

  void parseHeader_LastCategoryWins() {
    // Header with multiple category columns - last one should win
    // This simulates: "Catégorie CE"_L1, "Sous catégorie CE"_L1, "Catégorie"_L1
    QString header =
        u"Date,Libellé simplifié,Libellé,Réference,Informations "_s
        u"complémentaires,Type opération,Catégorie CE,Sous "_s
        u"catégorie CE,Débit,Crédit,Date opération,Date de "_s
        u"valeur,Pointage opération,Montant,Solde,\"559,87 "_s
        u"€\",Date budget,Catégorie,Compte,Check"_s;
    QStringList fields = parseCsvLine(header, ',');
    CsvFieldIndices idx = parseHeader(fields);

    QCOMPARE(idx.date, 0);
    QCOMPARE(idx.label, 1);
    QCOMPARE(idx.category, 17);  // Last match: "Catégorie"_L1 at index 17
    QCOMPARE(idx.debit, 8);
    QCOMPARE(idx.credit, 9);
    QCOMPARE(idx.amount, 13);
    QVERIFY(idx.isValid());
  }

  void parseHeader_DateVsDateBudget() {
    // "Date budget"_L1 should NOT match as the date column - only "Date"_L1 should
    // Header from real CSV with both "Date"_L1 (0) and "Date budget"_L1 (15)
    QString header =
        u"Date,Libellé simplifié,Libellé,Réference,Informations "_s
        u"complémentaires,Type opération,Catégorie CE,Sous "_s
        u"catégorie CE,Débit,Crédit,Date opération,Date de "_s
        u"valeur,Pointage opération,Montant,Solde,Date budget,Catégorie,Compte,Check"_s;
    QStringList fields = parseCsvLine(header, ',');
    CsvFieldIndices idx = parseHeader(fields);

    // Date should be index 0 ("Date"_L1), not index 15 ("Date budget"_L1)
    QCOMPARE(idx.date, 0);
    QCOMPARE(idx.budgetDate, 15);  // "Date budget"_L1 is detected separately
    QCOMPARE(fields[0], u"Date"_s);
    QCOMPARE(fields[15], u"Date budget"_s);
  }

  void parseHeader_OperationAsLabel() {
    // Header from Budget - Cash.csv uses "Opération"_L1 as label column
    QString header = u"Date,Montant,Opération,Catégorie,Solde,\"618,93 €\",Date budget,Compte"_s;
    QStringList fields = parseCsvLine(header, ',');
    CsvFieldIndices idx = parseHeader(fields);

    QCOMPARE(idx.date, 0);
    QCOMPARE(idx.label, 2);       // "Opération"_L1 maps to label
    QCOMPARE(idx.amount, 1);      // "Montant"_L1
    QCOMPARE(idx.category, 3);    // "Catégorie"_L1
    QCOMPARE(idx.budgetDate, 6);  // "Date budget"_L1
    QVERIFY(idx.isValid());
  }

  void parseHeader_NormalizeAccents() {
    QCOMPARE(normalizeHeader(u"Débit"_s), u"debit"_s);
    QCOMPARE(normalizeHeader(u"Crédit"_s), u"credit"_s);
    QCOMPARE(normalizeHeader(u"Catégorie"_s), u"categorie"_s);
    QCOMPARE(normalizeHeader(u"Libellé simplifié"_s),
             u"libelle simplifie"_s);
  }

  // getField tests
  void getField_Valid() {
    QStringList fields = { u"a"_s, u"b"_s, u"c"_s };
    QCOMPARE(getField(fields, 1), u"b"_s);
  }

  void getField_OutOfBounds() {
    QStringList fields = { u"a"_s, u"b"_s, u"c"_s };
    QCOMPARE(getField(fields, 10), u""_s);
  }

  void getField_Negative() {
    QStringList fields = { u"a"_s, u"b"_s, u"c"_s };
    QCOMPARE(getField(fields, -1), u""_s);
  }

  void getField_Trimmed() {
    QStringList fields = { u"  value  "_s };
    QCOMPARE(getField(fields, 0), u"value"_s);
  }

  // isEmptyLine tests
  void isEmptyLine_Empty() { QVERIFY(isEmptyLine(u""_s, ';')); }

  void isEmptyLine_OnlyDelimiters() { QVERIFY(isEmptyLine(u";;;"_s, ';')); }

  void isEmptyLine_WithContent() { QVERIFY(!isEmptyLine(u"a;b;c"_s, ';')); }

  void isEmptyLine_Whitespace() { QVERIFY(isEmptyLine(u"  ;  ;  "_s, ';')); }

  // Integration tests with actual CSV files
  void integration_ExampleImportCsv() {
    // Line 2 from example_import.csv
    QString line =
        u"28/11/2025;CARREFOUR MARKET;CB CARREFOUR MARKE FACT 261125;;;Carte "_s
        u"bancaire;Alimentation;Hyper/supermarche;-52,30;;26/11/2025;28/11/"_s
        u"2025;0"_s;
    QStringList fields = parseCsvLine(line, ';');

    // Parse header first
    QString header =
        u"Date de comptabilisation;Libelle simplifie;Libelle "_s
        u"operation;Reference;Informations complementaires;Type "_s
        u"operation;Categorie;Sous categorie;Debit;Credit;Date "_s
        u"operation;Date de valeur;Pointage operation"_s;
    CsvFieldIndices idx = parseHeader(parseCsvLine(header, ';'));

    QCOMPARE(getField(fields, idx.date), u"28/11/2025"_s);
    QCOMPARE(getField(fields, idx.label), u"CARREFOUR MARKET"_s);
    // Last match is "Sous categorie"_L1 at index 7, which maps to "Hyper/supermarche"_L1
    QCOMPARE(getField(fields, idx.category), u"Hyper/supermarche"_s);
    QCOMPARE(parseAmount(getField(fields, idx.debit)), -52.30);
  }

  void integration_ACsv() {
    // Line 2 from a.csv (with accents and quoted fields)
    QString line =
        u"26/11/2022,M NICK LARSONO,VIR SEPA M NICK "_s
        u"LARSONO,9876543XY0012345,VIREMENT VERS CPT DEPOT "_s
        u"PART.-,Virement,Transaction exclue,Virement interne,\"-5 428,69 "_s
        u"€\",,26/11/2022,16/11/2022,0,\"-5 428,69 €\",\"9 103,13 "_s
        u"€\",,01/12/2022,Virement interne,Livret A2,\"9 103,13 €\""_s;
    QStringList fields = parseCsvLine(line, ',');

    // Parse header first - this header has "Catégorie CE"_L1 (6), "Sous catégorie CE"_L1 (7), and "Catégorie"_L1 (17)
    QString header =
        u"Date,Libellé simplifié,Libellé,Réference,Informations "_s
        u"complémentaires,Type opération,Catégorie CE,Sous "_s
        u"catégorie CE,Débit,Crédit,Date opération,Date de "_s
        u"valeur,Pointage opération,Montant,Solde,\"559,87 "_s
        u"€\",Date budget,Catégorie,Compte,Check"_s;
    CsvFieldIndices idx = parseHeader(parseCsvLine(header, ','));

    QCOMPARE(getField(fields, idx.date), u"26/11/2022"_s);
    QCOMPARE(getField(fields, idx.label), u"M NICK LARSONO"_s);
    // Last match is "Catégorie"_L1 at index 17, which maps to "Virement interne"_L1
    QCOMPARE(getField(fields, idx.category), u"Virement interne"_s);

    // Test amount parsing - this is the key test!
    QString debitStr = getField(fields, idx.debit);
    QCOMPARE(parseAmount(debitStr), -5428.69);

    // Also test the montant column
    QString montantStr = getField(fields, idx.amount);
    QCOMPARE(parseAmount(montantStr), -5428.69);
  }

  void integration_PositiveCredit() {
    // Line with positive credit from example_import.csv
    QString line =
        u"02/11/2025;ENTREPRISE MARTIN SA;VIR SEPA ENTREPRISE MARTIN "_s
        u"SA;2528285K10567890;SALAIRE NOVEMBRE 2025-;Virement recu;Revenus et "_s
        u"rentrees d'argent;Salaires;;+2500,00;02/11/2025;02/11/2025;0"_s;
    QStringList fields = parseCsvLine(line, ';');

    QString header =
        u"Date de comptabilisation;Libelle simplifie;Libelle "_s
        u"operation;Reference;Informations complementaires;Type "_s
        u"operation;Categorie;Sous categorie;Debit;Credit;Date "_s
        u"operation;Date de valeur;Pointage operation"_s;
    CsvFieldIndices idx = parseHeader(parseCsvLine(header, ';'));

    QString debitStr = getField(fields, idx.debit);
    QString creditStr = getField(fields, idx.credit);

    // Debit is empty, credit has +2500,00
    QCOMPARE(debitStr, u""_s);
    QCOMPARE(parseAmount(creditStr), 2500.00);
  }

  void integration_BudgetCashCsv() {
    // Line from Budget - Cash.csv - uses "Opération"_L1 instead of "Libellé"_L1
    QString header = u"Date,Montant,Opération,Catégorie,Solde,\"618,93 €\",Date budget,Compte"_s;
    QString line = u"13/05/2020,\"-15,00 €\",Panier producteur,Courses du quotidien,\"110,00 €\",,13/05/2020,Cash"_s;

    QStringList headerFields = parseCsvLine(header, ',');
    QStringList fields = parseCsvLine(line, ',');
    CsvFieldIndices idx = parseHeader(headerFields);

    QCOMPARE(getField(fields, idx.date), u"13/05/2020"_s);
    QCOMPARE(getField(fields, idx.label), u"Panier producteur"_s);
    QCOMPARE(getField(fields, idx.category), u"Courses du quotidien"_s);
    QCOMPARE(parseAmount(getField(fields, idx.amount)), -15.00);
    QCOMPARE(getField(fields, idx.budgetDate), u"13/05/2020"_s);
  }
};

QTEST_GUILESS_MAIN(CsvParserTest)
#include "CsvParserTest.moc"
