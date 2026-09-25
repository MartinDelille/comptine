#include <QGuiApplication>
#include <QLocale>
#include <QQmlApplicationEngine>

#include "AppSettings.h"
#include "TranslationManager.h"

using namespace Qt::StringLiterals;

TranslationManager::TranslationManager(QGuiApplication& app, QQmlApplicationEngine& engine,
                                       AppSettings& settings, QObject* parent) :
    QObject(parent),
    _app(app),
    _engine(engine),
    _settings(settings) {
  // Load initial translation
  loadTranslation();

  // Connect to settings for live language switching
  connect(&_settings, &AppSettings::languageChangeRequested, this, &TranslationManager::loadTranslation);
}

void TranslationManager::loadTranslation() {
  _app.removeTranslator(&_translator);

  QString lang = _settings.language();
  if (lang.isEmpty()) {
    // System default
    if (_translator.load(QLocale(), "comptine"_L1, "_"_L1, ":/i18n"_L1)) {
      _app.installTranslator(&_translator);
    }
  } else if (lang == "fr"_L1) {
    if (_translator.load(":/i18n/comptine_fr.qm"_L1)) {
      _app.installTranslator(&_translator);
    }
  }
  // If lang == "en", don't load any translator (English is source)

  // Retranslate QML
  _engine.retranslate();
}
