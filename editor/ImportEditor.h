#pragma once

#include <QObject>
#include <QUrl>

class FileController;

class ImportEditor : public QObject {
  Q_OBJECT

public:
  explicit ImportEditor(FileController& fileController, QObject* parent = nullptr);

  Q_INVOKABLE bool importCsv(const QUrl& fileUrl, const QString& accountName = {},
                             bool useCategories = false);

private:
  FileController& _fileController;
};
