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


void read_vme(int32_t handle, cur_status *status)
{
  uint32_t i, old_data = 0;
  CVErrorCodes ret, old_ret=cvSuccess;

  if (status->num_cyc == 0)
    printf("Running.. any key to stop.");

  for (i=0; ( (status->num_cyc == 0) || (i < status->num_cyc) ) && !kbhit(); i++)
  {
    ret = CAENVME_ReadCycle(handle, status->addr, &status->data, status->am, status->dtsize); 

    if((i==0) || (ret != old_ret))
    {
      switch (ret)
      {
        case cvSuccess   : printf(" Cycle(s) completed normally\n");
                           if( (i==0) || (old_data != status->data) )
                           {
                             if ( status->dtsize == cvD32 )
                             printf("Data Read: %08X", status->data); 

                             if ( status->dtsize == cvD16 )
                             printf("Data Read: %04X", status->data); 

                             if ( status->dtsize == cvD8 )
                             printf("Data Read: %02X", status->data); 
                           }
                           break ;

        case cvBusError  : printf(" Bus Error !!!");
                           break ;
        case cvCommError : printf(" Communication Error !!!");
                           break ;
        default          : printf(" Unknown Error !!!");
                           break ;
      }

    } 

    old_data = status->data;
    old_ret = ret;

    if (status->autoinc)
    {
      status->addr += status->dtsize;
    }
  }
}

void write_vme(int32_t handle, cur_status *status)
{
  uint32_t i;
  CVErrorCodes ret, old_ret=cvSuccess;
 
  if (status->num_cyc == 0)
    printf("Running.. any key to stop.");

  for (i=0; ( (status->num_cyc == 0) || (i < status->num_cyc) ) && !kbhit(); i++) 
  {
    ret = CAENVME_WriteCycle(handle, status->addr, &status->data, status->am, status->dtsize);

    if((i==0) || (ret != old_ret))
    {
      switch (ret)
      {
        case cvSuccess   : printf(" Cycle(s) completed normally\n");
                           break ;
        case cvBusError  : printf(" Bus Error !!!");
                           break ;
        case cvCommError : printf(" Communication Error !!!");
                           break ;
        default          : printf(" Unknown Error !!!");
                           break ;
      }
    }

    old_ret = ret;
    if(status->autoinc)
    {
      status->addr += status->dtsize;
    }
  }
}

void reset(int32_t handle, cur_status *status)
{
  status->addr = status->base_addr + V1729_RESET_BOARD;
  status->data = 1;

  write_vme(handle, &status); 
}

void vernier(int32_t handle, cur_status *status)
{
  int i, tp, col, mask;

  reset(handle, &status);

  status->addr = status->base_addr + V1729_TRIGGER_TYPE;
  read_vme(handle, &status);
  tp = status->data&0x3f;
  
  status->data = 0x8;
  write_vme(handle, &status); 

  status->addr = status->base_addr + V1729_NB_OF_COLS_TO_READ;
  read_vme(handle, &status);
  col = status->data&0xff;

  status->data = 0;
  write_vme(handle, &status);

  status->addr = status->base_addr + V1729_CHANNEL_MASK;
  mask = status->data&0xf;
  status->data = 15;
  write_vme(handle, &status);

  start_acq(handle, &status);
  wait(handle, &status);
  
  for(i=0; i<4; i++)
  {
    MAXVER[i] = 0;
    MINVER[i] = 0xffff;
  }

  for(i=0; i<1000; i++)
  {
    status->addr = status->base_addr + V1729_RAM_DATA_VME;

    if( (status->data>>16) < MINVER[3] ) MINVER[3] = (status->data>>16);
    if( (status->data>>16) > MAXVER[3] ) MAXVER[3] = (status->data>>16);
    if( (status->data&0xffff) < MINVER[2] ) MINVER[2] = (status->data&0xffff);
    if( (status->data&0xffff) > MAXVER[2] ) MAXVER[2] = (status->data&0xffff);


    status->addr = status->base_addr + V1729_RAM_DATA_VME;

    if( (status->data>>16) < MINVER[3] ) MINVER[3] = (status->data>>16);
    if( (status->data>>16) > MAXVER[3] ) MAXVER[3] = (status->data>>16);
    if( (status->data&0xffff) < MINVER[2] ) MINVER[2] = (status->data&0xffff);
    if( (status->data&0xffff) > MAXVER[2] ) MAXVER[2] = (status->data&0xffff);
  }

  status->addr = status->base_addr + V1729_TRIGGER_TYPE;
  status->data = tp;
  write_vme(handle, &status);

  status->addr = status->base_addr + V1729_NB_OF_COLS_TO_READ;
  status->data = col;
  write_vme(handle, &status);

  status->addr = status->base_addr + V1729_CHANNEL_MASK;
  status->data = mask;
  write_vme(handle, &status);
}

void pedestal(int32_t handle, cur_status *status)
{
  int i, j, k, ch, tp, col, mask;
  float meanpedestal[4];
  meanpedestal[0] = 0;
  meanpedestal[1] = 0;
  meanpedestal[2] = 0;
  meanpedestal[3] = 0;

  status->addr = status->base_addr + V1729_TRIGGER_TYPE;
  read_vme(handle, &status);
  tp = status->data&0x3f;
  
  status->data = 0x8;
  write_vme(handle, &status); 

  status->addr = status->base_addr + V1729_NB_OF_COLS_TO_READ;
  read_vme(handle, &status);
  col = status->data&0xff;

  status->data = 128;
  write_vme(handle, &status);

  status->addr = status->base_addr + V1729_CHANNEL_MASK;
  mask = status->data&0xf;
  status->data = 15;
  write_vme(handle, &status);

  for (k = 0; k < 10252; k++) pattern[k] = 0;

  for (i = 0; i < 50; i++)
  {
    start_acq(handle, &status);
    wait(handle, &status);
    read_vme_ram(handle, &status);
    mask_buffer(handle, &status);

    for (j = 0; j < 2560; j++)
      for (ch = 0; ch < 4; ch++)
        pattern[j*4+ch] = pattern[j*4+ch] + buffer16[12+j*4+ch];
  }

  for (j = 0; j < 2560; j++)
    for (ch = 0; ch < 4; ch++)
      {
        pattern[j*4+ch] = pattern[j*4+ch] / (50);
        meanpedestal[ch] =meanpedestal[ch] + pattern[j*4+ch];
      }

  for (ch = 0; ch < 4; ch++) meanpedestal[ch] = meanpedestal[ch]/2560;

  for (k = 0; k < 2560; k++)
    for (ch = 0; ch < 4; ch++)
      pattern[k*4+ch] = pattern[k*4+ch] - (int)meanpedestal[ch];

  status->addr = status->base_addr + V1729_TRIGGER_TYPE;
  status->data = tp;
  write_vme(handle, &status);

  status->addr = status->base_addr + V1729_NB_OF_COLS_TO_READ;
  status->data = col;
  write_vme(handle, &status);

  status->addr = status->base_addr + V1729_CHANNEL_MASK;
  status->data = mask;
  write_vme(handle, &status);
}

int wait(int32_t handle, cur_status *status)
{
  unsigned int w = 0;
  unsigned int Interrupt = 0;

  status->addr  =  status->base_addr + V1729_INTERRUPT;
  while (Interrupt == 0)
    {
    w++;
    read_vme(handle, &status);
    Interrupt = status->data & 1;
    if (w > 0x1fff) return 0; /* time-out */
  }
return 1;
}


void start_acq(int32_t handle, cur_status *status)
{
   status->addr = status->base_addr + V1729_START_ACQUISITION;
   status->data = 0x00000001;

   write_vme(handle, &status);

}

void write_blt(int32_t handle, cur_status *status)
{
  int nb;
  uint32_t i, incr;
  CVAddressModifier am;
  CVErrorCodes ret, old_ret=cvSuccess;

  //Get Data to Write
  printf("First Data [hex] : \n");
  scanf("%x", &status->data);

  //Get Increment for Data
  printf("Data Increment [hex] : \n");
  scanf("%x", &incr);

  //Fill the Data Buffer
  for(i=0; i < (status->blts/4); i++)
    status->buff[i] = status->data + incr*i;

  if (status->dtsize == cvD64)
  {
    if (status->am == cvA24_U_DATA)
      am = cvA24_U_MBLT;
    else
      am = cvA32_U_MBLT;
  }

  else
  {
    if (status->am == cvA24_U_DATA)
      am = cvA24_U_BLT;
    else
      am = cvA32_U_BLT;
  }

  if(status->num_cyc == 0)        // Infinite Loop
    printf(" Running ...    Press any key to stop.");

  for (i=0; ( (status->num_cyc==0) || (i < status->num_cyc) ) && !kbhit(); i++)
  {

    ret = CAENVME_BLTWriteCycle(handle,status->addr,(char *)status->buff,status->blts,am,status->dtsize,&nb);

    if( (i==0) || (ret != old_ret) )
      {
        switch (ret)
        {
          case cvSuccess   : printf(" Cycle(s) completed normally\n");
                             printf(" Written %u bytes",nb);
                             break ;
          case cvBusError  : printf(" Bus Error !!!\n");
                             printf(" Written %u bytes",nb);
                             break ;
          case cvCommError : printf(" Communication Error !!!");
                             break ;
          default          : printf(" Unknown Error !!!");
                             break ;
        }
      }

    old_ret = ret;

   }
}

void read_blt(int32_t handle, cur_status *status)
{
  int nb;
  uint32_t i, j;
  CVAddressModifier am;
  CVErrorCodes ret, old_ret=cvSuccess;

  if (status->dtsize == cvD64)
  {
    if (status->am == cvA24_U_DATA)
      am = cvA24_U_MBLT;
    else
      am = cvA32_U_MBLT;
  }

  else
  {
    if (status->am == cvA24_U_DATA)
      am = cvA24_U_BLT;
    else
      am = cvA32_U_BLT;
  }

  if(status->num_cyc == 0)        // Infinite Loop
    printf(" Running ...    Press any key to stop.");

  for (i=0; ( (status->num_cyc==0) || (i < status->num_cyc) ) && !kbhit(); i++)
  {
    for (j = 0; j < (status->blts/4); j++)
      status->buff[j]=0;

    ret = CAENVME_BLTReadCycle(handle,status->addr,(char *)status->buff,status->blts,am,status->dtsize,&nb);

    if( (i==0) || (ret != old_ret) )
      {
        switch (ret)
        {
          case cvSuccess   : printf(" Cycle(s) completed normally\n");
                             printf(" Written %u bytes",nb);
                             break ;
          case cvBusError  : printf(" Bus Error !!!\n");
                             printf(" Written %u bytes",nb);
                             break ;
          case cvCommError : printf(" Communication Error !!!");
                             break ;
          default          : printf(" Unknown Error !!!");
                             break ;
        }
      }

    old_ret = ret;

   }
}

void view_blt_data(int32_t handle, cur_status *status)
{
  unsigned short i;
  unsigned short j;
  uint32_t ndata;
  uint32_t *d32;
  unsigned short *d16;
  unsigned char *d8;
  char msg[80];
  FILE *fsave;

  d32 = status->buff;
  d16 = (unsigned short *)status->buff;
  d8 = (unsigned char *)status->buff;
  CVDataWidth dtsize = status->dtsize;
  ndata = status->blts/dtsize;

  printf("Enter file name for BLT data : \n");

  if ( scanf("%s", msg) == EOF)
    return;

  if ( (fsave = fopen(msg, "w")) == NULL )
    printf("Can't open file!");

  for(j = 0; j < ndata; j++)
  {
    if( dtsize == cvD32 || dtsize == cvD64 )
      fprintf(fsave,"%05u\t%08X\t%-10d\n",j,d32[j],d32[j]);
    if( dtsize == cvD16 )
      fprintf(fsave,"%05u\t%04X\t%-6d\n",j,d16[j],d16[j]);
    if( dtsize == cvD8 )
      fprintf(fsave,"%05u\t%02X\t%-4d\n",j,d8[j],d8[j]);
  }      

  printf("Successfully saved BLT Data");
}
//---------------------------------------------------------------
void mask_buffer(int32_t handle, cur_status *status)
{
int i,mask;

  status->addr = status->base_addr + V1729_MODE_REGISTER;
  read_vme(handle, &status);
  if( (status->data & 0x2) == 2 ) mask = 0x3fff; /* 14 bit */
  else mask = 0xfff;/* 12 bit */

  for (i = 0; i < V1729_RAM_DEPH; i=i+2) 
  { 
    buffer16[i+1] = mask & buffer32[i/2];
    buffer16[i] =   mask & (buffer32[i/2]>>16);
 	}    
}

//---------------------------------------------------------------
void read_vme_ram(int32_t handle, cur_status *status)
{
  status->addr = status->base_addr + V1729_RAM_DATA_VME;

  read_blt(handle, &status);

  status->addr = status->base_addr + V1729_TRIG_REC;
  read_vme(handle, &status);
  trig_rec = status->addr & 0xFF;

}

//--------------------------------------------------------------
void reorder(void)
{
	int i,j,end_cell,NoC,cor_ver = 0;
	float ver[4];

  for (i = 0; i < 4; i++)
  {
    ver[i] = (float)(buffer16[1+3-i]-MINVER[i])/(float)(MAXVER[i]-MINVER[i]);
    ;
    cor_ver = cor_ver + (int)(20*ver[i]/4);
  }

  end_cell = ( 20*(128-(trig_rec)+ post_trig) +1) % 2560;

  NoC = num_cols * 20;
  for (i = 0; i < NoC; i++)
	{
    j =  (NoC + i + end_cell - cor_ver+20) % NoC;
		
    ch3[i] = buffer16[4*j+12];
    ch2[i] = buffer16[4*j+13];
    ch1[i] = buffer16[4*j+14];
    ch0[i] = buffer16[4*j+15];
	}

}


//---------------------------------------------------------------
void save(int32_t handle, cur_status *status)
{
  FILE *ch[4]; 

  int channel_mask, i, NoC;
  char s[30];
  NoC = num_cols * 20;
  
  status->addr = status->base_addr + V1729_CHANNEL_MASK; 
  read_vme(handle, &status);
  channel_mask = status->data & 0xf;


  if(channel_mask&0x1)
  {
    ch[0] =fopen("Ch_0.dat","w+b");

	  for (i=40;i<NoC;i++)
    {
      sprintf(s,"%d\n",ch0[i]);
    	fwrite(s,1,strlen(s),ch[0]);
    }

    fclose(ch[0]);
  }


  if(channel_mask&0x2)
  {
    ch[1] =fopen("Ch_1.dat","w+b");

    for (i=40;i<NoC;i++)
    {
      sprintf(s,"%d\n",ch1[i]);
      fwrite(s,1,strlen(s),ch[1]);
    }

    fclose(ch[1]);
  }

  if(channel_mask&0x4)
  {
    ch[2] =fopen("Ch_2.dat","w+b");

    for (i=40;i<NoC;i++)
    {
      sprintf(s,"%d\n",ch2[i]);
      fwrite(s,1,strlen(s),ch[2]);
    }

    fclose(ch[2]);
  }

  if(channel_mask&0x8)
  {
    ch[3] =fopen("Ch_3.dat","w+b");
	  
    for (i=40;i<NoC;i++)
    {
      sprintf(s,"%d\n",ch3[i]);
	    fwrite(s,1,strlen(s),ch[3]);
    }

    fclose(ch[3]);
  }
}
