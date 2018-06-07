#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Configuration.hh"

#include <bitset>
#include <boost/format.hpp>
 
// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#include "IMPL/LCEventImpl.h"
#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/LCCollectionVec.h"
#include "lcio.h"
#endif

#include "../include/compressor.h"

const size_t RAW_EV_SIZE_32 = 123160;
const int nSCA=13;

// Size of ZS data (per channel)
const char hitSizeZS = 33;

namespace eudaq {
  
  // The event type for which this converter plugin will be registered
  // Modify this to match your actual event type (from the Producer)
  //static const char *EVENT_TYPE = "RPI";
  static const char *EVENT_TYPE = "HexaBoard";

  // Declare a new class that inherits from DataConverterPlugin
  class HexaBoardConverterPlugin : public DataConverterPlugin {

    public:
      // This is called once at the beginning of each run.
      // You may extract information from the BORE and/or configuration
      // and store it in member variables to use during the decoding later.
      virtual void Initialize(const Event &bore, const Configuration &cnf) {
	
#ifndef WIN32 // some linux Stuff //$$change
	(void)cnf; // just to suppress a warning about unused parameter cnf
#endif


	std::cout<<"Config file name in HexaBoard Converter: "<<cnf.Name()<<std::endl;

	cnf.SetSection("RunControl");
	cnf.Print();

	cnf.SetSection("Producer.CMS-HGCAL");
	m_compressed = cnf.Get("DoCompression", false);
	
	cnf.SetSection("CMSHGCAL-OnlineMon");
	cnf.Print();

      	m_runMode = cnf.Get("runMode", 0);

	m_skiMask[0] = std::stoul(cnf.Get("Mask_RDB1", "0xFFFFFFFF"), nullptr, 16);
	m_skiMask[1] = std::stoul(cnf.Get("Mask_RDB2", "0xFFFFFFFF"), nullptr, 16);
	m_skiMask[2] = std::stoul(cnf.Get("Mask_RDB3", "0xFFFFFFFF"), nullptr, 16);
	m_skiMask[3] = std::stoul(cnf.Get("Mask_RDB4", "0xFFFFFFFF"), nullptr, 16);
	m_skiMask[4] = std::stoul(cnf.Get("Mask_RDB4", "0xFFFFFFFF"), nullptr, 16);

	/*
	// ------ HARDCODED -------
	// -- For running in developement mode (can try different config files):
	const std::string confFileName = "/scratch/eudaq/producers/cmshgcal/conf/AllInOneProducer.conf";
	std::ifstream confFile(confFileName.c_str());
	if (confFile.is_open()) {
	  Configuration config(confFile, "CMSHGCAL-OnlineMon");
	  config.Set("Name", confFileName);
	  config.Print();
	}
	// -- offline mode end --
	*/
	
      }

      // This should return the trigger ID (as provided by the TLU)
      // if it was read out, otherwise it can either return (unsigned)-1,
      // or be left undefined as there is already a default version.
      virtual unsigned GetTriggerID(const Event &ev) const {
	static const unsigned TRIGGER_OFFSET = 8;
	// Make sure the event is of class RawDataEvent
	if (const RawDataEvent *rev = dynamic_cast<const RawDataEvent *>(&ev)) {
	  // This is just an example, modified it to suit your raw data format
	  // Make sure we have at least one block of data, and it is large enough
	  if (rev->NumBlocks() > 0 &&
	      rev->GetBlock(0).size() >= (TRIGGER_OFFSET + sizeof(short))) {
	    // Read a little-endian unsigned short from offset TRIGGER_OFFSET
	    return getlittleendian<unsigned short>( &rev->GetBlock(0)[TRIGGER_OFFSET]);
	  }
	}
	// If we are unable to extract the Trigger ID, signal with (unsigned)-1
	return (unsigned)-1;
      }

      // Here, the data from the RawDataEvent is extracted into a StandardEvent.
      // The return value indicates whether the conversion was successful.
      // Again, this is just an example, adapted it for the actual data layout.
      virtual bool GetStandardSubEvent(StandardEvent &sev, const Event &ev) const {

	// If the event type is used for different sensors
	// they can be differentiated here
	const std::string sensortype = "HexaBoard";

	std::cout<<"\t Dans GetStandardSubEvent()  "<<std::endl;

	const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> ( &ev );

	//rev->Print(std::cout);

	const unsigned nBlocks = rev->NumBlocks();
	std::cout<<"Number of Raw Data Blocks: "<<nBlocks<<std::endl;

	int RDBOARD = 0;

	for (unsigned blo=0; blo<nBlocks; blo++){

	  std::cout<<"Block = "<<blo<<"  Raw GetID = "<<rev->GetID(blo)<<std::endl;


	  const RawDataEvent::data_t & bl = rev->GetBlock(blo);

	  std::cout<<"size of block: "<<bl.size()<<std::endl;

	  if (blo%2==0){
	    // This block contains a string with
	    std::vector<int> brdID;
	    //brdID.push_back(0); // Temporary set the id for some debugging to work
	    brdID.resize(bl.size() / sizeof(int));
	    std::memcpy(&brdID[0], &bl[0], bl.size());
	    RDBOARD = brdID[0];
	    std::cout<<"RDBRD ID = "<<RDBOARD<<std::endl;

	    //const unsigned nPlanes = nSkiPerBoard[RDBOARD-1]/4;
	    //std::cout<<"Number of Planes: "<<nPlanes<<std::endl;

	    continue;
	  }

	  else {
	    
	    // This block contains the data
	    std::cout<<" We are in the data block. blo="<<blo<<std::endl;
	    
	    // We will put the data in this vector:
	    // ----------------->>>>>>
	    std::vector<uint32_t> rawData32;
	    // <<<<<<<-----------------
	    
	    if (bl.size()!=RAW_EV_SIZE_32 && !m_compressed){
	      EUDAQ_WARN("There is something wrong with the data. Size= "+eudaq::to_string(bl.size()));
	      std::cout<<" Bad data size in block "<<blo<<"   size: "<<bl.size()<<std::endl;
	      return true;
	    }
	    else if (bl.size()!=RAW_EV_SIZE_32 && m_compressed){
	      // std::cout<<" This data are compressed! Data size in block "<<blo<<"   size: "<<bl.size()<<std::endl;
	      // Let's decompress it!
	      //std::string decompressed=decompressor::decompress(bl); // Taking the function from this class
	      std::string decompressed=compressor::decompress(bl); // Taking the function from compressor.h
	      
	      const std::vector<uint8_t> decompData( decompressed.begin(), decompressed.end() );
	      rawData32.resize(decompData.size() / sizeof(uint32_t));
	      std::memcpy(&rawData32[0], &decompData[0], decompData.size());
	      
	      if (rawData32.size()*4 != RAW_EV_SIZE_32){
		std::cout << "Size of raw data after decompression is wong! size = "<<rawData32.size()*4<<" bytes"<<std::endl;
		return true;
	      }
	      
	    }
	    else if (bl.size()==RAW_EV_SIZE_32 && m_compressed){
	      std::cout<<"Hmmm. You says that the data are compressed, but the data size says otherwisse..\n"
		       <<"Who is right? Machine is always right. Human is wrong.\n"
		       <<"Data size in block "<<blo<<"   size: "<<bl.size()<<std::endl;
	      return true;
	    }
	    else if (bl.size()==RAW_EV_SIZE_32 && !m_compressed){
	      // This is the simplest case, data are not compressed. Just mem-copy them:
	      rawData32.resize(bl.size() / sizeof(uint32_t));
	      std::memcpy(&rawData32[0], &bl[0], bl.size());
	    }

	    
	    const std::vector<std::array<unsigned int,1924>> decoded = decode_raw_32bit(rawData32, RDBOARD);
	    //std::cout << "4444" << std::endl;
	    // Here we parse the data per hexaboard and per ski roc and only leave meaningful data (Zero suppress and finding main frame):
	    const std::vector<std::vector<unsigned short>> dataBlockZS = GetZSdata(decoded, RDBOARD);
	    
	    for (unsigned h = 0; h < dataBlockZS.size(); h++){
	      
	      //std::cout<<"Hexa plane = "<<h<<std::endl;
	      
	      std::ostringstream oss;
	      oss << sensortype <<"-RDB" << RDBOARD;
	      StandardPlane plane(h, EVENT_TYPE, oss.str());
	      //StandardPlane plane(blo*8+h, EVENT_TYPE, sensortype);
	      
	      // -------------
	      // Now we can use the data to fill the euDAQ Planes
	      // -------------
	      const unsigned nHits  = dataBlockZS[h].size()/hitSizeZS;
	      
	      //std::cout<<"Number of Hits (above ZS threshold): "<<nHits<<std::endl;
	      
	      plane.SetSizeZS(4, 64, nHits, hitSizeZS-1);
	      
	      for (int ind=0; ind<nHits; ind++){
		
		// APZ DBG. Test the monitoring:
		//for (int ts=0; ts<16; ts++)
		//plane.SetPixel(ind, 2, 32+ind, 500-10*ind, false, ts);
		
		
		const unsigned skiID_1 = dataBlockZS[h][hitSizeZS*ind]/100;
		// This should give us a number between 0 and 3:
		const unsigned skiID_2 = 3 - (skiID_1)%4;
		
		if (skiID_2 > 3){
		  std::cout<<" ERROR in ski. It is "<<skiID_2<<std::endl;
		  EUDAQ_WARN("There is another error with encoding. ski="+eudaq::to_string(skiID_2));
		}
		else {
		  const unsigned pix = dataBlockZS[h][hitSizeZS*ind]-skiID_1*100;
		  
		  //if (skiID==0 && pix==0)
		  //std::cout<<" Zero-Zero problem. ind = "<<ind<<"  ID:"<<data[hitSizeZS*ind]<<std::endl;
		  
		  for (int ts=0; ts<hitSizeZS-1; ts++)
		    plane.SetPixel(ind, skiID_2, pix, dataBlockZS[h][hitSizeZS*ind+1 + ts], false, ts);
		}
		
	      }
	      
	      // Set the trigger ID
	      plane.SetTLUEvent(ev.GetEventNumber());
	      // Add the plane to the StandardEvent
	      sev.AddPlane(plane);
	      //eudaq::mSleep(10);
	      
	      
	      /* APZ DBG
	      // These are just to test the planes in onlinemonitor:
	      if (bl.size()>3000){
	      plane.SetSizeZS(4, 64, 5, 2);
	      
	      plane.SetPixel(0, 2, 32, 500, false, 0);
	      plane.SetPixel(1, 2, 33, 400, false, 0);
	      plane.SetPixel(2, 2, 31, 400, false, 0);
	      plane.SetPixel(3, 1, 32, 400, false, 0);
	      plane.SetPixel(4, 3, 32, 400, false, 0);
	      
	      
	      plane.SetPixel(0, 2, 32, 300, false, 1);
	      plane.SetPixel(1, 2, 33, 100, false, 1);
	      plane.SetPixel(2, 2, 31, 100, false, 1);
	      plane.SetPixel(3, 1, 32, 100, false, 1);
	      plane.SetPixel(4, 3, 32, 100, false, 1);
	      }
	      else {
	      plane.SetSizeZS(4, 64, 3, 2);
	      
	      plane.SetPixel(0, 1, 15, 500, false, 0);
	      plane.SetPixel(1, 1, 16, 400, false, 0);
	      plane.SetPixel(2, 1, 14, 400, false, 0);
	      
	      
	      plane.SetPixel(0, 1, 15, 300, false, 1);
	      plane.SetPixel(1, 1, 16, 100, false, 1);
	      plane.SetPixel(2, 1, 14, 100, false, 1);
	      }
	      */
	      
	    }
	  }
	  
	}

	std::cout<<"St Ev NumPlanes: "<<sev.NumPlanes()<<std::endl;
	
	// Indicate that data was successfully converted
	return true;
      }
      
      unsigned int gray_to_brady(unsigned int gray) const{
	// Code from:
	//https://github.com/CMS-HGCAL/TestBeam/blob/826083b3bbc0d9d78b7d706198a9aee6b9711210/RawToDigi/plugins/HGCalTBRawToDigi.cc#L154-L170
	unsigned int result = gray & (1 << 11);
	result |= (gray ^ (result >> 1)) & (1 << 10);
	result |= (gray ^ (result >> 1)) & (1 << 9);
	result |= (gray ^ (result >> 1)) & (1 << 8);
	result |= (gray ^ (result >> 1)) & (1 << 7);
	result |= (gray ^ (result >> 1)) & (1 << 6);
	result |= (gray ^ (result >> 1)) & (1 << 5);
	result |= (gray ^ (result >> 1)) & (1 << 4);
	result |= (gray ^ (result >> 1)) & (1 << 3);
	result |= (gray ^ (result >> 1)) & (1 << 2);
	result |= (gray ^ (result >> 1)) & (1 << 1);
	result |= (gray ^ (result >> 1)) & (1 << 0);
	return result;
      }


      std::vector<std::array<unsigned int,1924>> decode_raw_32bit(std::vector<uint32_t>& raw, const int board_id) const{
	
	//const uint32_t ch_mask = m_skiMask[board_id-1];
	const uint32_t ch_mask = raw[0];
	
	//std::cout<<"In decoder"<<std::endl;
	//printf("\t SkiMask: 0x%08x;   Length of Raw: %d\n", ch_mask, raw.size());

	// Check that an external mask agrees with first 32-bit word in data
	if (ch_mask!=m_skiMask[board_id-1])
	  EUDAQ_WARN("You extarnal mask ("+eudaq::to_hex(m_skiMask[board_id-1])+") does not agree with the one found in data ("+eudaq::to_hex(raw[0])+")");


	//for (int b=0; b<2; b++)
	//std::cout<< boost::format("Pos: %d  Word in Hex: 0x%08x ") % b % raw[b]<<std::endl;

	//std::cout<< boost::format("Pos: %d  Word in Hex: 0x%08x ") % 30786 % raw[30786]<<std::endl;
	//std::cout<< boost::format("Pos: %d  Word in Hex: 0x%08x ") % 30787 % raw[30787]<<std::endl;

	// First, we need to determine how many skiRoc data is presnt

	const std::bitset<32> ski_mask(ch_mask);
	//std::cout<<"ski mask: "<<ski_mask<<std::endl;

	const int mask_count = ski_mask.count();
	//if (mask_count!= nSkiPerBoard[board_id-1]) {
	//EUDAQ_WARN("The mask does not agree with expected number of SkiRocs. Mask count:"+ eudaq::to_string(mask_count));
	//}

	std::vector<std::array<unsigned int, 1924>> ev(mask_count, std::array<unsigned int, 1924>());


	uint32_t x;
	const int offset = 1; // Due to FF or other things in data head
	for(int  i = 0; i < 1924; i++){
	  for (int j = 0; j < 16; j++){
	    x = raw[offset + i*16 + j];
	    //x = x & ch_mask; // <-- This is probably not needed
	    int k = 0;
	    for (int fifo = 0; fifo < 32; fifo++){
	      if (ch_mask & (1<<fifo)){
		ev[k][i] = ev[k][i] | (unsigned int) (((x >> fifo ) & 1) << (15 - j));
		k++;
	      }
	    }
	  }
	}

	return ev;

      }

      char GetRollMaskEnd(const unsigned int r) const {
	//printf("Roll mask = %d \n", r);
	int k1 = -1, k2 = -1;
	for (int p=0; p<nSCA; p++){
	  //printf("pos = %d, %d \n", p, r & (1<<12-p));
	  if (r & (1<<p)) {
	    if (k1==-1)
	      k1 = 12-p;
	    else if (k2==-1)
	      k2 = 12-p;
	    else
	      EUDAQ_WARN("Error: more than two positions in roll mask! RollMask="+eudaq::to_hex(r,8));
	  }
	}

	//printf("ROLL MASK:  k1 = %d, k2 = %d \n", k1, k2);

	// Check that k1 and k2 are consecutive
	char last = -1;
	if (k1==12 && k2==0) { last = 0;}

	//else if (abs(k1-k2)>1)
	//EUDAQ_WARN("The k1 and k2 are not consecutive! abs(k1-k2) = "+ eudaq::to_string(abs(k1-k2)) +" rollMask="+eudaq::to_hex(r,8)+ " k1="+eudaq::to_string(k1)+"  k2="+eudaq::to_string(k2));
	//printf("The k1 and k2 are not consecutive! abs(k1-k2) = %d\n", abs(k1-k2));
	else
	  last = k1;

	//printf("last = %d\n", last);

	return last;
      }


      /*
	std::array<int, 13> rollPositions(const unsigned int r) const {

	//std::bitset<nSCA> bitstmp = r;
	std::bitset<nSCA> bits = r;
	//for( size_t i=0; i<nSCA; i++ )
	//bits[i]=bitstmp[nSCA-1-i];

	std::array<int, nSCA> roll;
	if(bits.test(0) && bits.test(nSCA-1)){
	roll[0]=0;
	for(size_t i=1; i<nSCA; i++)
	roll[i]=(i-1);
	}
	else{
	int pos_trk1 = -1;
	for(size_t i=0; i<nSCA; i++)
	if(bits.test(i)){
	pos_trk1 = i;
	break;
	}
	for(size_t i=0; i<nSCA; i++)
	if( (int)i <= pos_trk1 + 1 )
	roll[i]=i + 12 - (pos_trk1 + 1);
	else
	roll[i]=i - 1 - (pos_trk1 + 1);
	}
	return roll;
	}
      */
      
      std::vector<std::vector<unsigned short>> GetZSdata(const std::vector<std::array<unsigned int,1924>> &decoded, const int board_id) const {
	
	//std::cout<<"In GetZSdata() method"<<std::endl;
	
	const int nSki  =  decoded.size();
	const int nHexa =  nSki/4;
	if (nSki%4!=0)
	  EUDAQ_WARN("Number of SkiRocs is not right: "+ eudaq::to_string(nSki));

	std::bitset<32> skibits(m_skiMask[board_id-1]);
	if (nHexa != skibits.count()/4)
	  EUDAQ_WARN("Number of HexaBoards is not right: "+ eudaq::to_string(nHexa));

	// A vector per HexaBoard of vector of Hits from 4 ski-rocs
	std::vector<std::vector<unsigned short>> dataBlockZS(nHexa);

	unsigned int toa_sum = 0;
	unsigned int n_toa_fired = 0;
	unsigned int lg_sum = 0;
	      
	for (int ski = 0; ski < nSki; ski++ ){
	  const int hexa = ski/4;

	  //fprintf(fout, "Event %d Chip %d RollMask 0x%08x \n", m_ev, ski, decoded[ski][1920]);

	  // ----------
	  // -- Based on the rollmask, lets determine which time-slices (frames) to add
	  //
	  const unsigned int r = decoded[ski][1920];

	  // This is the position of "track" in TS array:
	  const int TS0 = (GetRollMaskEnd(r)+1)%13;

	  //const int mainFrame = (TS0+3)%13;

	  /*
	    std::vector<unsigned short> tmp_adc;
	    for (int ch = 0; ch < 64; ch+=2){
	    // Here lets estimate the pedestal and noise by averaging over all channels
	    const int chArrPos = 63-ch; // position of the hit in array

	    unsigned short adc = 0;
	    adc = gray_to_brady(decoded[ski][mainFrame*128 + 64 + chArrPos] & 0x0FFF);
	    if (adc==0) adc=4096; // Taking care of overflow
	    tmp_adc.push_back(adc);
	    }

	    std::sort(tmp_adc.begin(), tmp_adc.end());


	    unsigned median = 0 ;

	    if (tmp_adc.size() == 32){
	    median = tmp_adc[15];
	    //// We could also do mean and stdev if we wanted to:
	    //double sum = std::accumulate(tmp_adc.begin(), tmp_adc.end(), 0.0);
	    //double mean = sum / tmp_adc.size();

	    //double sq_sum = std::inner_product(tmp_adc.begin(), tmp_adc.end(), tmp_adc.begin(), 0.0);
	    //double stdev = std::sqrt(sq_sum / tmp_adc.size() - mean * mean);
	    ////
	    }
	    else
	    std::cout<<"There is something wrong with your tmp_adc.size()"<<tmp_adc.size()<<std::endl;


	    //std::cout<<" Median of all channels:\n"<<median
	    //	 <<"\t also, first guy:"<<tmp_adc.front()<<"  and last guy:"<<tmp_adc.back()<<std::endl;

	    tmp_adc.clear();

	    //const int ped = 150;
	    //const int ped = median;

	  */

	  /*
	  const std::array<int, 13> rollPos = rollPositions(r);

	  std::cout<<"Roll positions array"<<std::endl;
	  for (int i=0; i<nSCA; i++)
	    std::cout<<"  "<<rollPos[i];
	  std::cout<<std::endl;

	  std::cout<<"Subtracted Roll positions array"<<std::endl;
	  for (int i=0; i<nSCA; i++)
	    std::cout<<"  "<<nSCA-1-rollPos[i];
	  std::cout<<std::endl;
	  */

	  for (int ch = 0; ch < 64; ch+=2){

	    const int chArrPos = 63-ch; // position of the hit in array

	    // This channel is not conected. Notice that ski-roc numbering here is reverted by (3-ski) relation.
	    // (Ie, the actual disconnected channel is (1,60), but in this numbering it's (2.60))
	    if (ski==2 && ch==60) continue;

	    // ----------
	    // Temporarly!
	    // Let's find the frame with max charge, per channel.
	    //
	    /*
	      int maxADC = -10;
	      int tmp_TS = -10;
	      for (int ts = 0; ts < 13; ts++){

	      const int ts1 = 12-ts;
	      const int chargeLG = gray_to_brady(decoded[ski][ts1*128 + chArrPos] & 0x0FFF);
	      //const int chargeHG = gray_to_brady(decoded[ski][ts1*128 + 64 + chArrPos] & 0x0FFF);

	      if (chargeLG > maxADC){
	      //std::cout<<ch <<": chargeHG="<<chargeHG<<"  at ts="<<ts<<std::endl;
	      maxADC = chargeLG;
	      tmp_TS = ts1;
	      }
	      }

	      // ZeroSuppressed:
	      //if (maxADC > 300)
	      //&& maxADC - (ped+noi) > thresh)
	      //std::cout<<ch <<": Max charge of HG="<<maxADC<<"  at ts="<<tmp_TS
	      //	     << "  mainFrame = "<<mainFrame<<"  TS0 = "<<TS0<<std::endl;

	      */

	    unsigned short adc = 0;
	    
	    /*
	    const int TS2 = (TS0+2)%13;
	    const int TS3 = (TS0+3)%13;
	    const int TS4 = (TS0+4)%13;

	    int chargeHG_avg_in3TS = 0;

	    adc = gray_to_brady(decoded[ski][TS2*128 + 64 + chArrPos] & 0x0FFF);
	    chargeHG_avg_in3TS += adc;
	    adc = gray_to_brady(decoded[ski][TS3*128 + 64 + chArrPos] & 0x0FFF);
	    chargeHG_avg_in3TS += adc;
	    adc = gray_to_brady(decoded[ski][TS4*128 + 64 + chArrPos] & 0x0FFF);
	    chargeHG_avg_in3TS += adc;

	    chargeHG_avg_in3TS = chargeHG_avg_in3TS/3;
	    
	    */
	    
	    //adc = gray_to_brady(decoded[ski][mainFrame*128 + chArrPos] & 0x0FFF);
	    //if (adc==0) adc=4096;

	    //const int chargeLG = adc;

	    //std::cout<<ch <<": charge LG:"<<chargeLG
	    //<<"ped + noi = "<<ped+noi<<"   charge-(ped+noi) = "<<chargeLG - (ped+noi)<<std::endl;

	    // Check that all Ha hits are the same for a given channel
	    //if ((decoded[ski][chArrPos] & 0x1000) != (decoded[ski][chArrPos + 64] & 0x1000))
	    //std::cout<<"Warning: HA is not what we think it is..."<<std::endl;

	    if (m_runMode==0)
	      ; // Pass. This is pedestal mode, no zero suppression is needed 
	    else if (m_runMode==1){
	      // ZeroSuppress it:
	      //if (chargeHG_avg_in3TS < thresh)  // - Based on ADC in LG/HG
	      if (! (decoded[ski][chArrPos] & 0x1000)) // - Based on HA bit (TOA hit)
		continue;
	    }
	    else
	      EUDAQ_WARN("This run-mode is not yet implemented: "+eudaq::to_string(m_runMode));

	    
	    dataBlockZS[hexa].push_back((ski%4)*100+ch);

	    // Low gain (save nSCA time-slices after track):
	    //std::cout<<"\t Roll positions:"<<std::endl;
	    unsigned int LG_TS3 = 0;
	    unsigned int LG_TS0 = 0;

	    for (int ts=0; ts<nSCA; ts++){
	      const int t = 12-(TS0+ts)%13; // subtract from 12 (nSCA-1) because the data in SkiRock memory is saved backwards
	      adc = gray_to_brady(decoded[ski][t*128 + chArrPos] & 0x0FFF);
	      if (adc==0) adc=4096;
	      dataBlockZS[hexa].push_back(adc);
	    
	    	if (t==0) LG_TS0 = adc;
	    	if (t==3) LG_TS3 = adc;

	      //std::cout<<"  ts:"<<ts<<" adc="<<adc;
	    }
	    
	    if (LG_TS3 - LG_TS0 > 3)		//5 LG ADC ~ 1 MIP
	      lg_sum += LG_TS3-LG_TS0;
	    
	    //std::cout<<std::endl;

	    /*
	      std::cout<<"\t From track::"<<std::endl;
	      for (int ts=0; ts<nSCA; ts++){
	      const int t = 12-(TRACK+ts)%13;
	      adc = gray_to_brady(decoded[ski][t*128 + chArrPos] & 0x0FFF);
	      if (adc==0) adc=4096;
	      dataBlockZS[hexa].push_back(adc);
	      std::cout<<"  ts:"<<ts<<" adc="<<adc;
	      }
	      std::cout<<std::endl;
	    */


	    // High gain:
	    for (int ts=0; ts<nSCA; ts++){
	      const int t = 12-(TS0+ts)%13;

	      adc = gray_to_brady(decoded[ski][t*128 + 64 + chArrPos] & 0x0FFF);
	      if (adc==0) adc=4096;
	      dataBlockZS[hexa].push_back(adc);
	    }

	    // Filling TOA (stop falling clock)
	    adc = gray_to_brady(decoded[ski][1664 + chArrPos] & 0x0FFF);
	    if (adc==0) adc=4096;
	    dataBlockZS[hexa].push_back(adc);

	    // Summing all TOA values of the pixels which fire it (to do average later)
	    if (adc!=4){
	      toa_sum += adc;
	      n_toa_fired += 1;
	    }
	    // Filling TOA (stop rising clock)
	    adc = gray_to_brady(decoded[ski][1664 + 64 + chArrPos] & 0x0FFF);
	    if (adc==0) adc=4096;
	    dataBlockZS[hexa].push_back(adc);

	    // Filling TOT (fast)
	    adc = gray_to_brady(decoded[ski][1664 + 2*64 + chArrPos] & 0x0FFF);
	    if (adc==0) adc=4096;
	    dataBlockZS[hexa].push_back(adc);

	    // Filling TOT (slow)
	    adc = gray_to_brady(decoded[ski][1664 + 3*64 +chArrPos] & 0x0FFF);
	    if (adc==0) adc=4096;
	    dataBlockZS[hexa].push_back(adc);

	    // This frame is reserved to hold per-module variables.
	    // (which are set outside of this loop):
	    dataBlockZS[hexa].push_back(0);

	    
	    /* Let's not save this for the moment (no need)

	    // Global TS 14 MSB (it's gray encoded?). Not decoded here!
	    // Not sure how to decode Global Time Stamp yet...
	    adc = decoded[ski][1921];
	    if (adc==0) adc=4096;
	    dataBlockZS[hexa].push_back(adc);

	    // Global TS 12 LSB + 1 extra bit (binary encoded)
	    adc = decoded[ski][1922];
	    if (adc==0) adc=4096;
	    dataBlockZS[hexa].push_back(adc);
	    */

	  }

	  if (ski%4==3){
	    if (toa_sum > 0 && n_toa_fired > 0){
	      // Put here the average TOA value of the hexaboard
	      dataBlockZS[hexa][31] = toa_sum/n_toa_fired;
	      // Reset them:
	      toa_sum = 0;
	      n_toa_fired = 0;
	    }
	    // Put the sum of LG values  
	    dataBlockZS[hexa][32] = lg_sum;
	    lg_sum = 0;
	  }

	}
	
	return dataBlockZS;

      }

#if USE_LCIO
      // This is where the conversion to LCIO is done
      virtual lcio::LCEvent *GetLCIOEvent(const Event * /*ev*/) const {
	return 0;
      }
#endif

    private:
      // The constructor can be private, only one static instance is created
      // The DataConverterPlugin constructor must be passed the event type
      // in order to register this converter for the corresponding conversions
      // Member variables should also be initialized to default values here.
      HexaBoardConverterPlugin()
	: DataConverterPlugin(EVENT_TYPE) {}

      // Information extracted in Initialize() can be stored here:
      uint32_t m_skiMask[5];

      int m_runMode;
      bool m_compressed;
      
      // The single instance of this converter plugin
      static HexaBoardConverterPlugin m_instance;
  }; // class HexaBoardConverterPlugin

  // Instantiate the converter plugin instance
  HexaBoardConverterPlugin HexaBoardConverterPlugin::m_instance;

} // namespace eudaq
