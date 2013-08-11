
//Necessary Steps:    
// 1. Loading and preparation of data
//   * Load in *Ch_0.dat file which contains values read from the ADC. 
//   * Integrate all this data using ????
//
//2. Set up canvas and line settings
//   
//3. Create and fill histogram
//
//4. Apply Gaussian Fit

void pmt_testing(char* input_file) {

  Double_t width = 1000;
  Double_t height = 1000;
  TCanvas * canvas = new TCanvas("canvas", "PMT Testing", 0, 0, width, height);
  canvas->SetWindowSize(width + (width - canvas->GetWw()), 
                        height + (height - canvas->GetWh()));

  //Parameters
  Int_t nbins = 40;
  Double_t min = 200000; //hist min/max values
  Double_t max = 300000; //Must be better way to get these!

  //Initialized variables
  Int_t num_data_points = 0;
  Int_t pulse;
  Int_t channel;
  Int_t total_channels;
  const Int_t pedestal = 2020; //how to determine this?

  TFile *file = new TFile("adc_data.root", "recreate"); 

  //Style settings
  gROOT->SetStyle("Plain");
  gStyle->SetPalette(53);
  gStyle->SetOptStat(111111);
  gStyle->SetOptFit(1111);
  gStyle->SetStatBorderSize(0);
  gStyle->SetOptTitle(1); 

  
  //Prepare Data
  //Create nTuple
  TTree *t1 = new TTree("t1", "ADC Data");
  t1->Branch("pulse", &pulse, "pulse/I");
  t1->Branch("channel", &channel, "channel/I");

 
  ifstream in;
  in.open(input_file);
  //Fill TTree
  while ( in >> channel) {
    if (num_data_points % 2520 == 0)
      pulse++;  
    t1->Fill();
    num_data_points++;
  }
  in.close();

  TH1F * spectrum = new TH1F("spectrum", "ADC Spectrum; Channels; Counts", nbins, min, max); 
  //spectrum->SetBit(TH1::kCanRebin);
  spectrum->Sumw2(); 
  for(Int_t i = 0; i < t1->GetEntries(); i++) {
    t1->GetEntry(i);
    if (i % 2520 == 0) {
      cout << "total_channels = " << total_channels << endl;
      spectrum->Fill(total_channels);
      total_channels = 0;
    }
    total_channels += TMath::Abs(channel - pedestal);
  }

  spectrum->SetLineWidth(2);
  spectrum->SetMarkerColor(kBlack);
  spectrum->SetLineColor(kBlack);

  // Fit Gaussian to curve and Draw
  spectrum->Fit("gaus");
  spectrum->GetFunction("gaus")->SetLineColor(kRed);
  spectrum->Draw("Same");
  t1->Write();
  spectrum->Write();
}


