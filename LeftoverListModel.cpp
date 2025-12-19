#include "LeftoverListModel.h"

#include <cmath>

#include "CategoryController.h"

LeftoverListModel::LeftoverListModel(QObject* parent) : QAbstractListModel(parent) {}

int LeftoverListModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid())
    return 0;
  return _items.size();
}

QVariant LeftoverListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid())
    return QVariant();

  const int row = index.row();
  if (row < 0 || row >= _items.size())
    return QVariant();

  const LeftoverItem& item = _items[row];

  switch (static_cast<Roles>(role)) {
    case NameRole:
      return item.name;
    case BudgetLimitRole:
      return item.budgetLimit;
    case SpentRole:
      return item.spent;
    case AccumulatedRole:
      return item.accumulated;
    case LeftoverRole:
      return item.leftover;
    case IsIncomeRole:
      return item.isIncome;
    case SaveAmountRole:
      return item.saveAmount;
    case ReportAmountRole:
      return item.reportAmount;
    case IsBalancedRole: {
      // Balanced when: save + report = leftover (within tolerance)
      return std::abs(item.saveAmount + item.reportAmount - item.leftover) < 0.01;
    }
    default:
      return QVariant();
  }
}

QHash<int, QByteArray> LeftoverListModel::roleNames() const {
  return {
    { NameRole, "name" },
    { BudgetLimitRole, "budgetLimit" },
    { SpentRole, "spent" },
    { AccumulatedRole, "accumulated" },
    { LeftoverRole, "leftover" },
    { IsIncomeRole, "isIncome" },
    { SaveAmountRole, "saveAmount" },
    { ReportAmountRole, "reportAmount" },
    { IsBalancedRole, "isBalanced" }
  };
}

void LeftoverListModel::setCategoryController(CategoryController* controller) {
  if (_controller == controller)
    return;

  // Disconnect from old controller
  if (_controller) {
    disconnect(_controller, nullptr, this, nullptr);
  }

  _controller = controller;

  // Connect to new controller
  if (_controller) {
    connect(_controller, &CategoryController::leftoverDataChanged, this,
            &LeftoverListModel::onLeftoverDataChanged);
  }
}

void LeftoverListModel::setYear(int year) {
  if (_year == year)
    return;
  _year = year;
  emit yearChanged();
}

void LeftoverListModel::setMonth(int month) {
  if (_month == month)
    return;
  _month = month;
  emit monthChanged();
}

void LeftoverListModel::refresh() {
  if (!_controller || _year == 0 || _month == 0)
    return;

  beginResetModel();

  _items.clear();
  QVariantList data = _controller->leftoverSummary(_year, _month);

  for (const QVariant& v : data) {
    QVariantMap map = v.toMap();
    LeftoverItem item;
    item.name = map["name"].toString();
    item.budgetLimit = map["budgetLimit"].toDouble();
    item.spent = map["spent"].toDouble();
    item.accumulated = map["accumulated"].toDouble();
    item.leftover = map["leftover"].toDouble();
    item.isIncome = map["isIncome"].toBool();
    item.saveAmount = map["saveAmount"].toDouble();
    item.reportAmount = map["reportAmount"].toDouble();
    _items.append(item);
  }

  endResetModel();

  refreshTotals();
}

double LeftoverListModel::getSaveAmount(int row) const {
  if (row < 0 || row >= _items.size())
    return 0.0;
  return _items[row].saveAmount;
}

double LeftoverListModel::getReportAmount(int row) const {
  if (row < 0 || row >= _items.size())
    return 0.0;
  return _items[row].reportAmount;
}

bool LeftoverListModel::getIsBalanced(int row) const {
  if (row < 0 || row >= _items.size())
    return false;
  const LeftoverItem& item = _items[row];
  // Balanced when: save + report = leftover (within tolerance)
  return std::abs(item.saveAmount + item.reportAmount - item.leftover) < 0.01;
}

void LeftoverListModel::refreshTotals() {
  if (!_controller)
    return;

  QVariantMap totals = _controller->leftoverTotals(_year, _month);
  _totalToSave = totals["toSave"].toDouble();
  _totalToReport = totals["toReport"].toDouble();
  _totalFromReport = totals["fromReport"].toDouble();
  _netReport = totals["netReport"].toDouble();
  emit totalsChanged();
}

void LeftoverListModel::onLeftoverDataChanged() {
  if (!_controller || _year == 0 || _month == 0)
    return;

  // Get updated data
  QVariantList data = _controller->leftoverSummary(_year, _month);

  // Build a map of name -> updated data for fast lookup
  QHash<QString, QVariantMap> dataByName;
  for (const QVariant& v : data) {
    QVariantMap map = v.toMap();
    dataByName[map["name"].toString()] = map;
  }

  // Update only decision-related fields for existing items
  for (int i = 0; i < _items.size(); ++i) {
    LeftoverItem& item = _items[i];
    auto it = dataByName.find(item.name);
    if (it == dataByName.end())
      continue;

    const QVariantMap& map = it.value();
    double newSaveAmount = map["saveAmount"].toDouble();
    double newReportAmount = map["reportAmount"].toDouble();

    if (item.saveAmount != newSaveAmount || item.reportAmount != newReportAmount) {
      item.saveAmount = newSaveAmount;
      item.reportAmount = newReportAmount;

      QModelIndex idx = createIndex(i, 0);
      // Specify which roles changed so the delegate updates properly
      emit dataChanged(idx, idx, { SaveAmountRole, ReportAmountRole, IsBalancedRole });
    }
  }

  refreshTotals();
}
