// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
#import <Cocoa/Cocoa.h>
NSMutableDictionary *NXProjectNode(NSString *kind, NSString *name, NSString *path);
NSMutableDictionary *NXReadWorkspace(NSString *path, NSError **error);
BOOL NXWriteWorkspace(NSDictionary *root, NSString *path, NSError **error);
BOOL NXValidateProject(id node, NSUInteger *count, unsigned depth);

@interface NXWorkspace : NSViewController <NSOutlineViewDataSource, NSOutlineViewDelegate, NSSearchFieldDelegate>
@property(copy) void (^openFile)(NSString *path);
@property(copy) void (^goToPosition)(NSInteger position);
@property(copy) void (^changed)(void);
@property(copy) void (^refreshRequested)(void);
@property(nonatomic, readonly) NSArray *projects;
@property(nonatomic, readonly) NSArray *symbols;
@property(nonatomic, readonly) NSInteger activePanel;
- (instancetype)initWithState:(NSDictionary *)state;
- (NSDictionary *)record;
- (void)addFolderRoot:(NSString *)path;
- (void)showPanel:(NSInteger)panel;
- (void)updateFunctions:(NSData *)text path:(NSString *)path lexer:(NSString *)lexer;
- (void)addProjectNamed:(NSString *)name;
- (BOOL)addPaths:(NSArray<NSString *> *)paths toNode:(NSMutableDictionary *)parent error:(NSError **)error;
- (BOOL)loadWorkspace:(NSString *)path panel:(NSInteger)panel error:(NSError **)error;
- (BOOL)saveWorkspace:(NSString *)path panel:(NSInteger)panel error:(NSError **)error;
@end
