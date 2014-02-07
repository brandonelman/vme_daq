#include "V6521M.h"
#define CURRENT 20
int main(int argc, char *argv[]){
  
  if (argc < 6) {
    printf("Usage: volt_up PMT0_VOLTAGE PMT1_VOLTAGE PMT2_VOLTAGE PMT3_VLTAGE");
    return 1;
  }

  int volts_ch0 = atoi(argv[1]);
  int volts_ch1 = atoi(argv[2]);
  int volts_ch2 = atoi(argv[3]);
  int volts_ch3 = atoi(argv[4]);
  int volts_ch4 = atoi(argv[5]);
  int volts_ch5 = atoi(argv[6]);

  int32_t handle;
  int ch;
  CVBoardTypes vme_board = cvV2718;
  CVErrorCodes ret;

  ret = CAENVME_Init(vme_board, 0, 0, &handle);
  if (ret  != cvSuccess) {
    printf("\n\n Error Opening VX2718! \n\n");
    CAENVME_End(handle);
    return 1;
  }

  for (ch = 0; ch < NUM_CHANNELS; ch++){
    ret = set_svmax(ch, MAX_VOLTAGE);
    if (ret  != cvSuccess) {
      printf("Error %d when Setting SVMAX!\n", ret);
      CAENVME_End(handle);
      return 1;
    }

    ret = ramp_down_channel(ch); /*Sets POWERDOWN mode to RAMP_DOWN*/
    if (ret  != cvSuccess) {
      printf("Error %d when Setting RAMP_DOWN at power down!\n", ret);
      CAENVME_End(handle);
      return 1;
    }

    ret = set_ramp_down(ch, RAMP_SPEED);
    if (ret  != cvSuccess) {
      printf("Error %d when Setting RAMP_DOWN rate!\n", ret);
      CAENVME_End(handle);
      return 1;
    }

    ret = set_ramp_up(ch, RAMP_SPEED);
    if (ret  != cvSuccess) {
      printf("Error %d when Setting RAMP_UP rate!\n", ret);
      CAENVME_End(handle);
      return 1;
    }

    ret = set_current(ch, CURRENT);
    if (ret  != cvSuccess) {
      printf("Error %d when Setting current!\n", ret);
      CAENVME_End(handle);
      return 1;
    }
  }

  /* Set Channel Voltages */
  ret = set_voltage(0, volts_ch0);
  if (ret  != cvSuccess) {
   printf("Error %d when setting ch 0 voltage!\n", ret);
   CAENVME_End(handle);
   return 1;
  }
  ret = set_voltage(1, volts_ch1);
  if (ret  != cvSuccess) {
   printf("Error %d when setting ch 1 voltage!\n", ret);
   CAENVME_End(handle);
   return 1;
  }

  ret = set_voltage(2, volts_ch2);
  if (ret  != cvSuccess) {
   printf("Error %d when setting ch 2 voltage!\n", ret);
   CAENVME_End(handle);
   return 1;
  }

  ret = set_voltage(3, volts_ch3);
  if (ret  != cvSuccess) {
   printf("Error %d when setting ch 3 voltage!\n", ret);
   CAENVME_End(handle);
   return 1;
  }  

  ret = set_voltage(4, volts_ch4);
  if (ret  != cvSuccess) {
   printf("Error %d when setting ch 4 voltage!\n", ret);
   CAENVME_End(handle);
   return 1;
  }  
  
  ret = set_voltage(5, volts_ch5);
  if (ret  != cvSuccess) {
   printf("Error %d when setting ch 5 voltage!\n", ret);
   CAENVME_End(handle);
   return 1;
  }

  /* Enable Channels 0 (negative, witness ) and 3,4,5 (positive) */
  ret = enable_channel(0);  
  if (ret  != cvSuccess) {
   printf("Error %d when enabling channel 0!\n", ret);
   CAENVME_End(handle);
   return 1;
  }

  ret = enable_channel(1);  
  if (ret  != cvSuccess) {
   printf("Error %d when enabling channel 0!\n", ret);
   CAENVME_End(handle);
   return 1;
  }  
  ret = enable_channel(2);  
  if (ret  != cvSuccess) {
   printf("Error %d when enabling channel 0!\n", ret);
   CAENVME_End(handle);
   return 1;
  }
  ret = enable_channel(3);  
  if (ret  != cvSuccess) {
   printf("Error %d when enabling channel 3!\n", ret);
   CAENVME_End(handle);
   return 1;
  } 
  ret = enable_channel(4);  
  if (ret  != cvSuccess) {
   printf("Error %d when enabling channel 4!\n", ret);
   CAENVME_End(handle);
   return 1;
  }
  
  ret = enable_channel(5);  
  if (ret  != cvSuccess) {
   printf("Error %d when enabling channel 5!\n", ret);
   CAENVME_End(handle);
   return 1;
  }
  CAENVME_End(handle);
  return 0;
}

