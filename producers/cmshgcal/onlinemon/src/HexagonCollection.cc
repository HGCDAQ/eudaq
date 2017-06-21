#include "HexagonCollection.hh"
#include "OnlineMon.hh"

static int counting = 0;
static int events = 0;

bool HexagonCollection::isPlaneRegistered(eudaq::StandardPlane p) {
  std::map<eudaq::StandardPlane, HexagonHistos *>::iterator it;
  it = _map.find(p);
  return (it != _map.end());
}


void HexagonCollection::fillHistograms(const eudaq::StandardPlane &pl) {
  std::cout<<"In HexagonCollection::fillHistograms(StandardPlane)"<<std::endl;


  if (!isPlaneRegistered(pl)) {
    registerPlane(pl);
    isOnePlaneRegistered = true;
  }

  HexagonHistos *hexmap = _map[pl];
  hexmap->Fill(pl);

  ++counting;

}


void HexagonCollection::bookHistograms(const eudaq::StandardEvent &ev) {
  std::cout<<"In HexagonCollection::bookHistograms(StandardEvent)"<<std::endl;

  for (int plane = 0; plane < ev.NumPlanes(); plane++) {
    const eudaq::StandardPlane Plane = ev.GetPlane(plane);
    if (!isPlaneRegistered(Plane)) {
      registerPlane(Plane);
    }
  }
}

void HexagonCollection::Write(TFile *file) {
  if (file == NULL) {
    // cout << "HexagonCollection::Write File pointer is NULL"<<endl;
    exit(-1);
  }  

  if (gDirectory != NULL) // check if this pointer exists
  {
    gDirectory->mkdir("Hexagon");
    gDirectory->cd("Hexagon");

    std::map<eudaq::StandardPlane, HexagonHistos *>::iterator it;
    for (it = _map.begin(); it != _map.end(); ++it) {

      char sensorfolder[255] = "";
      sprintf(sensorfolder, "%s_%d", it->first.Sensor().c_str(), it->first.ID());
      
      // cout << "Making new subfolder " << sensorfolder << endl;
      gDirectory->mkdir(sensorfolder);
      gDirectory->cd(sensorfolder);
      it->second->Write();

      // gDirectory->ls();
      gDirectory->cd("..");
    }
    gDirectory->cd("..");
  }
}

void HexagonCollection::Calculate(const unsigned int currentEventNumber) {
  if ((currentEventNumber > 10 && currentEventNumber % 1000 * _reduce == 0)) {
    std::map<eudaq::StandardPlane, HexagonHistos *>::iterator it;
    for (it = _map.begin(); it != _map.end(); ++it) {
      // std::cout << "Calculating ..." << std::endl;
      it->second->Calculate(currentEventNumber / _reduce);
    }
  }
}

void HexagonCollection::Reset() {
  std::map<eudaq::StandardPlane, HexagonHistos *>::iterator it;
  for (it = _map.begin(); it != _map.end(); ++it) {
    (*it).second->Reset();
  }
}


void HexagonCollection::Fill(const eudaq::StandardEvent &ev) {
  std::cout<<"In HexagonCollection::Fill(StandardEvent)"<<std::endl;

  for (int plane = 0; plane < ev.NumPlanes(); plane++) {
    const eudaq::StandardPlane &Plane = ev.GetPlane(plane);
    fillHistograms(Plane);
  }
}

//void HexagonCollection::Fill(const SimpleStandardEvent &ev) {
//  std::cout<<"In HexagonCollection::Fill(SimpleStandardEvent)"<<std::endl;
  // It's needed here because the mother class - BasicCollection - expects it
//}


HexagonHistos *HexagonCollection::getHexagonHistos(std::string sensor, int id) {
  const eudaq::StandardPlane p(id, "HexaBoard", sensor);
  return _map[p];
}

void HexagonCollection::registerPlane(const eudaq::StandardPlane &p) {
  std::cout<<"In HexagonCollection::registerPlane(StandardPlane)"<<std::endl;

  HexagonHistos *tmphisto = new HexagonHistos(p, _mon);
  _map[p] = tmphisto;

  // std::cout << "Registered Plane: " << p.Sensor() << " " << p.ID() <<
  // std::endl;
  // PlaneRegistered(p.Sensor(),p.ID());
  
  if (_mon != NULL) {
    if (_mon->getOnlineMon() == NULL) {
      return; // don't register items
    }
    // cout << "HexagonCollection:: Monitor running in online-mode" << endl;
    char tree[1024], folder[1024];
    sprintf(folder, "%s", p.Sensor().c_str());
    
    sprintf(tree, "%s/Module %i/Occ_ADC_HG", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getHexagonsOccAdcHisto(), "COLZL TEXT");
    _mon->getOnlineMon()->addTreeItemSummary(folder, tree);

    
    sprintf(tree, "%s/Module %i/Occ_TOT", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getHexagonsOccTotHisto(), "COLZL TEXT");
    _mon->getOnlineMon()->addTreeItemSummary(folder, tree);
    
    sprintf(tree, "%s/Module %i/Occ_TOA", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getHexagonsOccToaHisto(), "COLZL TEXT");
    _mon->getOnlineMon()->addTreeItemSummary(folder, tree);

    sprintf(tree, "%s/Module %i/RawHitmap", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getHitmapHisto(), "COLZ", 0);
    //_mon->getOnlineMon()->addTreeItemSummary(folder, tree);
    
    sprintf(tree, "%s/Module %i/RawHitmap X Projection", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getHitXmapHisto());

    sprintf(tree, "%s/Module %i/RawHitmap Y Projection", p.Sensor().c_str(),p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getHitYmapHisto());
    
    sprintf(tree, "%s/Module %i/NumHits", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getNHitsHisto());


    // PER EVENT Histograms:
    sprintf(tree, "%s/Module %i/HG_ADC_in_last_event", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getHexagonsChargeHisto(), "COLZ TEXT");
    //_mon->getOnlineMon()->addTreeItemSummary(folder, tree);

    
    sprintf(tree, "%s/Module %i", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->makeTreeItemSummary(tree);

    
  }
}
