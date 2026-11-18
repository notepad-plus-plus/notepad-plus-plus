// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Bounded Boost.Regex search for native documents, added 2026-11-01.
#import "FileSearch.h"
#import "Storage.h"
#define BOOST_REGEX_STANDALONE
#define BOOST_REGEX_MAX_STATE_COUNT 1000000
#include <boost/regex.hpp>
#include <string>
#include <vector>
static std::wstring Wide(NSString *text) {NSData *data=[text dataUsingEncoding:NSUTF32LittleEndianStringEncoding];return std::wstring(static_cast<const wchar_t *>(data.bytes),data.length/sizeof(wchar_t));}
static NSString *Native(const std::wstring &text) {return [[NSString alloc]initWithBytes:text.data() length:text.size()*sizeof(wchar_t) encoding:NSUTF32LittleEndianStringEncoding];}
NSArray *NXBoostMatches(NSString *text, NSDictionary *options, NSError **error) {
    try {
        std::wstring source=Wide(text),pattern=Wide(options[@"query"]),replacement=Wide(options[@"replacement"] ?: @"");
        boost::regex::flag_type flags=boost::regex::perl;if(![options[@"matchCase"] boolValue])flags|=boost::regex::icase;
        boost::wregex regex(pattern,flags);NSMutableArray *matches=[NSMutableArray new];
        std::vector<NSUInteger> offsets(source.size()+1),lines(source.size()+1);NSUInteger offset=0,line=1;
        for(size_t i=0;i<source.size();++i){offsets[i]=offset;lines[i]=line;auto c=source[i];offset+=c<=0x7f?1:c<=0x7ff?2:c<=0xffff?3:4;if(c=='\n'||(c=='\r'&&(i+1==source.size()||source[i+1]!='\n')))++line;}offsets.back()=offset;lines.back()=line;
        NSTimeInterval deadline=NSProcessInfo.processInfo.systemUptime+2;
        for(boost::wsregex_iterator it(source.begin(),source.end(),regex),end;it!=end;++it){
            if(NSProcessInfo.processInfo.systemUptime>deadline){if(error)*error=NXError(@"This search exceeded the per-document time limit.");return nil;}
            const auto &match=*it;size_t start=match.position(),finish=start+match.length();
            if([options[@"wholeWord"] boolValue]){auto word=[](wchar_t c){return c=='_'||[NSCharacterSet.alphanumericCharacterSet longCharacterIsMember:c];};if((start&&word(source[start-1]))||(finish<source.size()&&word(source[finish])))continue;}
            NSString *value=Native(match.format(replacement));if(!value){if(error)*error=NXError(@"The replacement produced invalid Unicode.");return nil;}
            size_t a=start,b=finish;while(a&&source[a-1]!='\n'&&source[a-1]!='\r')--a;while(b<source.size()&&source[b]!='\n'&&source[b]!='\r'&&b-a<240)++b;
            [matches addObject:@{@"position":@(offsets[start]),@"length":@(offsets[finish]-offsets[start]),@"line":@(lines[start]),@"excerpt":Native(source.substr(a,b-a)) ?: @"",@"replacement":value}];if(matches.count>=20000)break;
        }return matches;
    }catch(const std::exception &exception){if(error)*error=NXError([NSString stringWithFormat:@"The regular expression could not be evaluated: %s",exception.what()]);return nil;}
}
