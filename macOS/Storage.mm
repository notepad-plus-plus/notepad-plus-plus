// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Native macOS storage, created 2026-11-01.
#import "Storage.h"
#import "Workspace.h"
#import "Configuration.h"
#import <CommonCrypto/CommonDigest.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>

NSError *NXError(NSString *message) {
    return [NSError errorWithDomain:@"NoteX" code:1
                           userInfo:@{NSLocalizedDescriptionKey: message}];
}
NSString *NXCanonicalPath(NSString *path) {return path.length?path.stringByStandardizingPath.stringByResolvingSymlinksInPath:@"";}
NSData *NXDigest(NSData *data) {
    unsigned char digest[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256_CTX context;
    CC_SHA256_Init(&context);
    const auto *bytes = static_cast<const unsigned char *>(data.bytes);
    for (NSUInteger offset = 0; offset < data.length;) {
        auto count = static_cast<CC_LONG>(std::min<NSUInteger>(data.length - offset, 1 << 20));
        CC_SHA256_Update(&context, bytes + offset, count);
        offset += count;
    }
    CC_SHA256_Final(digest, &context);
    return [NSData dataWithBytes:digest length:sizeof digest];
}
BOOL NXWrite(NSData *data, NSString *path, NSError **error) {
    NSString *temp = [path stringByAppendingFormat:@".tmp-%@", NSUUID.UUID.UUIDString];
    int fd = open(temp.fileSystemRepresentation, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd < 0) {
        if (error) *error = [NSError errorWithDomain:NSPOSIXErrorDomain code:errno userInfo:nil];
        return NO;
    }
    struct stat original{};
    if(lstat(path.fileSystemRepresentation,&original)==0 && S_ISREG(original.st_mode) && fchmod(fd,original.st_mode&0777)!=0){int failure=errno;close(fd);unlink(temp.fileSystemRepresentation);if(error)*error=[NSError errorWithDomain:NSPOSIXErrorDomain code:failure userInfo:nil];return NO;}
    auto bytes = static_cast<const char *>(data.bytes);
    NSUInteger offset = 0;
    while (offset < data.length) {
        ssize_t count = write(fd, bytes + offset, data.length - offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) break;
        offset += count;
    }
    bool ok = offset == data.length && fsync(fd) == 0;
    int savedError = errno;
    close(fd);
    if (ok) { ok = rename(temp.fileSystemRepresentation, path.fileSystemRepresentation) == 0; savedError = errno; }
    if (!ok) {
        unlink(temp.fileSystemRepresentation);
        if (error) *error = [NSError errorWithDomain:NSPOSIXErrorDomain code:savedError ?: EIO userInfo:nil];
        return NO;
    }
    int directory = open(path.stringByDeletingLastPathComponent.fileSystemRepresentation, O_RDONLY);
    if (directory >= 0) { fsync(directory); close(directory); }
    return YES;
}
BOOL NXOwnedCompanion(NSData *bytes) {
    return bytes.length >= 8 && (!memcmp(bytes.bytes, "NOTEXSC1", 8) || !memcmp(bytes.bytes, "NPPSIDE1", 8));
}
NSData *NXCompanion(NSData *text, const NoteX::State &state) {
    NoteX::State compact;
    for (auto run : state.runs) {
        const auto &format = state.formats.at(run.style);
        auto found = std::find(compact.formats.begin(), compact.formats.end(), format);
        if (found == compact.formats.end()) {
            run.style = static_cast<unsigned>(compact.formats.size());
            compact.formats.push_back(format);
        } else run.style = static_cast<unsigned>(found - compact.formats.begin());
        compact.runs.push_back(run);
    }
    NSMutableData *data = [NSMutableData dataWithBytes:"NOTEXSC1" length:8];
    [data appendData:NXDigest(text)];
    auto encoded = NoteX::encode(compact);
    [data appendBytes:encoded.data() length:encoded.size()];
    return data;
}
BOOL NXReadCompanion(NSData *bytes, NSData *text, NoteX::State &state) {
    if (bytes.length < 64 || bytes.length > 64 * 1024 * 1024 || !NXOwnedCompanion(bytes)) return NO;
    if (memcmp(static_cast<const char *>(bytes.bytes) + 8, NXDigest(text).bytes, 32)) return NO;
    return NoteX::decode(std::string(static_cast<const char *>(bytes.bytes) + 40, bytes.length - 40), state, text.length);
}
NSDictionary *NXDecodeSession(NSData *data) {
    if (!data) return nil;
    id root = [NSPropertyListSerialization propertyListWithData:data options:NSPropertyListImmutable format:nil error:nil];
    if (![root isKindOfClass:NSDictionary.class] || ![root[@"schema"] isEqual:@1] ||
        ![root[@"documents"] isKindOfClass:NSArray.class] || ![root[@"selected"] isKindOfClass:NSNumber.class]) return nil;
    NSArray *documents = root[@"documents"];
    if (documents.count > 1000) return nil;
    if(root[@"configuration"]&&!NXValidateConfiguration(root[@"configuration"]))return nil;
    if(root[@"secondary"]){id secondary=root[@"secondary"];if(![secondary isKindOfClass:NSDictionary.class])return nil;
        if([secondary count]){if(![secondary[@"document"] isKindOfClass:NSString.class])return nil;
            for(NSString *key in @[@"vertical",@"focused",@"synchronized",@"caret",@"anchor",@"firstLine"])if(![secondary[key] isKindOfClass:NSNumber.class])return nil;
            for(NSString *key in @[@"ratio",@"zoom",@"xOffset",@"syncVertical",@"syncHorizontal"])if(secondary[key]&&![secondary[key] isKindOfClass:NSNumber.class])return nil;
        }
    }
    for (NSString *key in @[@"dark", @"wrap", @"sidebarVisible"])
        if (root[key] && ![root[key] isKindOfClass:NSNumber.class]) return nil;
    // Optional fields preserve compatibility with the first native release.
    // Reject damaged additions instead of silently resetting project or macro data.
    if (root[@"workspace"]) {
        id workspace = root[@"workspace"];
        if (![workspace isKindOfClass:NSDictionary.class] || ![workspace[@"projects"] isKindOfClass:NSArray.class] || [workspace[@"projects"] count] != 3 || ![workspace[@"folders"] isKindOfClass:NSArray.class]) return nil;
        for (NSString *key in @[@"panel", @"sorted"])
            if (workspace[key] && ![workspace[key] isKindOfClass:NSNumber.class]) return nil;
        if (workspace[@"ruleOverride"] && ![workspace[@"ruleOverride"] isKindOfClass:NSString.class]) return nil;
        for (id path in workspace[@"folders"]) if (![path isKindOfClass:NSString.class]) return nil;
        for (id project in workspace[@"projects"]) {
            NSUInteger count = 0;
            if (![project isKindOfClass:NSDictionary.class] || !NXValidateProject(project[@"root"], &count, 0) || ![project[@"path"] isKindOfClass:NSString.class] || ![project[@"hash"] isKindOfClass:NSData.class] || ![project[@"dirty"] isKindOfClass:NSNumber.class]) return nil;
        }
    }
    for(NSString *layoutKey in @[@"menuLayout",@"contextLayout"]) if (root[layoutKey]) {
        id layout = root[layoutKey];
        if (![layout isKindOfClass:NSDictionary.class] || ![layout[@"order"] isKindOfClass:NSDictionary.class] || ![layout[@"hidden"] isKindOfClass:NSArray.class]) return nil;
        for (id key in layout[@"order"]) {
            if (![key isKindOfClass:NSString.class] || ![layout[@"order"][key] isKindOfClass:NSArray.class]) return nil;
            for (id item in layout[@"order"][key]) if (![item isKindOfClass:NSString.class]) return nil;
        }
        for (id item in layout[@"hidden"]) if (![item isKindOfClass:NSString.class]) return nil;
        if(layout[@"shortcuts"]){if(![layout[@"shortcuts"] isKindOfClass:NSDictionary.class])return nil;
            for(id key in layout[@"shortcuts"]){id shortcut=layout[@"shortcuts"][key];if(![key isKindOfClass:NSString.class]||![shortcut isKindOfClass:NSDictionary.class]||![shortcut[@"key"] isKindOfClass:NSString.class]||[shortcut[@"key"] length]>1||![shortcut[@"modifiers"] isKindOfClass:NSNumber.class])return nil;}
        }
    }
    if (root[@"preferences"]) {
        id preferences = root[@"preferences"];
        if(!NXValidatePreferences(preferences))return nil;
        if (![preferences isKindOfClass:NSDictionary.class] || ![preferences[@"font"] isKindOfClass:NSString.class]) return nil;
        for (NSString *key in @[@"fontSize", @"tabWidth", @"useTabs", @"lineNumbers", @"whitespace", @"autoIndent", @"braceMatch", @"autoComplete"])
            if (![preferences[key] isKindOfClass:NSNumber.class]) return nil;
    }
    if (root[@"macros"]) {
        if (![root[@"macros"] isKindOfClass:NSArray.class] || [root[@"macros"] count] > 1000) return nil;
        for (id macro in root[@"macros"]) {
            if (![macro isKindOfClass:NSDictionary.class] || ![macro[@"name"] isKindOfClass:NSString.class] || ![macro[@"actions"] isKindOfClass:NSArray.class] || [macro[@"actions"] count] > 10000) return nil;
            for (id action in macro[@"actions"]) {
                if (![action isKindOfClass:NSDictionary.class]) return nil;
                for (NSString *key in @[@"message", @"w", @"l"]) if (![action[key] isKindOfClass:NSNumber.class]) return nil;
                if (action[@"text"] && (![action[@"text"] isKindOfClass:NSData.class] || [action[@"text"] length] > 1024 * 1024)) return nil;
            }
        }
    }
    for (id doc in documents) {
        if (![doc isKindOfClass:NSDictionary.class]) return nil;
        if(doc[@"builtinLanguage"]&&![doc[@"builtinLanguage"] isKindOfClass:NSString.class])return nil;
        if(doc[@"customLanguage"]&&![doc[@"customLanguage"] isKindOfClass:NSString.class])return nil;
        for(NSString *key in @[@"zoom",@"xOffset",@"pinned",@"tabColor",@"secondaryOnly"])if(doc[key]&&![doc[key] isKindOfClass:NSNumber.class])return nil;
        if (doc[@"eol"] && (![doc[@"eol"] isKindOfClass:NSNumber.class] || [doc[@"eol"] integerValue] < 0 || [doc[@"eol"] integerValue] > 2)) return nil;
        if (doc[@"bookmarks"]) {
            if (![doc[@"bookmarks"] isKindOfClass:NSArray.class]) return nil;
            for (id line in doc[@"bookmarks"]) if (![line isKindOfClass:NSNumber.class]) return nil;
        }
        for (NSString *key in @[@"id", @"path", @"title", @"lexer"])
            if (![doc[key] isKindOfClass:NSString.class]) return nil;
        for (NSString *key in @[@"text", @"format", @"diskHash"])
            if (![doc[key] isKindOfClass:NSData.class]) return nil;
        for (NSString *key in @[@"caret", @"anchor", @"firstLine", @"encoding", @"bom", @"dirty"])
            if (![doc[key] isKindOfClass:NSNumber.class]) return nil;
        NSData *text = doc[@"text"];
        if (![[NSString alloc] initWithData:text encoding:NSUTF8StringEncoding]) return nil;
        NoteX::State format;
        if (!NXReadCompanion(doc[@"format"], text, format)) return nil;
    }
    return root;
}
@implementation NXStorage {
    int _lock;
    NSString *_directory;
    BOOL _recoveredPrevious;
}
- (instancetype)initWithDirectory:(NSString *)directory error:(NSError **)error {
    self = [super init];
    if (!self) return nil;
    _lock = -1;
    _directory = [directory copy];
    if (![NSFileManager.defaultManager createDirectoryAtPath:directory withIntermediateDirectories:YES
                                                attributes:@{NSFilePosixPermissions:@0700} error:error]) return nil;
    NSString *lock = [directory stringByAppendingPathComponent:@"profile.lock"];
    _lock = open(lock.fileSystemRepresentation, O_CREAT | O_RDWR, 0600);
    if (_lock < 0 || flock(_lock, LOCK_EX | LOCK_NB) != 0) {
        if (error) *error = NXError(@"This NoteX profile is already open, or its lock cannot be accessed.");
        return nil;
    }
    return self;
}
- (NSString *)directory { return _directory; }
- (BOOL)recoveredPrevious { return _recoveredPrevious; }
- (void)dealloc { if (_lock >= 0) close(_lock); }
- (NSDictionary *)load:(NSError **)error {
    NSString *path = [_directory stringByAppendingPathComponent:@"session.plist"];
    if (![NSFileManager.defaultManager fileExistsAtPath:path] &&
        ![NSFileManager.defaultManager fileExistsAtPath:[path stringByAppendingString:@".previous"]]) return nil;
    NSData *current = [NSData dataWithContentsOfFile:path];
    NSDictionary *result = NXDecodeSession(current);
    if (result) return result;
    result = NXDecodeSession([NSData dataWithContentsOfFile:[path stringByAppendingString:@".previous"]]);
    if (result) {
        if (current && !NXWrite(current, [_directory stringByAppendingPathComponent:
            [NSString stringWithFormat:@"recovery-%@.plist", NSUUID.UUID.UUIDString]], error)) return nil;
        _recoveredPrevious = YES;
        return result;
    }
    if (error) *error = NXError(@"The session and its previous snapshot could not be read. Your files were preserved. Back up the NoteX Application Support folder before repairing it.");
    return nil;
}
- (BOOL)save:(NSDictionary *)session error:(NSError **)error {
    NSData *data = [NSPropertyListSerialization dataWithPropertyList:session format:NSPropertyListBinaryFormat_v1_0 options:0 error:error];
    if (!data || !NXDecodeSession(data)) { if (error && !*error) *error = NXError(@"The session could not be validated."); return NO; }
    NSString *path = [_directory stringByAppendingPathComponent:@"session.plist"];
    NSData *old = [NSData dataWithContentsOfFile:path];
    if (NXDecodeSession(old) && !NXWrite(old, [path stringByAppendingString:@".previous"], error)) return NO;
    return NXWrite(data, path, error);
}
@end
