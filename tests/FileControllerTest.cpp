#include <QObject>
#include <QSignalSpy>
#include <QUndoStack>
#include <QtTest/QtTest>
#include "FileController.h"

// Stub definitions for dependencies
class AppSettings {};
class BudgetData {};
class CategoryController {};
class NavigationController {};
class RuleController {};

class FileControllerTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase() {
    // Initialize dependencies
    mockAppSettings = new AppSettings;
    mockBudgetData = new BudgetData;
    mockCategoryController = new CategoryController;
    mockNavigationController = new NavigationController;
    mockRuleController = new RuleController;
    mockUndoStack = new QUndoStack;

    // Create FileController
    fileController = new FileController(*mockAppSettings, *mockBudgetData, *mockCategoryController, *mockNavigationController, *mockRuleController, *mockUndoStack);
  }

  void cleanupTestCase() {
    // Clean up dependencies
    delete fileController;
    delete mockAppSettings;
    delete mockBudgetData;
    delete mockCategoryController;
    delete mockNavigationController;
    delete mockRuleController;
    delete mockUndoStack;
  }

  void testClear() {
    fileController->clear();

    QSignalSpy spy(fileController, &FileController::dataLoaded);
    QCOMPARE(spy.count(), 0);
  }

private:
  AppSettings* mockAppSettings;
  BudgetData* mockBudgetData;
  CategoryController* mockCategoryController;
  NavigationController* mockNavigationController;
  RuleController* mockRuleController;
  QUndoStack* mockUndoStack;
  FileController* fileController;
};

QTEST_MAIN(FileControllerTest)
#include "FileControllerTest.moc"
