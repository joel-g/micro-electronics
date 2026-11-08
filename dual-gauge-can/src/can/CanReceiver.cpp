#include "can_receiver.h"
#include "diagnostics.h"
#include <cstring>
void CanReceiver::begin() {
    lastAttempt=millis();
    if(handle || running) return;
    twai_timing_config_t timing{};
    switch(bitrate) {
        case 125000:timing=TWAI_TIMING_CONFIG_125KBITS();break;
        case 250000:timing=TWAI_TIMING_CONFIG_250KBITS();break;
        case 500000:timing=TWAI_TIMING_CONFIG_500KBITS();break;
        case 1000000:timing=TWAI_TIMING_CONFIG_1MBITS();break;
        default:lastError=ESP_ERR_INVALID_ARG;logLine("CAN INIT unsupported bitrate=%lu\n",(unsigned long)bitrate);return;
    }
    twai_general_config_t general=TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_5,GPIO_NUM_6,acknowledgments?TWAI_MODE_NORMAL:TWAI_MODE_LISTEN_ONLY);
    general.tx_queue_len=0; general.rx_queue_len=128;
    general.alerts_enabled=TWAI_ALERT_BUS_ERROR|TWAI_ALERT_RX_QUEUE_FULL|TWAI_ALERT_RX_FIFO_OVERRUN|
        TWAI_ALERT_ERR_PASS|TWAI_ALERT_BUS_OFF|TWAI_ALERT_ABOVE_ERR_WARN;
    twai_filter_config_t filter=TWAI_FILTER_CONFIG_ACCEPT_ALL();
    lastError=twai_driver_install_v2(&general,&timing,&filter,&handle);
    if(lastError==ESP_OK) {
        lastError=twai_start_v2(handle);
        if(lastError==ESP_OK) running=true;
        else {twai_driver_uninstall_v2(handle);handle=nullptr;}
    }
    logLine("CAN INIT mode=%s tx=5 rx=6 bitrate=%lu ack_enabled=%d running=%d result=%s; application_tx=0\n",
            modeName,(unsigned long)bitrate,acknowledgments,running,esp_err_to_name(lastError));
}
void CanReceiver::observe(const CanFrame &f) {
    if(f.id==LinkGenericDashDecoder::canId && f.extended==instrumentConfig::canExtended && !f.rtr && f.dlc==8 && f.data[1]==0 && f.data[0]<16) {
        auto &sample=mux[f.data[0]];
        memcpy(sample.bytes,f.data,8); ++sample.count; sample.at=millis();
    }
    uint32_t key=f.id|(f.extended?0x80000000u:0);
    bool known=false;
    for(uint32_t i=0;i<idCount;++i) if(ids[i]==key) {known=true;break;}
    if(!known) {
        if(idCount<128) ids[idCount++]=key;
        else ++idOverflowFrames;
    }
    if(frames>1 && millis()-lastPrint<200) {++logSkipped;return;}
    lastPrint=millis();
    char bytes[25]{};
    if(!f.rtr) for(unsigned i=0;i<f.dlc;++i) snprintf(bytes+i*3,sizeof(bytes)-i*3,"%02X ",f.data[i]);
    if(!logLine("CAN ID: 0x%lX FORMAT: %s RTR: %d DLC: %u DATA: %s\n",
        (unsigned long)f.id,f.extended?"EXT":"STD",f.rtr,f.dlc,bytes)) ++logSkipped;
}
void CanReceiver::service(EcuDecoder &decoder,engine::Data &data) {
    if(!running) {if(millis()-lastAttempt>=5000) begin();return;}
    uint32_t started=micros(), alerts=0;
    auto result=twai_read_alerts_v2(handle,&alerts,0);
    if(result==ESP_OK) pendingAlerts|=alerts;
    else if(result!=ESP_ERR_TIMEOUT) {lastError=result;++errors;}
    if(alerts&TWAI_ALERT_BUS_OFF) {
        // Stop and retry after five seconds in the explicitly selected build mode.
        // Recovery never upgrades a listen-only build to acknowledging mode.
        twai_stop_v2(handle); twai_driver_uninstall_v2(handle); handle=nullptr;
        running=false; lastAttempt=millis(); ++errors; return;
    }
    for(unsigned n=0;n<64 && micros()-started<750;++n) {
        twai_message_t message{};
        result=twai_receive_v2(handle,&message,0);
        if(result==ESP_ERR_TIMEOUT) break;
        if(result!=ESP_OK) {lastError=result;++errors;break;}
        if(message.data_length_code>8) {++errors;continue;}
        CanFrame frame; frame.id=message.identifier;frame.extended=message.extd;
        frame.rtr=message.rtr;frame.dlc=message.data_length_code;
        if(!frame.rtr) memcpy(frame.data,message.data,frame.dlc);
        ++frames;lastFrameMs=millis();
        decoder.onFrame(frame,lastFrameMs,data); observe(frame);
    }
    if(listing<idCount && millis()-lastIdPrint>=200) {
        lastIdPrint=millis();auto key=ids[listing];
        if(logLine("CAN OBSERVED index=%lu id=0x%lX format=%s\n",(unsigned long)listing,
            (unsigned long)(key&0x1FFFFFFFu),key&0x80000000u?"EXT":"STD")) ++listing;
    }
    // Explicit j snapshots cover every multiplexor without 20 Hz/5 Hz sampling alias.
    if(dumpMux<16 && millis()-lastMuxPrint>=200) {
        while(dumpMux<16 && !mux[dumpMux].count) ++dumpMux;
        if(dumpMux<16) {
            const auto &sample=mux[dumpMux];
            char bytes[25]{};
            for(unsigned i=0;i<8;++i) snprintf(bytes+i*3,sizeof(bytes)-i*3,"%02X ",sample.bytes[i]);
            lastMuxPrint=millis();
            if(logLine("CAN SNAPSHOT id=0x%lX mux=%u count=%lu age_ms=%lu DATA: %s\n",
                (unsigned long)LinkGenericDashDecoder::canId,dumpMux,(unsigned long)sample.count,(unsigned long)(millis()-sample.at),bytes)) ++dumpMux;
        }
    }
    uint32_t elapsed=micros()-started;if(elapsed>maxServiceUs) maxServiceUs=elapsed;
}
void CanReceiver::report() {
    uint32_t now=millis(), elapsed=now-previousReport;
    float fps=elapsed ? (frames-previousFrames)*1000.0f/elapsed : 0;
    previousReport=now;previousFrames=frames;
    twai_status_info_t status{};
    esp_err_t statusResult=handle?twai_get_status_info_v2(handle,&status):ESP_ERR_INVALID_STATE;
    if(statusResult!=ESP_OK && handle) {lastError=statusResult;++errors;}
    logLine("CAN STATUS running=%d mode=%s bitrate=%lu rx=%lu fps=%.1f ids=%lu ids_capacity=128 id_overflow_frames=%lu last_ms=%ld alerts=0x%lX driver_err=%lu last_error=%s state=%d rxq=%lu missed=%lu overrun=%lu bus_errors=%lu tec=%lu rec=%lu logged_skipped=%lu max_us=%lu decoder=LINK_GENERIC_DASH\n",
        running,modeName,(unsigned long)bitrate,(unsigned long)frames,fps,(unsigned long)idCount,
        (unsigned long)idOverflowFrames,frames?long(uint32_t(now-lastFrameMs)):-1L,
        (unsigned long)pendingAlerts,(unsigned long)errors,esp_err_to_name(lastError),statusResult==ESP_OK?int(status.state):-1,
        (unsigned long)status.msgs_to_rx,(unsigned long)status.rx_missed_count,(unsigned long)status.rx_overrun_count,
        (unsigned long)status.bus_error_count,(unsigned long)status.tx_error_counter,(unsigned long)status.rx_error_counter,
        (unsigned long)logSkipped,(unsigned long)maxServiceUs);
    pendingAlerts=0;
}
