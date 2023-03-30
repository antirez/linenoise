
#import <stddef.h>
#import "linenoise.h"
#import "Interference.h"

#define NSARGS NSProcessInfo.processInfo.arguments

static NSArray*(^completions)(NSString*) = NULL;

void compHandler(const char *buf, linenoiseCompletions *lc) { if (!completions) return;

  for (id x in completions([NSString stringWithUTF8String:buf])) linenoiseAddCompletion(lc, [x UTF8String]);
}

@implementation Interference

- init { if (!(self = super.init)) return nil;

  if (NSARGS.count <= 1) return self;

  if ([NSARGS containsObject:@"--multiline"]) { ///  enable multi line editing.

    NSUInteger lines = [NSARGS[[NSARGS indexOfObject:@"--multiline"] + 1] integerValue];
    [self setMultiLine:lines];
    printf("Multi-line mode enabled (%lu).\n", lines);

  } else if ([NSARGS containsObject:@"--keycodes"]) return [self printKeyCodes], nil;

  else return fprintf(stderr, "Usage: %s [--multiline] [--keycodes]\n", [NSARGS[0] UTF8String]), nil;

  return self;
}

- (void) setCompletionHandler:(NSArray*(^)(NSString*))comps { completions = [comps copy];

  linenoiseSetCompletionCallback(compHandler);
}
- (void) setHistoryLoadPath:(NSString *)historyLoadPath {

  linenoiseHistoryLoad((_historyLoadPath = historyLoadPath).UTF8String);
}
- (void) saveHistory {

  linenoiseHistorySave((_historySavePath = _historySavePath ?: _historyLoadPath).UTF8String);
}
- (void) setHistoryLength:(NSUInteger)historyLength {

  linenoiseHistorySetMaxLen(_historyLength = historyLength);
}
- (NSString*) historySavePath { return _historySavePath =

  [NSFileManager.defaultManager isWritableFileAtPath:_historySavePath = _historySavePath ?: _historyLoadPath]
  ? _historySavePath : [NSTemporaryDirectory() stringByAppendingPathComponent:NSUUID.UUID.UUIDString];
}
- (NSArray*) history {

  id x = [NSFileManager.defaultManager fileExistsAtPath:self.historySavePath]
       ? [NSString stringWithContentsOfFile:_historySavePath encoding:NSUTF8StringEncoding error:nil] : nil;

  return x && [x length] ? [x componentsSeparatedByString:@"\n"] : @[];
}
- (void) setHistory:(NSArray *)history { NSAssert(self.historySavePath, @"Needs a save path!");

  [[history componentsJoinedByString:@"\n"] writeToFile:_historySavePath atomically:YES
                                               encoding:NSUTF8StringEncoding error:nil];
  [self setHistoryLoadPath:_historySavePath];
}
- (void) clearScreen { linenoiseClearScreen(); }

- (void) printKeyCodes { linenoisePrintKeyCodes(); }

- (void) setMultiLine:(NSUInteger)mline { linenoiseSetMultiLine(_multiLine = mline); }

- (void) prompt:(NSString*)prompt withBlock:(BOOL(^)(NSString*))cmd {

  NSParameterAssert(cmd); char * line;

  /*! Now, THIS is the 'main' loop of the typical linenoise-based application.
      The call to linenoise() will block as long as the user types something and presses enter.
      The typed string is returned as a malloc() allocated string by linenoise, so the user needs to free() it. */

  while ((line = linenoise(prompt.UTF8String))) { NSString *str;

    if (cmd && (str = [NSString stringWithUTF8String:line]).length && cmd(str)) {

      linenoiseHistoryAdd(line); [self saveHistory];
    }
    
    free(line);
  }
}

@end
