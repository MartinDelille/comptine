#pragma once

#include <qqmlintegration.h>
#include <QJSEngine>
#include <QQmlEngine>

#include "AccountEditor.h"
#include "CategoryEditor.h"
#include "ImportEditor.h"
#include "OperationEditor.h"
#include "RuleEditor.h"
#include "utils/ForeignBase.h"

struct AccountEditorForeign : ForeignBase<AccountEditor> {
  Q_GADGET
  QML_FOREIGN(AccountEditor)
  QML_SINGLETON
  QML_NAMED_ELEMENT(AccountEditor)
};

struct CategoryEditorForeign : ForeignBase<CategoryEditor> {
  Q_GADGET
  QML_FOREIGN(CategoryEditor)
  QML_SINGLETON
  QML_NAMED_ELEMENT(CategoryEditor)
};

struct OperationEditorForeign : ForeignBase<OperationEditor> {
  Q_GADGET
  QML_FOREIGN(OperationEditor)
  QML_SINGLETON
  QML_NAMED_ELEMENT(OperationEditor)
};

struct RuleEditorForeign : ForeignBase<RuleEditor> {
  Q_GADGET
  QML_FOREIGN(RuleEditor)
  QML_SINGLETON
  QML_NAMED_ELEMENT(RuleEditor)
};

struct ImportEditorForeign : ForeignBase<ImportEditor> {
  Q_GADGET
  QML_FOREIGN(ImportEditor)
  QML_SINGLETON
  QML_NAMED_ELEMENT(ImportEditor)
};
