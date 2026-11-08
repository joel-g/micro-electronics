#pragma once
#include <map>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
namespace fake {
inline std::map<std::string,std::vector<uint8_t>> nvs;
inline bool failWrites=false;
inline unsigned writeAttempts=0;
}
class Preferences {
public:
    void end() {}
    bool begin(const char *name,bool) { prefix=std::string(name)+"/"; return true; }
    uint8_t getUChar(const char *key,uint8_t fallback) {
        auto it=fake::nvs.find(prefix+key);
        return it!=fake::nvs.end() && it->second.size()==1 ? it->second[0] : fallback;
    }
    size_t putUChar(const char *key,uint8_t value) { return putBytes(key,&value,1); }
    size_t getBytesLength(const char *key) {
        auto it=fake::nvs.find(prefix+key); return it==fake::nvs.end() ? 0 : it->second.size();
    }
    size_t getBytes(const char *key,void *data,size_t length) {
        auto it=fake::nvs.find(prefix+key);
        if(it==fake::nvs.end() || it->second.size()!=length) return 0;
        memcpy(data,it->second.data(),length); return length;
    }
    size_t putBytes(const char *key,const void *data,size_t length) {
        ++fake::writeAttempts;
        if(fake::failWrites) return 0;
        auto *bytes=static_cast<const uint8_t *>(data);
        fake::nvs[prefix+key]={bytes,bytes+length}; return length;
    }
    bool isKey(const char *key) { return fake::nvs.count(prefix+key); }
    bool remove(const char *key) { return fake::nvs.erase(prefix+key)!=0; }
private:
    std::string prefix;
};
