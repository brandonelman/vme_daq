#ifndef V1729_HH
#define V1729_HH 1
#ifndef LINUX
#define LINUX 1
#endif

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include<sys/types.h>
#include<sys/stat.h>

#ifdef WIN32
    #include <time.h>
    #include <sys/timeb.h>
    #include <conio.h>
    #include <process.h>
#else
    #include <unistd.h>
    #include <sys/time.h>
    #define Sleep(t)  usleep((t)*1000)
#endif

#include "CAENVMElib.h"

// Register addresses
#define V1729_RESET_BOARD 			0x0800
#define V1729_START_ACQUISITION		0x1700
#define V1729_TRIGGER_TYPE 			0x1D00
#define V1729_FP_FREQUENCY 			0x8100
#define V1729_INTERRUPT				0x8000
#define V1729_LOAD_TRIGGER_THS		0x0900
#define V1729_THRESHOLD      		0x0A00
#define V1729_SOFTWARE_TRIGGER		0x1C00
#define V1729_RAM_DATA_VME			0x0D00
#define V1729_RAM_INT_ADD_LSB		0x0E00
#define V1729_RAM_INT_ADD_MSB		0x0F00
#define V1729_PRETRIG_LSB			0x1800
#define V1729_PRETRIG_MSB			0x1900
#define V1729_POSTTRIG_LSB			0x1A00
#define V1729_POSTTRIG_MSB			0x1B00
#define V1729_TRIGGER_CHANNEL_SRC	0x1E00
#define V1729_TRIG_REC				0x2000
#define V1729_FAST_READ_MODES		0x2100
#define V1729_NB_OF_COLS_TO_READ	0x2200
#define V1729_CHANNEL_MASK			0x2300
#define V1729_FPGA_VERSION			0x8200
#define V1729_EN_VME_IRQ			0x8300
#define V1729_EN_SYNC_OUT			0x1100
#define V1729_MODE_REGISTER         0x0300
#define V1729_NUMBER_OF_CHANNELS    0x3400 
#define V1729                       0x3
#define V1729A                      0xF

#define CAENBASEADDRESS  0x30010000
#define V1729_RAM_DEPH 10252
#define V1729_NOS 2560
#define V1729_VERNIER_DEPH 65536
#define MAX_STRING_LENGTH 50

#include "CAENVMElib.h"
/************************************
GLOBAL VARIABLES
************************************/

extern int32_t handle; //Handle linked to Crate Controller for use in 
                       //CAEN functions.
                      
extern uint32_t vme_data; //Data read over VME Bus when read functions
                          //are called.

typedef struct Config{
  uint32_t trigger_channel_src;
  uint32_t num_channels_per_pulse;
  uint32_t trigger_type;
  int trigger_threshold_mv;
  uint32_t num_pulses;
  uint32_t mode_register;
  uint32_t fp_frequency;
  uint32_t nb_of_cols_to_read;
  uint32_t channel_mask;
  uint32_t pretrig_lsb;
  uint32_t pretrig_msb;
  uint32_t posttrig_lsb;
  uint32_t posttrig_msb;
  uint32_t run_num;
  char mode[MAX_STRING_LENGTH];
  char tag[MAX_STRING_LENGTH];
  char output_folder[MAX_STRING_LENGTH];
  char pmt_serials[4][MAX_STRING_LENGTH];
  uint32_t pmt_voltages[6];
  uint32_t lamp_voltage;
  uint32_t lamp_frequency;
} Config;

/*Function Declarations*/

CVErrorCodes write_to_vme(uint32_t vme_addr, uint32_t data_to_write);
/* Function: write_to_vme
 ----------------------
Purpose: Wrapper function for CAENVME_WriteCycle. Allows you to perform
         a single write cycle of data_to_write at the register vme_addr. 

Parameters: 
  vme_addr: integer register address to write data to (see register 
            addresses above)
  data_to_write: integer data to be written to register address

returns: Error code defined in CAENVMEtypes.h
*/


CVErrorCodes read_from_vme(uint32_t vme_addr); 
/* Function: read_from_vme
 ----------------------
Purpose: Wrapper function for CAENVME_ReadCycle. Allows you to perform
         a single read cycle at the register vme_addr which is then 
         stored in the vme_data variable

Parameters: 
  vme_addr: integer register address to read contents of (see register 
            addresses above)
   
returns: Error code defined in CAENVMEtypes.h
*/

CVErrorCodes reset_vme(void);
/* Function: reset_vme
 ----------------------
Purpose: Send RESET command over vme. This does not change the values
         currently stored in the registers of the board, but rather
         resets the the current 'state' of the board
   
returns: Error code defined in CAENVMEtypes.h
*/

CVErrorCodes start_acq(void);
/* Function: start_acq
 ----------------------
Purpose: Changes V1729A board state to acquisition.

returns: Error code defined in CAENVMEtypes.h
*/

CVErrorCodes read_vme_ram(unsigned int buffer32[V1729_RAM_DEPH/2]);
/* Function: read_vme_ram
 ----------------------
Purpose: Read the RAM of the ADC by realizing N successive readings of 
         the RAM register.

Parameters:
  buffer32: 32-bit buffer to store data from RAM 

returns: Error code defined in CAENVMEtypes.h
*/

CVErrorCodes vernier(unsigned int MAXVER[4], unsigned int MINVER[4]);
/* Function: vernier
 ----------------------
Purpose: Performs temporal interpolator (vernier) calibration. It is 
         necessary after any changes in the sampling frequency. This
         is the fastest method mentioned in the V1729A manual. 

Parameters:
  MAXVER: 1/pilot_frequency (see V1729A manual for better descriptions)
  MINVER: Zero of the vernier

returns: Error code defined in CAENVMEtypes.h
*/


void wait_for_interrupt(void); 
/* Function: wait_for_interrupt
----------------------
Purpose: Standard way of discovering interrupt by scanning interrupt 
         register. Runs until the INTERRUPT register contains a value
         of 1. 
*/


int get_pedestals(int pedestals[V1729_RAM_DEPH],                            
                  unsigned int buffer32[V1729_RAM_DEPH/2], 
                  unsigned int buffer16[V1729_RAM_DEPH], 
                  float mean_pedestal[4]);
/* Function: get_pedestals
----------------------
Purpose: Removes variance in the pedestals of the channels so that, when 
         no signal is attached, the values stored in the RAM do not greatly
         fluctuate. Does this by doing some simple acquisition runs with 
         no signal attached to the boards and finding the average of the 
         readouts. 

Parameters: 
  pedestals: stores data to be subtracted later from every readout.
  buffer32:  stores 32-bit data from the RAM which is converted to 16-bit
             using mask_buffer.
  buffer16:  stores 16-bit data after conversion from 32-bit
  mean_pedestal: mean value of pedestals for each channel found
                 by summing the pedestal values.

returns:  1 if successful
          0 if failure
*/

void mask_buffer(unsigned int buffer32[V1729_RAM_DEPH/2], 
                unsigned int buffer16[V1729_RAM_DEPH]);
/* Function: mask_buffer
 ----------------------
Purpose: Converts data in 32-bit buffer into its 16-bit form. 

Parameters:
  buffer32: Stores 32-bit data 
  buffer16: stores 16-bit data 

*/

void reorder(unsigned int trig_rec, unsigned int post_trig, 
            unsigned int MINVER[4], unsigned int MAXVER[4], 
            unsigned int buffer16[V1729_RAM_DEPH], unsigned short ch0[2560],
            unsigned short ch1[2560], unsigned short ch2[2560], 
            unsigned short ch3[2560]);
/* Function: reorder
 ----------------------
Purpose: Performs temporal reordering operation described in manual. Unrolls 
         the circular buffer into four new arrays based on the channel number.
Parameters:
  trig_rec: Used along with post_trig to determine correct indices. Described
            thoroughly in V1729A manual.
  post_trig: See trig_rec
  buffer16: 16-bit buffer storing data from the RAM 
  MINVER, MAXVER: Store temporal calibration data
  ch0,ch1,ch2,ch3: store reordered data
*/

void save_config(Config *config, FILE *file);
/* Function: save_config
 ----------------------
Purpose: Saves DAQ configuration parameters and hardware settings for 
         future reference. The resulting file can be used as the 
         configuration file for future runs.
        
Parameters:
  config: See Config struct above. Contains all parameters and hardware
          settings. 
  file: File to save config information to. 
*/
void save_data(unsigned short ch0[2560], unsigned short ch1[2560], 
              unsigned short ch2[2560], unsigned short ch3[2560], 
              Config * config, FILE *file); 
/* Function: save_data
 ----------------------
Purpose: Saves DAQ data output to file. Note that there are numerous different
         methods of formatting the file based on both the num_channels_per_pulse
         and mode options in the config struct.
        
Parameters:
  ch0, ch1, ch2, ch3: data to be saved
  config: See Config struct above. Contains all parameters and hardware
          settings. 
  file: File to save data to. 
*/
#endif

