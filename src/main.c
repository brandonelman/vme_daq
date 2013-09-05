#define NBINS 1000
#define LOWER_LIM_X 0
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
#include <iostream>
#ifndef LINUX
#define LINUX 1
#endif

#include "V1729.h"
#include "V812.h"
#include "CAENVMElib.h"

#include "TCanvas.h"
#include "TApplication.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TROOT.h"
#include "TStyle.h"

using namespace std;
void subtract_pedestals(unsigned int buffer16[V1729_RAM_DEPH], int pedestals[V1729_RAM_DEPH]) {
  //Subtracts the value in "pedestals" from buffer16. In actuality pedestals
  //is an array containing the variance of the pedestals. 
 
  int i;
  int buffer;
  int ch;
  for (i = 0; i < 2560; i++)
    for (ch = 0; ch < 4; ch++) {
      buffer = (int)(0xffff&buffer16[12 + i*4 + ch]);
      if( (buffer - pedestals[i*4 + ch]) < 0) buffer16[12+i*4+ch] = 0;
      else buffer16[12+i*4+ch] = (unsigned int)(buffer - pedestals[i*4 + ch]);
    }
  return;
}

CVErrorCodes set_parameters (uint32_t trig_lev) {
  CVErrorCodes ret;
 
  // Writing to TRIGGER_CHANNEL_SRC enables TRIGGER
  // to be sent on all four chnanels simultaneously.
  write_to_vme(V1729_TRIGGER_CHANNEL_SRC, 0xf);
  write_to_vme(V1729_LOAD_TRIGGER_THS, 1);

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
  ret = write_to_vme(V1729_MODE_REGISTER, 0x3); //0x3 == 0b11 -> 14Bit and IRQ
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

  //Channel Mask determine numbers of active channels ret = write_to_vme(V1729_CHANNEL_MASK, 0xf); 
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
  ret = write_to_vme(V1729_POSTTRIG_LSB, 0x40); 
  //ret = write_to_vme(V1729_POSTTRIG_LSB, 0x1e); 
  if (ret != cvSuccess) {
    printf("Setting POSTTRIG_LSB failed with error %d \n", ret);
    return ret;
  }

  ret = write_to_vme(V1729_POSTTRIG_MSB, 0x0); 
  if (ret != cvSuccess) {
    printf("Setting POSTTRIG_MSB failed with error %d \n", ret);
    return ret;
  }

  // Trigger Type
  ret = write_to_vme(V1729_TRIGGER_TYPE, 0x1); // Trigger on Pulse based on TRIG_LEV
  //ret = write_to_vme(V1729_TRIGGER_TYPE, 0x2); // Trigger on EXT TRIG Input Rising Edge
  //ret = write_to_vme(V1729_TRIGGER_TYPE, 0x6); // Trigger on EXT TRIG Input Rising Edge
  if (ret != cvSuccess) {
    printf("Setting TRIGGER_TYPE failed with error %d \n", ret);
    return ret;
  }

//ret = set_channel_threshold(0, 50);
//if (ret != cvSuccess) {
//  printf("Setting TRIGGER_TYPE failed with error %d \n", ret);
// return ret;
//}
  return ret;
}

int32_t handle;
uint32_t vme_data;

int adc_spectrum() {

  //Variables for TTree
  int channel;
  int pulse;

  char fn[100];
 
  CVBoardTypes vme_board = cvV2718;
  CVErrorCodes ret; // Error Codes for Debugging
  int value; // Error Codes for functions which return ints rather than CVErrorCodes

  //Parameters
  // Let x be the desired triggering threshold value. 
  // (x+1000)*((16^3)/2000) = trig_lev
  uint32_t trig_lev = 0x3ff; // -250 mV Threshold
  int num_acquisitions = 1000; // Number of times to loop acquisition
  int num_quadrants = 1;  // Number of times to repeat num_acquisitions
  
  uint32_t active_channel; // active number of channels (0b1111 means all 4 channels)
  uint32_t num_columns; // number of columns to read from MATACQ matrix
  uint32_t post_trig;  // Post trigger value
  unsigned int trig_rec;//Helps determine the trigger's position in the acquisition window
  int mask;
  
  //Looping variables
  int cur_quadrant;
  int ch;
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
  char key; //Used to pause after pedestal correction to attach signal to board

  //TFile *dark_file = new TFile("analysis/dark_run.root", "read"); 
  //TTree *dark_tree = (TTree*)dark_file->Get("t1");
  //TH1F *dark = new TH1F("dark","",NBINS,LOWER_LIM_X,UPPER_LIM_X);
  
  //Add Manually
  //dark_tree->Draw("channel>>dark");

  //dark_file->Close();
  
  //  t1->Draw("channel>>spectrum(1000,0,9000)");
  //dark_file->ls();
  //TH1F *dark = NULL;
  //dark_file->GetObject("htemp",dark);

  //TH1F * dark = (TH1F*)dark_file->Get("htemp");

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
  ret = set_parameters(trig_lev); if (ret != cvSuccess) {
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

  //Get Channel Mask
  ret = read_from_vme(V1729_CHANNEL_MASK);
  if (ret != cvSuccess) {
    printf(" Loading CHANNEL_MASK failed with error %d\n", ret);
    return 0;
  }

  //Get Post-Trig
  ret = read_from_vme(V1729_POSTTRIG_LSB);
  if (ret != cvSuccess) {
    printf(" Loading POSTTRIG_LSB failed with error %d\n", ret);
    return 0;
  }
  post_trig = vme_data&0xff;

  ret = read_from_vme(V1729_POSTTRIG_MSB);
  if (ret != cvSuccess) {
    printf(" Loading POSTTRIG_MSB failed with error %d\n", ret);
    return 0;
  }
  post_trig = post_trig + (vme_data&0xff)*256;
  
  // Calibration & Pedestal Correction
  // Vernier Calibration
  if (vernier(MAXVER, MINVER) != 1) {
    printf("Failed vernier calibration!\n");
    CAENVME_End(handle);
    return 0;
  }


  // Get Pedestals
  // More accurately, it finds the variance of the pedestals
  // for each channel and subtracts it
  if (get_pedestals(pedestals, buffer32, buffer16, mean_pedestal) == 0) {
    printf("Failed to get pedestals!\n");
    CAENVME_End(handle);
    return 0;
  }

  printf("Mean pedestal for Ch. 0: %f\n", mean_pedestal[0]);

  // Pedestals mus tbe calculated before attaching a signal for best results 
  printf("Please now attach your signal to the board. Press RETURN when read.\n");
  key = getchar();
  
  //Style Settings
  gROOT->SetStyle("Plain");
  gStyle->SetPalette(53);
  gStyle->SetOptStat(000000);
  gStyle->SetOptFit(1111);
  gStyle->SetStatBorderSize(0);
  gStyle->SetOptTitle(0);

  //Canvas Height/Width
  Double_t width = 1000;
  Double_t height = 1000;

  //Create Canvas
  TCanvas * canvas = new TCanvas("canvas", "PMT Testing", 0, 0, width, height);
  canvas->SetWindowSize(width + (width-canvas->GetWw()),
                        height + (height-canvas->GetWh()));
  canvas->Divide(1,1);


  for (cur_quadrant = 0; cur_quadrant < num_quadrants; cur_quadrant++) {
    //TTree for storing data in a root file
    TTree *t1 = new TTree("t1", "ADC Data");
    t1->Branch("pulse", &pulse, "pulse/I"); 
    t1->Branch("channel", &channel, "channel/I");

    while (interrupts <= num_acquisitions) {

      //Start Acquisition
      ret = start_acq();
      if (ret != cvSuccess) {
        printf("Start Acquisition failed with error %d\n", ret);
        return 0;
      }

      value = wait_for_interrupt();
      if (value == 0) {
        return 0; //Timeout Error
      }
  
      interrupts++;
      
      //After receiving interrupt must acknowledge by writing 0 in interrupt register
      ret = write_to_vme(V1729_INTERRUPT, 0);
      if (ret != cvSuccess) {
        printf("Acknowleding interrupt failed with error %d\n", ret);
        return 0;
      }

      //Read RAM To get data
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

      value = mask_buffer(buffer32, buffer16);
      if (value == 0) {
        printf("Masking Buffer Failed!");
        return 0;
      }

      //subtract pedestals
      subtract_pedestals(buffer16, pedestals); 

      //Reorder Data
      value = reorder(trig_rec, post_trig, num_columns, MINVER, MAXVER, 
              buffer16, ch0, ch1, ch2, ch3);
      
      if (value == 2) { 
        printf("Overflow detected!");
        interrupts--;
        continue;
      }

      //Save to ASCII File
      //save(ch0, ch1, ch2, ch3, cur_position);

      pulse = interrupts - 1;  
      for (i = 40; i < 2560; i++) {
        channel = ch0[i];
        printf("Ch0[i] = %d\n", ch0[i]);
        t1->Fill();
      }
    }

    //PLOTTING
    int size_of_array = t1->GetEntries()/2520; 
    
    //Initialized Min/Max. Actual values found later.
    int min = 99999999;
    int max = 0;
    
    sprintf(fn, "analysis/adc_spectrum_%d.root", cur_quadrant);
    TFile *file = new TFile(fn, "recreate");
    t1->Write(); //Write To File

    canvas->cd(1);
    gPad->SetLogy();
    TH1F *spectrum = new TH1F("spectrum","",NBINS,LOWER_LIM_X,UPPER_LIM_X);
    //t1->Draw("channel>>spectrum(1000,0,9000)");
    t1->Draw("channel>>spectrum");
    //spectrum->Add(dark, -1); 
    //cout << spectrum->GetNbinsX();
    //cout << dark->GetNbinsX();
    spectrum->Draw("Same");

    canvas->Update(); //Draw on Canvas
    file->Close();
    delete(t1);
    printf("Change testing quadrant. Press enter when ready for another acquisition.\n");
  
    key = getchar();
    interrupts = 0;
    canvas->Clear();
  }

  printf("Closing board post-acquisition...\n ");
  CAENVME_End(handle);
  return 1;
}

#ifndef __CINT__
void StandaloneApplication(int argc, char **argv){
  adc_spectrum();
}

int main(int argc, char **argv){
  TApplication app("Root application", &argc, argv);
  StandaloneApplication(app.Argc(), app.Argv());
  app.Run();
  return 0;
}
#endif //__CINT__
