#include "TransactionModel.h"
#include <QDate>
#include <QDebug>
#include <QFile>
#include <QTextStream>

TransactionModel::TransactionModel(QObject *parent)
    : QAbstractTableModel(parent) {}

int TransactionModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return m_transactions.size();
}

int TransactionModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return 13; // Number of columns in the CSV
}

QVariant TransactionModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= m_transactions.size())
    return QVariant();

  const Transaction &transaction = m_transactions[index.row()];

  if (role == Qt::DisplayRole) {
    switch (index.column()) {
    case 0:
      return transaction.accountingDate.toString("dd/MM/yyyy");
    case 1:
      return transaction.simplifiedLabel;
    case 2:
      return transaction.operationLabel;
    case 3:
      return transaction.reference;
    case 4:
      return transaction.additionalInfo;
    case 5:
      return transaction.operationType;
    case 6:
      return transaction.category;
    case 7:
      return transaction.subCategory;
    case 8:
      return transaction.debit == 0.0 ? QString()
                                       : QString::number(transaction.debit, 'f', 2);
    case 9:
      return transaction.credit == 0.0 ? QString()
                                        : QString::number(transaction.credit, 'f', 2);
    case 10:
      return transaction.operationDate.toString("dd/MM/yyyy");
    case 11:
      return transaction.valueDate.toString("dd/MM/yyyy");
    case 12:
      return transaction.checkStatus;
    }
  } else {
    // For QML ListView/TableView
    switch (role) {
    case AccountingDateRole:
      return transaction.accountingDate.toString("dd/MM/yyyy");
    case SimplifiedLabelRole:
      return transaction.simplifiedLabel;
    case OperationLabelRole:
      return transaction.operationLabel;
    case ReferenceRole:
      return transaction.reference;
    case AdditionalInfoRole:
      return transaction.additionalInfo;
    case OperationTypeRole:
      return transaction.operationType;
    case CategoryRole:
      return transaction.category;
    case SubCategoryRole:
      return transaction.subCategory;
    case DebitRole:
      return transaction.debit;
    case CreditRole:
      return transaction.credit;
    case OperationDateRole:
      return transaction.operationDate.toString("dd/MM/yyyy");
    case ValueDateRole:
      return transaction.valueDate.toString("dd/MM/yyyy");
    case CheckStatusRole:
      return transaction.checkStatus;
    }
  }

  return QVariant();
}

QVariant TransactionModel::headerData(int section, Qt::Orientation orientation,
                                      int role) const {
  if (role != Qt::DisplayRole)
    return QVariant();

  if (orientation == Qt::Horizontal) {
    switch (section) {
    case 0:
      return tr("Date comptabilisation");
    case 1:
      return tr("Libellé simplifié");
    case 2:
      return tr("Libellé opération");
    case 3:
      return tr("Référence");
    case 4:
      return tr("Informations complémentaires");
    case 5:
      return tr("Type opération");
    case 6:
      return tr("Catégorie");
    case 7:
      return tr("Sous catégorie");
    case 8:
      return tr("Débit");
    case 9:
      return tr("Crédit");
    case 10:
      return tr("Date opération");
    case 11:
      return tr("Date de valeur");
    case 12:
      return tr("Pointage");
    default:
      return QVariant();
    }
  }
  return QVariant();
}

QHash<int, QByteArray> TransactionModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[AccountingDateRole] = "accountingDate";
  roles[SimplifiedLabelRole] = "simplifiedLabel";
  roles[OperationLabelRole] = "operationLabel";
  roles[ReferenceRole] = "reference";
  roles[AdditionalInfoRole] = "additionalInfo";
  roles[OperationTypeRole] = "operationType";
  roles[CategoryRole] = "category";
  roles[SubCategoryRole] = "subCategory";
  roles[DebitRole] = "debit";
  roles[CreditRole] = "credit";
  roles[OperationDateRole] = "operationDate";
  roles[ValueDateRole] = "valueDate";
  roles[CheckStatusRole] = "checkStatus";
  return roles;
}

bool TransactionModel::loadFromCsv(const QString &filePath) {
  qDebug() << "Loading CSV from:" << filePath;
  
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "Failed to open file:" << file.errorString();
    return false;
  }

  beginResetModel();
  m_transactions.clear();

  QTextStream in(&file);
  // Skip header line
  if (!in.atEnd()) {
    QString header = in.readLine();
    qDebug() << "Header:" << header;
  }

  int lineCount = 0;
  while (!in.atEnd()) {
    QString line = in.readLine();
    lineCount++;
    QStringList fields = line.split(';');

    if (lineCount == 1) {
      qDebug() << "First line fields count:" << fields.size();
      qDebug() << "First line:" << line;
    }

    if (fields.size() >= 13) {
      Transaction transaction;
      transaction.accountingDate =
          QDate::fromString(fields[0].trimmed(), "dd/MM/yyyy");
      transaction.simplifiedLabel = fields[1].trimmed();
      transaction.operationLabel = fields[2].trimmed();
      transaction.reference = fields[3].trimmed();
      transaction.additionalInfo = fields[4].trimmed();
      transaction.operationType = fields[5].trimmed();
      transaction.category = fields[6].trimmed();
      transaction.subCategory = fields[7].trimmed();

      // Parse debit (negative values)
      QString debitStr = fields[8].trimmed().replace(',', '.');
      if (!debitStr.isEmpty()) {
        transaction.debit = -debitStr.toDouble(); // Remove the minus sign
      } else {
        transaction.debit = 0.0;
      }

      // Parse credit (positive values with +)
      QString creditStr = fields[9].trimmed();
      if (!creditStr.isEmpty()) {
        creditStr = creditStr.replace('+', "").replace(',', '.');
        transaction.credit = creditStr.toDouble();
      } else {
        transaction.credit = 0.0;
      }

      transaction.operationDate =
          QDate::fromString(fields[10].trimmed(), "dd/MM/yyyy");
      transaction.valueDate =
          QDate::fromString(fields[11].trimmed(), "dd/MM/yyyy");
      transaction.checkStatus = fields[12].trimmed().toInt();

      m_transactions.append(transaction);
    }
  }

  file.close();
  qDebug() << "Loaded" << m_transactions.size() << "transactions";
  
  endResetModel();
  emit countChanged();

  return true;
}

void TransactionModel::clear() {
  beginResetModel();
  m_transactions.clear();
  endResetModel();
  emit countChanged();
}
