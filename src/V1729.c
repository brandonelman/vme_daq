/* V1729.C : source code for V1729 module 
   Last Updated: 7/28/2013 Brandon Elman */
#include "V1729.h"

/* write_to_vme allows you to perform a single write cycle
   of data_to_write at the register vme_addr */
CVErrorCodes write_to_vme(uint32_t vme_addr, uint32_t data_to_write) 
{
  CVDataWidth data_size = cvD32;
  unsigned short addr_mode = cvA32_U_DATA;
  vme_addr = CAENBASEADDRESS + vme_addr;
  return CAENVME_WriteCycle(handle, vme_addr, &data_to_write, addr_mode, data_size); 
}

/* read_from_vme allows you to perform a single read cycle
   at the register vme_addr which is then stored in the 
   vme_data variable */
CVErrorCodes read_from_vme(uint32_t vme_addr)                  
{
  CVDataWidth data_size = cvD32;
  unsigned short addr_mode = cvA32_U_DATA;
  vme_addr = CAENBASEADDRESS + vme_addr;
  return CAENVME_ReadCycle(handle, vme_addr, &vme_data, addr_mode, data_size);
}

/* reset_vme() sends a reset order to the board. This does 
   NOT change the values currently stored in the registers
   of the board, but rather simply resets the current state */
CVErrorCodes reset_vme()
{
  return write_to_vme(V1729_RESET_BOARD, 1); 
}

/* Places the V1729A Board in "acquisition" mode */
CVErrorCodes start_acq()
{
  return write_to_vme(V1729_START_ACQUISITION, 1); 
}

/* Mask Buffer places the data stored in buffer32 into buffer16 */
int mask_buffer(unsigned int buffer32[V1729_RAM_DEPH/2], unsigned int buffer16[V1729_RAM_DEPH])
{
  int i;
  int mask;
  CVErrorCodes ret;
  ret = read_from_vme(V1729_MODE_REGISTER); 

  if (ret != cvSuccess)
  {
    printf("     Reading MODE REGISTER failed with error %d\n", ret);
    reset_vme();
    return 0;
  }  

  if( (vme_data&0x2) == 2) 
    mask = 0x3fff; /*14 bit*/

  else mask = 0xfff; /*12 bit*/ 

  for (i = 0; i < V1729_RAM_DEPH; i=i+2) 
  {
    buffer16[i+1] = mask&buffer32[i/2];
    buffer16[i] = mask&(buffer32[i/2]>>16);

    if (buffer16[i] > 60000)
    {
      printf("Overflow! Restarting!");
      return 2;
    }

  }

  return 1;
}

/* Standard way of discovering interrupt by scanning register. More noisy than IRQ 
   method (see: wait_for_interrupt_vme); however, the other method isn't quite working 
   yet possibly due to incorrect mask choices */
int wait_for_interrupt(void)
{
  unsigned int timeout_counter =  0; /*timeout counter for waiting on interrupt*/
  unsigned int interrupt = 0; /* When 1 an interrupt has successfully been read */
                              /* When 3 there was an overflow of the buffer so your data is bad */
  CVErrorCodes ret;
  while(interrupt != 0x1)
  {
    timeout_counter++;
    ret = read_from_vme(V1729_INTERRUPT); 

    interrupt = vme_data&0x3;

    if (interrupt == 0x3 || interrupt == 0x2)
    {
      printf("Overflow detected!");
      return 2;
    }

    if(timeout_counter > 0x1fffff)
    {
    printf(" Wait for interrupt has timed out.\n");
    return 0;
    }
  }
  return 1;
}

/* Preferred method of interrupt discovery.. Not tested yet */
int wait_for_interrupt_vme(void)
{
  int vector;
  CVErrorCodes ret;
  ret = CAENVME_IRQEnable(handle, 0xf);
  if (ret != cvSuccess)
  {
    printf("Failed enabling IRQ3 with error %d", ret); 
    return 0;
  }

  ret = CAENVME_IRQWait(handle, cvIRQ3, 0xf); 
  if (ret != cvSuccess)
  {
    printf("Failed waiting for interrupt with error %d", ret); 
    return 0;
  }

  ret = CAENVME_IACKCycle(handle, cvIRQ3, &vector, cvD32);
  if (ret != cvSuccess)
  {
    printf("Failed IACKCycle with error %d", ret); 
    return 0;
  }


  return 1;
}

/* Read the RAM of the ADC by realizing N successive readings of the 
   RAM register. BLT is preferred but apparently not working */
CVErrorCodes read_vme_ram(unsigned int buffer32[V1729_RAM_DEPH/2])
{
  int i;

  for (i =0; i < V1729_RAM_DEPH/2; i++)
  {
    CVErrorCodes ret = read_from_vme(V1729_RAM_DATA_VME);
    if (ret != cvSuccess)
    {
      printf("Failed reading from RAM at iteration %d", i); 
      return ret;
    }

    buffer32[i] = vme_data; 
    if (buffer32[i] > 1900000000) /*Protects from overflow acquisitions! 
                                0xea60 = 60000*/
    {
      printf("Overflow: Buffer32[i] = %d", buffer32[i]);
      return -3;
    }   
  }

  return cvSuccess;

/* BROKEN BLT VERSION
  uint32_t vme_addr = CAENBASEADDRESS + V1729_RAM_DATA_VME; 
  CVDataWidth data_size = cvD32;
  int count; number of bytes transferred

  return  CAENVME_BLTReadCycle(handle, vme_addr, buffer32, V1729_RAM_DEPH/2, 
                               cvA32_S_BLT, data_size, &count);  
*/
}


/* This function performs the temporal interpolator (vernier) calibration. It is necessary after any changes 
   in the sampling frequency, and is valid for weeks. It's similar to a regular acquisition sequence, except 
   TRIG_REC need not be read. This is one of the faster methods, but can make it more precise if necessary. */
int vernier(unsigned int MAXVER[4], unsigned int MINVER[4])
{
  CVErrorCodes ret; 
  int i;
  uint32_t old_cols; 
  uint32_t trig_type;
  uint32_t ch_mask; 

  /* Reset the Board */
  printf("    Attempting to reset board... ");
  ret = reset_vme();
  if(ret != cvSuccess)
  {
    printf(" Reset failed with error %d \n", ret);
    return 0;
  }  
  else printf(" Reset succesfully\n");

  /* Saving value of TRIGGER_TYPE register for resetting later */
  printf("    Saving value of TRIGGER_TYPE for resetting later..");
  ret = read_from_vme(V1729_TRIGGER_TYPE);
  if (ret != cvSuccess)
  {
    printf(" Loading TRIGGER_TYPE failed with error: %d \n", ret);
    return 0;
  }  
  else printf(" Load TRIGGER_TYPE successful\n");
  trig_type = vme_data&0x3f;

  /* Saving value of CHANNEL_MASK register for resetting later */
  printf("    Saving value of CHANNEL_MASK for resetting later..");
  ret = read_from_vme(V1729_CHANNEL_MASK);
  if (ret != cvSuccess)
  {
    printf(" Loading CHANNEL_MASK failed with error: %d \n", ret);
    return 0;
  }  
  else printf(" Load CHANNEL_MASK successful\n");
  ch_mask = vme_data&0xf;


  /*Set Trigger to Random Software*/
  printf("    Setting trigger to random software ... "); 
  ret = write_to_vme(V1729_TRIGGER_TYPE, 0x8); 
  if (ret != cvSuccess)
  {
    printf(" Setting trigger type failed with error: %d \n", ret);
    return 0;
  }  
  else printf(" Set Trigger Type successful\n");

  /* Setting mask to allow 4 Channels */
  printf("    Setting mask to allow 4 Channels ... "); 
  ret = write_to_vme(V1729_CHANNEL_MASK, 15);
  if (ret != cvSuccess)
  {
    printf(" Setting channel mask failed with error: %d \n", ret);
    return 0;
  }  
  else printf(" Set channel mask successfully\n");

  /* Saving V1729_NB_OF_COLS_TO_READ's original value for resetting later*/
  printf("    Saving number of columns for resetting later..");
  ret = read_from_vme(V1729_NB_OF_COLS_TO_READ);
  if (ret != cvSuccess)
  {
    printf(" Loading num. of columns failed with error: %d \n", ret);
    return 0;
  }  
  else printf(" Load number of columns successful\n");
  old_cols = vme_data&0xff;


  /*As specified in manual, 0 Columns are used for fast calibration*/
  printf("    Setting number of columns to read to 0...");
  ret = write_to_vme(V1729_NB_OF_COLS_TO_READ, 0); 
  if (ret != cvSuccess)
  {
    printf(" Changing num. of columns failed with error: %d \n", ret);
    return 0;
  }  
  else printf(" Changing number of columns successful\n");

  /*Begins acquisition sequence*/
  printf("    Attempting to begin acquisition sequence... ");
  ret = start_acq();
  if (ret != cvSuccess)
  {
    printf(" Sending Start Acquisition signal failed with error %d\n", ret);
    return 0;
  }  
  else printf(" Sending Start Acquisition signal succesful\n");

  /* Wait for Interrupt */
  printf("    Waiting for interrupt from V1729A...");
  if(wait_for_interrupt() == 0) return 0;

  /* Initialize array values */
  for (i = 0; i < 4; i++)
  {
    MAXVER[i] = 0;
    MINVER[i] = 0xffff;
  } 

  for (i = 0; i < 1000; i++)
  {
    read_from_vme(V1729_RAM_DATA_VME); 
    if( (vme_data>>16) < MINVER[3] ) MINVER[3] = (vme_data>>16);
    if( (vme_data>>16) > MAXVER[3] ) MAXVER[3] = (vme_data>>16);
    if( (vme_data&0xffff) < MINVER[2] ) MINVER[2] = (vme_data&0xffff);
    if( (vme_data&0xffff) > MAXVER[2] ) MAXVER[2] = (vme_data&0xffff);
  }  

  for (i = 0; i < 1000; i++)
  {
    read_from_vme(V1729_RAM_DATA_VME); 
    if( (vme_data>>16) < MINVER[1] ) MINVER[1] = (vme_data>>16);
    if( (vme_data>>16) > MAXVER[1] ) MAXVER[1] = (vme_data>>16);
    if( (vme_data&0xffff) < MINVER[0] ) MINVER[0] = (vme_data&0xffff);
    if( (vme_data&0xffff) > MAXVER[0] ) MAXVER[0] = (vme_data&0xffff);
  }  

  /*Resetting NB_OF_COLS_TO_READ, CHANNEL_MASK, TRIGGER_TYPE */
  printf("    Resetting columns to read to initial value...");
  ret = write_to_vme(V1729_NB_OF_COLS_TO_READ, old_cols);
  if (ret != cvSuccess)
  {
    printf(" Resetting num. of columns failed with error: %d \n", ret);
    return 0;
  }  
  else printf(" Resetting number of columns successful\n");

  printf("    Resetting TRIGGER_TYPE to initial value...");
  ret = write_to_vme(V1729_TRIGGER_TYPE, trig_type);
  if (ret != cvSuccess)
  {
    printf(" Resetting TRIGGER_TYPE failed with error: %d \n", ret);
    return 0;
  }  
  else printf(" Resetting TRIGGER_TYPE successful\n");

  printf("    Resetting CHANNEL_MASK to initial value...");
  ret = write_to_vme(V1729_CHANNEL_MASK, ch_mask);
  if (ret != cvSuccess)
  {
    printf(" Resetting CHANNEL_MASK failed with error: %d \n", ret);
    return 0;
  }  
  else printf(" Resetting CHANNEL_MASK successful\n");
  
  return 1;
}

/*This functions gets the pedestals that will later be subtracted
  during the acquisition. This method was taken from CAEN's demo.
  Technically it removes the variance in the readings of the ADC
  with no signals attached, so that all channels are uniform. */
int get_pedestals(int pedestals[V1729_RAM_DEPH], 
                  unsigned int buffer32[V1729_RAM_DEPH/2],
                  unsigned int buffer16[V1729_RAM_DEPH]) 
{

  int i,j,k; 
  int ch; /*Channels*/
  float meanpedestal[4]; /*Keeps track of mean pedestal values*/
  uint32_t trig_type, ch_mask, old_cols; /*stores values for resetting later*/
  CVErrorCodes ret; /*Stores error codes return by CAEN functions */

  /*Initialize array*/
  meanpedestal[0] = 0;
  meanpedestal[1] = 0;
  meanpedestal[2] = 0;
  meanpedestal[3] = 0;

  /* Saving value of TRIGGER_TYPE register for resetting later */
  printf("    Saving value of TRIGGER_TYPE for resetting later..");
  ret = read_from_vme(V1729_TRIGGER_TYPE);
  if (ret != cvSuccess)
  {
    printf(" Loading TRIGGER_TYPE failed with error: %d \n", ret);
    return 0;
  }  
  else printf(" Load TRIGGER_TYPE successful\n");
  trig_type = vme_data&0x3f;

  /* Saving value of CHANNEL_MASK register for resetting later */
  printf("    Saving value of CHANNEL_MASK for resetting later..");
  ret = read_from_vme(V1729_CHANNEL_MASK);
  if (ret != cvSuccess)
  {
    printf(" Loading CHANNEL_MASK failed with error: %d \n", ret);
    return 0;
  }  
  else printf(" Load CHANNEL_MASK successful\n");
  ch_mask = vme_data&0xf;

  /* Saving V1729_NB_OF_COLS_TO_READ's original value for resetting later */
  printf("    Saving number of columns for resetting later..");
  ret = read_from_vme(V1729_NB_OF_COLS_TO_READ);
  if (ret != cvSuccess)
  {
    printf(" Loading num. of columns failed with error: %d \n", ret);
    return 0;
  }  
  else printf(" Load number of columns successful\n");
  old_cols = vme_data&0xff;


  /* 128 Columns */
  printf("    Setting number of columns to read to 128...");
  ret = write_to_vme(V1729_NB_OF_COLS_TO_READ, 128); 
  if (ret != cvSuccess)
  {
    printf(" Changing num. of columns failed with error: %d \n", ret);
    return 0;
  }  
  else printf(" Changing number of columns successful\n");


  /*Sets trigger to random software */ 
  printf("    Setting trigger to random software for finding pedestals..."); 
  ret = write_to_vme(V1729_TRIGGER_TYPE, 0x8);  
  
  if (ret != cvSuccess)
  {
    printf(" Setting trigger type failed with error: %d \n", ret);
    return 0;
  }  
  else printf(" Set Trigger Type successful\n");

  /*Sets mask to allow 4 Channels */
  printf("    Setting mask to allow 4 Channels for finding pedestals..."); 
  ret = write_to_vme(V1729_CHANNEL_MASK, 15); 
  if (ret != cvSuccess)
  {
    printf(" Setting channel mask failed with error: %d \n", ret);
    return 0;
  }  
  else printf(" Set channel mask successfully\n");

  for (i = 0; i < 10252; i++) pedestals[i] = 0; /*Initialize pedestal array*/
   
  /*Doing 50 acquisition runs to get pedestals.*/
  printf("    Starting 50 acquisition sequences to find mean pedestals\n"); 
  for (i = 0; i < 50; i++)
  { 
    /* Start Acquisition */
    ret = start_acq(); 
    if (ret != cvSuccess)
    {
      printf("        Sending Start Acquisition signal failed with error %d\n", ret);
      reset_vme();
      return 0;
    }  

    /*Wait for Interrupt*/
    if(wait_for_interrupt() == 0)
    {
      printf("    Failed waiting for interrupt!");
      reset_vme();
      CAENVME_End(handle); 
    }
    /*After receiving interrupt must acknowledge
      by writing 0 in interrupt register.
    ret = write_to_vme(V1729_INTERRUPT, 0); 
    if (ret != cvSuccess)
    {
      printf("    Interrupt Acknowledge failed with error %d\n", ret);
      return 0;
    }  
    */

    /*Read VME Ram*/
    ret = read_vme_ram(buffer32);
    if (ret != cvSuccess)
    {
      printf("        Reading VME RAM failed with error %d\n", ret);
      reset_vme();
      return 0;
    }  

    /*Mask Buffer*/
    if( mask_buffer(buffer32, buffer16) == 0 ) 
      {
      printf("        Masking the buffer has failed with error %d\n", ret);
      reset_vme();
      return 0;
      }

    /*Find Pedestals*/
    for (j = 0; j < 2560; j++)
      for (ch = 0; ch < 4; ch++)
        pedestals[j*4 + ch] = pedestals[j*4 + ch] + buffer16[12 + j*4 + ch]; 
  }

  for (j = 0; j < 2560; j++)
    for (ch = 0; ch < 4; ch++)
    {
      pedestals[j*4 + ch] = pedestals[j*4 + ch] / (50);
      meanpedestal[ch] = meanpedestal[ch] + (float)pedestals[j*4 + ch]; 
    }

  for (ch = 0; ch < 4; ch++) meanpedestal[ch] = meanpedestal[ch]/2560;

  for (k = 0; k < 2560; k++)
    for (ch = 0; ch < 4; ch++)
      pedestals[k*4 + ch] = pedestals[k*4 + ch] - (int)meanpedestal[ch]; 

  /*Resetting NB_OF_COLS_TO_READ, CHANNEL_MASK, TRIGGER_TYPE */
  printf("    Resetting columns to read to initial value...");
  ret = write_to_vme(V1729_NB_OF_COLS_TO_READ, old_cols);
  if (ret != cvSuccess)
  {
    printf(" Resetting num. of columns failed with error: %d \n", ret);
    return 0;
  }  

  else printf(" Resetting number of columns successful\n");

  printf("    Resetting TRIGGER_TYPE to initial value...");
  ret = write_to_vme(V1729_TRIGGER_TYPE, trig_type);
  if (ret != cvSuccess)
  {
    printf(" Resetting TRIGGER_TYPE failed with error: %d \n", ret);
    return 0;
  }  

  else printf(" Resetting TRIGGER_TYPE successful\n");

  printf("    Resetting CHANNEL_MASK to initial value...");
  ret = write_to_vme(V1729_CHANNEL_MASK, ch_mask);
  if (ret != cvSuccess)
  {
    printf(" Resetting CHANNEL_MASK failed with error: %d \n", ret);
    return 0;
  }  

  else printf(" Resetting CHANNEL_MASK successful\n");
 


  return 1;
} 

/* Reorders data as described by equations in manual */
int reorder(unsigned int trig_rec, unsigned int post_trig, uint32_t num_columns, 
            unsigned int MINVER[4], unsigned int MAXVER[4], unsigned int buffer16[V1729_RAM_DEPH], 
            unsigned short ch0[2560],unsigned short ch1[2560], unsigned short ch2[2560], 
            unsigned short ch3[2560])
{
  int i, j;
  int end_cell;
  int cor_ver = 0;
  float ver[4];

  for (i = 0; i < 4; i++)
  {
    ver[i] = (float)(buffer16[4-i] - MINVER[i])/(float)(MAXVER[i]-MINVER[i]);
    ;
    cor_ver = cor_ver + (int)(20*ver[i]/4);
  }
  
  /* These Formulas look wrong compared to manual, but they were used by CAEN */
  end_cell = (20 * (128 - trig_rec + post_trig) + 1) % 2560;

  for (i = 0; i < 2560; i++)
  {
    j = (2560 + i + end_cell - cor_ver+20) % 2560;
    ch3[i] = buffer16[4*j+12];
    ch2[i] = buffer16[4*j+13]; 
    ch1[i] = buffer16[4*j+14];
    ch0[i] = buffer16[4*j+15]; 
  }

  return 1;
}

/* Saves data to files Ch#.dat */
int save(unsigned short ch0[2560], unsigned short ch1[2560], 
         unsigned short ch2[2560], unsigned short ch3[2560],
         int run_num)
{
  FILE *ch[4];
  int channel_mask;
  int i;
  char s[30];
  int num_cols;
  CVErrorCodes ret;


  /* Finding number of columns to read */
  ret = read_from_vme(V1729_NB_OF_COLS_TO_READ);
  if (ret != cvSuccess)
  {
    printf(" Loading num. of columns failed with error: %d \n", ret);
    return 0;
  }  
  num_cols = vme_data&0xff;
  
  /* Finding value of channel mask */
  ret = read_from_vme(V1729_CHANNEL_MASK);
  if (ret != cvSuccess)
  {
    printf(" Loading CHANNEL_MASK failed with error: %d \n", ret);
    return 0;
  }  
  channel_mask = vme_data&0xf;

  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  char filename0[100]; 
/*  char filename1[100];
  char filename2[100];
  char filename3[100];*/

  sprintf(filename0, "analysis/%d-%d-%d_Ch_0_%d.dat", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, run_num);
/*  sprintf(filename1, "analysis/%d-%d-%d_Ch_1.dat", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);
  sprintf(filename2, "analysis/%d-%d-%d_Ch_2.dat", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);
  sprintf(filename3, "analysis/%d-%d-%d_Ch_3.dat", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);*/
  /* Saving files based on your channel mask selection */

  if(channel_mask&0x1)
  {
    ch[0] = fopen(filename0, "a+b");
    for (i = 40; i < 2560; i++)
    {
      sprintf(s, "%d\n", ch0[i]-1000); /*Subtracting 1000 to remove extra 1000
                                         added during masking phase to ensure that 
                                         the entire signal is taken into consideration */
      fwrite(s, 1, strlen(s), ch[0]);
    }
   fclose(ch[0]);
  }
/* 
  if(channel_mask&0x2)
  {
    ch[1] = fopen(filename1, "a+b");
    for (i = 40; i < 2560; i++)
    {
      sprintf(s, "%d\n", ch1[i]-1000);
      fwrite(s, 1, strlen(s), ch[1]);
    }
    fclose(ch[1]);
  }

  if(channel_mask&0x4)
  {
    ch[2] = fopen(filename2, "a+b");
    for (i = 40; i < 2560; i++)
    {
      sprintf(s, "%d\n", ch2[i]-1000);
      fwrite(s, 1, strlen(s), ch[2]);
    }
    fclose(ch[2]);
  }

  if(channel_mask&0x8)
  {
    ch[3] = fopen(filename3, "a+b");
    for (i = 40; i < 2560; i++)
    {
      sprintf(s, "%d\n", ch3[i]-1000);
      fwrite(s, 1, strlen(s), ch[3]);
    }
    fclose(ch[3]);
  }
*/ 
  return 1;
}
