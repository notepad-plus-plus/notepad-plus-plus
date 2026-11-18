// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Commands run only after an explicit Run action; output is bounded in memory.
#import "RunTool.h"
#import "Storage.h"
#include <spawn.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>
#include <signal.h>
#include <vector>
#include <string>
@implementation NXCommandTask
- (void)terminate {if(self.running&&self.processIdentifier>0)kill(-self.processIdentifier,SIGTERM);}
@end
@implementation NXRunTool {
    NSTextField *_name,*_command,*_status;
    NSPopUpButton *_saved;
    NSTextView *_outputView;
    NSButton *_run,*_stop;
}
- (instancetype)init {return [super initWithWindow:nil];}
- (BOOL)launchCommand:(NSString *)command environment:(NSDictionary *)environment error:(NSError **)error {
    if(self.task.running){if(error)*error=NXError(@"A command is already running. Stop it before starting another.");return NO;}
    if(!command.length||command.length>65536){if(error)*error=NXError(@"Enter a command of at most 65,536 characters.");return NO;}
    for(NSString *key in environment)if([key containsString:@"="]||[environment[key] lengthOfBytesUsingEncoding:NSUTF8StringEncoding]>131000||[environment[key] rangeOfString:[NSString stringWithFormat:@"%C",(unichar)0]].location!=NSNotFound){if(error)*error=NXError(@"Document variables contain a null byte or exceed the environment size limit. Narrow the selection before running.");return NO;}
    self.captured=[NSMutableData new];self.output=@"";
    NSMutableDictionary *variables=[NSProcessInfo.processInfo.environment mutableCopy];[variables addEntriesFromDictionary:environment ?: @{}];
    std::vector<std::string> storage;for(NSString *key in variables)storage.push_back(std::string(key.UTF8String)+"="+[variables[key] UTF8String]);
    std::vector<char *> env;for(auto &entry:storage)env.push_back(entry.data());env.push_back(nullptr);
    std::string executable="/bin/zsh",option="-lc",script=command.UTF8String;char *arguments[]={executable.data(),option.data(),script.data(),nullptr};
    int descriptors[2];if(pipe(descriptors)!=0){if(error)*error=[NSError errorWithDomain:NSPOSIXErrorDomain code:errno userInfo:nil];return NO;}
    posix_spawn_file_actions_t actions;posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions,descriptors[1],STDOUT_FILENO);posix_spawn_file_actions_adddup2(&actions,descriptors[1],STDERR_FILENO);
    posix_spawn_file_actions_addclose(&actions,descriptors[0]);posix_spawn_file_actions_addclose(&actions,descriptors[1]);posix_spawn_file_actions_addopen(&actions,STDIN_FILENO,"/dev/null",O_RDONLY,0);
    if([environment[@"NOTEX_DIRECTORY"] length])posix_spawn_file_actions_addchdir_np(&actions,[environment[@"NOTEX_DIRECTORY"] fileSystemRepresentation]);
    posix_spawnattr_t attributes;posix_spawnattr_init(&attributes);posix_spawnattr_setflags(&attributes,POSIX_SPAWN_SETPGROUP);posix_spawnattr_setpgroup(&attributes,0);
    pid_t pid=0;int failure=posix_spawn(&pid,executable.c_str(),&actions,&attributes,arguments,env.data());posix_spawn_file_actions_destroy(&actions);posix_spawnattr_destroy(&attributes);close(descriptors[1]);
    if(failure){close(descriptors[0]);if(error)*error=[NSError errorWithDomain:NSPOSIXErrorDomain code:failure userInfo:nil];return NO;}
    fcntl(descriptors[0],F_SETFL,fcntl(descriptors[0],F_GETFL)|O_NONBLOCK);
    self.task=[NXCommandTask new];self.task.processIdentifier=pid;self.task.running=YES;NXCommandTask *task=self.task;
    _run.enabled=NO;_stop.enabled=YES;_status.stringValue=@"Running…";int fd=descriptors[0];
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED,0),^{
        BOOL exited=NO,eof=NO;int status=0;NSTimeInterval exitTime=0;
        while(!exited||!eof){
            struct pollfd descriptor={fd,POLLIN|POLLHUP,0};poll(&descriptor,1,50);char bytes[16384];ssize_t length;
            while((length=read(fd,bytes,sizeof bytes))>0){NSUInteger available=self.captured.length<1024*1024?1024*1024-self.captured.length:0;if(available)[self.captured appendBytes:bytes length:MIN((NSUInteger)length,available)];}
            if(length==0)eof=YES;
            if(!exited&&waitpid(pid,&status,WNOHANG)==pid){exited=YES;exitTime=NSProcessInfo.processInfo.systemUptime;}
            // Detached descendants must redirect their own output. Never hang the UI
            // or retain a command indefinitely just because they inherited a pipe.
            if(exited&&NSProcessInfo.processInfo.systemUptime-exitTime>0.2)break;
        }
        close(fd);self.output=[[NSString alloc]initWithData:self.captured encoding:NSUTF8StringEncoding] ?: @"The command produced non-UTF-8 output.";
        task.terminationStatus=WIFEXITED(status)?WEXITSTATUS(status):128+WTERMSIG(status);task.running=NO;
        dispatch_async(dispatch_get_main_queue(),^{self->_outputView.string=self.output;self->_status.stringValue=[NSString stringWithFormat:@"Exit status: %d%@",task.terminationStatus,self.captured.length>=1024*1024?@" — output limited to 1 MB":@""];self->_run.enabled=YES;self->_stop.enabled=NO;});
    });return YES;
}
- (void)run:(id)sender {NSError *error=nil;if(![self launchCommand:_command.stringValue environment:self.documentEnvironment?self.documentEnvironment():@{} error:&error])[NSApp presentError:error];}
- (void)stop:(id)sender {if(self.task.running)[self.task terminate];}
- (void)select:(id)sender {NSInteger index=_saved.indexOfSelectedItem-1;if(index>=0){_name.stringValue=self.commands[index][@"name"];_command.stringValue=self.commands[index][@"command"];}else{_name.stringValue=@"";_command.stringValue=@"";}}
- (void)refresh {[_saved removeAllItems];[_saved addItemWithTitle:@"New Command"];for(NSDictionary *command in self.commands)[_saved addItemWithTitle:command[@"name"]];}
- (void)save:(id)sender {
    NSString *name=[_name.stringValue stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];if(!name.length||name.length>100||!_command.stringValue.length){[NSApp presentError:NXError(@"Enter a name of at most 100 characters and a command.")];return;}
    NSMutableArray *commands=[self.commands mutableCopy] ?: [NSMutableArray new];NSInteger selected=_saved.indexOfSelectedItem-1;
    for(NSUInteger i=0;i<commands.count;++i)if((NSInteger)i!=selected&&[commands[i][@"name"] caseInsensitiveCompare:name]==NSOrderedSame){[NSApp presentError:NXError(@"Choose a unique command name.")];return;}
    NSDictionary *record=@{@"name":name,@"command":_command.stringValue};if(selected>=0)commands[selected]=record;else [commands addObject:record];
    if(self.saveCommands&&self.saveCommands(commands)){self.commands=commands;[self refresh];[_saved selectItemWithTitle:name];}
}
- (void)remove:(id)sender {NSInteger index=_saved.indexOfSelectedItem-1;if(index<0)return;NSMutableArray *commands=[self.commands mutableCopy];[commands removeObjectAtIndex:index];if(self.saveCommands&&self.saveCommands(commands)){self.commands=commands;[self refresh];[self select:nil];}}
- (void)showForWindow:(NSWindow *)parent {
    if(!self.window){self.window=[[NSWindow alloc]initWithContentRect:NSMakeRect(0,0,720,480) styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable backing:NSBackingStoreBuffered defer:NO];self.window.releasedWhenClosed=NO;self.window.title=@"Run Command";
        _saved=[[NSPopUpButton alloc]initWithFrame:NSMakeRect(16,432,688,28) pullsDown:NO];_saved.target=self;_saved.action=@selector(select:);[self.window.contentView addSubview:_saved];
        _name=[[NSTextField alloc]initWithFrame:NSMakeRect(16,394,688,26)];_name.placeholderString=@"Command name";[self.window.contentView addSubview:_name];
        _command=[[NSTextField alloc]initWithFrame:NSMakeRect(16,356,688,26)];_command.placeholderString=@"Shell command (zsh)";[self.window.contentView addSubview:_command];
        NSTextField *help=[NSTextField wrappingLabelWithString:@"Commands run in zsh. Use quoted environment variables such as \"$NOTEX_FILE\", \"$NOTEX_DIRECTORY\", \"$NOTEX_SELECTION\", and \"$NOTEX_LINE\". Selecting or saving a command does not run it."];help.frame=NSMakeRect(16,290,688,56);[self.window.contentView addSubview:help];
        NSArray *names=@[@"Run",@"Stop",@"Save Command",@"Delete Command"];SEL actions[]={@selector(run:),@selector(stop:),@selector(save:),@selector(remove:)};
        for(NSUInteger i=0;i<names.count;++i){NSButton *button=[NSButton buttonWithTitle:names[i] target:self action:actions[i]];button.frame=NSMakeRect(16+i*170,250,160,32);[self.window.contentView addSubview:button];if(i==0)_run=button;if(i==1){_stop=button;button.enabled=NO;}}
        NSScrollView *scroll=[[NSScrollView alloc]initWithFrame:NSMakeRect(16,48,688,190)];scroll.hasVerticalScroller=YES;_outputView=[[NSTextView alloc]initWithFrame:scroll.bounds];_outputView.editable=NO;_outputView.font=[NSFont monospacedSystemFontOfSize:12 weight:NSFontWeightRegular];scroll.documentView=_outputView;[self.window.contentView addSubview:scroll];
        _status=[NSTextField labelWithString:@"Ready"];_status.frame=NSMakeRect(16,12,688,26);[self.window.contentView addSubview:_status];[self refresh];[self.window center];
    }self.window.appearance=parent.appearance;[self.window makeKeyAndOrderFront:nil];
}
@end
