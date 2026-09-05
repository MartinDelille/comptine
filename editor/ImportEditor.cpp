#include "ImportEditor.h"

#include "FileController.h"

ImportEditor::ImportEditor(FileController& fileController, QObject* parent) :
    QObject(parent), _fileController(fileController) {
}

bool ImportEditor::importCsv(const QUrl& fileUrl, const QString& accountName,
                             bool useCategories) {
  return _fileController.importFromCsv(fileUrl, accountName, useCategories);
}
