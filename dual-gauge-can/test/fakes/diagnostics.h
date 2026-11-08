#pragma once
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>
namespace fake { inline std::vector<std::string> logs; }
inline bool logLine(const char *format,...) {
    char line[1024]; va_list args; va_start(args,format);
    int length=vsnprintf(line,sizeof(line),format,args); va_end(args);
    if(length<0 || length>=1024) return false;
    fake::logs.emplace_back(line); return true;
}
