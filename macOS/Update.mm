// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Created for NoteX on 2026-11-18.
#import "Update.h"
#import <Cocoa/Cocoa.h>
#include "../updater/Version.h"
void NXUpdate(NSString *mode, NSString *profile) {
    NSTask *task=[NSTask new];
    task.executableURL=[NSBundle.mainBundle.executableURL.URLByDeletingLastPathComponent URLByAppendingPathComponent:@"NoteXUpdater"];
    task.arguments=@[@"-mode",mode,@"-root",NSBundle.mainBundle.bundlePath,@"-profile",profile,@"-version",@NOTEX_UPDATE_VERSION,@"-parent",[@(NSProcessInfo.processInfo.processIdentifier) stringValue]];
    NSError *error=nil;
    if(![task launchAndReturnError:&error] && ![mode isEqual:@"auto"]) {
        NSAlert *alert=[NSAlert new];alert.messageText=@"NoteX Updates";alert.informativeText=@"The updater could not be started. Reinstall the complete NoteX application.";[alert runModal];
    }
}
void NXUpdateHealthy(void) {
    NSString *bundle=NSBundle.mainBundle.bundlePath;
    NSString *dir=[bundle.stringByDeletingLastPathComponent stringByAppendingPathComponent:[NSString stringWithFormat:@".%@-update",bundle.lastPathComponent]];
    NSError *error=nil;
    if([NSFileManager.defaultManager createDirectoryAtPath:dir withIntermediateDirectories:YES attributes:nil error:&error])
        [@NOTEX_UPDATE_VERSION writeToFile:[dir stringByAppendingPathComponent:@"healthy-" NOTEX_UPDATE_VERSION] atomically:YES encoding:NSUTF8StringEncoding error:&error];
}
