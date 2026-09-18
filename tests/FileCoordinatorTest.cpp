#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include "services/FileCoordinator.h"

using namespace Qt::StringLiterals;

class FileCoordinatorTest : public QObject {
  Q_OBJECT

private slots:
  void readsExistingFile() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString filePath = tempDir.filePath(u"fictional-data.txt"_s);
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    const QByteArray expected("fictional file contents\n"_L1);
    QCOMPARE(file.write(expected), expected.size());
    file.close();

    QByteArray content;
    QString errorMessage;
    QVERIFY(FileCoordinator::readFile(filePath, content, errorMessage));
    QCOMPARE(content, expected);
    QVERIFY(errorMessage.isEmpty());
  }

  void reportsMissingFile() {
    QByteArray content("unchanged"_L1);
    QString errorMessage;

    QVERIFY(!FileCoordinator::readFile("/fictional/path/does-not-exist"_L1, content, errorMessage));
    QVERIFY(!errorMessage.isEmpty());
  }
};

QTEST_GUILESS_MAIN(FileCoordinatorTest)
#include "FileCoordinatorTest.moc"
