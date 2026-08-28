#include <qqmlintegration.h>
#include <QUndoStack>

#include "AppSettings.h"
#include "AppState.h"
#include "BudgetData.h"
#include "CategoryController.h"
#include "FileController.h"
#include "RuleController.h"
#include "UpdateController.h"
#include "editors/AccountEditor.h"
#include "editors/CategoryEditor.h"
#include "editors/ImportEditor.h"
#include "editors/OperationEditor.h"
#include "editors/RuleEditor.h"

template <typename T>

struct ForeignBase {
  inline static T* instance = nullptr;

  inline static QJSEngine* engine = nullptr;

  static T* create(QQmlEngine*, QJSEngine* engine)

  {
    Q_ASSERT(instance);
    Q_ASSERT(engine->thread() == instance->thread());

    if (ForeignBase::engine) {
      Q_ASSERT(engine == ForeignBase::engine);
    } else {
      ForeignBase::engine = engine;
    }

    QJSEngine::setObjectOwnership(instance, QJSEngine::CppOwnership);
    return instance;
  }
};

struct AppSettingsForeign : ForeignBase<AppSettings> {
  Q_GADGET
  QML_FOREIGN(AppSettings)
  QML_SINGLETON
  QML_NAMED_ELEMENT(AppSettings)
};

struct AppStateForeign : ForeignBase<AppState> {
  Q_GADGET
  QML_FOREIGN(AppState)
  QML_SINGLETON
  QML_NAMED_ELEMENT(AppState)
};

struct BudgetDataForeign : ForeignBase<BudgetData> {
  Q_GADGET
  QML_FOREIGN(BudgetData)
  QML_SINGLETON
  QML_NAMED_ELEMENT(BudgetData)
};

struct CategoryControllerForeign : ForeignBase<CategoryController> {
  Q_GADGET
  QML_FOREIGN(CategoryController)
  QML_SINGLETON
  QML_NAMED_ELEMENT(CategoryController)
};

struct FileControllerForeign : ForeignBase<FileController> {
  Q_GADGET
  QML_FOREIGN(FileController)
  QML_SINGLETON
  QML_NAMED_ELEMENT(FileController)
};

struct RuleControllerForeign : ForeignBase<RuleController> {
  Q_GADGET
  QML_FOREIGN(RuleController)
  QML_SINGLETON
  QML_NAMED_ELEMENT(RuleController)
};

struct UndoStackForeign : ForeignBase<QUndoStack> {
  Q_GADGET
  QML_FOREIGN(QUndoStack)
  QML_SINGLETON
  QML_NAMED_ELEMENT(UndoStack)
};

struct UpdateControllerForeign : ForeignBase<UpdateController> {
  Q_GADGET
  QML_FOREIGN(UpdateController)
  QML_SINGLETON
  QML_NAMED_ELEMENT(UpdateController)
};

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
