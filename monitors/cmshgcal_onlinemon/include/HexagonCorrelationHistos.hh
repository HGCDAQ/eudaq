//Thorben Quast, thorben.quast@cern.ch
//June 2018

// -*- mode: c -*-

#ifndef HEXAGONCORRELATIONHISTOS_HH_
#define HEXAGONCORRELATIONHISTOS_HH_

#include <TH2F.h>
#include <TH2I.h>
#include <TH1F.h>
#include <TH2Poly.h>
#include <TFile.h>

#include <map>

#include "eudaq/StandardEvent.hh"

using namespace std;

class RootMonitor;

class HexagonCorrelationHistos {
protected:
  int _id;
  int _maxX;
  int _maxY;
  bool _wait;
  
  std::map<int, TH2F *> _correlationSignalHGSum;    //example 06 June 2018: HG Signal sum
  
public:
  HexagonCorrelationHistos(eudaq::StandardPlane p, RootMonitor *mon);

  void Fill(const eudaq::StandardPlane &plane1, const eudaq::StandardPlane &plane2);
  void Reset();

  void Calculate(const int currentEventNum);
  void Write();
  
  TH2F *getCorrelationSignalHGSum(int ref_planeIndex) { return _correlationSignalHGSum[ref_planeIndex]; }  

  void setRootMonitor(RootMonitor *mon) { _mon = mon; }

private:
  int **plane_map_array; // store an array representing the map
  int zero_plane_array(); // fill array with zeros;
  int SetHistoAxisLabelx(TH1 *histo, string xlabel);
  int SetHistoAxisLabely(TH1 *histo, string ylabel);
  int SetHistoAxisLabels(TH1 *histo, string xlabel, string ylabel);
  int filling_counter; // we don't need occupancy to be refreshed for every
                       // single event
  
  RootMonitor *_mon;
};


#ifdef __CINT__
#pragma link C++ class HexagonCorrelationHistos - ;
#endif

#endif /* HEXAGONCORRELATIONHISTOS_HH_ */
