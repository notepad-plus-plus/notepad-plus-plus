// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Independent native processes keep separate recoverable profiles, added 2026-11-01.
@implementation NXApp (Instances)
- (NSTask *)launchInstanceWithDocument:(NXDocument *)document testMode:(BOOL)test error:(NSError **)error {
    if(![self snapshot:error])return nil;
    NSString *directory=[self.storage.directory stringByAppendingPathComponent:[NSString stringWithFormat:@"Instances/%@%@",test?@"test-":@"",NSUUID.UUID.UUIDString]];
    NSMutableDictionary *session=[self.lastSnapshot mutableCopy];NSMutableDictionary *record=[document.record mutableCopy];if(record)record[@"secondaryOnly"]=@NO;
    session[@"documents"]=record?@[record]:@[];session[@"selected"]=@0;session[@"secondary"]=@{};[session removeObjectForKey:@"workspace"];
    NXStorage *storage=[[NXStorage alloc]initWithDirectory:directory error:error];if(!storage||![storage save:session error:error])return nil;storage=nil;
    NSTask *task=[NSTask new];task.executableURL=NSBundle.mainBundle.executableURL;task.arguments=test?@[@"--self-test",@"instance-verify"]:@[];
    NSMutableDictionary *environment=[NSProcessInfo.processInfo.environment mutableCopy];environment[@"NOTEX_PROFILE"]=directory;task.environment=environment;
    if(![task launchAndReturnError:error])return nil;return task;
}
- (void)openInstance:(id)sender {NSError *error=nil;if(![self launchInstanceWithDocument:[sender tag]?self.current:nil testMode:NO error:&error])Alert(error.localizedDescription);}
- (void)reopenInstance:(id)sender {
    NSString *root=[self.storage.directory stringByAppendingPathComponent:@"Instances"];NSMutableArray *paths=[NSMutableArray new],*titles=[NSMutableArray new];
    for(NSString *name in [NSFileManager.defaultManager contentsOfDirectoryAtPath:root error:nil]){NSString *path=[root stringByAppendingPathComponent:name];NSDictionary *session=NXDecodeSession([NSData dataWithContentsOfFile:[path stringByAppendingPathComponent:@"session.plist"]]);if(!session)continue;[paths addObject:path];NSArray *documents=session[@"documents"];[titles addObject:[NSString stringWithFormat:@"%@ — %@",[documents.firstObject objectForKey:@"title"] ?: @"Empty Window",name]];}
    NSInteger index=NXChoose(@"Reopen Independent Window",titles,self.window);if(index<0)return;NSError *error=nil;NXStorage *storage=[[NXStorage alloc]initWithDirectory:paths[index] error:&error];if(!storage){Alert(error.localizedDescription);return;}storage=nil;
    NSTask *task=[NSTask new];task.executableURL=NSBundle.mainBundle.executableURL;NSMutableDictionary *environment=[NSProcessInfo.processInfo.environment mutableCopy];environment[@"NOTEX_PROFILE"]=paths[index];task.environment=environment;if(![task launchAndReturnError:&error])Alert(error.localizedDescription);
}
- (void)buildInstanceMenus {
    [self item:[NSApp.mainMenu itemWithTitle:@"File"].submenu title:@"New Independent Window" action:@selector(openInstance:) key:@"" tag:0];
    [self item:[NSApp.mainMenu itemWithTitle:@"View"].submenu title:@"Open in New Instance" action:@selector(openInstance:) key:@"" tag:1];
    [self item:[NSApp.mainMenu itemWithTitle:@"File"].submenu title:@"Reopen Independent Window…" action:@selector(reopenInstance:) key:@"" tag:0];
}
@end
