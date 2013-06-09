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

/*This is an attempt to rewrite the previous main.c file.
  Please note that no signal should be attached to the board
  when first running this, because pedestals must be found before
  the signal is attached. */

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

UPDATE: Can't seem to find interrupt with more precise method, so attemptign to use faster method instead. */


int vernier(int32_t handle, unsigned short addr_mode, 
             CVDataWidth data_size, unsigned int MAXVER[4],
             unsigned int MINVER[4])
{
  uint32_t vme_addr;
  uint32_t vme_data;
  CVErrorCodes ret;
  printf("    Attempting to reset board.\n");
  vme_addr = CAENBASEADDRESS + V1729_RESET_BOARD;
  vme_data = 1;
  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 

  if (ret != cvSuccess)
  {
    printf("    Reset failed with error %d \n", ret);
  }  

  else printf("    Reset succesfully\n");

  printf("    Setting trigger to random software ...\n"); 
  vme_addr = CAENBASEADDRESS + V1729_TRIGGER_TYPE;
  vme_data = 0x8; //trigger random sofware
  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
  
  if (ret != cvSuccess)
  {
    printf("     Setting trigger type failed with error: %d \n", ret);
    
    return 0;
  }  

  else printf("    Set Trigger Type successful\n");

  printf("    Setting mask to allow 4 Channels ...\n"); 
  vme_addr = CAENBASEADDRESS + V1729_CHANNEL_MASK;
  vme_data = 15; //trigger random sofware
  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 

  if (ret != cvSuccess)
  {
    printf("    Setting channel mask failed with error: %d \n", ret);
    return 0;
  }  

  else printf("    Set channel mask successfully\n");

  printf("    Saving number of columns for resetting later..\n");
  vme_addr = CAENBASEADDRESS + V1729_NB_OF_COLS_TO_READ;
  ret = CAENVME_ReadCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
  if (ret != cvSuccess)
  {
    printf("    Loading num. of columns failed with error: %d \n", ret);
    return 0;
  }  

  else printf("    Load number of columns successful\n");

  uint32_t cols = vme_data&0xff;
  vme_data = 0; //Sets # of cols to 0 for fast calibration
  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 

  if (ret != cvSuccess)
  {
    printf("    Changing num. of columns failed with error: %d \n", ret);
    return 0;
  }  

  else printf("    Changing number of columns successful\n");

  //Begins acquisition sequence
  printf("    Attempting to begin acquisition sequence...\n");
  vme_addr = CAENBASEADDRESS + V1729_START_ACQUISITION;
  vme_data = 1;
  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
   
  if (ret != cvSuccess)
  {
    printf("    Sending Start Acquisition signal failed with error %d\n", ret);
    return 0;
  }  

  else printf("    Sending Start Acquisition signal succesful\n");

  printf("    Waiting for interrupt from V1729A...");
  unsigned int timeout_counter =  0; 
  vme_addr = CAENBASEADDRESS + V1729_INTERRUPT;
  unsigned int interrupt = 0;

  while(interrupt == 0)
  {
    timeout_counter++;
    ret = CAENVME_ReadCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
    interrupt = vme_data & 1;

    if(timeout_counter > 0x1ffff)
    {
    printf(" Wait for interrupt has timed out.\n");
    //Added reset here so board isn't stuck in acquisition mode after failing. 
    vme_addr = CAENBASEADDRESS + V1729_RESET_BOARD;
    vme_data = 1;
    ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
    CAENVME_End(handle);
    return 0;
    }
  }

  printf("Resetting columns to read to initial value\n");
  vme_addr = CAENBASEADDRESS + V1729_NB_OF_COLS_TO_READ;
  vme_data = cols; //Sets # of cols to 0 for fast calibration
  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 

  if (ret != cvSuccess)
  {
    printf("    Resetting num. of columns failed with error: %d \n", ret);
    return 0;
  }  

  else printf("    Resetting number of columns successful\n");

  //I don't completely understand everything that's going on here.
  //This part is mainly a copy+paste from the CAEN Demo version of this 
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
int get_pedestals(int32_t handle, unsigned short addr_mode,
                  CVDataWidth data_size, uint32_t trig_type,
                  int pedestals[V1729_RAM_DEPH], 
                  unsigned int buffer32[V1729_RAM_DEPH/2],
                  unsigned int buffer16[V1729_RAM_DEPH]) 
{

  int i,j,k; //dummy variables for counting
  int ch; //Channels
  uint32_t vme_data;
  uint32_t vme_addr;
  float meanpedestal[4]; //Keeps track of mean pedestal values
  int count; //Stores number of bytes transferred from RAM
  CVErrorCodes ret;
  //Initialize array
  meanpedestal[0] = 0;
  meanpedestal[1] = 0;
  meanpedestal[2] = 0;
  meanpedestal[3] = 0;
 
  printf("    Setting trigger to random software for finding pedestals...\n"); 
  vme_addr = CAENBASEADDRESS + V1729_TRIGGER_TYPE;
  vme_data = 0x8; //trigger random sofware
  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
  
  if (ret != cvSuccess)
  {
    printf("     Setting trigger type failed with error: %d \n", ret);
    
    return 0;
  }  

  else printf("    Set Trigger Type successful\n");

  printf("    Setting mask to allow 4 Channels for finding pedestals...\n"); 
  vme_addr = CAENBASEADDRESS + V1729_CHANNEL_MASK;
  vme_data = 15; //trigger random sofware
  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
  if (ret != cvSuccess)
  {
    printf("    Setting channel mask failed with error: %d \n", ret);
    return 0;
  }  

  else printf("    Set channel mask successfully\n");

  for (i = 0; i < 10252; i++) pedestals[i] = 0; //Initialize pedestal array

  //Doing 50 acquisition runs to get pedestals.
  for (i = 0; i < 50; i++)
  {
    //Send start acquisition signal  
    vme_addr = CAENBASEADDRESS + V1729_START_ACQUISITION;
    vme_data = 1;
    ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
   
    if (ret != cvSuccess)
    {
      printf("    Sending Start Acquisition signal failed with error %d\n", ret);

      vme_addr = CAENBASEADDRESS + V1729_RESET_BOARD;
      vme_data = 1;
      ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
      return 0;
    }  

    else printf("    Sending Start Acquisition signal succesful\n");

    //Wait for Interrupt
    printf("    Waiting for interrupt from V1729A...");
    unsigned int timeout_counter =  0; 
    vme_addr = CAENBASEADDRESS + V1729_INTERRUPT;
    unsigned int interrupt = 0;

    while(interrupt == 0)
    {
      timeout_counter++;
      ret = CAENVME_ReadCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
      interrupt = vme_data & 1;

      if(timeout_counter > 0x1ffff)
      {
      printf("    Wait for interrupt has timed out.\n");
      vme_addr = CAENBASEADDRESS + V1729_RESET_BOARD;
      vme_data = 1;
      ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
      CAENVME_End(handle);
      return 0;
      }
    }

    //Read VME Ram
    printf("    Attempting to read vme ram");
    vme_addr = CAENBASEADDRESS + V1729_RAM_DATA_VME; 
    int count; //number of bytes transferred
    ret = CAENVME_BLTReadCycle(handle, vme_addr, buffer32, V1729_RAM_DEPH/2, 
                               addr_mode, data_size, &count);  

    if (ret != cvSuccess)
    {
      printf("    Reading VME RAM failed with error %d\n", ret);
      vme_addr = CAENBASEADDRESS + V1729_RESET_BOARD;
      vme_data = 1;
      ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
      return 0;
    }  

    else printf("    Reading VME RAM succesful\n");

    //Mask Buffer
    int c; //New dummy variable for counting 
    int mask;

    vme_addr = CAENBASEADDRESS + V1729_MODE_REGISTER;
    ret = CAENVME_ReadCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
    if (ret != cvSuccess)
    {
      printf("     Reading Channel Mask failed with error %d\n", ret);
      vme_addr = CAENBASEADDRESS + V1729_RESET_BOARD;
      vme_data = 1;
      ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
      return 0;
    }  

    else printf("    Reading Channel Mask succesful\n");

    if( (vme_data & 0x2) == 2) mask = 0x3fff; //14 bit
   
    else mask = 0xfff; //12 bit 

    for (c = 0; c < V1729_RAM_DEPH; c = c + 2) 
    {
      buffer16[c+1] = mask & buffer32[c/2];
      buffer16[c] = mask & (buffer32[c/2]>>16);
    }

    //Find Pedestals
    for (j = 0; j < 2560; j ++)
      for (ch = 0; ch < 4; ch ++)
        pedestals[j*4 + ch] = pedestals[j*4 + ch] + buffer16[12 + j*4 + ch]; 
  }

  for (j = 0; j < 2560; j++)
    for (ch = 0; ch < 4; ch ++)
    {
      pedestals[j*4 + ch] = pedestals[j*4 + ch] / (50);
      meanpedestal[ch] = meanpedestal[ch] + pedestals[j*4 + ch]; }

  for (ch =4; ch ,4; ch++) meanpedestal[ch] = meanpedestal[ch]/2560;

  for (k = 0; k < 2560; k ++)
    for (ch = 0; ch < 4; ch ++)
      pedestals[k*4 + ch] = pedestals[k*4 + ch] - (int)meanpedestal[ch]; 

  //Set trig_type back to original value;
  printf("    Resetting trigger to original value after finding pedestals...\n"); 
  vme_addr = CAENBASEADDRESS + V1729_TRIGGER_TYPE;
  vme_data = trig_type; //trigger random sofware
  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
  if (ret != cvSuccess)
  {
    printf("    Resetting trigger type failed with error: %d \n", ret);
    vme_addr = CAENBASEADDRESS + V1729_RESET_BOARD;
    vme_data = 1;
    ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
    return 0;
  }  

  else printf("Resetting Trigger Type successful\n");
  return 1;
} 

/* Reorders data as described by equations in manual */
int reorder(unsigned int trig_rec, unsigned int post_trig, uint32_t num_columns, 
            unsigned int MINVER[4], unsigned int MAXVER[4], unsigned int buffer16[V1729_RAM_DEPH/2], 
            unsigned short ch0[2560],unsigned short ch1[2560], unsigned short ch2[2560], 
            unsigned short ch3[2560])
{
  int i, j;
  int end_cell;
  int new_num_col;
  int cor_ver = 0;
  float ver[4];

  for (i = 0; i < 4; i++)
  {
    ver[i] = (float)(buffer16[1 + 3 - i] - MINVER[i])/(float)(MAXVER[i]-MINVER[i]);
    cor_ver = cor_ver + (int)(20*ver[i]/4);
  }

  end_cell = (20 * (128 - (trig_rec) + (post_trig) + 1) % 2560);
  new_num_col = num_columns * 20;
  for (i =0; i < new_num_col; i++)
  {
    j = (new_num_col + i + end_cell - cor_ver+20) % new_num_col;
    ch3[i] = buffer16[4*j+12];
    ch2[i] = buffer16[4*j+13]; 
    ch1[i] = buffer16[4*j+14];
    ch0[i] = buffer16[4*j+15]; 
  }

  return 1;
}

int save(int32_t handle, unsigned short addr_mode, CVDataWidth data_size, uint32_t num_columns, 
         unsigned short ch0[2560], unsigned short ch1[2560], unsigned short ch2[2560], unsigned short ch3[2560])
{
  FILE *ch[4];
  int channel_mask;
  int i;
  char s[30];
  int new_num_cols;
  uint32_t vme_addr;
  uint32_t vme_data;
  CVErrorCodes ret;
 
  vme_addr = CAENBASEADDRESS + V1729_CHANNEL_MASK; 
  ret = CAENVME_ReadCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 

  if (ret != cvSuccess)
  {
    printf("Reading Channel Mask failed with error %d\n", ret);
    return 0;
  }  

  else printf("Reading Channel Mask succesful\n");

  channel_mask = vme_data & 0xf;

  if(channel_mask & 0x1)
  {
    ch[0] = fopen("Ch_0.dat", "w+b");
    for (i = 40; i < new_num_cols; i++)
    {
      sprintf(s, "%d\n", ch0[i]);
      fwrite(s, 1, strlen(s), ch[0]);
    }
  
   fclose(ch[0]);
  }
  
  if(channel_mask & 0x2)
  {
    ch[1] = fopen("Ch_1.dat", "w+b");
    for (i = 40; i < new_num_cols; i++)
    {
      sprintf(s, "%d\n", ch1[i]);
      fwrite(s, 1, strlen(s), ch[1]);
    }

    fclose(ch[1]);
  }

  if(channel_mask & 0x4)
  {
    ch[2] = fopen("Ch_2.dat", "w+b");
    for (i = 40; i < new_num_cols; i++)
    {
      sprintf(s, "%d\n", ch2[i]);
      fwrite(s, 1, strlen(s), ch[2]);
    }

    fclose(ch[2]);
  }
  if(channel_mask & 0x8)
  {
    ch[3] = fopen("Ch_3.dat", "w+b");
    for (i = 40; i < new_num_cols; i++)
    {
      sprintf(s, "%d\n", ch3[i]);
      fwrite(s, 1, strlen(s), ch[3]);
    }
  
    fclose(ch[3]);
  }
  
  return 1;
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

  uint32_t trig_lev = 0x6ff; //trigger level 
  uint32_t active_channel; //active channel on the frontend of board
  uint32_t trig_type; //type of trigger (software, auto, external, etc)
  uint32_t num_columns; //number of columns to read from MATACQ matrix
  uint32_t post_trig; //post trigger value

  int count; //number of bytes transferred for BLT
  int mask;
  int ch;
  int buffer;
  unsigned int buffer32[V1729_RAM_DEPH/2]; //Two buffers for storing BLT data.
  unsigned int buffer16[V1729_RAM_DEPH]; 
  int pedestals[V1729_RAM_DEPH];
  unsigned int MAXVER[4], MINVER[4]; /*Correspond to 
                                     1/pilot_frequency and the 
                                     zero of the vernier, 
                                     respectively. Need help
                                     understanding these.*/

  unsigned int trig_rec; /* TRIG_REC must be read to automatically
                            restart acquisitions. Its value helps 
                            determine the position of the trigger in 
                            the acquisition window. */

  /* These arrays store the data that will be saved to file post-reordering */
  unsigned short ch0[2560], ch1[2560], ch2[2560], ch3[2560];

  /* Parameters */
  unsigned short addr_mode = cvA32_U_DATA;
  //unsigned short num_cycles = 1; -> multiple read cycles not implemented yet
  CVDataWidth data_size = cvD32;  

  /* Create opaque handle for interacting with VX2718 Board */
  if ( CAENVME_Init(vme_board, link, device, &handle) != cvSuccess )
  {
    printf("\n\n Error opening VX2718\n");
    return 0;
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
    return 0;
  }  

  else printf("Reset succesfully\n");

  printf("Attempting to determine trigger type...\n");
  vme_addr = CAENBASEADDRESS + V1729_TRIGGER_TYPE;
  ret = CAENVME_ReadCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
  if (ret != cvSuccess)
  {
    printf("Loading trigger type failed with error: %d \n", ret);
    return 0;
  }  

  else printf("Load Trigger Type successful\n");

  trig_type = vme_data & 0x3f; 

   /* Set Trigger Threshold */
  printf("Attempting to set trigger level\n");

  vme_addr = CAENBASEADDRESS + V1729_THRESHOLD;
  vme_data = trig_lev;
  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
  if (ret != cvSuccess)
  {
    printf("Setting trigger level failed with error %d\n", ret);
    return 0;
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
    return 0; 
  }  

  else printf("Load Trigger threshold command successful\n");

  printf("Attempting to determine number of columns to read from MATACQ \n");
  vme_addr = CAENBASEADDRESS + V1729_NB_OF_COLS_TO_READ;
  ret = CAENVME_ReadCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
  if (ret != cvSuccess)
  {
    printf("Loading num. of columns failed with error: %d \n", ret);
    return 0;
  }  

  else printf("Load number of columns successful\n");

  num_columns = vme_data & 0xff; /*0xff is 11111111 in binary. 
                                 Default value is 128 so
                                 128&0xff = 128, all the columns */

  printf("Attempting to determine active channel...\n");
  vme_addr = CAENBASEADDRESS + V1729_CHANNEL_MASK;
  ret = CAENVME_ReadCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
  if (ret != cvSuccess)
  {
    printf("Loading active channel failed with error: %d \n", ret);
    return 0;
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
    return 0;
  }  

  else printf("Load POSTTRIG_LSB successful\n");

  post_trig = vme_data & 0x3f; 

  vme_addr = CAENBASEADDRESS + V1729_POSTTRIG_MSB;
  ret = CAENVME_ReadCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 

  if (ret != cvSuccess)
  {
    printf("Loading POSTTRIG_MSB failed with error: %d \n", ret);
    return 0;
  }  

  else printf("Load POSTTRIG_MSB successful\n");

  post_trig = post_trig + (vme_data & 0xFF)*256; 

  printf("Attempting to perform Vernier calibration\n");

  if (vernier(handle, addr_mode, data_size,
              MAXVER,  MINVER) != 1)
  {
    printf("Failed vernier calibration. \n");
    return 0;
  }

  else printf("Successful vernier calibration!\n");
  

  if (get_pedestals(handle, addr_mode, data_size, trig_type, pedestals, buffer32, buffer16) == 0)
  {
    printf("Failed to get pedestals.\n");
    CAENVME_End(handle);
    return 0;
  }
  else printf("Successfully found pedestals"); 

  printf("Please now attach your signal to the board. Press RETURN when ready.");
  char key = getchar();
  


  //Send start acquisition signal  
  vme_addr = CAENBASEADDRESS + V1729_START_ACQUISITION;
  vme_data = 1;
  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
 
  if (ret != cvSuccess)
  {
    printf("Sending Start Acquisition signal failed with error %d\n", ret);

    vme_addr = CAENBASEADDRESS + V1729_RESET_BOARD;
    vme_data = 1;
    ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
    return 0;
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
    vme_addr = CAENBASEADDRESS + V1729_RESET_BOARD;
    vme_data = 1;
    ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
    return 0;
  }  

  else printf("Sending Software Trigger succesful\n");

  //Wait for Interrupt from V1729A
  printf("Waiting for interrupt from V1729A...");
  unsigned int timeout_counter =  0; 
  vme_addr = CAENBASEADDRESS + V1729_INTERRUPT;
  vme_data = 0;
  unsigned int interrupt = 0;

  while(interrupt == 0)
  {
    
    timeout_counter++;
    ret = CAENVME_ReadCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
    interrupt = vme_data & 1;

    if(timeout_counter > 0x1fff)
    {
    printf("Wait for interrupt has timed out.\n");
    vme_addr = CAENBASEADDRESS + V1729_RESET_BOARD;
    vme_data = 1;
    ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
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
    return 0;
  }  

  else printf("Interrupt Acknowledged succesfully\n");

  //Read VME Ram
  printf("Attempting to read vme ram");
  vme_addr = CAENBASEADDRESS + V1729_RAM_DATA_VME; 
  ret = CAENVME_BLTReadCycle(handle, vme_addr, buffer32, V1729_RAM_DEPH/2, 
                             addr_mode, data_size, &count);  

  if (ret != cvSuccess)
  {
    printf("Reading VME RAM failed with error %d\n", ret);
    vme_addr = CAENBASEADDRESS + V1729_RESET_BOARD;
    vme_data = 1;
    ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
    return 0;
  }  

  else printf("Reading VME RAM succesful\n");

  //Read TRIG_REC after VME RAM
  vme_addr = CAENBASEADDRESS + V1729_TRIG_REC;
  ret = CAENVME_ReadCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 

  if (ret != cvSuccess)
  {
    printf("Reading TRIG_REC failed with error %d\n", ret);
    vme_addr = CAENBASEADDRESS + V1729_RESET_BOARD;
    vme_data = 1;
    ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
    return 0;
  }  

  else printf("Reading TRIG_REC succesful\n");

  trig_rec = vme_data & 0xFF;

  //Mask Buffer
  int c; //New dummy variable for counting 

  vme_addr = CAENBASEADDRESS + V1729_MODE_REGISTER;
  ret = CAENVME_ReadCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
  if (ret != cvSuccess)
  {
    printf("Reading Channel Mask failed with error %d\n", ret);
    vme_addr = CAENBASEADDRESS + V1729_RESET_BOARD;
    vme_data = 1;
    ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
    return 0;
  }  

  else printf("Reading Channel Mask succesful\n");

  if( (vme_data & 0x2) == 2) mask = 0x3fff; //14 bit
 
  else mask = 0xfff; //12 bit 

  for (c = 0; c < V1729_RAM_DEPH; c = c + 2) 
  {
    buffer16[c+1] = mask & buffer32[c/2];
    buffer16[c] = mask & (buffer32[c/2]>>16);
  }
  printf("Subtracting pedestals... \n");
  if ((active_channel == 0xf) && (num_columns == 128)) 
  {
    for (c = 0; c < 2560; c++)
      for (ch = 0; ch < 4; ch++)
      {
        buffer = (int)(0xffff & buffer16[12 + c*4 + ch]);
        
        if( (buffer - pedestals[c*4 + ch] < 0) ) buffer16[12 + c*4 + ch] = 0;

        else buffer16[12 + c*4 + ch] = (unsigned int)(buffer - pedestals[c*4 + ch]);
      }
    printf("Reordering data.\n");
    reorder(trig_rec, post_trig, num_columns, MINVER, MAXVER, buffer16, ch0, ch1, ch2, ch3);
    printf("Saving data... \n");
    save(handle, addr_mode, data_size, num_columns, ch0, ch1, ch2, ch3);
 
  }
 
  else 
  {
    printf("Mask Channel not Valid\n"); 
    vme_addr = CAENBASEADDRESS + V1729_RESET_BOARD;
    vme_data = 1;
    ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
    return 0;
  }





  //Send Reset to End Acquisition
  printf("Attempting to reset board.\n");
  vme_addr = CAENBASEADDRESS + V1729_RESET_BOARD;
  vme_data = 1;
  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
  if (ret != cvSuccess)
  {
    printf("Reset failed with error %d\n", ret);
    return 0;
  }  

  else printf("Reset succesfully\n");

  printf("Closing board post-acquisition");
  ret = CAENVME_End(handle);  
  if (ret != cvSuccess)
  {
    printf("Closing board failed with error %d\n", ret);
    return 0;
  }  

  else printf("Closed Board succesfully\n");

  return 1; 
}
