// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Created for NoteQu on 2026-11-18.
#import "Update.h"
#import <Cocoa/Cocoa.h>
#include "../updater/Version.h"
void NXUpdate(NSString *mode, NSString *profile) {
    NSTask *task=[NSTask new];
    task.executableURL=[NSBundle.mainBundle.executableURL.URLByDeletingLastPathComponent URLByAppendingPathComponent:@"NoteQuUpdater"];
    task.arguments=@[@"-mode",mode,@"-root",NSBundle.mainBundle.bundlePath,@"-profile",profile,@"-version",@NOTEQU_UPDATE_VERSION,@"-parent",[@(NSProcessInfo.processInfo.processIdentifier) stringValue]];
    NSError *error=nil;
    if(![task launchAndReturnError:&error] && ![mode isEqual:@"auto"]) {
        NSAlert *alert=[NSAlert new];alert.messageText=@"NoteQu Updates";alert.informativeText=@"The updater could not be started. Reinstall the complete NoteQu application.";[alert runModal];
    }
}
void NXUpdateHealthy(void) {
    NSString *bundle=NSBundle.mainBundle.bundlePath;
    NSString *dir=[bundle.stringByDeletingLastPathComponent stringByAppendingPathComponent:[NSString stringWithFormat:@".%@-update",bundle.lastPathComponent]];
    NSError *error=nil;
    if([NSFileManager.defaultManager createDirectoryAtPath:dir withIntermediateDirectories:YES attributes:nil error:&error])
        [@NOTEQU_UPDATE_VERSION writeToFile:[dir stringByAppendingPathComponent:@"healthy-" NOTEQU_UPDATE_VERSION] atomically:YES encoding:NSUTF8StringEncoding error:&error];
}
