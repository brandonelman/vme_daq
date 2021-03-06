#define NBINS 100000
#define LOWER_LIM_X 0000
#define UPPER_LIM_X 16000

//Parameter Defaults
#define DTRIGGER_CHANNEL_SRC 15 
#define DTRIGGER_TYPE 2
#define DTRIGGER_THRESHOLD_MV 300
#define DNUM_PULSES 5000
#define DMODE_REGISTER 6
#define DFP_FREQUENCY 1
#define DNB_OF_COLS_TO_READ 128
#define DCHANNEL_MASK 15
#define DPRETRIG_LSB 0
#define DPRETRIG_MSB 40 
#define DPOSTTRIG_LSB 50
#define DPOSTTRIG_MSB 0

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
#include "V6521M.h"
#include "CAENVMElib.h"

int32_t handle;
uint32_t vme_data;

void replace(char *str, char old, char new){
  char *temp = str;
  while(*temp)
  {
    if(*temp == old)
      *temp = new;
    ++temp;
  }
}

void strstrip(char *str){
  size_t size;
  char *end;
  
  size = strlen(str);
  if (!size)
    return;

  end = str + size - 1;
  while (end >= str && isspace(*end))
    end--;
  *(end+1) = '\0';

  while(*str && isspace(*str))
    str++;
}

void removeSpaces(char *str){
  char *inp = str;
  char *outp = str;
  int prevSpace = 0;

  while(*inp){
    if(isspace(*inp)) {
      if(!prevSpace){
        *outp++ = ' ';
        prevSpace = 1;
      }
    }
    else {
      *outp++ = *inp;
      prevSpace = 0;
    }
    ++inp;
  }
  *outp = '\0';
}

void parseConfig(const char *fn, Config *config){
 FILE * fp;

 int value;
 ssize_t read;
 size_t len = 0;

 char * line = (char*)malloc(MAX_STRING_LENGTH*sizeof(char));
 char * paraF = (char*)malloc(MAX_STRING_LENGTH*sizeof(char)); //parameter name from file
 char * paraN = (char*)malloc(MAX_STRING_LENGTH*sizeof(char)); //Expected Parameter Names
 char * paraV = (char*)malloc(MAX_STRING_LENGTH*sizeof(char)); 

 fp = fopen(fn, "r");
 if (fp == NULL)
   exit(1);

 while((read = getline(&line, &len, fp)) != -1) {

   if (strstr(line, "=") != NULL) {
     replace(line, '=', ' ');
   }
   if (strstr(line, "#") != NULL) {
    replace(line, '#', '\0');   
   }
   removeSpaces(line);
   strstrip(line);
   sscanf(line, "%s %s", paraF, paraV);  //Set element of struct corresponding to "para" to "value" 

   sprintf(paraN, "%s", "pmt-id-1");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){
     strncpy(config->pmt_serials[1], paraV, MAX_STRING_LENGTH); 
     continue;
   }

   sprintf(paraN, "%s", "pmt-id-2");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){
     strncpy(config->pmt_serials[2], paraV, MAX_STRING_LENGTH); 
     continue;
   }

   sprintf(paraN, "%s", "pmt-id-3");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){
     strncpy(config->pmt_serials[3], paraV, MAX_STRING_LENGTH); 
     continue;
   }

   sprintf(paraN, "%s", "pmt-id-0");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){
     strncpy(config->pmt_serials[0], paraV, MAX_STRING_LENGTH); 
     continue;
   }
   
   sprintf(paraN, "%s", "output-folder");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){
     strncpy(config->output_folder, paraV, MAX_STRING_LENGTH); 
     continue;
   }

   sprintf(paraN, "%s", "descriptor");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){
     strncpy(config->descriptor, paraV, MAX_STRING_LENGTH); 
     continue;
   }

   value = atoi(paraV); //All other values are integers rather than strings 
   sprintf(paraN, "%s", "trigger-channel-src");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){
     config->trigger_channel_src = value;
     continue;
   }
   sprintf(paraN, "%s", "trigger-type");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){
     config->trigger_type = value;
     continue;
   }
   sprintf(paraN, "%s", "trigger-threshold-mv");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){
     config->trigger_threshold_mv = value;
     continue;
   }
   sprintf(paraN, "%s", "num-pulses");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){ 
     config->num_pulses = value;
     continue;
   }
   sprintf(paraN, "%s", "mode-register");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){ 
     config->mode_register = value;
     continue;
   }
   sprintf(paraN, "%s", "nb-of-cols-to-read");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){ 
     config->nb_of_cols_to_read = value;
     continue;
   }
   sprintf(paraN, "%s", "channel-mask");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){ 
     config->channel_mask = value;
     continue;
   }
   sprintf(paraN, "%s", "pretrig-lsb");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){ 
     config->pretrig_lsb = value;
     continue;
   }
   sprintf(paraN, "%s", "pretrig-msb");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){ 
     config->pretrig_msb = value;
     continue;
   }
   sprintf(paraN, "%s", "posttrig-lsb");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){ 
     config->posttrig_lsb = value;
     continue;
   }
   sprintf(paraN, "%s", "posttrig-msb");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){ 
     config->posttrig_msb = value;
     continue;
   }
   sprintf(paraN, "%s", "fp-frequency");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){ 
     config->fp_frequency = value;
     continue;
   }
   sprintf(paraN, "%s", "pmt-voltages-0");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){ 
     config->pmt_voltages[0] = value;
     continue;
   }
   sprintf(paraN, "%s", "pmt-voltages-1");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){ 
     config->pmt_voltages[1] = value;
     continue;
   }
   sprintf(paraN, "%s", "pmt-voltages-2");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){ 
     config->pmt_voltages[2] = value;
     continue;
   }   
   sprintf(paraN, "%s", "pmt-voltages-3");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){ 
     config->pmt_voltages[3] = value;
     continue;
   }   
   sprintf(paraN, "%s", "pmt-voltages-4");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){ 
     config->pmt_voltages[4] = value;
     continue;
   }   
   sprintf(paraN, "%s", "pmt-voltages-5");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){ 
     config->pmt_voltages[5] = value;
     continue;
   }  
   sprintf(paraN, "%s", "lamp-voltage");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){ 
     config->lamp_voltage = value;
     continue;
   }
   sprintf(paraN, "%s", "lamp-frequency");
   if (strncmp(paraF, paraN, MAX_STRING_LENGTH) == 0){ 
     config->lamp_frequency = value;
     continue;
   }
 }
 if(line)
   free(line);
 if(paraF)
  free(paraF);
 if(paraN)
  free(paraN);
 if(paraV)
   free(paraV);
 return;
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
  ret = write_to_vme(V1729_TRIGGER_CHANNEL_SRC, config.trigger_channel_src);
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
  uint32_t trig_lev = (uint32_t)(config.trigger_threshold_mv+1000)*((16*16*16)/2000.0); 
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
  ret = write_to_vme(V1729_MODE_REGISTER, config.mode_register); //0x3 == 0b11 -> 14Bit and IRQ
                                                //0x2 == 0b10 -> 14Bit
  if (ret != cvSuccess) {
    printf("Setting mode register failed with error %d \n", ret);
    return ret;
  }

  //Pilot Frequency 
  ret = write_to_vme(V1729_FP_FREQUENCY, config.fp_frequency); //0x01 -> 2 GS/sec
  if (ret != cvSuccess) {
    printf("Setting FP_FREQUENCY failed with error %d \n", ret);
    return ret;
  }

  //Number of Columns
  ret = write_to_vme(V1729_NB_OF_COLS_TO_READ, config.nb_of_cols_to_read); //0x80 -> 128 Columns (All)
  if (ret != cvSuccess) {
    printf("Setting NB_OF_COLS_TO_READ failed with error %d \n", ret);
    return ret;
 }

  //Channel Mask determine numbers of active channels
  ret = write_to_vme(V1729_CHANNEL_MASK, config.channel_mask); 
  if (ret != cvSuccess) {
    printf("Setting CHANNEL_MASK failed with error %d \n", ret);
    return ret;
  }

  // Pre-Trigger
  ret = write_to_vme(V1729_PRETRIG_LSB, config.pretrig_lsb); 
  if (ret != cvSuccess) {
    printf("Setting PRETRIG_LSB failed with error %d \n", ret);
    return ret;
  }

  ret = write_to_vme(V1729_PRETRIG_MSB, config.pretrig_msb); 
  if (ret != cvSuccess) {
    printf("Setting PRETRIG_MSB failed with error %d \n", ret);
    return ret;
  }

  // Post-Trigger
  ret = write_to_vme(V1729_POSTTRIG_LSB, config.posttrig_lsb); 
  if (ret != cvSuccess) {
    printf("Setting POSTTRIG_LSB failed with error %d \n", ret);
    return ret;
  }

  ret = write_to_vme(V1729_POSTTRIG_MSB, config.posttrig_msb); 
  if (ret != cvSuccess) {
    printf("Setting POSTTRIG_MSB failed with error %d \n", ret);
    return ret;
  }

  // Number of Channels determines whether you do a readout over 4 Channels (0x4)
  // 1 channel (0x1) or 2 channels (0x2). Less channels means more sampling depth. 
  //ret = write_to_vme(V1729_NUMBER_OF_CHANNELS, 0x1); 
  //ret = write_to_vme(V1729_NUMBER_OF_CHANNELS, 0x2); 
  ret = write_to_vme(V1729_NUMBER_OF_CHANNELS, 0x4); 
  if (ret != cvSuccess) {
    printf("Setting NUMBER_OF_CHANNELS failed with error %d \n", ret);
    return ret;
  }

  //ret = write_to_vme(V1729_TRIGGER_TYPE, 0x2); // Trigger on EXT TRIG Input Rising Edge
  //ret = write_to_vme(V1729_TRIGGER_TYPE, 0x6); // Trigger on EXT TRIG Input Falling Edge
  //ret = write_to_vme(V1729_TRIGGER_TYPE, 0x1); // Trigger on Pulse based on TRIG_LEV
  // Trigger Type
  ret = write_to_vme(V1729_TRIGGER_TYPE, config.trigger_type); 
  if (ret != cvSuccess) {
    printf("Setting TRIGGER_TYPE failed with error %d \n", ret);
    return ret;
  }

  return ret;
}

void setDefaultConf(Config *config){
  config->trigger_channel_src = DTRIGGER_CHANNEL_SRC;
  config->trigger_type = DTRIGGER_TYPE;
  config->trigger_threshold_mv = DTRIGGER_THRESHOLD_MV;
  config->num_pulses = DNUM_PULSES;
  config->mode_register = DMODE_REGISTER;
  config->fp_frequency = DFP_FREQUENCY;
  config->nb_of_cols_to_read = DNB_OF_COLS_TO_READ;
  config->channel_mask = DCHANNEL_MASK;
  config->pretrig_lsb = DPRETRIG_LSB;
  config->pretrig_msb = DPRETRIG_MSB;
  config->posttrig_lsb = DPOSTTRIG_LSB;
  config->posttrig_msb = DPOSTTRIG_MSB;
  strncpy(config->pmt_serials[0], "none", MAX_STRING_LENGTH);
  strncpy(config->pmt_serials[1], "none", MAX_STRING_LENGTH);
  strncpy(config->pmt_serials[2], "none", MAX_STRING_LENGTH);
  strncpy(config->pmt_serials[3], "none", MAX_STRING_LENGTH);
  strncpy(config->descriptor, "none", MAX_STRING_LENGTH);
  strncpy(config->tag, "none", MAX_STRING_LENGTH);
  strncpy(config->mode, "misc", MAX_STRING_LENGTH);
  strncpy(config->output_folder, "/daq/misc", MAX_STRING_LENGTH);
}

int doesFileExist(const char *filename) {
  struct stat st;
  int result = stat(filename, &st);
  return result == 0;
}

int main(int argc, char **argv) {

  //int bad_read = 0;
  if (argc < 5){
    printf("USAGE: ./bin/daq -r [RUN_NUM] -m [MODE_NAME] -t [TAG]  [CONFIG_FILE_NAME]\n");
    printf("e.g. ./bin/daq -r 1000 -t pre -m gain config.conf \n");
    printf("This produces a readout based on the parameters in the config file\n"); 
    printf("Mode options: gain, spe, surf, misc\n");
    printf("Tag options: pre, post, ang0, ang90, ang180, ang270\n");
    exit(1);
  }

  Config config;
  int integrate = 0;
  int i;
  for (i = 1; i < argc; i++) {
    if(strncmp(argv[i], "-r", MAX_STRING_LENGTH) == 0){
      config.run_num = atoi(argv[i+1]);
      i += 1; //Skips unnecessary check on next element
      continue;
    }

    if(strncmp(argv[i], "-t", MAX_STRING_LENGTH) == 0){
      strncpy(config.tag, argv[i+1], MAX_STRING_LENGTH);
      i += 1; 
      continue;
    }

    if(strncmp(argv[i], "-m", MAX_STRING_LENGTH) == 0) {
      strncpy(config.mode, argv[i+1], MAX_STRING_LENGTH);
      i += 1; 
      continue;
    }

    if(strncmp(argv[i], "--integrate", MAX_STRING_LENGTH) == 0) {
      integrate = 1;
      continue;
    }


  }
  CVBoardTypes vme_board = cvV2718; 
  CVErrorCodes ret; // Error Codes for Debugging

  //setDefaultConf(&config);
  parseConfig(argv[argc-1], &config);

  int num_acquisitions = config.num_pulses; // Number of times to loop acquisition

  if(1){
  printf("\nACTUAL PARAMETERS\n");
  printf("TRIGGER_CHANNEL_SRC = %d\n", config.trigger_channel_src);
  printf("TRIGGER_TYPE = %d\n", config.trigger_type);
  printf("TRIGGER_THRESHOLD_MV = %d\n", config.trigger_threshold_mv);
  printf("NUM_PULSES = %d\n", config.num_pulses);
  printf("MODE_REGISTER = %d\n", config.mode_register);
  printf("FP_FREQUENCY = %d\n", config.fp_frequency);
  printf("NB_OF_COLS = %d\n", config.nb_of_cols_to_read);
  printf("CHANNEL_MASK = %d\n", config.channel_mask);
  printf("PRETRIG_LSB = %d\n", config.pretrig_lsb);
  printf("PRETRIG_MSB = %d\n", config.pretrig_msb);
  printf("POSTTRIG_LSB = %d\n", config.posttrig_lsb);
  printf("POSTTRIG_MSB = %d\n", config.posttrig_msb);
  printf("PMT_SERIALS = %s %s %s %s\n", config.pmt_serials[0], config.pmt_serials[1],
                                        config.pmt_serials[2], config.pmt_serials[3]);
  }

  uint32_t post_trig; // Post trigger value
  unsigned int trig_rec; //Helps determine the trigger's position in the acquisition window
  
  //Looping variables
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
  FILE *data_files[4];
  FILE *conf_file;
  char data_filename0[MAX_STRING_LENGTH]; 
  char data_filename1[MAX_STRING_LENGTH]; 
  char data_filename2[MAX_STRING_LENGTH]; 
  char data_filename3[MAX_STRING_LENGTH]; 
  char conf_filename[MAX_STRING_LENGTH]; 

  sprintf(conf_filename, "%s/%s_%05d/%s_%05d_%s.conf", config.output_folder, 
          config.mode, config.run_num, config.mode, config.run_num, config.tag);

  printf("New configuration filename: %s\n", conf_filename);

  if (!doesFileExist(conf_filename))
  {
    conf_file = fopen(conf_filename, "w+b");
    save_config(&config, conf_file);
  }
  else {
    printf("Warning! Configuration file with run number %d and tag %s already exists!\n", 
            config.run_num, config.tag);
    exit(1);
  }
  fclose(conf_file);

  sprintf(data_filename0, "%s/%s_%05d/%s_%05d_%s_pmt0.dat", config.output_folder,  
          config.mode, config.run_num, config.mode, config.run_num, config.tag);
  data_files[0] = fopen(data_filename0, "w+b");

  sprintf(data_filename1, "%s/%s_%05d/%s_%05d_%s_pmt1.dat", config.output_folder,  
          config.mode, config.run_num, config.mode, config.run_num, config.tag);
  data_files[1] = fopen(data_filename1, "w+b");

  sprintf(data_filename2, "%s/%s_%05d/%s_%05d_%s_pmt2.dat", config.output_folder,  
          config.mode, config.run_num, config.mode, config.run_num, config.tag);
  data_files[2] = fopen(data_filename2, "w+b");

  sprintf(data_filename3, "%s/%s_%05d/%s_%05d_%s_pmt3.dat", config.output_folder,  
          config.mode, config.run_num, config.mode, config.run_num, config.tag);
  data_files[3] = fopen(data_filename3, "w+b");

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
    reset_vme();
    CAENVME_End(handle);
    return 0;
  }

  // Set Parameters
  ret = set_parameters(config); 
  if (ret != cvSuccess) {
    printf("Setting run paramaters failed with error %d \n", ret);
    CAENVME_End(handle);
    return 0;
  }

  //Get Post-Trig
  ret = read_from_vme(V1729_POSTTRIG_LSB);
  if (ret != cvSuccess) {
    reset_vme();
    CAENVME_End(handle);
    printf(" Loading POSTTRIG_LSB failed with error %d\n", ret);
    return 0;
  }
  post_trig = vme_data & 0xff;

  ret = read_from_vme(V1729_POSTTRIG_MSB);
  if (ret != cvSuccess) {
    reset_vme();
    CAENVME_End(handle);
    printf(" Loading POSTTRIG_MSB failed with error %d\n", ret);
    return 0;
  }
  post_trig += (vme_data & 0xff)*256;
  
  // Calibration & Pedestal Correction
  // Vernier Calibration
  if (vernier(MAXVER, MINVER) != cvSuccess) {
    printf("Failed vernier calibration!\n");
    reset_vme();
    CAENVME_End(handle);
    return 0;
  }


  // Get Pedestals
  if (get_pedestals(pedestals, buffer32, buffer16, mean_pedestal) == 0) {
    printf("Failed to get pedestals!\n");
    reset_vme();
    CAENVME_End(handle);
    return 0;
  }

  printf("Mean pedestal for Ch. 0: %f\n", mean_pedestal[0]);
  printf("Mean pedestal for Ch. 1: %f\n", mean_pedestal[1]);
  printf("Mean pedestal for Ch. 2: %f\n", mean_pedestal[2]);
  printf("Mean pedestal for Ch. 3: %f\n", mean_pedestal[3]);

  // Pedestals mus tbe calculated before attaching a signal for best results 
  //printf("Please now attach your signal to the board. Press ENTER when ready.\n");
  //while(getchar() != '\n');
  ret = start_acq();
  if (ret != cvSuccess) {
    printf("Start Acquisition failed with error %d\n", ret);
    reset_vme();
    CAENVME_End(handle);
    return 0;
  }

  while (interrupts < num_acquisitions) {
    wait_for_interrupt();
    interrupts++;
    
    ret = read_vme_ram(buffer32);
    if (ret != cvSuccess) {
      printf("Reading VME RAM failed with error %d\n", ret);
      reset_vme(); 
      CAENVME_End(handle);
      return 0;
    }

    ret = read_from_vme(V1729_RAM_DATA_VME); //Read TRIG_REC from RAM to 
                                             //automatically restart acq.
    if (ret != cvSuccess) {
      printf("Read TRIG_REC from RAM failed with error %d\n", ret);
      reset_vme(); 
      CAENVME_End(handle);
      return 0;
    }
    trig_rec = vme_data&0xff;
//  ret = read_from_vme(V1729_TRIG_REC);

    mask_buffer(buffer32, buffer16);

    subtract_pedestals(buffer16, pedestals); 

    reorder(trig_rec, post_trig, MINVER, MAXVER, 
            buffer16, ch0, ch1, ch2, ch3);
      
    //Save to ASCII File
    //Catches overflow error by returning 1
    if (save_data(ch0, ch1, ch2, ch3, &config, data_files, integrate) == 1) {
      interrupts--;
      printf("Warning overflow detected! Pulse ignored!");
      continue;
    }
  }
  
  for (i = 0; i < 4; i ++){
    fclose(data_files[i]);
  }
  CAENVME_End(handle);
  printf("Closing board post-acquisition...\n ");
  return 0;
}
