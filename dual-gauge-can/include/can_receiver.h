#pragma once
#include <Arduino.h>
#include <driver/twai.h>
#include "ecu_decoder.h"
#include <instrument_config.h>
class CanReceiver {
public:
    void begin();
    void service(EcuDecoder &,engine::Data &);
    void report();
    void listIds() { listing=0; dumpMux=0; }
    bool ready() const { return running; }
    uint32_t frames=0, errors=0, logSkipped=0, idOverflowFrames=0, maxServiceUs=0;
    uint32_t lastFrameMs=0, pendingAlerts=0;
    static constexpr uint32_t bitrate=instrumentConfig::canBitrate;
    static constexpr bool acknowledgments=instrumentConfig::canAck;
    static constexpr const char *modeName=acknowledgments?"NORMAL_RX":"LISTEN_ONLY";
private:
    twai_handle_t handle=nullptr;
    bool running=false;
    uint32_t ids[128]{}, idCount=0, listing=128;
    uint32_t lastAttempt=0, lastPrint=0, lastIdPrint=0;
    uint32_t previousFrames=0, previousReport=0;
    esp_err_t lastError=ESP_OK;
    struct MuxSample { uint8_t bytes[8]{}; uint32_t count=0, at=0; };
    MuxSample mux[16]{};
    unsigned dumpMux=16;
    uint32_t lastMuxPrint=0;
    void observe(const CanFrame &);
};
