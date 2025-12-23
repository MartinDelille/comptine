#include "RuleListModel.h"

#include "CategorizationRule.h"
#include "RuleController.h"

RuleListModel::RuleListModel(QObject* parent) :
    QAbstractListModel(parent) {
}

void RuleListModel::setRuleController(RuleController* controller) {
  _controller = controller;
}

int RuleListModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid() || !_controller) {
    return 0;
  }
  return _controller->rules().size();
}

QVariant RuleListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || !_controller) {
    return QVariant();
  }

  int row = index.row();
  if (row < 0 || row >= _controller->rules().size()) {
    return QVariant();
  }

  const CategorizationRule* rule = _controller->rules().at(row);
  if (!rule) {
    return QVariant();
  }

  switch (static_cast<Roles>(role)) {
    case CategoryRole:
      if (rule->category()) {
        return rule->category()->name();
      }
      break;
    case LabelPrefixRole:
      return rule->labelPrefix();
  }
  return QVariant();
}

QHash<int, QByteArray> RuleListModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[CategoryRole] = "category";
  roles[LabelPrefixRole] = "labelPrefix";
  return roles;
}

void RuleListModel::refresh() {
  beginResetModel();
  endResetModel();
}
