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

#include "V1729.h"
#include "CAENVMElib.h"

/*This is an attempt to rewrite the previous main.c file.
  Please note that no signal should be attached to the board
  when first running this, because pedestals must be found before
  the signal is attached. */

int main(int argc, void *argv[])
{
  CVBoardTypes vme_board = cvV2718; /* Type of Board: VX2718 */
  short link = 0; /* Link number */ 
  short device = 0; /* Device Number */
  CVErrorCodes ret; /* Stores Error Codes for Debugging */

  /* desired trigger treshhold = (2000/16^3)(trig_lev) - 1000 */
  uint32_t trig_lev = 0x400;
  uint32_t active_channel; /* active channels on the frontend of board */
  uint32_t num_columns; /* number of columns to read from MATACQ matrix */
  uint32_t post_trig; /* post trigger value */

  int mask;
  int ch;
  int buffer;
  int i;
  int interrupts;
  /* Two buffers for storing data from RAM */
  unsigned int buffer32[V1729_RAM_DEPH/2]; 
  unsigned int buffer16[V1729_RAM_DEPH]; 

  for (i=0; i<V1729_RAM_DEPH/2; i++)
  {
    buffer32[i]=0;
    buffer16[2*i]=0;
    buffer16[(2*i)+1]=0;
  }

  int pedestals[V1729_RAM_DEPH];
  unsigned int MAXVER[4], MINVER[4]; /*Correspond to 
                                     1/pilot_frequency and the 
                                     zero of the vernier, 
                                     respectively. Need help
                                     understanding these.*/

  unsigned int trig_rec; /* TRIG_REC must be read to automatically
                            restart acquisitions. Its value also helps 
                            determine the position of the trigger in 
                            the acquisition window. */

  /* These arrays store the data that will be saved to file post-reordering */
  unsigned short ch0[2560], ch1[2560], ch2[2560], ch3[2560];

  /* Create opaque handle for interacting with VX2718 Board */
  if ( CAENVME_Init(vme_board, link, device, &handle) != cvSuccess )
  {
    printf("\n\n Error opening VX2718\n");
    return 0;
  }

  else printf("Successfully opened VX2718!\n"); 


  /* Send Reset Order to Board  */
  printf("Attempting to reset board...");

  ret = reset_vme();

  if (ret != cvSuccess)
  {
    printf("  Reset failed with error %d \n", ret);
    return 0;
  }  
  else printf("  Reset succesfully\n");

  /*Write to TRIGGER_CHANNEL_SRC */
  /* Enables TRIGGER to be sent from all four channels */
  write_to_vme(V1729_TRIGGER_CHANNEL_SRC, 0xf);
  write_to_vme(V1729_LOAD_TRIGGER_THS, 1);
 
  /* Set Trigger Threshold Level to trig_lev, previously declared */ 
  printf("Setting trigger threshold level... ");
  ret = write_to_vme(V1729_THRESHOLD, trig_lev);

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
  ret = write_to_vme(V1729_LOAD_TRIGGER_THS, 1);

  if (ret != cvSuccess)
  {
    printf("Loading trigger threshold command failed with error: %d\n", ret);
    return 0; 
  }  
  else printf("Load Trigger threshold command successful\n");

  /********************************************************************
   Initialize Necessary Parameters
  ********************************************************************/
 
    /* FP Frequency */
  printf("Attempting to change pilot frequency... ");
  ret = write_to_vme(V1729_FP_FREQUENCY, 0x01);//2 GHz
  if (ret != cvSuccess)
  {
    printf("Changing pilot frequency failed with error: %d \n", ret);
    return 0;
  }  
  else printf("Change pilot frequency successful\n");
  

    /* Num Columns */
  printf("Attempting to set num columns...  ");
  ret = write_to_vme(V1729_NB_OF_COLS_TO_READ, 0x80); 
  if (ret != cvSuccess)
  {
    printf("Changing num columns to read  failed with error: %d \n", ret);
    return 0;
  }  
  else printf("Change num columns successful\n");
  
    /* Channel Mask  */
  printf("Attempting to set active channel...  ");
  ret = write_to_vme(V1729_CHANNEL_MASK, 0xf); 
  if (ret != cvSuccess)
  {
    printf("Changing active channel failed with error: %d \n", ret);
    return 0;
  }  
  else printf("Change active channel successful\n");
  
    /* Pre Trigger: Can be modified at LSB and MSB */
  printf("Setting PRETRIG LSB... ");
  ret = write_to_vme(V1729_PRETRIG_LSB, 0x00); 
  if (ret != cvSuccess)
  {
    printf("Setting PRETRIG_LSB failed with error: %d \n", ret);
    return 0;
  }  
  else printf("Setting PRETRIG_LSB successful\n");

  printf("Setting PRETRIG MSB... ");
  ret = write_to_vme(V1729_PRETRIG_MSB, 0x28); 
  if (ret != cvSuccess)
  {
    printf("Setting PRETRIG_MSB failed with error: %d \n", ret);
    return 0;
  }  
  else printf("Setting PRETRIG_MSB successful\n");

    /* Post Trigger: Can be modified at LSB and MSB */
  printf("Setting POST_TRIG LSB... ");
  ret = write_to_vme(V1729_POSTTRIG_LSB, 0x40);
  if (ret != cvSuccess)
  {
    printf("Setting POSTTRIG_LSB failed with error: %d \n", ret);
    return 0;
  }  
  else printf("Setting POSTTRIG_LSB successful\n");

  printf("Setting POST_TRIG MSB... ");
  ret = write_to_vme(V1729_POSTTRIG_MSB, 0x0);
  if (ret != cvSuccess)
  {
    printf("Setting POSTTRIG_MSB failed with error: %d \n", ret);
    return 0;
  }  
  else printf("Setting POSTTRIG_MSB successful\n");

    /* Trigger Type */
  printf("Setting value of TRIGGER_TYPE ..");
  ret = write_to_vme(V1729_TRIGGER_TYPE, 0x7); /* Trigger on Software OR Discriminator Falling Edge */
  if (ret != cvSuccess)
  {
    printf(" Setting TRIGGER_TYPE failed with error: %d \n", ret);
    return 0;
  }  
  else printf(" Set TRIGGER_TYPE successful\n");


  /********************************************************************
   Loading of Parameters for Reordering and Saving Data Later 
  ********************************************************************/
    /* Num Columns */
  printf("Loading number of columns ..");
  ret = read_from_vme(V1729_NB_OF_COLS_TO_READ);
  if (ret != cvSuccess)
  {
    printf(" Loading num. of columns failed with error: %d \n", ret);
    return 0;
  }  
  else printf(" Load number of columns successful\n");
  num_columns = vme_data&0xff; /*0xff is 11111111 in binary. 
                                 Default value is 128 so
                                 128&0xff = 128, all the columns */

      /* Channel Mask */
  printf("Attempting to determine active channel...  ");
  ret = read_from_vme(V1729_CHANNEL_MASK); 
  if (ret != cvSuccess)
  {
    printf("Loading active channel failed with error: %d \n", ret);
    return 0; }  
  else printf("Load active channel successful\n");

  active_channel = vme_data&0xf; 

  
    /* POSTTRIG_MSB and POSTTRIG_LSB. */
  printf("Attempting to determine POSTTRIG...  \n");
  ret = read_from_vme(V1729_POSTTRIG_LSB); 
  if (ret != cvSuccess)
  {
    printf("Loading POSTTRIG_LSB failed with error: %d \n", ret);
    return 0;
  }  
  else printf("    Load POSTTRIG_LSB successful\n");
  post_trig = vme_data&0xff; 
  printf("Post Trig after LSB: %d\n", post_trig);

  ret = read_from_vme(V1729_POSTTRIG_MSB); 
  if (ret != cvSuccess)
  {
    printf("    Loading POSTTRIG_MSB failed with error: %d \n", ret);
    return 0;
  }  
  else printf("    Load POSTTRIG_MSB successful\n");
  post_trig = post_trig + (vme_data&0xff)*256; 

  printf("Post Trig after MSB: %d\n", post_trig);
  printf("Current active channel: %d\n", active_channel);
  printf("num_columns: %d\n", num_columns);


  /*********************************************************
   Perform Pre-Acquisition Calibrations 
   *********************************************************/

  /* Perform Fast Vernier Calibration */
  printf("Attempting to perform Vernier calibration\n");
  if (vernier(MAXVER,  MINVER) != 1)
  {
    printf("Failed vernier calibration. \n");
    return 0;
  }
  else printf("Successful vernier calibration!\n");

  /* Find Pedestals which will later be subtracted*/

  printf("Attempting to find pedestals... \n");
  if (get_pedestals(pedestals, buffer32, buffer16) == 0)
  {
    printf("Failed to get pedestals.\n");
    CAENVME_End(handle);
    return 0;
  }
  else printf("Successfully found pedestals\n"); 
 
  /* Pedestals must be calculated before attaching a signal for best results */
  printf("Please now attach your signal to the board. Press RETURN when ready.\n");
  char key = getchar(); /* Probably a better way to do this, but 
                           need nothing connected to board prior
                           to getting pedestals */
  


  /*************************************************** 
   Begin Actual Acquisition
   **************************************************/
  while (interrupts < 5)
  {
    /*Send start acquisition signal*/
    printf("Attempting to start acquisition...  ");
    ret = start_acq();  
    if (ret != cvSuccess)
    {
      printf("Sending Start Acquisition signal failed with error %d\n", ret);
      reset_vme();
      return 0;
    }  
    else printf("Sending Start Acquisition signal succesful\n");

    /*Wait for Interrupt from V1729A*/
    printf("Waiting for interrupt from V1729A...");
    if ( wait_for_interrupt() == 0 )
    {
      reset_vme();
      CAENVME_End(handle);
    }
    else printf(" Successfully found interrupt.\n");
    interrupts++;

    /*After receiving interrupt must acknowledge
        by writing 0 in interrupt register.*/
    printf("Interrupt found. Attempting to acknowledge..\n");
    ret = write_to_vme(V1729_INTERRUPT, 0); 
    if (ret != cvSuccess)
    {
      printf("Interrupt Acknowledge failed with error %d\n", ret);
      return 0;
    }  
    else printf("Interrupt Acknowledged succesfully\n");

    /*Read VME Ram*/
    printf("Attempting to read vme ram...");
    ret = read_vme_ram(buffer32);
    if (ret != cvSuccess)
    {
      printf("Reading VME RAM failed with error %d\n", ret);
      reset_vme();
      return 0;
    }  
    else printf("Reading VME RAM succesful\n");

    /*Read TRIG_REC after VME RAM: Necessary for determining
      trigger position in window */
    printf("Attempting to read TRIG_REC... ");
    ret = read_from_vme(V1729_TRIG_REC); 
    if (ret != cvSuccess)
    {
      printf("Reading TRIG_REC failed with error %d\n", ret);
      reset_vme();
      return 0;
    }  
    else printf("Reading TRIG_REC succesful\n");
    trig_rec = vme_data&0xff;

    /*Mask Buffer*/
    printf("Attempting to mask buffer ... ");
    if( mask_buffer(buffer32, buffer16) == 0 ) 
      {
      printf(" Masking the buffer has failed with error %d\n", ret);
      reset_vme();
      return 0;
      }
    else
      printf(" Buffer masked successfully\n"); 

    /*************************************************** 
      Temporal and Pedestal Correction of Data
    ***************************************************/

    /* Subtraction of Pedestals */
    printf("Subtracting pedestals... \n");
    if ((active_channel == 0xf) && (num_columns == 128)) 
    {
      for (i = 0; i < 2560; i++)
        for (ch = 0; ch < 4; ch++)
        {
          buffer = (int)(0xffff&buffer16[12 + i*4 + ch]);

          if( (buffer - pedestals[i*4 + ch]) < 0 ) buffer16[12 + i*4 + ch] = 0;
          else buffer16[12 + i*4 + ch] = (unsigned int)(buffer - pedestals[i*4 + ch]);
        }
    
      /* Reorder Data */
      printf("Reordering data.\n");
      reorder(trig_rec, post_trig, num_columns, MINVER, MAXVER, buffer16, ch0, ch1, ch2, ch3);

      printf("Saving data... \n");
      save(ch0, ch1, ch2, ch3);
    }
   
    else 
    {
      printf("active_channel: %d", active_channel);
      printf("num_columns: %d", num_columns);
      printf("Mask Channel not Valid\n"); 
      return 0;
    }
  }
 /**************************************************  
  End Acquisition and Close Board 
  **************************************************/

  /*Send Reset to End Acquisition*/
  printf("Attempting to reset board to end acquisition... ");
  ret = reset_vme();
  if (ret != cvSuccess)
  {
    printf("Reset failed with error %d\n", ret);
    return 0;
  }  

  else printf("Reset succesfully\n");
  
  /* Close Board */
  printf("Closing board post-acquisition... ");
  ret = CAENVME_End(handle);  
  if (ret != cvSuccess)
  {
    printf("Closing board failed with error %d\n", ret);
    return 0;
  }  
  else printf("Closed Board succesfully\n");

  return 1; 
}
