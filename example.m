
#import "Interference.h"

int main(int argc, char **argv) {

  Interference * lineNoise = Interference.new;

  /// Set the completion callback. This will be called every time the user uses the <tab> key.

  [lineNoise setCompletions:^NSArray*(NSString*c){

    return [c isEqualToString:@"h"] ? @[@"hello", @"hello there"] : nil;
  }];

  /// The history file is just a plain text file where entries are separated by newlines.

  [lineNoise setHistoryLoadPath:@"/usr/share/dict/propernames"]; /// Loads a history file

  [lineNoise setHistorySavePath:@"history.txt "]; /// Optionally, set an alt. path to save history.

  [lineNoise prompt:@"linenoise > " withBlock:^BOOL(NSString *l) { /// deal with stdin, return YES to save line to history

    if (!l.length) return NO; unichar l0 = [l characterAtIndex:0];

    if (l0 != '\0' && l0 != '/') { /// Do something with the string.

      printf("echo: '%s'\n", l.UTF8String); return YES; // default, catchall "echo"

    } else if ([l hasPrefix:@"/historylen"])  /// The "/historylen" command will change the history len.

       [lineNoise setHistoryLength:[[l substringFromIndex:11] integerValue]];

    else if (l0 == '/') printf("Unreconized command: %s\n", l.UTF8String);

    return NO;
  }];

  return EXIT_SUCCESS;
}
