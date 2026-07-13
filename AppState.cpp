#include "AppState.h"

AppState::AppState(QObject* parent) :
    QObject(parent),
    _data(_undoStack),
    _navigation(_data),
    _categories(_data, _navigation, _undoStack),
    _rules(_data, _undoStack),
    _file(_settings, _data, _categories, _navigation, _rules, _undoStack),
    _update(_settings) {
}
