
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


int main(int argc, void *argv[])
{
  CVBoardTypes vme_board = cvV2718; //Type of Board
  short link = 0; //Link number 
  short device = 0; //Device Number
  int32_t handle; //Handle used for interacting with board
  CVErrorCodes ret; //Stores Error Codes for Debugging
  uint32_t vme_addr; //Address to Access VME Registers
  uint32_t vme_data; //Data holder for writing and reading
  
  uint32_t trig_lev = 0x6ff; 

  /* Parameters */
  unsigned short addr_mode = cvA32_U_DATA;
  unsigned short num_cycles = 1;
  CVDataWidth data_size = cvD32;  

  /* Create opaque handle for interacting with VX2718 Board */
  if ( CAENVME_Init(vme_board, linkm device, &handle) != cvSuccess )
  {
    printf("\n\n Error opening VX2718\n");
    exit(1);
  }

  else printf("Successfully opened VX2718!\n"); 


  /* Send Reset Order to Board  */
  printf("Attempting to reset board.");
  vme_addr = CAENBASEADDRESS + V1729_RESET_BOARD;
  vme_data = 1;
  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
  if (ret != cvSuccess)
  {
    printf("Reset failed with error %d", ret);
  }  

  else printf("Reset succesfully");

  /* Set Trigger Threshold */
  printf("Attempting to set trigger level");

  vme_addr = CAENBASEADDRESS + V1729_THRESHOLD;
  vme_data = trig_lev;
  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
  if (ret != cvSuccess)
  {
    printf("Setting trigger level failed with error %d", ret);
  }  

  else printf("Trigger level set succesfully");

  /*From Manual: After loading of V1729_THRESHOLD, one must transfer the
    value in the analog converter via the LOAD_TRIGGER THRESHOLD DAC (09)
    command.*/
  printf("Attempting to enact LOAD_TRIGGER_THRESHOLD DAC");
  vme_addr = CAENBASEADDRESS + V1729_LOAD_TRIGGER_THS;
  vme_data = 1;
  ret = CAENVME_WriteCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 
  if (ret != cvSuccess)
  {
    printf("Loading trigger threshold command failed with error: %d", ret);
  }  

  else printf("Load Trigger threshold command successful");

  printf("Attempting to load pilot frequency"); 
  vme_addr = CAENBASEADDRESS + V1729_FP_FREQUENCY;

  ret = CAENVME_ReadCycle(handle, vme_addr, &vme_data, addr_mode, data_size); 

  if (ret != cvSuccess)
  {
    printf("Loading pilot frequency failed with error: %d", ret);
  }  

  else printf("Loading pilot frequency successful");

}
