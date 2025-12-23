// Unit tests for CSV parsing functions
#include <QTest>

#include "../CsvParser.h"

using namespace CsvParser;

class CsvParserTest : public QObject {
  Q_OBJECT

private slots:
  // parseAmount tests
  void parseAmount_SimpleNegative() { QCOMPARE(parseAmount("-52,30"), -52.30); }

  void parseAmount_SimplePositive() { QCOMPARE(parseAmount("+45,00"), 45.00); }

  void parseAmount_PositiveNoSign() {
    QCOMPARE(parseAmount("2500,00"), 2500.00);
  }

  void parseAmount_FrenchFormatWithEuro() {
    // From a.csv: "-5 428,69 €"
    QCOMPARE(parseAmount("-5 428,69 €"), -5428.69);
  }

  void parseAmount_FrenchFormatWithEuroQuoted() {
    // From a.csv (quoted): "\"-5 428,69 €\""
    QCOMPARE(parseAmount("\"-5 428,69 €\""), -5428.69);
  }

  void parseAmount_SmallNegative() {
    QCOMPARE(parseAmount("-379,99 €"), -379.99);
  }

  void parseAmount_PositiveCredit() {
    // From example_import.csv credit column: "+2500,00"
    QCOMPARE(parseAmount("+2500,00"), 2500.00);
  }

  void parseAmount_NonBreakingSpace() {
    // Amount with non-breaking space (0xA0)
    QString amountWithNbsp = QString("-5") + QChar(0xA0) + QString("428,69");
    QCOMPARE(parseAmount(amountWithNbsp), -5428.69);
  }

  void parseAmount_NarrowNoBreakSpace() {
    // Amount with narrow no-break space (U+202F) - used by French banks
    QString amountWithNnbsp =
        QString("-5") + QChar(0x202F) + QString("428,69 €");
    QCOMPARE(parseAmount(amountWithNnbsp), -5428.69);
  }

  void parseAmount_NarrowNoBreakSpaceQuoted() {
    // Exact bytes from a.csv: "-5 428,69 €" with U+202F (narrow no-break space)
    // UTF-8: 22 2d 35 e2 80 af 34 32 38 2c 36 39 20 e2 82 ac 22
    QByteArray rawBytes;
    rawBytes.append("\"-5");
    rawBytes.append("\xe2\x80\xaf");  // U+202F narrow no-break space
    rawBytes.append("428,69 ");
    rawBytes.append("\xe2\x82\xac");  // € Euro sign
    rawBytes.append("\"");
    QString amountStr = QString::fromUtf8(rawBytes);
    QCOMPARE(parseAmount(amountStr), -5428.69);
  }

  void parseAmount_Empty() { QCOMPARE(parseAmount(""), 0.0); }

  void parseAmount_Whitespace() { QCOMPARE(parseAmount("   "), 0.0); }

  // parseCsvLine tests
  void parseCsvLine_Semicolon() {
    QString line =
        "28/11/2025;CARREFOUR MARKET;CB CARREFOUR MARKE FACT 261125;;;Carte "
        "bancaire;Alimentation;Hyper/supermarche;-52,30;;26/11/2025;28/11/"
        "2025;0";
    QStringList fields = parseCsvLine(line, ';');
    QCOMPARE(fields.size(), 13);
    QCOMPARE(fields[0], QString("28/11/2025"));
    QCOMPARE(fields[1], QString("CARREFOUR MARKET"));
    QCOMPARE(fields[8], QString("-52,30"));
  }

  void parseCsvLine_Comma() {
    QString line = "26/11/2022,M NICK LARSONO,VIR SEPA M NICK LARSONO";
    QStringList fields = parseCsvLine(line, ',');
    QCOMPARE(fields.size(), 3);
    QCOMPARE(fields[0], QString("26/11/2022"));
    QCOMPARE(fields[1], QString("M NICK LARSONO"));
  }

  void parseCsvLine_QuotedField() {
    // From a.csv: quoted amounts like "-5 428,69 €"
    QString line = "26/11/2022,M NICK LARSONO,VIR SEPA,\"-5 428,69 €\"";
    QStringList fields = parseCsvLine(line, ',');
    QCOMPARE(fields.size(), 4);
    QCOMPARE(fields[3], QString("-5 428,69 €"));
  }

  void parseCsvLine_QuotedFieldWithComma() {
    QString line = "date,\"label, with comma\",amount";
    QStringList fields = parseCsvLine(line, ',');
    QCOMPARE(fields.size(), 3);
    QCOMPARE(fields[1], QString("label, with comma"));
  }

  void parseCsvLine_EscapedQuote() {
    QString line = "date,\"label with \"\"quotes\"\"\",amount";
    QStringList fields = parseCsvLine(line, ',');
    QCOMPARE(fields.size(), 3);
    QCOMPARE(fields[1], QString("label with \"quotes\""));
  }

  void parseCsvLine_EmptyFields() {
    QString line = "date;;;amount";
    QStringList fields = parseCsvLine(line, ';');
    QCOMPARE(fields.size(), 4);
    QCOMPARE(fields[1], QString(""));
    QCOMPARE(fields[2], QString(""));
  }

  // parseHeader tests
  void parseHeader_SemicolonFormat() {
    // Header from example_import.csv
    // Last category column wins: "Sous categorie" at index 7
    QString header =
        "Date de comptabilisation;Libelle simplifie;Libelle "
        "operation;Reference;Informations complementaires;Type "
        "operation;Categorie;Sous categorie;Debit;Credit;Date "
        "operation;Date de valeur;Pointage operation";
    QStringList fields = parseCsvLine(header, ';');
    CsvFieldIndices idx = parseHeader(fields);

    QCOMPARE(idx.date, 0);
    QCOMPARE(idx.label, 1);
    QCOMPARE(idx.category, 7);  // Last match: "Sous categorie"
    QCOMPARE(idx.debit, 8);
    QCOMPARE(idx.credit, 9);
    QVERIFY(idx.isValid());
  }

  void parseHeader_CommaFormat() {
    // Header from a.csv (with accents)
    // Last category column wins: "Sous catégorie CE" at index 7
    QString header =
        "Date,Libellé simplifié,Libellé,Réference,Informations "
        "complémentaires,Type opération,Catégorie CE,Sous "
        "catégorie CE,Débit,Crédit,Date opération,Date de "
        "valeur,Pointage opération,Montant,Solde";
    QStringList fields = parseCsvLine(header, ',');
    CsvFieldIndices idx = parseHeader(fields);

    QCOMPARE(idx.date, 0);
    QCOMPARE(idx.label, 1);
    QCOMPARE(idx.category, 7);  // Last match: "Sous catégorie CE"
    QCOMPARE(idx.debit, 8);
    QCOMPARE(idx.credit, 9);
    QCOMPARE(idx.amount, 13);
    QVERIFY(idx.isValid());
  }

  void parseHeader_LastCategoryWins() {
    // Header with multiple category columns - last one should win
    // This simulates: "Catégorie CE", "Sous catégorie CE", "Catégorie"
    QString header =
        "Date,Libellé simplifié,Libellé,Réference,Informations "
        "complémentaires,Type opération,Catégorie CE,Sous "
        "catégorie CE,Débit,Crédit,Date opération,Date de "
        "valeur,Pointage opération,Montant,Solde,\"559,87 "
        "€\",Date budget,Catégorie,Compte,Check";
    QStringList fields = parseCsvLine(header, ',');
    CsvFieldIndices idx = parseHeader(fields);

    QCOMPARE(idx.date, 0);
    QCOMPARE(idx.label, 1);
    QCOMPARE(idx.category, 17);  // Last match: "Catégorie" at index 17
    QCOMPARE(idx.debit, 8);
    QCOMPARE(idx.credit, 9);
    QCOMPARE(idx.amount, 13);
    QVERIFY(idx.isValid());
  }

  void parseHeader_DateVsDateBudget() {
    // "Date budget" should NOT match as the date column - only "Date" should
    // Header from real CSV with both "Date" (0) and "Date budget" (15)
    QString header =
        "Date,Libellé simplifié,Libellé,Réference,Informations "
        "complémentaires,Type opération,Catégorie CE,Sous "
        "catégorie CE,Débit,Crédit,Date opération,Date de "
        "valeur,Pointage opération,Montant,Solde,Date budget,Catégorie,Compte,Check";
    QStringList fields = parseCsvLine(header, ',');
    CsvFieldIndices idx = parseHeader(fields);

    // Date should be index 0 ("Date"), not index 15 ("Date budget")
    QCOMPARE(idx.date, 0);
    QCOMPARE(idx.budgetDate, 15);  // "Date budget" is detected separately
    QCOMPARE(fields[0], QString("Date"));
    QCOMPARE(fields[15], QString("Date budget"));
  }

  void parseHeader_OperationAsLabel() {
    // Header from Budget - Cash.csv uses "Opération" as label column
    QString header = "Date,Montant,Opération,Catégorie,Solde,\"618,93 €\",Date budget,Compte";
    QStringList fields = parseCsvLine(header, ',');
    CsvFieldIndices idx = parseHeader(fields);

    QCOMPARE(idx.date, 0);
    QCOMPARE(idx.label, 2);       // "Opération" maps to label
    QCOMPARE(idx.amount, 1);      // "Montant"
    QCOMPARE(idx.category, 3);    // "Catégorie"
    QCOMPARE(idx.budgetDate, 6);  // "Date budget"
    QVERIFY(idx.isValid());
  }

  void parseHeader_NormalizeAccents() {
    QCOMPARE(normalizeHeader("Débit"), QString("debit"));
    QCOMPARE(normalizeHeader("Crédit"), QString("credit"));
    QCOMPARE(normalizeHeader("Catégorie"), QString("categorie"));
    QCOMPARE(normalizeHeader("Libellé simplifié"),
             QString("libelle simplifie"));
  }

  // getField tests
  void getField_Valid() {
    QStringList fields = { "a", "b", "c" };
    QCOMPARE(getField(fields, 1), QString("b"));
  }

  void getField_OutOfBounds() {
    QStringList fields = { "a", "b", "c" };
    QCOMPARE(getField(fields, 10), QString(""));
  }

  void getField_Negative() {
    QStringList fields = { "a", "b", "c" };
    QCOMPARE(getField(fields, -1), QString(""));
  }

  void getField_Trimmed() {
    QStringList fields = { "  value  " };
    QCOMPARE(getField(fields, 0), QString("value"));
  }

  // isEmptyLine tests
  void isEmptyLine_Empty() { QVERIFY(isEmptyLine("", ';')); }

  void isEmptyLine_OnlyDelimiters() { QVERIFY(isEmptyLine(";;;", ';')); }

  void isEmptyLine_WithContent() { QVERIFY(!isEmptyLine("a;b;c", ';')); }

  void isEmptyLine_Whitespace() { QVERIFY(isEmptyLine("  ;  ;  ", ';')); }

  // Integration tests with actual CSV files
  void integration_ExampleImportCsv() {
    // Line 2 from example_import.csv
    QString line =
        "28/11/2025;CARREFOUR MARKET;CB CARREFOUR MARKE FACT 261125;;;Carte "
        "bancaire;Alimentation;Hyper/supermarche;-52,30;;26/11/2025;28/11/"
        "2025;0";
    QStringList fields = parseCsvLine(line, ';');

    // Parse header first
    QString header =
        "Date de comptabilisation;Libelle simplifie;Libelle "
        "operation;Reference;Informations complementaires;Type "
        "operation;Categorie;Sous categorie;Debit;Credit;Date "
        "operation;Date de valeur;Pointage operation";
    CsvFieldIndices idx = parseHeader(parseCsvLine(header, ';'));

    QCOMPARE(getField(fields, idx.date), QString("28/11/2025"));
    QCOMPARE(getField(fields, idx.label), QString("CARREFOUR MARKET"));
    // Last match is "Sous categorie" at index 7, which maps to "Hyper/supermarche"
    QCOMPARE(getField(fields, idx.category), QString("Hyper/supermarche"));
    QCOMPARE(parseAmount(getField(fields, idx.debit)), -52.30);
  }

  void integration_ACsv() {
    // Line 2 from a.csv (with accents and quoted fields)
    QString line =
        "26/11/2022,M NICK LARSONO,VIR SEPA M NICK "
        "LARSONO,9876543XY0012345,VIREMENT VERS CPT DEPOT "
        "PART.-,Virement,Transaction exclue,Virement interne,\"-5 428,69 "
        "€\",,26/11/2022,16/11/2022,0,\"-5 428,69 €\",\"9 103,13 "
        "€\",,01/12/2022,Virement interne,Livret A2,\"9 103,13 €\"";
    QStringList fields = parseCsvLine(line, ',');

    // Parse header first - this header has "Catégorie CE" (6), "Sous catégorie CE" (7), and "Catégorie" (17)
    QString header =
        "Date,Libellé simplifié,Libellé,Réference,Informations "
        "complémentaires,Type opération,Catégorie CE,Sous "
        "catégorie CE,Débit,Crédit,Date opération,Date de "
        "valeur,Pointage opération,Montant,Solde,\"559,87 "
        "€\",Date budget,Catégorie,Compte,Check";
    CsvFieldIndices idx = parseHeader(parseCsvLine(header, ','));

    QCOMPARE(getField(fields, idx.date), QString("26/11/2022"));
    QCOMPARE(getField(fields, idx.label), QString("M NICK LARSONO"));
    // Last match is "Catégorie" at index 17, which maps to "Virement interne"
    QCOMPARE(getField(fields, idx.category), QString("Virement interne"));

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
        "02/11/2025;ENTREPRISE MARTIN SA;VIR SEPA ENTREPRISE MARTIN "
        "SA;2528285K10567890;SALAIRE NOVEMBRE 2025-;Virement recu;Revenus et "
        "rentrees d'argent;Salaires;;+2500,00;02/11/2025;02/11/2025;0";
    QStringList fields = parseCsvLine(line, ';');

    QString header =
        "Date de comptabilisation;Libelle simplifie;Libelle "
        "operation;Reference;Informations complementaires;Type "
        "operation;Categorie;Sous categorie;Debit;Credit;Date "
        "operation;Date de valeur;Pointage operation";
    CsvFieldIndices idx = parseHeader(parseCsvLine(header, ';'));

    QString debitStr = getField(fields, idx.debit);
    QString creditStr = getField(fields, idx.credit);

    // Debit is empty, credit has +2500,00
    QCOMPARE(debitStr, QString(""));
    QCOMPARE(parseAmount(creditStr), 2500.00);
  }

  void integration_BudgetCashCsv() {
    // Line from Budget - Cash.csv - uses "Opération" instead of "Libellé"
    QString header = "Date,Montant,Opération,Catégorie,Solde,\"618,93 €\",Date budget,Compte";
    QString line = "13/05/2020,\"-15,00 €\",Panier producteur,Courses du quotidien,\"110,00 €\",,13/05/2020,Cash";

    QStringList headerFields = parseCsvLine(header, ',');
    QStringList fields = parseCsvLine(line, ',');
    CsvFieldIndices idx = parseHeader(headerFields);

    QCOMPARE(getField(fields, idx.date), QString("13/05/2020"));
    QCOMPARE(getField(fields, idx.label), QString("Panier producteur"));
    QCOMPARE(getField(fields, idx.category), QString("Courses du quotidien"));
    QCOMPARE(parseAmount(getField(fields, idx.amount)), -15.00);
    QCOMPARE(getField(fields, idx.budgetDate), QString("13/05/2020"));
  }
};

QTEST_GUILESS_MAIN(CsvParserTest)
#include "CsvParserTest.moc"
