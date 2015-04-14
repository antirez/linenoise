
#import <Foundation/Foundation.h>

@interface Interference : NSObject

/*! @note To load history, just set the @c historyLoadPath;
    @note if historySavePath is unset, @c saveHistory will save to historyLoadPath, if set.
 */
@property (nonatomic) NSString * historyLoadPath, *historySavePath;

@property (nonatomic) NSUInteger * historyLength, multiLine;

- (void) setCompletions:(NSArray*(^)(NSString*))completions;

- (void) saveHistory;
- (void) clearScreen;
- (void) printKeyCodes;

/*! Sets prompt's string, and passes each user-entered line to block.
    @note Return YES from block to save the line to history.
 */
- (void) prompt:(NSString*)promt withBlock:(BOOL(^)(NSString*))cmd;   /// AKA RUN!

@end

