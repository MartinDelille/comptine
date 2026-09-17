#include <QClipboard>
#include <QGuiApplication>
#include <QTest>

#include "model/Account.h"
#include "model/Operation.h"
#include "services/ClipboardController.h"

class ClipboardControllerTest : public QObject {
  Q_OBJECT

private slots:
  void copiesSelectedOperationsAsCsv() {
    Account account("Fictional Account", nullptr);
    account.addOperation(new Operation(&account, QDate(2026, 1, 2), -12.50,
                                       "Fictional Purchase"));
    ClipboardController2 controller(account);
    auto* clipboard = QGuiApplication::clipboard();

    clipboard->setText("unchanged");
    controller.copySelectedOperations();
    QCOMPARE(clipboard->text(), QString("unchanged"));

    account.selectAt(0);
    controller.copySelectedOperations();
    QCOMPARE(clipboard->text(), account.selectedOperationsAsCsv());
    QVERIFY(!clipboard->text().isEmpty());
  }
};

QTEST_MAIN(ClipboardControllerTest)
#include "ClipboardControllerTest.moc"
