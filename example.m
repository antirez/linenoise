

#import "Interference.h"

int main(int argc, char **argv) {

  Interference * lineNoise = Interference.new;

  while (argc > 1) { /// Parse options, with --multiline we enable multi line editing.

    argc--; argv++;

      !strcmp(*argv, "--multiline") ? [lineNoise setMultiLine:1], printf("Multi-line mode enabled.\n")
    : !strcmp(*argv,  "--keycodes") ? [lineNoise  printKeyCodes], exit(0)
    : fprintf(stderr, "Usage: %s [--multiline] [--keycodes]\n",
             NSProcessInfo.processInfo.processName.UTF8String),   exit(1);
  }

  /// Set the completion callback. This will be called every time the user uses the <tab> key.

  [lineNoise setCompletions:^NSArray*(NSString*c){

    return [c isEqualToString:@"h"] ? @[@"hello", @"hello there"] : nil;
  }];

  /// Load history from file. The history file is just a plain text file where entries are separated by newlines.

  [lineNoise setHistoryLoadPath:@"/usr/share/dict/propernames"]; /// Load the history at startup

  [lineNoise setHistorySavePath:@"history_objc.txt"]; /// Load the history at startup

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
