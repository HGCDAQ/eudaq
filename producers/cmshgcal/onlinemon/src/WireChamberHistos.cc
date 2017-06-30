// -*- mode: c -*-

#include <TROOT.h>
#include "WireChamberHistos.hh"
#include "OnlineMon.hh"
#include "TCanvas.h"
#include <cstdlib>
#include <sstream>

WireChamberHistos::WireChamberHistos(eudaq::StandardPlane p, RootMonitor *mon)
  :_sensor(p.Sensor()), _id(p.ID()), _maxX(p.XSize()),  _maxY(p.YSize()), _wait(false),
  _XYmap(NULL){
    

  char out[1024], out2[1024];

  _mon = mon;

  // std::cout << "WireChamberHistos::Sensorname: " << _sensor << " "<< _id<<
  // std::endl;

  if (_maxX != -1 && _maxY != -1) {

    sprintf(out, "%s %i XY map", _sensor.c_str(), _id);
    sprintf(out2, "h_XYmap_%s_%i", _sensor.c_str(), _id);
    _XYmap = new TH2F(out2, out, 100, -50., 50., 100, -50., 50.);
    SetHistoAxisLabels(_XYmap, "X (mm)", "Y (mm)");
    

    // make a plane array for calculating e..g hotpixels and occupancy

    plane_map_array = new int *[_maxX];

    if (plane_map_array != NULL) {
      for (int j = 0; j < _maxX; j++) {
        plane_map_array[j] = new int[_maxY];
        if (plane_map_array[j] == NULL) {
          cout << "WireChamberHistos :Error in memory allocation" << endl;
          exit(-1);
        }
      }
      zero_plane_array();
    }

  } else {
    std::cout << "No max sensorsize known!" << std::endl;
  }


}

int WireChamberHistos::zero_plane_array() {
  for (int i = 0; i < _maxX; i++) {
    for (int j = 0; j < _maxY; j++) {
      plane_map_array[i][j] = 0;
    }
  }
  return 0;
}


void WireChamberHistos::Fill(const eudaq::StandardPlane &plane) {
  // std::cout<< "FILL with a plane." << std::endl;

  float xl = plane.GetPixel(0);
  float xr = plane.GetPixel(1);
  float yu = plane.GetPixel(2);
  float yd = plane.GetPixel(3);

  int good_xl = xl >= 0 ? 1 : 0;
  int good_xr = xl >= 0 ? 1 : 0;
  int good_yu = xl >= 0 ? 1 : 0;
  int good_yd = xl >= 0 ? 1 : 0;
  int good_x = (good_xl+good_xr) == 2 ? 1: 0;
  int good_y = (good_yu+good_yd) == 2 ? 1: 0;
  int good_all = (good_x+good_y) == 2 ? 1: 0;
  
  float x = (xr-xl)*0.2;
  float y = (yd-yu)*0.2;
  

  if (good_all) {
    std::cout<<"In Filling of 2D-Hists: x="<<x<<" y="<<y<<std::endl;
    _XYmap->Fill(x,y);
  }
  
}

void WireChamberHistos::Reset() {

  _XYmap->Reset();

  // we have to reset the aux array as well
  zero_plane_array();
}

void WireChamberHistos::Calculate(const int currentEventNum) {
  _wait = true;

  _wait = false;
}

void WireChamberHistos::Write() {

  _XYmap->Write();

  //std::cout<<"Doing WireChamberHistos::Write() before canvas drawing"<<std::endl;

  /*
  gSystem->Sleep(100);

  gROOT->SetBatch(kTRUE);
  TCanvas *tmpcan = new TCanvas("tmpcan","Canvas for PNGs",600,600);
  tmpcan->cd();
  tmpcan->Close();
  gROOT->SetBatch(kFALSE);


  */

  //std::cout<<"Doing WireChamberHistos::Write() after canvas drawing"<<std::endl;

}

int WireChamberHistos::SetHistoAxisLabelx(TH1 *histo, string xlabel) {
  if (histo != NULL) {
    histo->GetXaxis()->SetTitle(xlabel.c_str());
  }
  return 0;
}

int WireChamberHistos::SetHistoAxisLabels(TH1 *histo, string xlabel, string ylabel) {
  SetHistoAxisLabelx(histo, xlabel);
  SetHistoAxisLabely(histo, ylabel);

  return 0;
}

int WireChamberHistos::SetHistoAxisLabely(TH1 *histo, string ylabel) {
  if (histo != NULL) {
    histo->GetYaxis()->SetTitle(ylabel.c_str());
  }
  return 0;
}
