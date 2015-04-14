
#import "Interference.h"

#define ALT_HISTORY [[[NSString stringWithContentsOfFile:@"/usr/share/dict/words" \
  encoding:NSUTF8StringEncoding error:nil] substringToIndex:3000] componentsSeparatedByString:@"\n"]

int main(int argc, char **argv) { @autoreleasepool {

    Interference * lineNoise = Interference.new;

    lineNoise.completionHandler = ^NSArray*(NSString*line){

      return [line isEqualToString:@"h"] ? @[@"hello", @"hello there"] :
             [line isEqualToString:@"/"] ? @[@"/althistory", @"/historylen"] : nil;

    };

    lineNoise.historyLoadPath = @"/usr/share/dict/propernames"; /// Loads a history file

    lineNoise.historySavePath = @"history.txt"; /// Optionally, set an alt. path to save history.

    [lineNoise prompt:@"linenoise > " withBlock:^BOOL(NSString *l) { /// returning YES saves the line to history

      unichar l0 = [l characterAtIndex:0];

      if ([l isEqualToString:@"?"]) printf("%s\n", lineNoise.history.description.UTF8String);

      else if (l0 != '\0' && l0 != '/') { /// Do something with the string.

        printf("echo: '%s'\n", l.UTF8String); return YES; // default, catchall "echo"

      } else if ([l hasPrefix:@"/historylen"])  /// The "/historylen" command will change the history len.

        lineNoise.historyLength = [[l substringFromIndex:11] integerValue];

      else if ([l hasPrefix:@"/althistory"]) // for this example, reset the history to some new array.

        lineNoise.history = ALT_HISTORY;

      else if (l0 == '/') printf("Unreconized command: %s\n", l.UTF8String);

      return NO;
    }];

  } return EXIT_SUCCESS;
}
