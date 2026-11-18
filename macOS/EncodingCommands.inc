// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Explicit lossless encoding selection, added 2026-11-01.
static NSArray *NXLegacyEncodings(void) {
    NSMutableArray *result=[NXSingleByteEncodings() mutableCopy];
    for(NSNumber *page in @[@932,@936,@949,@950,@1361,@51932,@51949]) {
        CFStringEncoding cf=CFStringConvertWindowsCodepageToEncoding(page.unsignedIntValue);if(cf==kCFStringEncodingInvalidId)continue;
        NSStringEncoding encoding=CFStringConvertEncodingToNSStringEncoding(cf);NSString *name=[NSString localizedNameOfStringEncoding:encoding];if(!name.length)continue;
        [result addObject:@{@"encoding":@(encoding),@"name":[NSString stringWithFormat:@"%@ (CP%@)",name,page]}];
    }return result;
}
@implementation NXApp (EncodingCommands)
- (void)legacyEncoding:(id)sender {
    NSArray *encodings=NXLegacyEncodings();NSInteger index=NXChoose([sender tag]?@"Convert Document Encoding":@"Open with Encoding",[encodings valueForKey:@"name"],self.window);if(index<0)return;NSStringEncoding encoding=[encodings[index][@"encoding"] unsignedIntegerValue];
    if([sender tag]){NSData *data=NXEncodeText(self.current.editor.string,encoding);NSString *roundTrip=data?NXDecodeEncoding(data,encoding):nil;if(![roundTrip isEqual:self.current.editor.string]){Alert(@"This encoding cannot preserve every character. The document was not changed.");return;}self.current.encoding=encoding;self.current.bom=NO;self.current.restoredDirty=YES;[self tick:nil];}
    else {NSOpenPanel *panel=[NSOpenPanel openPanel];panel.allowsMultipleSelection=YES;if([panel runModal]!=NSModalResponseOK)return;self.openingEncoding=encoding;for(NSURL *url in panel.URLs){NSError *error=nil;if(![self openPath:url.path error:&error])Alert(error.localizedDescription);}self.openingEncoding=0;}
}
- (void)buildEncodingMenus {
    NSMenu *encoding=[NSApp.mainMenu itemWithTitle:@"Encoding"].submenu;[self item:encoding title:@"Open with Legacy Encoding…" action:@selector(legacyEncoding:) key:@"" tag:0];[self item:encoding title:@"Convert to Legacy Encoding…" action:@selector(legacyEncoding:) key:@"" tag:1];
}
@end
