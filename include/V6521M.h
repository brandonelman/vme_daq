#ifndef V6521M_HH
#define V6521M_HH 1

#ifndef LINUX
#define LINUX
#endif
#define V6521M_VMAX 0x0050
#define V6521M_IMAX 0x0054
#define V6521M_STATUS 0x0058
#define V6521M_FWREL 0x005C

#define V6521M_VSET_CH0 0x0080
#define V6521M_ISET_CH0 0x0084
#define V6521M_VMON_CH0 0x0088
#define V6521M_IMONH_CH0 0x008C
#define V6521M_PW_CH0 0x0090
#define V6521M_CHSTATUS_CH0 0x0094
#define V6521M_TRIP_TIME_CH0 0x0098
#define V6521M_SVMAX_CH0 0x009C
#define V6521M_RAMP_DOWN_CH0 0x00A0
#define V6521M_RAMP_UP_CH0 0x00A4
#define V6521M_PWDOWN_CH0 0x00A8
#define V6521M_POLARITY_CH0 0x00AC
#define V6521M_TEMPERATURE_CH0 0x00B0
#define V6521M_IMON_RANGE_CH0 0x00B4
#define V6521M_IMONL_CH0 0x00B8

#define V6521M_VSET_CH1 0x0100
#define V6521M_ISET_CH1 0x0104
#define V6521M_VMON_CH1 0x0108
#define V6521M_IMONH_CH1 0x010C
#define V6521M_PW_CH1 0x0110
#define V6521M_CHSTATUS_CH1 0x0114
#define V6521M_TRIP_TIME_CH1 0x0118
#define V6521M_SVMAX_CH1 0x011C
#define V6521M_RAMP_DOWN_CH1 0x0120
#define V6521M_RAMP_UP_CH1 0x0124
#define V6521M_PWDOWN_CH1 0x0128
#define V6521M_POLARITY_CH1 0x012C
#define V6521M_TEMPERATURE_CH1 0x0130
#define V6521M_IMON_RANGE_CH1 0x0134
#define V6521M_IMONL_CH1 0x0138

#define V6521M_VSET_CH2 0x0180
#define V6521M_ISET_CH2 0x0184
#define V6521M_VMON_CH2 0x0188
#define V6521M_IMONH_CH2 0x018C
#define V6521M_PW_CH2 0x0190
#define V6521M_CHSTATUS_CH2 0x0194
#define V6521M_TRIP_TIME_CH2 0x0198
#define V6521M_SVMAX_CH2 0x019C
#define V6521M_RAMP_DOWN_CH2 0x01A0
#define V6521M_RAMP_UP_CH2 0x01A4
#define V6521M_PWDOWN_CH2 0x01A8
#define V6521M_POLARITY_CH2 0x01AC
#define V6521M_TEMPERATURE_CH2 0x01B0
#define V6521M_IMON_RANGE_CH2 0x01B4
#define V6521M_IMONL_CH2 0x01B8

#define V6521M_VSET_CH3 0x0200
#define V6521M_ISET_CH3 0x0204
#define V6521M_VMON_CH3 0x0208
#define V6521M_IMONH_CH3 0x020C
#define V6521M_PW_CH3 0x0210
#define V6521M_CHSTATUS_CH3 0x0214
#define V6521M_TRIP_TIME_CH3 0x0218
#define V6521M_SVMAX_CH3 0x021C
#define V6521M_RAMP_DOWN_CH3 0x0220
#define V6521M_RAMP_UP_CH3 0x0224
#define V6521M_PWDOWN_CH3 0x0228
#define V6521M_POLARITY_CH3 0x022C
#define V6521M_TEMPERATURE_CH3 0x0230
#define V6521M_IMON_RANGE_CH3 0x0234
#define V6521M_IMONL_CH3 0x0238

#define V6521M_VSET_CH4 0x0280
#define V6521M_ISET_CH4 0x0284
#define V6521M_VMON_CH4 0x0288
#define V6521M_IMONH_CH4 0x028C
#define V6521M_PW_CH4 0x0290
#define V6521M_CHSTATUS_CH4 0x0294
#define V6521M_TRIP_TIME_CH4 0x0298
#define V6521M_SVMAX_CH4 0x029C
#define V6521M_RAMP_DOWN_CH4 0x02A0
#define V6521M_RAMP_UP_CH4 0x02A4
#define V6521M_PWDOWN_CH4 0x02A8
#define V6521M_POLARITY_CH4 0x02AC
#define V6521M_TEMPERATURE_CH4 0x02B0
#define V6521M_IMON_RANGE_CH4 0x02B4
#define V6521M_IMONL_CH4 0x02B8

#define V6521M_VSET_CH5 0x0300
#define V6521M_ISET_CH5 0x0304
#define V6521M_VMON_CH5 0x0308
#define V6521M_IMONH_CH5 0x030C
#define V6521M_PW_CH5 0x0310
#define V6521M_CHSTATUS_CH5 0x0314
#define V6521M_TRIP_TIME_CH5 0x0318
#define V6521M_SVMAX_CH5 0x031C
#define V6521M_RAMP_DOWN_CH5 0x0320
#define V6521M_RAMP_UP_CH5 0x0324
#define V6521M_PWDOWN_CH5 0x0328
#define V6521M_POLARITY_CH5 0x032C
#define V6521M_TEMPERATURE_CH5 0x0330
#define V6521M_IMON_RANGE_CH5 0x0334
#define V6521M_IMONL_CH5 0x0338

#define V6521M_CHNUM 0x8100
#define V6521M_SERNUM 0x811E
#define V6521M_VME_FWREL 0x8120

#define V6521M_BASE_ADDRESS 0x32100000
#define CURRENT_RESOLUTION 0.005 /* 5 nA */
#define VOLTAGE_RESOLUTION 0.1 /* 0.1 V */
#define TIME_RESOLUTION 0.1 /* 0.1 s */
#define NUM_CHANNELS 6
#define MAX_VOLTAGE 2500
#define RAMP_SPEED 200

#include <sys/types.h>
#include <stdint.h>
#include "CAENVMElib.h"

extern uint32_t hv_data;
extern int32_t handle;

CVErrorCodes write_to_v6521m(uint32_t vme_addr, uint32_t data);
CVErrorCodes read_from_v6521m(uint32_t vme_addr);
CVErrorCodes set_svmax(uint32_t ch_num, uint32_t vmax);
CVErrorCodes set_current(uint32_t ch_num, uint32_t current);
CVErrorCodes set_voltage(uint32_t ch_num, uint32_t voltage);
CVErrorCodes set_ramp_up(uint32_t ch_num, uint32_t volts_per_sec);
CVErrorCodes set_ramp_down(uint32_t ch_num, uint32_t volts_per_sec);
CVErrorCodes set_trip_time(uint32_t ch_num, uint32_t trip_time);
int get_current(uint32_t ch_num);
int get_voltage(uint32_t ch_num);
CVErrorCodes enable_channel(uint32_t ch_num);
CVErrorCodes disable_channel(uint32_t ch_num);
CVErrorCodes kill_channel(uint32_t ch_num);
CVErrorCodes ramp_down_channel(uint32_t ch_num);
#endif /*V6521M_HH*/



