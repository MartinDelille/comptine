#include "FileCoordinator.h"

#import <Foundation/Foundation.h>

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

    if (coordinationError) {
      errorMessage = QString::fromNSString(coordinationError.localizedDescription);
      return false;
    }

    if (readError) {
      errorMessage = QString::fromNSString(readError.localizedDescription);
      return false;
    }

    if (data) {
      content = QByteArray::fromNSData(data);
    } else {
      content = QByteArray();
    }

    return true;
  }
}

}  // namespace FileCoordinator
