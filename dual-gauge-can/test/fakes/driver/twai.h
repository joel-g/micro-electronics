#pragma once
#include "Arduino.h"
#include <deque>
using esp_err_t=int;using gpio_num_t=int;using twai_handle_t=void*;
constexpr int ESP_OK=0,ESP_ERR_TIMEOUT=1,ESP_ERR_INVALID_STATE=2,ESP_ERR_INVALID_ARG=3;
constexpr int GPIO_NUM_5=5,GPIO_NUM_6=6,TWAI_MODE_LISTEN_ONLY=7,TWAI_MODE_NORMAL=8;
constexpr uint32_t TWAI_ALERT_BUS_ERROR=1,TWAI_ALERT_RX_QUEUE_FULL=2,TWAI_ALERT_RX_FIFO_OVERRUN=4,TWAI_ALERT_ERR_PASS=8,TWAI_ALERT_BUS_OFF=16,TWAI_ALERT_ABOVE_ERR_WARN=32;
struct twai_timing_config_t {int bitrate;};
#define TWAI_TIMING_CONFIG_125KBITS() {125000}
#define TWAI_TIMING_CONFIG_250KBITS() {250000}
#define TWAI_TIMING_CONFIG_500KBITS() {500000}
#define TWAI_TIMING_CONFIG_1MBITS() {1000000}
struct twai_general_config_t {int tx,rx,mode;unsigned tx_queue_len=5,rx_queue_len=5,alerts_enabled=0;};
#define TWAI_GENERAL_CONFIG_DEFAULT(tx,rx,mode) {tx,rx,mode}
struct twai_filter_config_t {bool all;};
#define TWAI_FILTER_CONFIG_ACCEPT_ALL() {true}
struct twai_message_t {uint32_t identifier=0;uint8_t data_length_code=0,data[8]{};bool extd=false,rtr=false;};
struct twai_status_info_t {int state=1;unsigned msgs_to_rx=0,rx_missed_count=0,rx_overrun_count=0,bus_error_count=0,tx_error_counter=0,rx_error_counter=0;};
namespace fake {
inline std::deque<twai_message_t> rx;
inline unsigned installs=0,starts=0,receives=0,alerts=0;
inline bool failInstall=false;
inline unsigned receiveCostUs=0;
inline int installedMode=-1, installedBitrate=0;
}
inline const char *esp_err_to_name(int e) {return e==ESP_OK?"ESP_OK":"ERROR";}
inline int twai_driver_install_v2(const twai_general_config_t *g,const twai_timing_config_t *t,const twai_filter_config_t *f,void **handle) {
    ++fake::installs;fake::installedMode=g->mode;fake::installedBitrate=t->bitrate;assert(g->tx==5 && g->rx==6);
    assert(g->tx_queue_len==0 && g->rx_queue_len==128 && f->all);
    if(fake::failInstall) return ESP_ERR_INVALID_STATE;
    *handle=reinterpret_cast<void*>(1);return ESP_OK;
}
inline int twai_start_v2(void*) {++fake::starts;return ESP_OK;}
inline int twai_stop_v2(void*) {return ESP_OK;}
inline int twai_driver_uninstall_v2(void*) {return ESP_OK;}
inline int twai_read_alerts_v2(void*,uint32_t *out,int timeout) {assert(timeout==0);*out=fake::alerts;fake::alerts=0;return *out?ESP_OK:ESP_ERR_TIMEOUT;}
inline int twai_receive_v2(void*,twai_message_t *out,int timeout) {
    assert(timeout==0);++fake::receives;fake::timeUs+=fake::receiveCostUs;if(fake::rx.empty()) return ESP_ERR_TIMEOUT;
    *out=fake::rx.front();fake::rx.pop_front();return ESP_OK;
}
inline int twai_get_status_info_v2(void*,twai_status_info_t *s) {*s={};s->msgs_to_rx=fake::rx.size();return ESP_OK;}
// Deliberately no transmit function: production code cannot link one in this test.
