#include "AppState.h"

AppState::AppState(QObject* parent) :
    QObject(parent),
    _budgetData(_undoStack),
    _categories(_budgetData, _undoStack),
    _rules(_budgetData, _undoStack),
    _file(_settings, _budgetData, _categories, _rules, _undoStack),
    _update(_settings) {
}
