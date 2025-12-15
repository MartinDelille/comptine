#pragma once

#include <QAbstractListModel>
#include <QObject>

class CategorizationRule;
class RuleController;

class RuleListModel : public QAbstractListModel {
  Q_OBJECT

public:
  enum Roles {
    CategoryRole = Qt::UserRole + 1,
    DescriptionPrefixRole,
  };

  explicit RuleListModel(QObject* parent = nullptr);

  void setRuleController(RuleController* controller);

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  Q_INVOKABLE void refresh();

private:
  RuleController* _controller = nullptr;
};
