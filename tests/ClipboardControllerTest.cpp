#include <QClipboard>
#include <QGuiApplication>
#include <QTest>

#include "model/Account.h"
#include "model/Operation.h"
#include "services/ClipboardController.h"

using namespace Qt::StringLiterals;

class ClipboardControllerTest : public QObject {
  Q_OBJECT

private slots:
  void copiesSelectedOperationsAsCsv() {
    Account account(u"Fictional Account"_s, nullptr);
    account.addOperation(new Operation(&account, QDate(2026, 1, 2), -12.50,
                                       u"Fictional Purchase"_s));
    ClipboardController2 controller(account);
    auto* clipboard = QGuiApplication::clipboard();

    clipboard->setText(u"unchanged"_s);
    controller.copySelectedOperations();
    QCOMPARE(clipboard->text(), u"unchanged"_s);

    account.selectAt(0);
    controller.copySelectedOperations();
    QCOMPARE(clipboard->text(), account.selectedOperationsAsCsv());
    QVERIFY(!clipboard->text().isEmpty());
  }
};

QTEST_MAIN(ClipboardControllerTest)
#include "ClipboardControllerTest.moc"
