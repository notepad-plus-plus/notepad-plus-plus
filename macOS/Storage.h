// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Native macOS storage, created 2026-11-01.
#import <Foundation/Foundation.h>
#include "NoteXFormat.h"

NSDictionary *NXDecodeSession(NSData *data);
NSError *NXError(NSString *message);
NSData *NXDigest(NSData *data);
NSString *NXCanonicalPath(NSString *path);
BOOL NXWrite(NSData *data, NSString *path, NSError **error);
NSData *NXCompanion(NSData *text, const NoteX::State &state);
BOOL NXReadCompanion(NSData *bytes, NSData *text, NoteX::State &state);
BOOL NXOwnedCompanion(NSData *bytes);

@interface NXStorage : NSObject
@property(nonatomic, readonly) NSString *directory;
@property(nonatomic, readonly) BOOL recoveredPrevious;
- (instancetype)initWithDirectory:(NSString *)directory error:(NSError **)error;
- (NSDictionary *)load:(NSError **)error;
- (BOOL)save:(NSDictionary *)session error:(NSError **)error;
@end
