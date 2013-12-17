#define NBINS 100000
#define LOWER_LIM_X 0000
#define UPPER_LIM_X 16000

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
//#include <iostream>
#ifndef LINUX
#define LINUX 1
#endif

#include "V1729.h"
#include "V812.h"
#include "CAENVMElib.h"

int32_t handle;
uint32_t vme_data;

void subtract_pedestals(unsigned int buffer16[V1729_RAM_DEPH], int pedestals[V1729_RAM_DEPH]) {
  int i;
  int buffer;
  int ch;
  for (i = 0; i < 2560; i++)
    for (ch = 0; ch < 4; ch++) {
      buffer = (int)(0xffff & buffer16[12+i*4+ch]);
       
      if( (buffer-pedestals[i*4+ch]) < 0) buffer16[12+i*4+ch] = 0;
      else buffer16[12+i*4+ch] = (unsigned int)(buffer - pedestals[i*4 + ch]);
    }
  return;
}

CVErrorCodes set_parameters (uint32_t trig_lev, uint32_t channels) {
  CVErrorCodes ret;
 
  // Writing to TRIGGER_CHANNEL_SRC enables TRIGGER
  // to be sent on all four chnanels simultaneously.
  ret = write_to_vme(V1729_TRIGGER_CHANNEL_SRC, 0xf);
  if (ret != cvSuccess) {
    printf("Setting trigger channel src with error %d \n", ret);
    return ret;
  }
  ret = write_to_vme(V1729_LOAD_TRIGGER_THS, 1);
  if (ret != cvSuccess) {
    printf("LOAD_TRIGGER_THRESHOLD_DAC failed with error %d \n", ret);
    return ret;
  }

  // Set Trigger Threshold Level to trig_lev 
  ret = write_to_vme(V1729_THRESHOLD, trig_lev);
  if (ret != cvSuccess) {
    printf("Setting trigger threshold failed with error %d \n", ret);
    return ret;
  }

  // After loading of V1729_THRESHOLD, one must transfer the value in the analog
  // converter via the LOAD_TRIGGER_THRESHOLD_DAC command  
  ret = write_to_vme(V1729_LOAD_TRIGGER_THS, 1);
  if (ret != cvSuccess) {
    printf("LOAD_TRIGGER_THRESHOLD_DAC failed with error %d \n", ret);
    return ret;
  }

  // Mode register controls whether IRQ lines are enabled (bit 0), 14bit or 12bit
  // is used (bit 1), and automatic or normal acquisition modes (bit 2)
  ret = write_to_vme(V1729_MODE_REGISTER, 0x2); //0x3 == 0b11 -> 14Bit and IRQ
                                                //0x2 == 0b10 -> 14Bit
  if (ret != cvSuccess) {
    printf("Setting mode register failed with error %d \n", ret);
    return ret;
  }

  //Pilot Frequency 
  ret = write_to_vme(V1729_FP_FREQUENCY, 0x01); //0x01 -> 2 GS/sec
  if (ret != cvSuccess) {
    printf("Setting FP_FREQUENCY failed with error %d \n", ret);
    return ret;
  }

  //Number of Columns
  ret = write_to_vme(V1729_NB_OF_COLS_TO_READ, 0x80); //0x80 -> 128 Columns (All)
  if (ret != cvSuccess) {
    printf("Setting NB_OF_COLS_TO_READ failed with error %d \n", ret);
    return ret;
 }

  //Channel Mask determine numbers of active channels
  ret = write_to_vme(V1729_CHANNEL_MASK, 0xf); 
  if (ret != cvSuccess) {
    printf("Setting CHANNEL_MASK failed with error %d \n", ret);
    return ret;
  }

  // Pre-Trigger
  ret = write_to_vme(V1729_PRETRIG_LSB, 0x0); 
  if (ret != cvSuccess) {
    printf("Setting PRETRIG_LSB failed with error %d \n", ret);
    return ret;
  }

  ret = write_to_vme(V1729_PRETRIG_MSB, 0x28); 
  if (ret != cvSuccess) {
    printf("Setting PRETRIG_MSB failed with error %d \n", ret);
    return ret;
  }

  // Post-Trigger
  //ret = write_to_vme(V1729_POSTTRIG_LSB, 0x40); 
  ret = write_to_vme(V1729_POSTTRIG_LSB, 0x32); 
  if (ret != cvSuccess) {
    printf("Setting POSTTRIG_LSB failed with error %d \n", ret);
    return ret;
  }

  ret = write_to_vme(V1729_POSTTRIG_MSB, 0x0); 
  if (ret != cvSuccess) {
    printf("Setting POSTTRIG_MSB failed with error %d \n", ret);
    return ret;
  }

  // Number of Channels determines whether you do a readout over 4 Channels (0x4)
  // 1 channel (0x1) or 2 channels (0x2). Less channels means more sampling depth. 
  //ret = write_to_vme(V1729_NUMBER_OF_CHANNELS, 0x1); 
  //ret = write_to_vme(V1729_NUMBER_OF_CHANNELS, 0x2); 
  //ret = write_to_vme(V1729_NUMBER_OF_CHANNELS, 0x4); 
  switch(channels){ 
    case (4):
      ret = write_to_vme(V1729_NUMBER_OF_CHANNELS, 0x1); 
      break;
    case (2):
      ret = write_to_vme(V1729_NUMBER_OF_CHANNELS, 0x2); 
      break;
    case (1):
      ret = write_to_vme(V1729_NUMBER_OF_CHANNELS, 0x4); 
      break;
  }
  if (ret != cvSuccess) {
    printf("Setting NUMBER_OF_CHANNELS failed with error %d \n", ret);
    return ret;
  }

  // Trigger Type
  ret = write_to_vme(V1729_TRIGGER_TYPE, 0x2); // Trigger on EXT TRIG Input Rising Edge
  //ret = write_to_vme(V1729_TRIGGER_TYPE, 0x6); // Trigger on EXT TRIG Input Falling Edge
  //ret = write_to_vme(V1729_TRIGGER_TYPE, 0x1); // Trigger on Pulse based on TRIG_LEV
  if (ret != cvSuccess) {
    printf("Setting TRIGGER_TYPE failed with error %d \n", ret);
    return ret;
  }

  return ret;
}


int main(int argc, char **argv) {

  //int bad_read = 0;
  if (argc < 3){
    printf("USAGE: ./bin/adc_spectrum [NUM_ACQUSITIONS] [TRIGGER LEVEL IN mV] [channels used for one pulse (4,2,1)]\n");
    printf("e.g. ./bin/adc_spectrum 5000 -300 4\n");
    printf("This produces a readout of 5000 pulses with -300 trigger over 4 channels\n");
    exit(1);
  }
  CVBoardTypes vme_board = cvV2718; 
  CVErrorCodes ret; // Error Codes for Debugging
  int num_acquisitions = atoi(argv[1]); // Number of times to loop acquisition
  int trigLevmV = atoi(argv[2]);
  uint32_t channels = atoi(argv[3]);
  uint32_t trig_lev; 

  printf("num acquisitions: %d\n", num_acquisitions);
  printf("trigLevmV: %d\n", trigLevmV);

  //Parameters
  // Let x be the desired triggering threshold value. 
  // (x+1000)*((16^3)/2000) = trig_lev

  trig_lev = (uint32_t)(trigLevmV+1000)*((16*16*16)/2000.0); 

  uint32_t num_columns; // number of columns to read from MATACQ matrix
  uint32_t post_trig; // Post trigger value
  unsigned int trig_rec; //Helps determine the trigger's position in the acquisition window
  
  //Looping variables
  int i; 
  int interrupts = 0;

  //Buffers for storing data
  unsigned int buffer32[V1729_RAM_DEPH/2];
  unsigned int buffer16[V1729_RAM_DEPH];
  unsigned short ch0[2560], ch1[2560], ch2[2560], ch3[2560]; //Corrected Data

  //Initialize buffers
  for (i = 0; i < V1729_RAM_DEPH/2; i++) {
    buffer32[i] = 0;
    buffer16[2*i] = 0;
    buffer16[(2*i)+1]=0;
  }

  //"Pedestals" really stores data to remove variance from pedestals
  int pedestals[V1729_RAM_DEPH];
  float mean_pedestal[4];
  unsigned int MAXVER[4], MINVER[4]; // MAXVER -> 1/pilot_frequency
                                     // MINVER -> Zero of the vernier

  //Create handle for interacting with VME Board
  ret = CAENVME_Init(vme_board, 0, 0, &handle);
  if (ret  != cvSuccess) {
    printf("\n\n Error Opening VX2718! \n\n");
    CAENVME_End(handle);
    return 0;
  }

  ret = reset_vme();
  if (reset_vme() != cvSuccess) {
    printf(" Reset failed with error %d \n", ret); CAENVME_End(handle);
    return 0;
  }

  // Set Parameters
  ret = set_parameters(trig_lev, channels); 
  if (ret != cvSuccess) {
    printf("Setting run paramaters failed with error %d \n", ret);
    CAENVME_End(handle);
    return 0;
  }

  // Get Number of Columns to Read from Matrix
  ret = read_from_vme(V1729_NB_OF_COLS_TO_READ);
  if (ret != cvSuccess) {
    printf(" Loading number of columns failed with error %d\n", ret);
    return 0;
  }
  num_columns = vme_data&0xff;

  //Get Post-Trig
  ret = read_from_vme(V1729_POSTTRIG_LSB);
  if (ret != cvSuccess) {
    printf(" Loading POSTTRIG_LSB failed with error %d\n", ret);
    return 0;
  }
  post_trig = vme_data & 0xff;

  ret = read_from_vme(V1729_POSTTRIG_MSB);
  if (ret != cvSuccess) {
    printf(" Loading POSTTRIG_MSB failed with error %d\n", ret);
    return 0;
  }
  post_trig += (vme_data & 0xff)*256;
  
  // Calibration & Pedestal Correction
  // Vernier Calibration
  if (vernier(MAXVER, MINVER) != cvSuccess) {
    printf("Failed vernier calibration!\n");
    CAENVME_End(handle);
    return 0;
  }


  // Get Pedestals
  if (get_pedestals(pedestals, buffer32, buffer16, mean_pedestal) == 0) {
    printf("Failed to get pedestals!\n");
    CAENVME_End(handle);
    return 0;
  }

  printf("Mean pedestal for Ch. 0: %f\n", mean_pedestal[0]);
  printf("Mean pedestal for Ch. 1: %f\n", mean_pedestal[1]);
  printf("Mean pedestal for Ch. 2: %f\n", mean_pedestal[2]);
  printf("Mean pedestal for Ch. 3: %f\n", mean_pedestal[3]);

  // Pedestals mus tbe calculated before attaching a signal for best results 
  printf("Please now attach your signal to the board. Press ENTER when ready.\n");
  while(getchar() != '\n');
  
  while (interrupts <= num_acquisitions) {

    ret = start_acq();
    if (ret != cvSuccess) {
      printf("Start Acquisition failed with error %d\n", ret);
      return 0;
    }

    wait_for_interrupt();
    interrupts++;
    
    ret = read_vme_ram(buffer32);
    if (ret != cvSuccess) {
      printf("Reading VME RAM failed with error %d\n", ret);
      return 0;
    }

    ret = read_from_vme(V1729_TRIG_REC);
    if (ret != cvSuccess) {
      printf("Read TRIG_REC failed with error %d\n", ret);
      return 0;
    }
    trig_rec = vme_data&0xff;

    mask_buffer(buffer32, buffer16);

    subtract_pedestals(buffer16, pedestals); 

    reorder(trig_rec, post_trig, num_columns, MINVER, MAXVER, 
            buffer16, ch0, ch1, ch2, ch3);
     
    //Save to ASCII File
    save(ch0, ch1, ch2, ch3, num_acquisitions, trigLevmV, channels);
  }

  printf("Closing board post-acquisition...\n ");
  CAENVME_End(handle);
  return 1;
}


