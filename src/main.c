#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>

#include <math.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "CAENVMElib.h"
#include "V1729.h"

int main(int argc, void *argv[])
{
  CVBoardTypes vme_board = cvV2718;
  short link = 0;
  short device = 0;
  static cur_status status;

  int32_t handle;
  unsigned int r ;
  unsigned int tp ;  // trigger code 
  unsigned int active_channel = 0xf; ///< channels mask
  unsigned int tl;
  unsigned int fc=1;
  int buffer;

  unsigned int t01=0;
  unsigned int t02=0;
  unsigned int t03=0;
  unsigned int t04=0;
  unsigned int t05=0;
 
 
  //-------------Initial Parameters---------   
  status.addr = 0xEE000000;
  status.level = 1;
  status.am = cvA32_U_DATA;
  status.dtsize = cvD32;
  status.base_addr = 0xEE000000;
  status.blts = 256;
  status.num_cyc = 1;
  status.autoinc = 0;
  status.buff = (uint32_t *)malloc(16*1024*1024);

  if (status.buff == NULL)
  {
    printf(" Error! Can't allocate memory for BLT buffer.");
    exit(1);
  }
  //-----------------------------------------


  /* Create Handle for VX2718 Board */
  if ( CAENVME_Init(vme_board, link, device, &handle) != cvSuccess ) 
  {
    printf("\n\n Error opening V2718\n");
    exit(1);
  }

  else printf("Successfully opened V2718\n");

  /* Set Address to Base Address of V1729A Board */

  status.base_addr = 0x30010000;

  /* Send Initial Reset to Boad */
  printf("Sending reset order to board...\n");
  reset(handle, &status);

  printf("Setting trigger level...\n");
  tl = 0x6ff; //Trigger level
  status.addr = status.base_addr + V1729_THRESHOLD;
  status.data = tl;
  write_vme(handle, &status);

  status.addr = status.base_addr + V1729_LOAD_TRIGGER_THS;
  status.data = 1;
  write_vme(handle, &status);

  printf("Setting pilot frequency...\n");
  status.addr = status.base_addr + V1729_FP_FREQUENCY;
  read_vme(handle, &status);

  status.data = status.data & 0x3f;
  if((status.data&0x3f) == 0x01)fc=1;
  if((status.data&0x3f) == 0x02)fc=2;
  if((status.data&0x3f) == 0x04)fc=3;
  if((status.data&0x3f) == 0x05)fc=4;
  if((status.data&0x3f) == 10)fc=5;
  if((status.data&0x3f) == 20)fc=6;
  if((status.data&0x3f) == 40)fc=7; 

  printf("Reading mode register ...\n");
  status.addr = status.base_addr + V1729_MODE_REGISTER;
  read_vme(handle, &status);
  r = (status.data&2)>1;

  printf("Reading number of columns  ...\n");
  status.addr  = status.base_addr + V1729_NB_OF_COLS_TO_READ;
  read_vme(handle, &status); 
  num_cols = status.data & 0xff;

  printf("Reading Trigger Type  ...\n");
  status.addr  = status.base_addr + V1729_TRIGGER_TYPE;
  read_vme(handle, &status); 
  tp  = status.data & 0x3f;

  t01 = tp&3;
  t02 =(tp&4)>>2;
  t03 =(tp&8)>>3;
  t04 =(tp&0x10)>>4;
  t05 =(tp&0x20)>>5;

  printf("Finding active channel ...\n");
  status.addr  = status.base_addr + V1729_CHANNEL_MASK;
  read_vme(handle, &status); 
  active_channel  = status.data & 0x3f;

  printf("Reading POSTTRIG....\n");
  status.addr  = status.base_addr + V1729_POSTTRIG_LSB;
  read_vme(handle, &status); 
  post_trig  = status.data & 0x3f;

  status.addr  = status.base_addr + V1729_POSTTRIG_MSB;
  read_vme(handle, &status); 
  post_trig  = post_trig +  (status.data & 0xFF)*256;

  printf("Setting Software Trigger....\n");
  status.data = 0x00000001;
  status.addr  = status.base_addr + V1729_SOFTWARE_TRIGGER;
  write_vme(handle, &status);

  /* Vernier Calibration */
  vernier(handle, &status);

  /* Pedestal Correction */
  pedestal(handle, &status);

  /* Start Acquisition */
  start_acq(handle, &status);

  return 0;
}

 

