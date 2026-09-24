#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

#include <signal.h>
#include <unistd.h>
#include <cstring>

static bool runTask(NSString* executable, NSArray<NSString*>* arguments) {
  NSTask* task = [[NSTask alloc] init];
  task.executableURL = [NSURL fileURLWithPath:executable];
  task.arguments = arguments;
  NSError* error = nil;
  @try {
    if (![task launchAndReturnError:&error]) {
      NSLog(@"Failed to launch %@: %@", executable, error.localizedDescription);
      return false;
    }
    [task waitUntilExit];
  } @catch (NSException*) {
    NSLog(@"Exception while launching %@", executable);
    return false;
  }
  if (task.terminationStatus != 0) {
    NSLog(@"%@ exited with status %d", executable, task.terminationStatus);
    return false;
  }
  return task.terminationStatus == 0;
}

static NSString* shellQuote(NSString* value) {
  return [NSString stringWithFormat:@"'%@'", [value stringByReplacingOccurrencesOfString:@"'"
                                                                              withString:@"'\\''"]];
}

static bool launchWorker(NSString* workerPath, NSString* executable, NSString* image, pid_t parentPid) {
  NSTask* task = [[NSTask alloc] init];
  task.executableURL = [NSURL fileURLWithPath:workerPath];
  task.arguments = @[ @"--worker", executable, image, [NSString stringWithFormat:@"%d", parentPid] ];
  NSError* error = nil;
  if (![task launchAndReturnError:&error]) {
    NSLog(@"Failed to launch updater worker %@: %@", workerPath, error.localizedDescription);
    return false;
  }
  NSLog(@"Updater worker launched: %@", workerPath);
  return true;
}

int main(int argc, const char* argv[]) {
  @autoreleasepool {
    bool workerMode = argc >= 2 && std::strcmp(argv[1], "--worker") == 0;
    if ((!workerMode && argc < 4) || (workerMode && argc < 5))
      return 2;

    int argumentOffset = workerMode ? 1 : 0;
    NSString* executable = [NSString stringWithUTF8String:argv[1 + argumentOffset]];
    NSString* image = [NSString stringWithUTF8String:argv[2 + argumentOffset]];
    pid_t parentPid = static_cast<pid_t>(strtol(argv[3 + argumentOffset], nullptr, 10));

    if (!workerMode) {
      NSString* source = [NSString stringWithUTF8String:argv[0]];
      NSString* workerPath = [NSTemporaryDirectory() stringByAppendingPathComponent:
                                                         [NSString stringWithFormat:@"ComptineUpdater-%d", getpid()]];
      [[NSFileManager defaultManager] removeItemAtPath:workerPath error:nil];
      if (![[NSFileManager defaultManager] copyItemAtPath:source toPath:workerPath error:nil]) {
        NSLog(@"Could not copy updater to temporary worker path %@", workerPath);
        return 3;
      }
      bool launched = launchWorker(workerPath, executable, image, parentPid);
      if (!launched)
        [[NSFileManager defaultManager] removeItemAtPath:workerPath error:nil];
      return launched ? 0 : 4;
    }

    NSString* appBundle = [[[executable stringByDeletingLastPathComponent]
        stringByDeletingLastPathComponent] stringByDeletingLastPathComponent];
    NSString* workerPath = workerMode ? [NSString stringWithUTF8String:argv[0]] : nil;

    if (![[NSFileManager defaultManager] fileExistsAtPath:image] || ![[NSFileManager defaultManager] fileExistsAtPath:executable] || parentPid <= 0) {
      NSLog(@"Invalid updater arguments: executable=%@ image=%@ parentPid=%d", executable, image, parentPid);
      return 3;
    }

    bool parentExited = false;
    for (int attempt = 0; attempt < 120; ++attempt) {
      if (kill(parentPid, 0) != 0) {
        parentExited = true;
        break;
      }
      sleep(1);
    }
    if (!parentExited) {
      NSLog(@"Timed out waiting for Comptine process %d to exit", parentPid);
      return 4;
    }

    NSString* mountPoint = [NSTemporaryDirectory() stringByAppendingPathComponent:
                                                       [NSString stringWithFormat:@"ComptineUpdate-%d", getpid()]];
    [[NSFileManager defaultManager] createDirectoryAtPath:mountPoint
                              withIntermediateDirectories:YES
                                               attributes:nil
                                                    error:nil];

    NSLog(@"Mounting update image %@ at %@", image, mountPoint);
    bool mounted = runTask(@"/usr/bin/hdiutil",
                           @[ @"attach", image, @"-nobrowse", @"-readonly",
                              @"-mountpoint", mountPoint ]);
    NSString* replacement = [mountPoint stringByAppendingPathComponent:@"Comptine.app"];
    if (!mounted || ![[NSFileManager defaultManager] fileExistsAtPath:replacement]) {
      NSLog(@"Update image did not contain Comptine.app: %@", replacement);
      if (mounted)
        runTask(@"/usr/bin/hdiutil", @[ @"detach", mountPoint, @"-quiet" ]);
      [[NSFileManager defaultManager] removeItemAtPath:mountPoint error:nil];
      return 5;
    }

    NSLog(@"Validating replacement application bundle %@", replacement);
    if (!runTask(@"/usr/bin/codesign", @[ @"--verify", @"--deep", @"--strict", replacement ])) {
      NSLog(@"Replacement application bundle has an invalid code signature");
      runTask(@"/usr/bin/hdiutil", @[ @"detach", mountPoint, @"-quiet" ]);
      [[NSFileManager defaultManager] removeItemAtPath:mountPoint error:nil];
      return 6;
    }

    NSLog(@"Replacing application bundle %@ with %@", appBundle, replacement);
    bool copied = mounted && runTask(@"/usr/bin/ditto", @[ replacement, appBundle ]);

    if (!copied) {
      NSLog(@"Direct replacement failed; requesting administrator authorization");
      NSString* command = [NSString stringWithFormat:@"/usr/bin/ditto %@ %@",
                                                     shellQuote(replacement), shellQuote(appBundle)];
      copied = runTask(@"/usr/bin/osascript",
                       @[ @"-e", [NSString stringWithFormat:
                                               @"do shell script %@ with administrator privileges",
                                               shellQuote(command)] ]);
    }

    if (mounted)
      runTask(@"/usr/bin/hdiutil", @[ @"detach", mountPoint, @"-quiet" ]);
    [[NSFileManager defaultManager] removeItemAtPath:mountPoint error:nil];

    if (!copied) {
      NSLog(@"Failed to replace application bundle");
      return 1;
    }

    [[NSFileManager defaultManager] removeItemAtPath:image error:nil];
    if (workerPath)
      [[NSFileManager defaultManager] removeItemAtPath:workerPath error:nil];
    NSLog(@"Update installed successfully; relaunching %@", appBundle);
    [[NSWorkspace sharedWorkspace] launchApplication:appBundle];
    return 0;
  }
}
