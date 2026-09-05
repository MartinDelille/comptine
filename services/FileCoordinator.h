#pragma once

#include <QByteArray>
#include <QString>

namespace FileCoordinator {

// Reads a file, triggering cloud download if needed (MacOS only).
// On other platforms, this is equivalent to QFile::readAll().
// Returns true on success, false on error (with errorMessage set).
bool readFile(const QString& filePath, QByteArray& content, QString& errorMessage);

}  // namespace FileCoordinator
