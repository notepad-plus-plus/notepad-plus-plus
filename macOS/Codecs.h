// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Lossless text codecs, added 2026-11-01.
#import <Foundation/Foundation.h>
NSArray *NXSingleByteEncodings(void);
NSString *NXDecodeEncoding(NSData *data, NSStringEncoding encoding);
NSData *NXEncodeText(NSString *text, NSStringEncoding encoding);
NSString *NXEncodingName(NSStringEncoding encoding);
