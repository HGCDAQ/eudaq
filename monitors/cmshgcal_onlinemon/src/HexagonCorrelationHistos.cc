// -*- mode: c -*-

#include <TROOT.h>
#include "HexagonCorrelationHistos.hh"
#include "OnlineMon.hh"
#include "TCanvas.h"
#include <cstdlib>
#include <sstream>

HexagonCorrelationHistos::HexagonCorrelationHistos(eudaq::StandardPlane p, RootMonitor *mon)
  : _id(p.ID()), _wait(false){

    
  char out[1024], out2[1024], out3[1024], out4[1024];

  _mon = mon;
  
  // std::cout << "HexagonCorrelationHistos::Sensorname: " << _sensor << " "<< _id<<
  // std::endl;

  for (int _brd=1; _brd<=NSENSORS; _brd++){
    
    for (int _ID=0; _ID<NHEXAGONS_PER_SENSOR; _ID++) {
      
      if (_id >= _ID) continue;

      std::cout<<" Initializing the histograms for brd = "<<_brd<<"  _id1="<<_id<<"  _id2="<<_ID<<std::endl;
      
      sprintf(out, "LG. BRD %i: module %i vs. %i LG ", _brd, _id, _ID);
      sprintf(out2, "h_SignalLGSum_Brd_%i_mod_%ivs%i", _brd, _id, _ID);
      const int superID = (_brd-1)*100+_id*10+_ID;
      //_correlationSignalLGSum[superID] = NULL;
      _correlationSignalLGSum[superID] = new TH2I(out2, out, 50, 0., 8*1200., 50, 0., 8*1200.);
      //TB 2017: Energy sum in a layer of the EE part for 90 GeV electrons barely reaches 1000 MIPs.
      // Also, 1 MIP ~ 5 LG ADC //Thorben Quast, 07 June 2018
      
      sprintf(out3, "LG (TS3-TS0) Sum, Module %i (ADC)", _id);
      sprintf(out4, "LG (TS3-TS0) Sum, Module %i (ADC)", _ID);
      SetHistoAxisLabels(_correlationSignalLGSum[superID], out3, out4);  

      sprintf(out, "TOA. BRD %i: module %i vs. %i", _brd, _id, _ID);
      sprintf(out2, "h_corrTOA_Brd_%i_mod_%ivs%i", _brd, _id, _ID);
      _correlationTOA[superID] = new TH2I(out2, out, 50, 0., 2000., 50, 0., 2000.);
      sprintf(out3, "AVG TOA,  Module %i (ADC)", _id);
      sprintf(out4, "AVG TOA,  Module %i (ADC)", _ID);
      SetHistoAxisLabels(_correlationTOA[superID], out3, out4);  

    }
  }
  
}

void HexagonCorrelationHistos::Fill(const eudaq::StandardPlane &plane1, const eudaq::StandardPlane &plane2) {

  
 int _ID = plane2.ID();

 
 char buffer[2];  buffer[0] = plane2.Sensor()[13];
 int brdID = atoi(buffer);
 //std::cout<<"Brd ID = "<<brdID<<std::endl;

 // TOA correlations
  if (plane1.HitPixels() > 0 && plane2.HitPixels() > 0){
    const unsigned int avgTOA_1 = plane1.GetPixel(0, 30);
    const unsigned int avgTOA_2 = plane2.GetPixel(0, 30);

    //_correlationTOA[(brdID-1)*100+_id*10+_ID]->Fill(avgTOA_1, avgTOA_2);

    //sum of LG in TS3 - TS0
    const unsigned int sumLG_TS3_1 = plane1.GetPixel(0, 31);
    const unsigned int sumLG_TS3_2 = plane2.GetPixel(0, 31);
    
    _correlationSignalLGSum[(brdID-1)*100+_id*10+_ID]->Fill(sumLG_TS3_1, sumLG_TS3_2);
  }

}

void HexagonCorrelationHistos::Reset() {

  for (int _brd=0; _brd<NSENSORS; _brd++){
    for (int _id1=0; _id1<NHEXAGONS_PER_SENSOR; _id1++) {
      for (int _id2=_id1+1; _id2<NHEXAGONS_PER_SENSOR; _id2++) {
	if (_correlationSignalLGSum.find(_brd*100+_id1*10+_id2)==_correlationSignalLGSum.end()) continue;
	_correlationSignalLGSum[_brd*100+_id1*10+_id2]->Reset();
	_correlationTOA[_brd*100+_id1*10+_id2]->Reset();
      }	
    }
  }
}

void HexagonCorrelationHistos::Calculate(const int currentEventNum) {
  _wait = false;
}

void HexagonCorrelationHistos::Write() {
  
  for (int _brd=0; _brd<NSENSORS; _brd++){
    for (int _id1=0; _id1<NHEXAGONS_PER_SENSOR; _id1++) {
      for (int _id2=_id1+1; _id2<NHEXAGONS_PER_SENSOR; _id2++) {
	if (_correlationSignalLGSum.find(_brd*100+_id1*10+_id2)==_correlationSignalLGSum.end()) continue;
	_correlationSignalLGSum[_brd*100+_id1*10+_id2]->Write();
	_correlationTOA[_brd*100+_id1*10+_id2]->Write();
      }	
    }
  }
  
}

int HexagonCorrelationHistos::SetHistoAxisLabelx(TH1 *histo, string xlabel) {
  if (histo != NULL) {
    histo->GetXaxis()->SetTitle(xlabel.c_str());
  }
  return 0;
}

int HexagonCorrelationHistos::SetHistoAxisLabels(TH1 *histo, string xlabel, string ylabel) {
  SetHistoAxisLabelx(histo, xlabel);
  SetHistoAxisLabely(histo, ylabel);

  return 0;
}

int HexagonCorrelationHistos::SetHistoAxisLabely(TH1 *histo, string ylabel) {
  if (histo != NULL) {
    histo->GetYaxis()->SetTitle(ylabel.c_str());
  }
  return 0;
}
