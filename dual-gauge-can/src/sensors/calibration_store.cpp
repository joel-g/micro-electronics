#include "calibration_store.h"
#include <Preferences.h>
#include <cstddef>

namespace {
Preferences preferences;
// Same versioned record as the source projects; button auto-save or serial 's'.
struct Record {
    uint32_t magic, version;
    float x, y, z;
    uint32_t checksum;
};
static_assert(sizeof(Record) == 24, "Calibration record layout changed");
uint32_t checksum(const Record &record) {
    const auto *data = reinterpret_cast<const uint8_t *>(&record);
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < offsetof(Record, checksum); ++i) hash = (hash ^ data[i]) * 16777619u;
    return hash;
}
bool validOffset(compass::Vector offset) {
    return std::isfinite(offset.x) && std::isfinite(offset.y) && std::isfinite(offset.z) &&
           std::fabs(offset.x) < 30000 && std::fabs(offset.y) < 30000 && std::fabs(offset.z) < 30000;
}
}

bool CalibrationStore::begin() {
    available = preferences.begin("qmc5883p", false);
    return available;
}
bool CalibrationStore::load(compass::Vector &offset) {
    if (!available || preferences.getBytesLength("hardiron") != sizeof(Record)) return false;
    Record record{};
    if (preferences.getBytes("hardiron", &record, sizeof(record)) != sizeof(record) ||
        record.magic != 0x514D4350 || record.version != 1 || record.checksum != checksum(record)) return false;
    compass::Vector result{record.x, record.y, record.z};
    if (!validOffset(result)) return false;
    offset = result;
    return true;
}
bool CalibrationStore::save(compass::Vector offset) {
    if (!available || !validOffset(offset)) return false;
    Record record{0x514D4350, 1, offset.x, offset.y, offset.z, 0};
    record.checksum = checksum(record);
    if (preferences.putBytes("hardiron", &record, sizeof(record)) != sizeof(record)) return false;
    compass::Vector verified;
    return load(verified) && verified.x == offset.x && verified.y == offset.y && verified.z == offset.z;
}
bool CalibrationStore::clear() {
    return available && (!preferences.isKey("hardiron") || preferences.remove("hardiron"));
}
