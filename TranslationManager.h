#pragma once

#include <QObject>
#include <QTranslator>

class QGuiApplication;
class QQmlApplicationEngine;
class AppSettings;

class TranslationManager : public QObject {
  Q_OBJECT

public:
  TranslationManager(QGuiApplication *app, QQmlApplicationEngine *engine,
                     AppSettings *settings, QObject *parent = nullptr);

public slots:
  void loadTranslation();

private:
  QGuiApplication *_app;
  QQmlApplicationEngine *_engine;
  AppSettings *_settings;
  QTranslator _translator;
};
