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

#define V1729                       0x3
#define V1729A                      0xF



#define CAENBASEADDRESS  0x30010000
#define V1729_RAM_DEPH 10252
#define V1729_NOS 2560
#define V1729_VERNIER_DEPH 65536


#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "CAENVMElib.h"
#include <termios.h>
#include <fcntl.h>

/*This is an attempt to rewrite the previous main.c file
  with no extra methods aside from kbhit; for debugging purposes */

int kbhit(void)
{
   struct termios oldt, newt;
   int ch;
   int oldf;

   tcgetattr(STDIN_FILENO, &oldt);
   newt = oldt;
   newt.c_lflag &= ~(ICANON | ECHO);
   tcsetattr(STDIN_FILENO, TCSANOW, &newt);
   oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
   fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

   ch = getchar();

   tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
   fcntl(STDIN_FILENO, F_SETFL, oldf);

   if(ch != EOF)
   {
      ungetc(ch, stdin);
      return 1;
   }

   return 0;
}


int main(int argc, void *argv[])
{
  CVBoardTypes vme_board = cvV2718; //Type of Board
  short link = 0; //Link number 
  short device = 0; //Device Number
  int32_t handle; //Handle used for interacting with board
  CVErrorCodes ret; //Stores Error Codes for Debugging
  uint32_t vme_addr; //Address to Access VME Registers
  uint32_t vme_data; //Data holder for writing and reading
  
  uint32_t trig_lev = 0x6ff; 
  uint32_t active_channel;
  uint32_t trig_type;
  uint32_t num_columns;
  uint32_t post_trig;

  /* Parameters */
  unsigned short addr_mode = cvA32_U_DATA;
  unsigned short num_cycles = 1;
  CVDataWidth data_size = cvD32;  

  /* Create opaque handle for interacting with VX2718 Board */
  if ( CAENVME_Init(vme_board, link, device, &handle) != cvSuccess )
  {
    printf("\n\n Error opening VX2718\n");
    exit(1);
  }

  else printf("Successfully opened VX2718!\n"); 


  /* Send Reset Order to Board  */
  printf("Attempting to reset board.\n");
  vme_addr = CAENBASEADDRESS + V1729_RESET_BOARD;
  vme_data = 1;
  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
  if (ret != cvSuccess)
  {
    printf("Reset failed with error %d \n", ret);
  }  

  else printf("Reset succesfully\n");

  /* Set Trigger Threshold */
  printf("Attempting to set trigger level\n");

  vme_addr = CAENBASEADDRESS + V1729_THRESHOLD;
  vme_data = trig_lev;
  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
  if (ret != cvSuccess)
  {
    printf("Setting trigger level failed with error %d\n", ret);
  }  

  else printf("Trigger level set succesfully\n");

  /*From Manual: After loading of V1729_THRESHOLD, one must transfer the
    value in the analog converter via the LOAD_TRIGGER THRESHOLD DAC (09)
    command.*/
  printf("Attempting to enact LOAD_TRIGGER_THRESHOLD DAC\n");
  vme_addr = CAENBASEADDRESS + V1729_LOAD_TRIGGER_THS;
  vme_data = 1;
  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
  if (ret != cvSuccess)
  {
    printf("Loading trigger threshold command failed with error: %d\n", ret);
  }  

  else printf("Load Trigger threshold command successful\n");

/*  printf("Attempting to load pilot frequency"); 
  vme_addr = CAENBASEADDRESS + V1729_FP_FREQUENCY;

  ret = CAENVME_ReadCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 

  if (ret != cvSuccess)
  {
    printf("Loading pilot freq uency failed with error: %d", ret);
  }  

  else printf("Loading pilot frequency successful");
  UNNECESSARY: Pilot Frequency defaults to 2Gs/s */

  //Initializing rest of parameters....

  printf("Attempting to determine number of columns to read from MATACQ \n");
  vme_addr = CAENBASEADDRESS + V1729_NB_OF_COLS_TO_READ;
  ret = CAENVME_ReadCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
  if (ret != cvSuccess)
  {
    printf("Loading num. of columns failed with error: %d \n", ret);
  }  

  else printf("Load number of columns successful\n");

  num_columns = vme_data & 0xff; /*0xff is 11111111 in binary. 
                                 Default value is 128 so
                                 128&0xff = 128, all the columns */

  printf("Attempting to determine trigger type...\n");
  vme_addr = CAENBASEADDRESS + V1729_TRIGGER_TYPE;
  ret = CAENVME_ReadCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
  if (ret != cvSuccess)
  {
    printf("Loading trigger type failed with error: %d \n", ret);
  }  

  else printf("Load Trigger Type successful\n");

  trig_type = vme_data & 0x3f; 

  printf("Attempting to determine active channel...\n");
  vme_addr = CAENBASEADDRESS + V1729_CHANNEL_MASK;
  ret = CAENVME_ReadCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
  if (ret != cvSuccess)
  {
    printf("Loading active channel failed with error: %d \n", ret);
  }  

  else printf("Load active channel successful\n");

  active_channel = vme_data & 0x3f; 

  /*Determining the POSTTRIG requires reading two registers:
    POSTTRIG_MSB and POSTTRIG_LSB. The value must be read for
    use in data reordering later. */
  printf("Attempting to determine POSTTRIG...\n");
  vme_addr = CAENBASEADDRESS + V1729_POSTTRIG_LSB;
  ret = CAENVME_ReadCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 

  if (ret != cvSuccess)
  {
    printf("Loading POSTTRIG_LSB failed with error: %d \n", ret);
  }  

  else printf("Load POSTTRIG_LSB successful\n");

  post_trig = vme_data & 0x3f; 

  vme_addr = CAENBASEADDRESS + V1729_POSTTRIG_MSB;
  ret = CAENVME_ReadCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 

  if (ret != cvSuccess)
  {
    printf("Loading POSTTRIG_MSB failed with error: %d \n", ret);
  }  

  else printf("Load POSTTRIG_MSB successful\n");

  post_trig = post_trig + (vme_data & 0xFF)*256; 

  //Send start acquisition signal  
  vme_addr = CAENBASEADDRESS + V1729_START_ACQUISITION;
  vme_data = 1;
  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
 
  //Send Software Trigger after waiting PRETRIG <- Not sure how to wait
  printf("Sending Software Trigger...\n"); 
  vme_data = 1;
  vme_addr = CAENBASEADDRESS + V1729_SOFTWARE_TRIGGER;

  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 

  if (ret != cvSuccess)
  {
    printf("Sending Software Trigger failed with error %d\n", ret);
  }  

  else printf("Sending Software Trigger succesful\n");

  //Send Reset to End Acquisition
  printf("Attempting to reset board.\n");
  vme_addr = CAENBASEADDRESS + V1729_RESET_BOARD;
  vme_data = 1;
  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
  if (ret != cvSuccess)
  {
    printf("Reset failed with error %d\n", ret);
  }  

  else printf("Reset succesfully\n");

  ret = CAENVME_End(handle);  
  if (ret != cvSuccess)
  {
    printf("Closing board failed with error %d\n", ret);
  }  

  else printf("Closed Board succesfully\n");

 
}
