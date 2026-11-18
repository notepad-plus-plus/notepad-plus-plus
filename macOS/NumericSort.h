// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Exact decimal sorting without floating-point conversion, added 2026-11-01.
#pragma once
#include <string>
#include <string_view>
struct NXNumber {
    bool valid=false,negative=false;
    std::string integer,fraction;
};
inline NXNumber NXParseNumber(std::string_view value,char separator) {
    NXNumber n;size_t a=value.find_first_not_of(" \t"),b=value.find_last_not_of(" \t");if(a==std::string::npos)return n;
    value=value.substr(a,b-a+1);if(value.front()=='-'||value.front()=='+'){n.negative=value.front()=='-';value.remove_prefix(1);}bool decimal=false,digit=false;
    for(char c:value){if(separator&&c==separator&&!decimal){decimal=true;continue;}if(c<'0'||c>'9')return n;digit=true;(decimal?n.fraction:n.integer).push_back(c);}
    if(!digit)return n;auto first=n.integer.find_first_not_of('0');n.integer=first==std::string::npos?"0":n.integer.substr(first);while(!n.fraction.empty()&&n.fraction.back()=='0')n.fraction.pop_back();if(n.integer=="0"&&n.fraction.empty())n.negative=false;n.valid=true;return n;
}
inline int NXCompareNumber(const NXNumber &a,const NXNumber &b) {
    if(a.negative!=b.negative)return a.negative?-1:1;int result=0;
    if(a.integer.size()!=b.integer.size())result=a.integer.size()<b.integer.size()?-1:1;
    else if(a.integer!=b.integer)result=a.integer<b.integer?-1:1;
    else {size_t length=std::max(a.fraction.size(),b.fraction.size());for(size_t i=0;i<length;++i){char x=i<a.fraction.size()?a.fraction[i]:'0',y=i<b.fraction.size()?b.fraction[i]:'0';if(x!=y){result=x<y?-1:1;break;}}}
    return a.negative?-result:result;
}
