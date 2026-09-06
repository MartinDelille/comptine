#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include "services/FileCoordinator.h"

class FileCoordinatorTest : public QObject {
  Q_OBJECT

private slots:
  void readsExistingFile() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString filePath = tempDir.filePath("fictional-data.txt");
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    const QByteArray expected("fictional file contents\n");
    QCOMPARE(file.write(expected), expected.size());
    file.close();

    QByteArray content;
    QString errorMessage;
    QVERIFY(FileCoordinator::readFile(filePath, content, errorMessage));
    QCOMPARE(content, expected);
    QVERIFY(errorMessage.isEmpty());
  }

  void reportsMissingFile() {
    QByteArray content("unchanged");
    QString errorMessage;

    QVERIFY(!FileCoordinator::readFile("/fictional/path/does-not-exist", content, errorMessage));
    QVERIFY(!errorMessage.isEmpty());
  }
};

QTEST_GUILESS_MAIN(FileCoordinatorTest)
#include "FileCoordinatorTest.moc"
