
#import <Foundation/Foundation.h>

@interface Interference : NSObject

/*! @abstract The history file is just a plain text file where entries are separated by newlines.
    @note To load history, just set the \c historyLoadPath
    @note If \c historySavePath is unset, \c saveHistory defaults to the \c historyLoadPath, if set.
 */
@property (nonatomic) NSString * historyLoadPath, *historySavePath;

- (void) saveHistory;

/*! Set the history from an array, or load it from the \c historySaveFile 
 */
@property NSArray * history;

@property (nonatomic) NSUInteger historyLength, multiLine;

/*! This block gets called every time the user hits the <tab> key.
    @param comps  The block provides you with the user's input. @return an array of valid comnpletions or nil
 */
- (void) setCompletionHandler:(NSArray*(^)(NSString*input))comps;

/*! Sets prompt's string, and passes each user-entered line to block.
    @param cmd The block provides the user-entered line for processing.
    @return `YES` (from the block) to save the line to history.
 */
- (void) prompt:(NSString*)promt withBlock:(BOOL(^)(NSString*))cmd;   /// AKA RUN!

- (void) clearScreen;
- (void) printKeyCodes;

@end

