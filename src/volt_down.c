#include "V6521M.h"

int main(int argc, char *argv[]){
  
  int32_t handle;
  int i;
  CVBoardTypes vme_board = cvV2718;
  CVErrorCodes ret;

  ret = CAENVME_Init(vme_board, 0, 0, &handle);
  if (ret  != cvSuccess) {
    printf("\n\n Error Opening VX2718! \n\n");
    CAENVME_End(handle);
    return 1;
  }
  
  /* Set Channel Voltages */
  for (i = 0; i < 6; i++){
    ret = set_voltage(i, 0);
    if (ret  != cvSuccess) {
      printf("Error %d when setting ch %d voltage!\n", ret, i);
      CAENVME_End(handle);
      return 1;
    }

    ret = disable_channel(i);
    if (ret  != cvSuccess) {
      printf("Error %d when disabling ch %d!\n", ret, i);
      CAENVME_End(handle);
      return 1;
    }
  }

  CAENVME_End(handle);
  return 0;
}

