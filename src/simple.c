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

/* 
This function performs the temporal interpolator (vernier) calibration. It is necessary after any changes in the sampling frequency, and is valid for weeks. It's similar to a regular acquisition sequence, except TRIG_REC need not be read. This is the slowest, but most precise calibration method. Could implement the faster method easily if desirable.  
*/
int vernier(int32_t handle, unsigned short addr_mode, 
             CVDataWidth data_size, unsigned int MAXVER[4],
             unsigned int MINVER[4])
{
  uint32_t vme_addr;
  uint32_t vme_data;
  CVErrorCodes ret;
  printf("Attempting to reset board.\n");
  vme_addr = CAENBASEADDRESS + V1729_RESET_BOARD;
  vme_data = 1;
  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 

  if (ret != cvSuccess)
  {
    printf("Reset failed with error %d \n", ret);
  }  

  else printf("Reset succesfully\n");

  vme_addr = CAENBASEADDRESS + V1729_START_ACQUISITION;
  vme_data = 1;
  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
 
  if (ret != cvSuccess)
  {
    printf("Sending Start Acquisition signal failed with error %d\n", ret);
  }  

  else printf("Sending Start Acquisition signal succesful\n");

  printf("Waiting for interrupt from V1729A...");
  unsigned int timeout_counter =  0; 
  vme_addr = CAENBASEADDRESS + V1729_INTERRUPT;
  vme_data = 0;

  while(vme_data == 0)
  {
    timeout_counter++;
    ret = CAENVME_ReadCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
    vme_data = vme_data & 1;

    if(timeout_counter > 0x1fff)
    {
    printf("Wait for interrupt has timed out.\n");
    CAENVME_End(handle);
    return 0;
    }
  }

  //I don't completely understand everything that's going on here.
  //This part is mainly a copy+paste of the CAEN Demo version of this 
  //function.
  int i;
  for (i = 0; i < 4; i++)
  {
    MAXVER[i] = 0;
    MINVER[i] = 0xffff;
  } 

  for (i = 0; i < 1000; i++)
  {
    vme_addr = CAENBASEADDRESS + V1729_RAM_DATA_VME;
    if( (vme_data>>16) < MINVER[3] ) MINVER[3] = (vme_data>>16);
    if( (vme_data>>16) > MAXVER[3] ) MAXVER[3] = (vme_data>>16);
    if( (vme_data&0xffff) < MINVER[2] ) MINVER[2] = (vme_data&0xffff);
    if( (vme_data&0xffff) > MAXVER[2] ) MAXVER[2] = (vme_data&0xffff);
  }  
  
  return 1;
}

/*This functions gets the pedestals that will later be subtracted
  during the acquisition. trig_type is passed so as to ensure
  calling the function doesn't alter the V1729_TRIGGER_TYPE 
  register, which must be set to random software for this
  step. */
/*int get_pedestals(int32_t handle, unsigned short addr_mode,
                  CVDataWidth data_size, uint32_t trig_type,
                  int pedestals[V1729_RAM_DEPH]) 
{

  int i,j,k; //dummy variables for counting
  uint32_t vme_data;
  uint32_t vme_addr;
  float meanpedestal[4];

  meanpedestal[0] = 0;
  meanpedestal[1] = 0;
  meanpedestal[2] = 0;
  meanpedestal[3] = 0;
 
  printf("Setting trigger to random software for finding pedestals...\n"); 
  vme_addr = CAENBASEADDRESS + V1729_TRIGGER_TYPE;
  vme_data = 0x8; //trigger randmo sofware
  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 

  if (ret != cvSuccess)
  {
    printf("Setting trigger type failed with error: %d \n", ret);
  }  

  else printf("Set Trigger Type successful\n");

  for (i = 0; i < 10252; i++) pedestals[i] = 0 //Initialize pedestal array

  //Doing 50 acquisition runs to get pedestals.
  for (i = 0; i < 50; i++)
  {
    //Send start acquisition signal  
    vme_addr = CAENBASEADDRESS + V1729_START_ACQUISITION;
    vme_data = 1;
    ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
   
    if (ret != cvSuccess)
    {
      printf("Sending Start Acquisition signal failed with error %d\n", ret);
    }  

    else printf("Sending Start Acquisition signal succesful\n");

    //Wait for Interrupt
    printf("Waiting for interrupt from V1729A...");
    unsigned int timeout_counter =  0; 
    vme_addr = CAENBASEADDRESS + V1729_INTERRUPT;
    vme_data = 0;

    while(vme_data == 0)
    {
      timeout_counter++;
      ret = CAENVME_ReadCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
      vme_data = vme_data & 1;

      if(timeout_counter > 0x1fff)
      {
      printf("Wait for interrupt has timed out.\n");
      CAENVME_End(handle);
      return 0;
      }
    }

    //Read VME Ram
    printf("Attempting to read vme ram");
    vme_addr = CAENBASEADDRESS + V1729_RAM_DATA_VME; 
    int count; //number of bytes transferred
    ret = CAENVME_BLTReadCycle(handle, vme_addr, buffer, , addr_mode,
                               data_size,  
  }
} 
*/
 


int main(int argc, void *argv[])
{
  CVBoardTypes vme_board = cvV2718; //Type of Board
  short link = 0; //Link number 
  short device = 0; //Device Number
  int32_t handle; //Handle used for interacting with board
  CVErrorCodes ret; //Stores Error Codes for Debugging
  uint32_t vme_addr; //Address to Access VME Registers
  uint32_t vme_data; //Data holder for writing and reading
  
  uint32_t trig_lev = 0x6ff; //trigger level 
  uint32_t active_channel; //active channel on the frontend of board
  uint32_t trig_type; //type of trigger (software, auto, external, etc)
  uint32_t num_columns; //number of columns to read from MATACQ matrix
  uint32_t post_trig; //post trigger value
  int pedestals[V1729_RAM_DEPH];
  unsigned int MAXVER[4], MINVER[4]; /*Correspond to 
                                     1/pilot_frequency and the 
                                     zero of the vernier, 
                                     respectively. Need help
                                     understanding these.*/

  /* Parameters */
  unsigned short addr_mode = cvA32_U_DATA;
  //unsigned short num_cycles = 1; -> multiple read cycles not implemented yet
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
  
  printf("Attempting to perform Vernier calibration");

  if (vernier(handle, addr_mode, data_size,
              MAXVER,  MINVER) != 1)
  {
    printf("Failed vernier calibration. \n");
    return 0;
  }

  else printf("Successful vernier calibration!");

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
 
  if (ret != cvSuccess)
  {
    printf("Sending Start Acquisition signal failed with error %d\n", ret);
  }  

  else printf("Sending Start Acquisition signal succesful\n");

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

  //Wait for Interrupt from V1729A
  printf("Waiting for interrupt from V1729A...");
  unsigned int timeout_counter =  0; 
  vme_addr = CAENBASEADDRESS + V1729_INTERRUPT;
  vme_data = 0;
  unsigned int interrupt;

  while(interrupt == 0)
  {
    
    timeout_counter++;
    ret = CAENVME_ReadCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
    interrupt = vme_data & 1;

    if(timeout_counter > 0x1fff)
    {
    printf("Wait for interrupt has timed out.\n");
    CAENVME_End(handle);
    return 0;
    }
  }
  //After receiving interrupt must acknowledge
  //by writing 0 in interrupt register.
  printf("Interrupt found. Attempting to acknowledge..\n");
  vme_data = 0;
  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
  if (ret != cvSuccess)
  {
    printf("Interrupt Acknowledge failed with error %d\n", ret);
  }  

  else printf("Interrupt Acknowledged succesfully\n");


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
