
#import "linenoise.h"
#import "Interference.h"

static NSArray*(^completions)(NSString*) = NULL;

void compHandler(const char *buf, linenoiseCompletions *lc) {

  if (completions) for (id x in completions([NSString stringWithUTF8String:buf]))

      linenoiseAddCompletion(lc, [x UTF8String]);
}

@implementation Interference

- init { if (!(self = super.init)) return nil;

  NSArray *args = NSProcessInfo.processInfo.arguments;
  if (args.count <= 1) return self;

  if ([args containsObject:@"--multiline"]) { ///  enable multi line editing.

    NSUInteger lines = [args[[args indexOfObject:@"--multiline"] + 1] integerValue];
    [self setMultiLine:lines];
    printf("Multi-line mode enabled (%lu).\n", lines);

  } else if ([args containsObject:@"--keycodes"]) {

      [self printKeyCodes];  return nil;

  } else { fprintf(stderr, "Usage: %s [--multiline] [--keycodes]\n", [args[0] UTF8String]); return nil; }

  return self;

}

- (void) setCompletions:(NSArray*(^)(NSString*))comps { completions = [comps copy];

  linenoiseSetCompletionCallback(compHandler);
}
- (void) setHistoryLoadPath:(NSString *)historyLoadPath {

  linenoiseHistoryLoad((_historyLoadPath = historyLoadPath).UTF8String);
}
- (void) saveHistory {

  linenoiseHistorySave((_historySavePath ?: _historySavePath ?: @"").UTF8String);
}
- (void) setHistoryLength:(NSUInteger *)historyLength {

  linenoiseHistorySetMaxLen(_historyLength = historyLength);
}
- (void) clearScreen { linenoiseClearScreen(); }

- (void) printKeyCodes { linenoisePrintKeyCodes(); }

- (void) setMultiLine:(NSUInteger)mline { linenoiseSetMultiLine(_multiLine = mline); }

- (void) prompt:(NSString*)prompt withBlock:(BOOL(^)(NSString*))cmd {

  NSParameterAssert(cmd);

  /*! Now, THIS is the 'main' loop of the typical linenoise-based application.
      The call to linenoise() will block as long as the user types something and presses enter.
      The typed string is returned as a malloc() allocated string by linenoise, so the user needs to free() it. */

  char  * line;
  while ((line = linenoise(prompt.UTF8String))) {

    if (cmd && cmd([NSString stringWithUTF8String:line])) {

      linenoiseHistoryAdd(line); [self saveHistory];
    }

    free(line);
  }
}

@end
