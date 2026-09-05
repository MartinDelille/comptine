#include "FileCoordinator.h"

#import <Foundation/Foundation.h>

#include <QFile>

namespace FileCoordinator {

bool readFile(const QString &filePath, QByteArray &content, QString &errorMessage) {
  @autoreleasepool {
    NSString *nsPath = filePath.toNSString();
    NSURL *url = [NSURL fileURLWithPath:nsPath];
    NSFileCoordinator *coordinator = [[NSFileCoordinator alloc] initWithFilePresenter:nil];

    __block NSData *data = nil;
    __block NSError *readError = nil;
    NSError *coordinationError = nil;

    [coordinator coordinateReadingItemAtURL:url
                                    options:0
                                      error:&coordinationError
                                 byAccessor:^(NSURL *newURL) {
                                   data = [NSData dataWithContentsOfURL:newURL options:0 error:&readError];
                                 }];

    if (data) {
      content = QByteArray::fromNSData(data);
      [coordinator release];
      return true;
    }

    [coordinator release];

    // Fall back to Qt's native reader when coordinated access does not return
    // data.
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
      content = file.readAll();
      return true;
    }

    if (readError) {
      errorMessage = QString::fromNSString(readError.localizedDescription);
    } else if (coordinationError) {
      errorMessage = QString::fromNSString(coordinationError.localizedDescription);
    } else {
      errorMessage = file.errorString();
    }
    return false;
  }
}

}  // namespace FileCoordinator
