// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Checked numeric column sequences, added 2026-11-01.
#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>
inline bool NXColumnNumbers(int64_t start,int64_t step,unsigned repeat,unsigned base,unsigned width,size_t count,std::vector<std::string> &result) {
    if(!repeat||repeat>1000000||(base!=2&&base!=8&&base!=10&&base!=16)||width>64||count>1000000)return false;
    result.clear();int64_t value=start;const char *digits="0123456789ABCDEF";
    for(size_t i=0;i<count;++i){
        if(i&&i%repeat==0&&__builtin_add_overflow(value,step,&value)){result.clear();return false;}
        uint64_t absolute=value<0?uint64_t(-(value+1))+1:uint64_t(value);std::string text;
        do{text.push_back(digits[absolute%base]);absolute/=base;}while(absolute);
        while(text.size()<width)text+='0';if(value<0)text+='-';std::reverse(text.begin(),text.end());result.push_back(text);
    }return true;
}
