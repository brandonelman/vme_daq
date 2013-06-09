/* V1729.C : source code for V1729 module */
/* created 14.10.2009 by Franco.LARI     */

#define __LINUX__
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>


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
#include "V1729.h"
#include <termios.h>
#include <fcntl.h> 

// ############################################################################ 

CVErrorCodes write_to_vme(uint32_t vme_addr, uint32_t * vme_data)                  
{
  vme_addr = CAENBASEADDRESS + vme_addr;
  return CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
}

CVErrorCodes read_from__vme(uint32_t vme_addr, uint32_t *vme_data)                  
{
  vme_addr = CAENBASEADDRESS + vme_addr;
  return CAENVME_ReadCycle(handle, vme_addr, &vme_data, addr_mode, data_size);
}

CVErrorCodes reset_vme()
{
  return write_to_vme(V1729_RESET_BOARD, 1); 
}

CVErrorCodes start_acq()
{
  return write_to_vme(V1729_START_ACQUISITION, 1); 
}

int wait_for_interrupt(void)
{
  unsigned int timeout_counter =  0; //timeout counter for waiting on interrupt 
  unsigned int interrupt = 0; //when 1 an interrupt has successfully been read

  while(interrupt == 0)
  {
    timeout_counter++;
    ret = read_from_vme(V1729_INTERRUPT, &interrupt); 

    if(timeout_counter > 0x1ffff)
    {
    printf(" Wait for interrupt has timed out.\n");
    return 0;
    }
  }
  
  return 1;

}
/* 
This function performs the temporal interpolator (vernier) calibration. It is necessary after any changes in the sampling frequency, and is valid for weeks. It's similar to a regular acquisition sequence, except TRIG_REC need not be read. This is the slowest, but most precise calibration method. Could implement the faster method easily if desirable.

UPDATE: Can't seem to find interrupt with more precise method, so attemptign to use faster method instead. */


int vernier(unsigned int MAXVER[4], unsigned int MINVER[4])
{
  CVErrorCodes ret; //stores error codes for debugging
  uint32_t old_cols; //stores previous value for resetting at end

  printf("    Attempting to reset board.\n");
  ret = reset_vme();

  if(ret != cvSuccess)
  {
    printf("    Reset failed with error %d \n", ret);
    return 0;
  }  
  else printf("    Reset succesfully\n");


  printf("    Setting trigger to random software ...\n"); 
  ret = write_to_vme(V1729_TRIGGER_TYPE, 0x8); 
  
  if (ret != cvSuccess)
  {
    printf("     Setting trigger type failed with error: %d \n", ret);
    return 0;
  }  

  else printf("    Set Trigger Type successful\n");

  printf("    Setting mask to allow 4 Channels ...\n"); 
  ret = write_to_vme(V1729_CHANNEL_MASK, 15);

  if (ret != cvSuccess)
  {
    printf("    Setting channel mask failed with error: %d \n", ret);
    return 0;
  }  

  else printf("    Set channel mask successfully\n");

  printf("    Saving number of columns for resetting later..\n");
  ret = read_from_vme(V1729_NB_OF_COLS_TO_READ, old_cols);
 
  if (ret != cvSuccess)
  {
    printf("    Loading num. of columns failed with error: %d \n", ret);
    return 0;
  }  

  else printf("    Load number of columns successful\n");

  old_cols = old_cols&0xff;


  //As specified in manual, 0 Columns are used for fast calibration
  ret = write_to_vme(V1729_NB_OF_COLS_TO_READ, 0); 

  if (ret != cvSuccess)
  {
    printf("    Changing num. of columns failed with error: %d \n", ret);
    return 0;
  }  

  else printf("    Changing number of columns successful\n");

  //Begins acquisition sequence
  printf("    Attempting to begin acquisition sequence...\n");
  ret = start_acq();
 
  if (ret != cvSuccess)
  {
    printf("    Sending Start Acquisition signal failed with error %d\n", ret);
    return 0;
  }  

  else printf("    Sending Start Acquisition signal succesful\n");

  printf("    Waiting for interrupt from V1729A...");
  vme_addr = CAENBASEADDRESS + V1729_INTERRUPT;

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
