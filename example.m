

#import "Interference.h"

int main(int argc, char **argv) {

  Interference * lineNoise = Interference.new;

  if (argc > 1) { /// Parse options

    NSArray *args = NSProcessInfo.processInfo.arguments;

    if ([args containsObject:@"--multiline"]) { ///  enable multi line editing.

      int lines = [args[[args indexOfObject:@"--multiline"] + 1] integerValue];
      [lineNoise setMultiLine:lines];
      printf("Multi-line mode enabled (%i).\n", lines);

    } else if ([args containsObject:@"--keycodes"])

        [lineNoise  printKeyCodes], exit(0);

    else { fprintf(stderr, "Usage: %s [--multiline] [--keycodes]\n", [args[0] UTF8String]); exit(1); }
  }

  /// Set the completion callback. This will be called every time the user uses the <tab> key.

  [lineNoise setCompletions:^NSArray*(NSString*c){

    return [c isEqualToString:@"h"] ? @[@"hello", @"hello there"] : nil;
  }];

  /// The history file is just a plain text file where entries are separated by newlines.

  [lineNoise setHistoryLoadPath:@"/usr/share/dict/propernames"]; /// Loads this history file

  [lineNoise setHistorySavePath:@"history.txt "]; /// Optionally, set an alt. path to save history.

  /// akin to example.c's main() function.

  [lineNoise prompt:@"linenoise > " withBlock:^BOOL(NSString *l) {

    if (!l.length) return NO;

    unichar l0 = [l characterAtIndex:0];

    if (l0 != '\0' && l0 != '/') {      /// Do something with the string.

          printf("echo: '%s'\n", l.UTF8String);
          return YES;

    } else if ([l hasPrefix:@"/historylen"])  /// The "/historylen" command will change the history len.

       [lineNoise setHistoryLength:/* int len */ [[l substringFromIndex:11] integerValue]];

    else if (l0 == '/') printf("Unreconized command: %s\n", l.UTF8String);

    return NO;
  }];

  return EXIT_SUCCESS;
}
