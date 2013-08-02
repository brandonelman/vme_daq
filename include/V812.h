#ifndef V812_HH
#define V812_HH 1 

#include <stdio.h>
#include <string.h>
#include "V1729.h" /* Handle is global */
#include "CAENVMElib.h"

/*Register Addresses */
#define V812_THRESHOLD_CH0  0x00
#define V812_THRESHOLD_CH1  0x02
#define V812_THRESHOLD_CH2  0x04
#define V812_THRESHOLD_CH3  0x06
#define V812_THRESHOLD_CH4  0x08
#define V812_THRESHOLD_CH5  0x0A
#define V812_THRESHOLD_CH6  0x0C
#define V812_THRESHOLD_CH7  0x0E
#define V812_THRESHOLD_CH8  0x10
#define V812_THRESHOLD_CH9  0x12
#define V812_THRESHOLD_CH10 0x14
#define V812_THRESHOLD_CH11 0x16
#define V812_THRESHOLD_CH12 0x18
#define V812_THRESHOLD_CH13 0x1A
#define V812_THRESHOLD_CH14 0x1C
#define V812_THRESHOLD_CH15 0x1E
#define V812_OUTPUT_WIDTH_1 0x40
#define V812_OUTPUT_WIDTH_2 0x42
#define V812_DEAD_TIME_1 0x44
#define V812_DEAD_TIME_2 0x46
#define V812_MAJORITY_THRESHOLD 0x48
#define V812_PATTERN_INHIBIT 0x4A
#define V812_TEST_PULSE 0x4C
#define V812_FIXED_CODE 0xFA
#define V812_MODULE_TYPE 0xFC
#define V812_VERSION_AND_SERIAL 0xFE

#define V812_BASE_ADDRESS 0xEE000000 

#ifndef LINUX
#define LINUX
#endif

/***************************************************
                Function Declarations
***************************************************/

CVErrorCodes write_to_v812(uint32_t, uint32_t);
CVErrorCodes set_channel_threshold(int ch_num, int threshold);

/* CH_SECTION means 1 or 2. Choose 1 for channels 0-7, and 2 for 8-15 */
/* See manual for count value correspondence to time */
CVErrorCodes set_output_width(int ch_section, int set_count);
CVErrorCodes set_dead_time(int ch_section, int set_count);
/* NOTE: Actual Dead Time is the greater of the set output width and
   dead time values */

/* MAJTHR = NINT[(MAJLEV*50 - 25)/4] where NINT is NEAREST INTEGER */
CVErrorCodes set_majority_threshold(int maj_lev);
CVErrorCodes send_test_pulse(void);

#endif

