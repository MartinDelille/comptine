#include "FileCoordinator.h"

#include <QFile>

namespace FileCoordinator {

bool readFile(const QString& filePath, QByteArray& content, QString& errorMessage) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    errorMessage = file.errorString();
    return false;
  }
  content = file.readAll();
  file.close();
  return true;
}

}  // namespace FileCoordinator
