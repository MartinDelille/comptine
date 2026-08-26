#include <QClipboard>
#include <QGuiApplication>

#include "ClipboardController.h"
#include "model/Account.h"

ClipboardController2::ClipboardController2(Account& account) :
    _account(account) {
}

void ClipboardController2::copySelectedOperations() const {
  QString csv = _account.selectedOperationsAsCsv();
  if (!csv.isEmpty()) {
    QGuiApplication::clipboard()->setText(csv);
  }
}
