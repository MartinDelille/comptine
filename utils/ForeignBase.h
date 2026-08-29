#pragma once

#include <qqmlintegration.h>
#include <QJSEngine>
#include <QQmlEngine>

template <typename T>
struct ForeignBase {
  inline static T* instance = nullptr;
  inline static QJSEngine* engine = nullptr;

  static T* create(QQmlEngine*, QJSEngine* engine) {
    Q_ASSERT(instance);
    Q_ASSERT(engine->thread() == instance->thread());
    if (ForeignBase::engine)
      Q_ASSERT(engine == ForeignBase::engine);
    else
      ForeignBase::engine = engine;
    QJSEngine::setObjectOwnership(instance, QJSEngine::CppOwnership);
    return instance;
  }
};
