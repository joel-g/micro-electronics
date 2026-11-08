#include "can_receiver.h"
#include "diagnostics.h"
#include <cassert>
#include <cstdio>
int main() {
    CanReceiver can;LinkGenericDashDecoder decoder;engine::Data data;
    fake::failInstall=true;can.begin();assert(!can.ready());
    can.service(decoder,data);assert(fake::installs==1);
    fake::timeUs=5000000;fake::failInstall=false;can.service(decoder,data);assert(can.ready());assert(fake::installedBitrate==int(CanReceiver::bitrate));assert(fake::installedMode==(CanReceiver::acknowledgments?TWAI_MODE_NORMAL:TWAI_MODE_LISTEN_ONLY));
    can.service(decoder,data);assert(can.frames==0 && can.errors==0);
    for(unsigned i=0;i<160;++i) {twai_message_t m;m.identifier=i;m.data_length_code=8;fake::rx.push_back(m);}
    can.service(decoder,data);assert(can.frames==64 && fake::rx.size()==96);
    can.service(decoder,data);can.service(decoder,data);can.service(decoder,data);
    assert(can.frames==160 && can.logSkipped==159);
    for(auto &c:data.channels) assert(!c.received);
    twai_message_t malformed;malformed.data_length_code=9;fake::rx.push_back(malformed);
    can.service(decoder,data);assert(can.frames==160 && can.errors==1);
    fake::alerts=TWAI_ALERT_BUS_ERROR;can.service(decoder,data);assert(can.pendingAlerts&TWAI_ALERT_BUS_ERROR);
    can.report();assert(can.pendingAlerts==0);
    fake::alerts=TWAI_ALERT_BUS_OFF;can.service(decoder,data);assert(!can.ready());
    fake::timeUs+=5000000;can.service(decoder,data);assert(can.ready());assert(fake::installedBitrate==int(CanReceiver::bitrate));assert(fake::installedMode==(CanReceiver::acknowledgments?TWAI_MODE_NORMAL:TWAI_MODE_LISTEN_ONLY));
    // Slow receives exercise elapsed-time budget independently of frame limit.
    for(unsigned i=0;i<20;++i) {twai_message_t m;m.identifier=2;fake::rx.push_back(m);}
    auto before=can.frames;fake::receiveCostUs=100;
    can.service(decoder,data);assert(can.frames-before>0 && can.frames-before<=8);
    assert(!fake::rx.empty());fake::rx.clear();fake::receiveCostUs=0;
    twai_message_t mux;mux.identifier=LinkGenericDashDecoder::canId;mux.extd=instrumentConfig::canExtended;mux.data_length_code=8;mux.data[0]=3;
    fake::rx.push_back(mux);can.service(decoder,data);can.listIds();fake::timeUs+=200000;
    can.service(decoder,data);bool snapshot=false;
    char expected[100];snprintf(expected,sizeof(expected),"CAN SNAPSHOT id=0x%lX mux=3 count=1",(unsigned long)LinkGenericDashDecoder::canId);
    for(const auto &line:fake::logs) if(line.find(expected)!=std::string::npos) snapshot=true;
    assert(snapshot);
    puts("PASS: explicit CAN mode/zero TX queue, retries preserve mode, frame/time budgets, sampled logs, mux snapshots, malformed DLC and alerts");
}
