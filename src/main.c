#define NBINS 100000
#define LOWER_LIM_X 0000
#define UPPER_LIM_X 16000

//Parameter Defaults
#define DTRIGGER_CHANNEL_SRC 0xf
#define DNUM_CHANNELS 0x4
#define DTRIGGER_TYPE 0x2
#define DTRIGGER_THRESHOLD_MV 300
#define DNUM_PULSES 5000
#define DMODE_REGISTER 0x2
#define DFP_FREQUENCY 0x01
#define DNB_OF_COLS_TO_READ 0x80
#define DCHANNEL_MASK 0xf
#define DPRETRIG_LSB 0x0
#define DPRETRIG_MSB 0x28
#define DPOSTTRIG_LSB 0x32
#define DPOSTTRIG_MSB 0x0

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

#ifndef LINUX
#define LINUX 1
#endif

#include "V1729.h"
#include "V812.h"
#include "CAENVMElib.h"

int32_t handle;
uint32_t vme_data;

void parseConfig(const char *fn, Config config){
 FILE * fp;
 int value;
 ssize_t read;
 size_t len = 0;
 const int maxLines = 50;
 int compareLimit = 50;

 char * line = (char*)malloc(maxLines*sizeof(char));
 char * paraF = (char*)malloc(compareLimit*sizeof(char)); //parameter name from file
 char * paraN = (char*)malloc(compareLimit*sizeof(char));

 fp = fopen(fn, "r");
 if (fp == NULL)
   exit(0);

 while((read = getline(&line, &len, fp)) != -1) {
   sscanf(line, "%s %d", paraF, &value);  //Set element of struct corresponding to "para" to "value" 
   printf(line);
   sprintf(paraN, "%s", "TRIGGER_CHANNEL_SRC");
   if (strncmp(paraF, paraN, compareLimit) == 0){
     config.TRIGGER_CHANNEL_SRC = value;
     continue;
   }
   sprintf(paraN, "%s", "TRIGGER_TYPE");
   if (strncmp(paraF, paraN, compareLimit) == 0){
     config.TRIGGER_TYPE = value;
     continue;
   }
   sprintf(paraN, "%s", "NUM_CHANNELS");
   if (strncmp(paraF, paraN, compareLimit) == 0){
     config.NUM_CHANNELS = value;
     continue;
   }
   sprintf(paraN, "%s", "TRIGGER_THRESHOLD_MV");
   if (strncmp(paraF, paraN, compareLimit) == 0){
     config.TRIGGER_THRESHOLD_MV = value;
     continue;
   }
   sprintf(paraN, "%s", "NUM_PULSES");
   if (strncmp(paraF, paraN, compareLimit) == 0){ 
     config.NUM_PULSES = value;
     continue;
   }
   sprintf(paraN, "%s", "MODE_REGISTER");
   if (strncmp(paraF, paraN, compareLimit) == 0){ 
     config.MODE_REGISTER = value;
     continue;
   }
   sprintf(paraN, "%s", "NB_OF_COLS_TO_READ");
   if (strncmp(paraF, paraN, compareLimit) == 0){ 
     config.NB_OF_COLS_TO_READ = value;
     continue;
   }
   sprintf(paraN, "%s", "CHANNEL_MASK");
   if (strncmp(paraF, paraN, compareLimit) == 0){ 
     config.CHANNEL_MASK = value;
     continue;
   }
   sprintf(paraN, "%s", "PRETRIG_LSB");
   if (strncmp(paraF, paraN, compareLimit) == 0){ 
     config.PRETRIG_LSB = value;
     continue;
   }
   sprintf(paraN, "%s", "PRETRIG_MSB");
   if (strncmp(paraF, paraN, compareLimit) == 0){ 
     config.PRETRIG_MSB = value;
     continue;
   }
   sprintf(paraN, "%s", "POSTTRIG_LSB");
   if (strncmp(paraF, paraN, compareLimit) == 0){ 
     config.POSTTRIG_LSB = value;
     continue;
   }
   sprintf(paraN, "%s", "POSTTRIG_MSB");
   if (strncmp(paraF, paraN, compareLimit) == 0){ 
     config.POSTTRIG_MSB = value;
     continue;
   }
   sprintf(paraN, "%s", "FP_FREQUENCY");
   if (strncmp(paraF, paraN, compareLimit) == 0){ 
     config.FP_FREQUENCY = value;
     continue;
   }
 }
 
 if(line)
   free(line);
 if(paraF)
  free(paraF);
 if(paraN)
  free(paraN);
}

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

CVErrorCodes set_parameters (Config config) {
  CVErrorCodes ret;
 
  // Writing to TRIGGER_CHANNEL_SRC enables TRIGGER
  // to be sent on all four chnanels simultaneously.
  ret = write_to_vme(V1729_TRIGGER_CHANNEL_SRC, config.TRIGGER_CHANNEL_SRC);
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
  uint32_t trig_lev = (uint32_t)(config.TRIGGER_THRESHOLD_MV+1000)*((16*16*16)/2000.0); 
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
  ret = write_to_vme(V1729_MODE_REGISTER, config.MODE_REGISTER); //0x3 == 0b11 -> 14Bit and IRQ
                                                //0x2 == 0b10 -> 14Bit
  if (ret != cvSuccess) {
    printf("Setting mode register failed with error %d \n", ret);
    return ret;
  }

  //Pilot Frequency 
  ret = write_to_vme(V1729_FP_FREQUENCY, config.FP_FREQUENCY); //0x01 -> 2 GS/sec
  if (ret != cvSuccess) {
    printf("Setting FP_FREQUENCY failed with error %d \n", ret);
    return ret;
  }

  //Number of Columns
  ret = write_to_vme(V1729_NB_OF_COLS_TO_READ, config.NB_OF_COLS_TO_READ); //0x80 -> 128 Columns (All)
  if (ret != cvSuccess) {
    printf("Setting NB_OF_COLS_TO_READ failed with error %d \n", ret);
    return ret;
 }

  //Channel Mask determine numbers of active channels
  ret = write_to_vme(V1729_CHANNEL_MASK, config.CHANNEL_MASK); 
  if (ret != cvSuccess) {
    printf("Setting CHANNEL_MASK failed with error %d \n", ret);
    return ret;
  }

  // Pre-Trigger
  ret = write_to_vme(V1729_PRETRIG_LSB, config.PRETRIG_LSB); 
  if (ret != cvSuccess) {
    printf("Setting PRETRIG_LSB failed with error %d \n", ret);
    return ret;
  }

  ret = write_to_vme(V1729_PRETRIG_MSB, config.PRETRIG_MSB); 
  if (ret != cvSuccess) {
    printf("Setting PRETRIG_MSB failed with error %d \n", ret);
    return ret;
  }

  // Post-Trigger
  ret = write_to_vme(V1729_POSTTRIG_LSB, config.POSTTRIG_LSB); 
  if (ret != cvSuccess) {
    printf("Setting POSTTRIG_LSB failed with error %d \n", ret);
    return ret;
  }

  ret = write_to_vme(V1729_POSTTRIG_MSB, config.POSTTRIG_MSB); 
  if (ret != cvSuccess) {
    printf("Setting POSTTRIG_MSB failed with error %d \n", ret);
    return ret;
  }

  // Number of Channels determines whether you do a readout over 4 Channels (0x4)
  // 1 channel (0x1) or 2 channels (0x2). Less channels means more sampling depth. 
  //ret = write_to_vme(V1729_NUMBER_OF_CHANNELS, 0x1); 
  //ret = write_to_vme(V1729_NUMBER_OF_CHANNELS, 0x2); 
  //ret = write_to_vme(V1729_NUMBER_OF_CHANNELS, 0x4); 
  switch(config.NUM_CHANNELS){ 
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

  //ret = write_to_vme(V1729_TRIGGER_TYPE, 0x2); // Trigger on EXT TRIG Input Rising Edge
  //ret = write_to_vme(V1729_TRIGGER_TYPE, 0x6); // Trigger on EXT TRIG Input Falling Edge
  //ret = write_to_vme(V1729_TRIGGER_TYPE, 0x1); // Trigger on Pulse based on TRIG_LEV
  // Trigger Type
  ret = write_to_vme(V1729_TRIGGER_TYPE, config.TRIGGER_TYPE); 
  if (ret != cvSuccess) {
    printf("Setting TRIGGER_TYPE failed with error %d \n", ret);
    return ret;
  }

  return ret;
}

void setDefaultConf(Config config){
  config.TRIGGER_CHANNEL_SRC = DTRIGGER_CHANNEL_SRC;
  config.NUM_CHANNELS = DNUM_CHANNELS;
  config.TRIGGER_TYPE = DTRIGGER_TYPE;
  config.TRIGGER_THRESHOLD_MV = DTRIGGER_THRESHOLD_MV;
  config.NUM_PULSES = DNUM_PULSES;
  config.MODE_REGISTER = DMODE_REGISTER;
  config.FP_FREQUENCY = DFP_FREQUENCY;
  config.NB_OF_COLS_TO_READ = DNB_OF_COLS_TO_READ;
  config.CHANNEL_MASK = DCHANNEL_MASK;
  config.PRETRIG_LSB = DPRETRIG_LSB;
  config.PRETRIG_MSB = DPRETRIG_MSB;
  config.POSTTRIG_LSB = DPOSTTRIG_LSB;
  config.POSTTRIG_MSB = DPOSTTRIG_MSB;
}

int main(int argc, char **argv) {

  //int bad_read = 0;
  if (argc < 1){
    printf("USAGE: ./bin/adc_spectrum [CONFIG_FILE_NAME]\n");
    printf("e.g. ./bin/adc_spectrum config.conf\n");
    printf("This produces a readout based on the parameters in the config file\n"); 
    exit(1);
  }

  Config config;
  CVBoardTypes vme_board = cvV2718; 
  CVErrorCodes ret; // Error Codes for Debugging

  setDefaultConf(config);
  parseConfig(argv[1], config);

  int num_acquisitions = config.NUM_PULSES; // Number of times to loop acquisition
  int trigLevmV = config.TRIGGER_THRESHOLD_MV;
  uint32_t channels = config.NUM_CHANNELS;

  printf("num acquisitions: %u\n", num_acquisitions);
  printf("trigLevmV: %d\n", trigLevmV);

  printf("%u\n", config.TRIGGER_CHANNEL_SRC);
  printf("%u\n", config.NUM_CHANNELS); 
  printf("%u\n", config.TRIGGER_TYPE);
  printf("%d\n",config.TRIGGER_THRESHOLD_MV);
  printf("%u\n",config.NUM_PULSES);
  printf("%u\n",config.MODE_REGISTER);
  printf("%u\n",config.FP_FREQUENCY);
  printf("%u\n",config.NB_OF_COLS_TO_READ);
  printf("%u\n",config.CHANNEL_MASK);
  printf("%u\n",config.PRETRIG_LSB);
  printf("%u\n",config.PRETRIG_MSB);
  printf("%u\n",config.POSTTRIG_LSB);
  printf("%u\n",config.POSTTRIG_MSB);
  exit(1);
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
  ret = set_parameters(config); 
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


