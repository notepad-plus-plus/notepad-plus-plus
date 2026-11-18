// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Explicit code-page maps reject undefined bytes and unrepresentable characters.
#import "Codecs.h"
NSArray *NXSingleByteEncodings(void){static NSArray *records;static dispatch_once_t once;dispatch_once(&once,^{NSData *data=[NSData dataWithContentsOfURL:[NSBundle.mainBundle URLForResource:@"encodings" withExtension:@"json"]];records=data?[NSJSONSerialization JSONObjectWithData:data options:0 error:nil]:@[];});return records;}
static NSDictionary *Encoding(NSStringEncoding encoding){for(NSDictionary *record in NXSingleByteEncodings())if([record[@"encoding"] unsignedLongLongValue]==encoding)return record;return nil;}
NSString *NXDecodeEncoding(NSData *data,NSStringEncoding encoding){
    if(!data)return nil;if(encoding<=UINT32_MAX)return [[NSString alloc]initWithData:data encoding:encoding];NSDictionary *record=Encoding(encoding);if(!record)return nil;NSMutableData *unicode=[NSMutableData dataWithCapacity:data.length*4];const unsigned char *bytes=static_cast<const unsigned char *>(data.bytes);for(NSUInteger i=0;i<data.length;++i){NSInteger code=[record[@"mapping"][bytes[i]] integerValue];if(code<0)return nil;uint32_t scalar=(uint32_t)code;[unicode appendBytes:&scalar length:4];}return [[NSString alloc]initWithData:unicode encoding:NSUTF32LittleEndianStringEncoding];
}
NSData *NXEncodeText(NSString *text,NSStringEncoding encoding){
    if(!text)return nil;if(encoding<=UINT32_MAX)return [text dataUsingEncoding:encoding allowLossyConversion:NO];NSDictionary *record=Encoding(encoding);if(!record)return nil;NSMutableDictionary *reverse=[NSMutableDictionary new];for(NSUInteger i=0;i<256;++i){NSNumber *code=record[@"mapping"][i];if(code.integerValue>=0&&!reverse[code])reverse[code]=@(i);}
    NSData *unicode=[text dataUsingEncoding:NSUTF32LittleEndianStringEncoding];NSMutableData *data=[NSMutableData dataWithCapacity:unicode.length/4];const uint32_t *scalars=static_cast<const uint32_t *>(unicode.bytes);for(NSUInteger i=0;i<unicode.length/4;++i){NSNumber *byte=reverse[@(scalars[i])];if(!byte)return nil;unsigned char value=byte.unsignedCharValue;[data appendBytes:&value length:1];}return data;
}
NSString *NXEncodingName(NSStringEncoding encoding){return encoding>UINT32_MAX?Encoding(encoding)[@"name"] ?: @"Unknown encoding":[NSString localizedNameOfStringEncoding:encoding];}
