#include "CAEN_V1290.hpp"


#define CAENV1290_DEBUG 

int CAEN_V1290::Init() {
  int status=0;
  std::cout<< "[CAEN_V1290]::[INFO]::++++++ CAEN V1290 INIT ++++++"<<std::endl;
  
  if (handle_<0) {
    std::cout<<"Handle is negative --> Init has failed."<<std::endl;
    return ERR_CONF_NOT_FOUND;
  }
  if (!IsConfigured()) {
    std::cout<<"Device has not been configured --> Init has failed."<<std::endl;
    return ERR_CONF_NOT_FOUND;
  }

  char* software_version = new char[100];
  CAENVME_SWRelease(software_version);
  std::cout<<"CAENVME library version: "<<software_version<<std::endl;

  //necessary: setup the communication board (VX2718)
  int *handle = new int;
  //corresponding values for the init function are taken from September 2016 configuration
  //https://github.com/cmsromadaq/H4DAQ/blob/master/data/H2_2016_08_HGC/config_pcminn03_RC.xml#L26

  status |= CAENVME_Init(static_cast<CVBoardTypes>(1), 0, 0, handle); 
  SetHandle(*handle);
  std::cout<<"Handle before is: "<<*handle<<std::endl;
  delete handle;
  if (status) {
    std::cout << "[CAEN_VX2718]::[ERROR]::Cannot open VX2718 board." << std::endl;
    return ERR_OPEN;
  }  
  //std::cout<<"The handle of the CAENV2718 optical link is: "<<handle_<<std::endl;

  //Read Version to check connection
  WORD data=0;
  
  sleep(0.2);
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+CAEN_V1290_FW_VER_REG,&data,CAEN_V1290_ADDRESSMODE,cvD16);
  if (status) {
    std::cout << "[CAEN_V1290]::[ERROR]::Cannot open V1290 board @0x" << std::hex << configuration_.baseAddress << std::dec << " " << status <<std::endl; 
    return ERR_OPEN;
  }    

  unsigned short FWVersion[2];
  for (int i=0;i<2;++i) {
    FWVersion[i]=(data>>(i*4))&0xF;
  }
  std::cout << "[CAEN_V1290]::[INFO]::Open V1290 board @0x" << std::hex << configuration_.baseAddress << std::dec << " FW Version Rev." << FWVersion[1] << "." << FWVersion[0] << std::endl;
  
  sleep(0.2);
  data=0;
  std::cout<<"[CAEN_V1290]::[INFO]::Software reset ... "<<std::endl;
  //status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V1290_SW_EVRESET_REG, &data,CAEN_V1290_ADDRESSMODE, cvD16);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V1290_SW_RESET_REG, &data,CAEN_V1290_ADDRESSMODE, cvD16);

  std::cout<<"Status: "<<status<<std::endl;
  if (status) {
    std::cout << "[CAEN_V1290]::[ERROR]::Cannot reset V1290 board " << status << std::endl; 
    return ERR_RESET;
  }    
  
  sleep(0.2);

  //Now the real configuration  

  
  channels_=32;
  if (configuration_.model == CAEN_V1290_N) {
    channels_=16;
  }

  std::cout << "[CAEN_V1290]::[INFO]::Enable empty events" << std::endl;
  if (configuration_.emptyEventEnable) {
    WORD data=0;
    sleep(0.2);
    status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V1290_CON_REG,&data,CAEN_V1290_ADDRESSMODE,cvD16);
    sleep(0.2);
    data |= CAEN_V1290_EMPTYEVEN_BITMASK; //enable emptyEvent
    status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V1290_CON_REG,&data,CAEN_V1290_ADDRESSMODE,cvD16);
  }
  
  sleep(0.2);
  // I step: set TRIGGER Matching mode via OPCODE 00xx 
  if (configuration_.triggerMatchMode) {
    std::cout << "[CAEN_V1290]::[INFO]::Enabled Trigger Match Mode" << std::endl;
    status |= OpWriteTDC(CAEN_V1290_TRMATCH_OPCODE);
  }
  
  CheckStatusAfterRead();

  
  sleep(0.2);
  // I step: set Edge detection via OPCODE 22xx 
  std::cout << "[CAEN_V1290]::[INFO]::EdgeDetection " << configuration_.edgeDetectionMode << std::endl;
  status |= OpWriteTDC(CAEN_V1290_EDGEDET_OPCODE);
  status |= OpWriteTDC(configuration_.edgeDetectionMode);
  
 
  sleep(0.2);
  // I step: set Time Reso via OPCODE 24xx 
  std::cout << "[CAEN_V1290]::[INFO]::TimeResolution " << configuration_.timeResolution << std::endl;
  status |= OpWriteTDC(CAEN_V1290_TIMERESO_OPCODE);
  status |= OpWriteTDC(configuration_.timeResolution);
  

    
  sleep(0.2);
  // II step: set TRIGGER Window Width to value n 
  std::cout << "[CAEN_V1290]::[INFO]::Set Trigger window width " << configuration_.windowWidth << std::endl;
  status |= OpWriteTDC(CAEN_V1290_WINWIDT_OPCODE); 
  status |= OpWriteTDC(configuration_.windowWidth);
    
  sleep(0.2);
  // III step: set TRIGGER Window Offset to value -n 
  std::cout << "[CAEN_V1290]::[INFO]::TimeWindowWidth " << configuration_.windowWidth << " TimeWindowOffset " << configuration_.windowOffset << std::endl;
  status |= OpWriteTDC(CAEN_V1290_WINOFFS_OPCODE); 
  status |= OpWriteTDC(configuration_.windowOffset);

  
  // IV step: enable channels
  //disable all channels
  //status |= OpWriteTDC(CAEN_V1290_DISALLCHAN_OPCODE); 
  /*
  for (unsigned int i=0;i<channels_;++i) {
    if (configuration_.enabledChannels & ( 1 << i ) ) {
      std::cout << "[CAEN_V1290]::[INFO]::Enabling channel " << i << std::endl;
      status |=OpWriteTDC(CAEN_V1290_ENCHAN_OPCODE+i);
      sleep(0.2);
    }
  }
  */
  //std::cout << "[CAEN_V1290]::[INFO]::Setting max hits per trigger " << configuration_.maxHitsPerEvent << std::endl;
  //status |= OpWriteTDC(CAEN_V1290_MAXHITS_OPCODE); 
  //status |= OpWriteTDC(configuration_.maxHitsPerEvent); 
  /*
  sleep(0.2);
  // IV step: Enable trigger time subtraction 
  if (configuration_.triggerTimeSubtraction) {
    std::cout << "[CAEN_V1290]::[INFO]::Enabling trigger time subtraction " << std::endl;
    status |= OpWriteTDC(CAEN_V1290_ENTRSUB_OPCODE);
  } else {
    std::cout << "[CAEN_V1290]::[INFO]::Disabling trigger time subtraction " << std::endl;
    status |= OpWriteTDC(CAEN_V1290_DISTRSUB_OPCODE);
  }
  */
  if (status) {
    std::cout << "[CAEN_V1290]::[ERROR]::Initialisation error" << status << std::endl;
    return ERR_CONFIG;
  } 
  std::cout << "[CAEN_V1290]::[INFO]::++++++ CAEN V1290 INITIALISED ++++++" << std::endl; 
  CheckStatusAfterRead();


  return 0;
} 


int CAEN_V1290::Clear() {
  //Send a software reset. Module has to be re-initialized after this
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  WORD data=0xFF;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V1290_SW_RESET_REG,&data,CAEN_V1290_ADDRESSMODE,cvD16);

  if (status) {
    std::cout << "[CAEN_V1290]::[ERROR]::Cannot reset V1290 board " << status << std::endl; 
    return ERR_RESET;
  }

  sleep(0.2);


  status |= CAENVME_SystemReset(handle_);
  if (status) {
    std::cout << "[CAEN_VX2718]::[ERROR]::Cannot reset VX2718 board " << status << std::endl; 
    return ERR_RESET;
  }

  status=Init();
  return status;
}      


int CAEN_V1290::BufferClear() {
 //Send a software reset. Module has to be re-initialized after this
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  WORD data=0xFF;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V1290_SW_CLEAR_REG,&data,CAEN_V1290_ADDRESSMODE,cvD16);

  if (status) {
    std::cout << "[CAEN_V1290]::[ERROR]::Cannot clear V1290 buffers " << status << std::endl; 
    return ERR_RESET;
  }

  return 0;
}
 
//19 May 2017: So far only some dummy values
int CAEN_V1290::Config() {
  GetConfiguration()->baseAddress=0x00AA0000;   //use default from September 2016's configuration
  GetConfiguration()->model=static_cast<CAEN_V1290_Model_t>(1);       //should correspond to 16ch version (LEMO inputs)
  GetConfiguration()->triggerTimeSubtraction=static_cast<bool>(1);
  GetConfiguration()->triggerMatchMode=static_cast<bool>(1);
  GetConfiguration()->emptyEventEnable=static_cast<bool>(1);
  GetConfiguration()->edgeDetectionMode=static_cast<CAEN_V1290_EdgeDetection_t>(3);   //ONLY LEADING EDGE DETECTION
  GetConfiguration()->timeResolution=static_cast<CAEN_V1290_TimeResolution_t>(3);   //CAEN_V1290_25PS_RESO
  GetConfiguration()->maxHitsPerEvent=static_cast<CAEN_V1290_MaxHits_t>(9);   //CAEN_V1290_THIRTYTWO_HITS
  GetConfiguration()->enabledChannels=0xFFFF; //use default from September 2016's configuration, this will enable channels 1-8....for 12 channels use 0x0FFF, 16 channels use 0xFFFF
  GetConfiguration()->windowWidth=0x40; //use default from September 2016's configuration
  GetConfiguration()->windowOffset=-1;  //use default from September 2016's configuration

  _isConfigured=true; 

  return 0;
}


//Read the ADC buffer and send it out in WORDS vector
int CAEN_V1290::Read(std::vector<WORD> &v) {
  //#ifdef CAENV1290_DEBUG
    //std::cout << "[CAEN_V1290]::[DEBUG]::READING" << std::endl;
  //#endif
  int status = 0; 

  status |= CheckStatusAfterRead();

  v.clear();
  if (handle_<0) {
    return ERR_CONF_NOT_FOUND;
  }


  WORD data;
  
  int v1290_rdy=0;
  int v1290_error=1;

  int ntry = 100, nt = 0;
  //Wait for a valid datum in the ADC
  while ( (v1290_rdy != 1 || v1290_error!=0 ) && nt<ntry ) {
    status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+CAEN_V1290_STATUS_REG,&data,CAEN_V1290_ADDRESSMODE,cvD16);
    v1290_rdy = data & CAEN_V1290_RDY_BITMASK;
    v1290_error = 0; 
    v1290_error |= data & CAEN_V1290_ERROR0_BITMASK;
    v1290_error |= data & CAEN_V1290_ERROR1_BITMASK;
    v1290_error |= data & CAEN_V1290_ERROR2_BITMASK;
    v1290_error |= data & CAEN_V1290_ERROR3_BITMASK;
    v1290_error |= data & CAEN_V1290_TRGLOST_BITMASK;
    ++nt;
  }

  if (status || v1290_rdy==0 || v1290_error!=0) {
    //std::cout << "[CAEN_V1290]::[ERROR]::Cannot get a valid data from V1290 board " << status << std::endl;   
    return ERR_READ;
  }  

  #ifdef CAENV1290_DEBUG
    std::cout << "[CAEN_V1290]::[DEBUG]::RDY " << v1290_rdy << " ERROR " << v1290_error << " NTRY " << nt << std::endl;
  #endif
  
  data=0;
  
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress + CAEN_V1290_OUTPUT_BUFFER,&data,CAEN_V1290_ADDRESSMODE,cvD32);

  #ifdef CAENV1290_DEBUG
    std::cout << "[CAEN_V1290]::[DEBUG]::IS EVENT HEADER " << ((data & 0x40000000)>>30) << std::endl;
  #endif


  if ( ! (data & 0x40000000) || status ) {
    std::cout << "[CAEN_V1290]::[ERROR]::First word not a Global header" << std::endl; 
    return ERR_READ;
  }

  int evt_num = data>>5 & 0x3FFFFF; 
  #ifdef CAENV1290_DEBUG
    std::cout << "[CAEN_V1290]::[DEBUG]::EVENT NUMBER " << evt_num << std::endl;
  #endif
  v.push_back( (0xA << 28) | (evt_num & 0xFFFFFFF ));     //defines the global header in the data stream that is sent to the producer
  //v.push_back( data );     //defines the global header in the data stream that is sent to the producer


  //Read until trailer
  int glb_tra=0;
  while (!glb_tra && !status) {
    data=0;
    status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress + CAEN_V1290_OUTPUT_BUFFER,&data,CAEN_V1290_ADDRESSMODE,cvD32);
    int wordType = (data >> 27) & CAEN_V1290_WORDTYE_BITMASK;
    #ifdef CAENV1290_DEBUG
      std::cout << "[CAEN_V1290]::[DEBUG]::EVENT WORDTYPE " << wordType << std::endl;
    #endif

    if (wordType == CAEN_V1290_GLBTRAILER) {
      #ifdef CAENV1290_DEBUG
        int TDC_ERROR = (data>>24) & 0x1;
        int OUTPUT_BUFFER_ERROR = (data>>25) & 0x1;
        int TRIGGER_LOST = (data>>26) & 0x1;
        int WORD_COUNT = (data>>5) & 0x10000;
        std::cout << "[CAEN_V1290]::[DEBUG]::WE FOUND THE EVENT TRAILER" << std::endl;
        std::cout << "TDC_ERROR: " << TDC_ERROR << std::endl;
        std::cout << "OUTPUT_BUFFER_ERROR: " << OUTPUT_BUFFER_ERROR << std::endl;
        std::cout << "TRIGGER_LOST: " << TRIGGER_LOST << std::endl;
        std::cout << "WORD_COUNT: " << WORD_COUNT << std::endl;
      #endif
      glb_tra=1;
      v.push_back(data);
    } else if (wordType == CAEN_V1290_TDCHEADER ) {
      #ifdef CAENV1290_DEBUG
        int TDCnr = (data>>24) & 0x3;
        int eventID = (data>>12) & 0xFFF;
        int bunchID = (data) & 0xFFF;
        std::cout << "[CAEN_V1290]::[DEBUG]::WE FOUND THE TDC HEADER of nr. " << TDCnr << " with eventID " << eventID << " and bunchID " << bunchID << std::endl;     
      #endif
    } else if (wordType == CAEN_V1290_TDCTRAILER ) {
      #ifdef CAENV1290_DEBUG
        int TDCnr = (data>>24) & 0x3;
        int eventID = (data>>12) & 0xFFF;
        int wordCount = (data) & 0xFFF;
        std::cout << "[CAEN_V1290]::[DEBUG]::WE FOUND THE TDC TRAILER of nr. " << TDCnr << " with eventID " << eventID << " and word count " << wordCount << std::endl;     

      #endif
    } else if (wordType == CAEN_V1290_TDCERROR ) {
      std::cout << "[CAEN_V1290]::[ERROR]::TDC ERROR!" << std::endl; 
      v.clear();
      return ERR_READ;
    } else if (wordType == CAEN_V1290_TDCMEASURE ) {
      v.push_back(data);
      //#ifdef CAENV1290_DEBUG
        int measurement = data & 0x1fffff;
        int channel = (data>>21) & 0x1f;
        int trailing = (data>>26) & 0x1;
        float tdc_time = (float)measurement*25./1000.;
        std::cout << "[CAEN_V1290]::[INFO]::HIT CHANNEL " << channel << " TYPE " << trailing << " TIME " << tdc_time << std::endl; 
      //#endif
    } else if (wordType == CAEN_V1290_GLBTRTIMETAG ) {
    } else {
      std::cout << "[CAEN_V1290]::[ERROR]::UNKNOWN WORD TYPE!" << std::endl; 
      v.clear();
      return ERR_READ;
    }
  }

  if (status) {
    std::cout << "[CAEN_V1290]::[ERROR]::READ ERROR!" << std::endl; 
    v.clear();
    return ERR_READ;
  }

  status |= CheckStatusAfterRead();

  if (status) {
    std::cout << "[CAEN_V1290]::[ERROR]::MEB Full or problems reading" << status << std::endl; 
    return ERR_READ;
  }  

  return 0;
}

int CAEN_V1290::CheckStatusAfterRead() {
  if (handle_<0) {
    return ERR_CONF_NOT_FOUND;
  }

  int status = 0; 
  WORD data;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+CAEN_V1290_STATUS_REG,&data,CAEN_V1290_ADDRESSMODE,cvD16);
  if (status) {
    std::cout << "[CAEN_V1290]::[ERROR]::Cannot get status after read from V1290 board " << status << std::endl; 
  }  

  int v1290_EMPTYEVEN       = data & CAEN_V1290_EMPTYEVEN_BITMASK;
  int v1290_RDY             = data & CAEN_V1290_RDY_BITMASK;
  int v1290_FULL            = data & CAEN_V1290_FULL_BITMASK;
  int v1290_TRGMATCH        = data & CAEN_V1290_TRGMATCH_BITMASK;
  int v1290_TERMON          = data & CAEN_V1290_TERMON_BITMASK;
  int v1290_ERROR0          = data & CAEN_V1290_ERROR0_BITMASK;
  int v1290_ERROR1          = data & CAEN_V1290_ERROR1_BITMASK;
  int v1290_ERROR2          = data & CAEN_V1290_ERROR2_BITMASK;
  int v1290_ERROR3          = data & CAEN_V1290_ERROR3_BITMASK;
  int v1290_TRGLOST         = data & CAEN_V1290_TRGLOST_BITMASK;
  int v1290_WORDTYE         = data & CAEN_V1290_WORDTYE_BITMASK;

 

  //std::cout<<"[CAEN_V1290]::[INFO]::Status: "<<std::endl;
  //std::cout<<"v1290_EMPTYEVEN: "<<v1290_EMPTYEVEN<<std::endl;
  //std::cout<<"v1290_RDY: "<<v1290_RDY<<std::endl;
  //std::cout<<"v1290_FULL: "<<v1290_FULL<<std::endl;
  //std::cout<<"v1290_TRGMATCH: "<<v1290_TRGMATCH<<std::endl;
  //std::cout<<"v1290_TERMON: "<<v1290_TERMON<<std::endl;
  //std::cout<<"v1290_ERROR0: "<<v1290_ERROR0<<std::endl;
  //std::cout<<"v1290_ERROR1: "<<v1290_ERROR1<<std::endl;
  //std::cout<<"v1290_ERROR2: "<<v1290_ERROR2<<std::endl;
  //std::cout<<"v1290_ERROR3: "<<v1290_ERROR3<<std::endl;
  //std::cout<<"v1290_TRGLOST: "<<v1290_TRGLOST<<std::endl;
  //std::cout<<"v1290_WORDTYPE: "<<v1290_WORDTYE<<std::endl;
  //std::cout<<std::endl;

  int v1290_error = 0; 
  v1290_error |= data & CAEN_V1290_ERROR0_BITMASK;
  v1290_error |= data & CAEN_V1290_ERROR1_BITMASK;
  v1290_error |= data & CAEN_V1290_ERROR2_BITMASK;
  v1290_error |= data & CAEN_V1290_ERROR3_BITMASK;
  v1290_error |= data & CAEN_V1290_TRGLOST_BITMASK;
  
  if( v1290_FULL || v1290_error ) { 
    std::cout << "[CAEN_V1290]::[INFO]::Trying to restore V1290 board. Reset" << status << std::endl; 
    status=BufferClear();
    if (status) {
      status=Clear();
    }
  } 
  if (status) {
    std::cout << "[CAEN_V1290]::[ERROR]::Cannot restore healthy state in V1290 board " << status << std::endl; 
    return ERR_READ;
  }  

  return 0;
}


int CAEN_V1290::EnableTDCTestMode(WORD testData) {
  int status = 0;

  // I step: Enable the test mode 
  status |= OpWriteTDC(CAEN_V1290_ENABLE_TEST_MODE_OPCODE);
  
  // II step: Write the test word 
  status |= OpWriteTDC(testData);
  
  // III step: Write a t=0x0044 into the test-word high
  status |= OpWriteTDC(0x0044);

  // IV step: set the test output 
  for (unsigned int channel=0; channel<=15; channel++) {  
    status |= OpWriteTDC(CAEN_V1290_SETTESTOUTPUT);
    sleep(0.2);
    status |= OpWriteTDC(channel*channel);
    sleep(0.2);
  }
  
  return status;
}

int CAEN_V1290::DisableTDCTestMode() {
  int status = 0;

  status |= OpWriteTDC(CAEN_V1290_DISABLE_TEST_MODE_OPCODE); 
  return status;
}

int CAEN_V1290::SoftwareTrigger() {
  int status = 0;
  WORD dummy_data = 0x0000;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress + CAEN_V1290_SOFTWARETRIGGERREG,&dummy_data, CAEN_V1290_ADDRESSMODE, cvD16);
  return status;
}


int CAEN_V1290::OpWriteTDC(WORD data) {
  int status;

  
  const int TIMEOUT = 100000;

  int time=0;
  /* Check the Write OK bit */
  WORD rdata=0;
  
  do {
    status = CAENVME_ReadCycle(handle_,configuration_.baseAddress + CAEN_V1290_MICROHANDREG ,&rdata, CAEN_V1290_ADDRESSMODE, cvD16);
    time++;
    //#ifdef CAENV1290_DEBUG
    //  std::cout << "[CAEN_V1290]::[INFO]::Handshake micro op writing " << rdata << " #" << time << " " << status << std::endl;
    //#endif
    sleep(0.01);
  } while (!(rdata & 0x1) && (time < TIMEOUT) );
  
  //std::cout<<"Time to wait for the handshake: "<<time*0.01<<" return data: "<<rdata<<std::endl;


  if ( time == TIMEOUT ) {
    std::cout << "[CAEN_V1290]::[ERROR]::Cannot handshake micro op writing " << status << std::endl; 
    return ERR_WRITE_OP;
  }
  

  status=0;
  
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress + CAEN_V1290_MICROREG, &data, CAEN_V1290_ADDRESSMODE, cvD16);
  if (status) {
    std::cout << "[CAEN_V1290]::[ERROR]::Cannot write micro op " << status << std::endl; 
    return ERR_WRITE_OP;
  }
  sleep(0.2);

  return 0;
}

/*----------------------------------------------------------------------*/

int CAEN_V1290::OpReadTDC(WORD* data) {
  int status;
  const int TIMEOUT = 100000;

  int time=0;
  /* Check the Write OK bit */
  WORD rdata=0;
  do {
    status = CAENVME_ReadCycle(handle_,configuration_.baseAddress +  CAEN_V1290_MICROHANDREG ,&rdata, CAEN_V1290_ADDRESSMODE, cvD16);
    time++;
    //#ifdef CAENV1290_DEBUG
      //std::cout << "[CAEN_V1290]::[INFO]::Handshake micro op reading " << rdata << " #" << time << " " << status << std::endl; 
    //#endif
  } while (!(rdata & 0x2) && (time < TIMEOUT) );

  if ( time == TIMEOUT ) {
    std::cout << "[CAEN_V1290]::[ERROR]::Cannot handshake micro op reading " << status << std::endl; 
    return ERR_WRITE_OP;
  }

  status=0;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress + CAEN_V1290_MICROREG,data, CAEN_V1290_ADDRESSMODE,cvD16);
  if (status) {
    std::cout << "[CAEN_V1290]::[ERROR]::Cannot read micro op " << status << std::endl; 
    return ERR_WRITE_OP;
  }

  return 0;
}







void CAEN_V1290::generatePseudoData(std::vector<WORD> &data) {
  std::default_random_engine generator;
  std::vector< std::normal_distribution<double> > distrs;
  distrs.push_back(std::normal_distribution<double>(600., 10.));
  distrs.push_back(std::normal_distribution<double>(400., 10.));
  distrs.push_back(std::normal_distribution<double>(650., 10.));
  distrs.push_back(std::normal_distribution<double>(390., 10.));
  distrs.push_back(std::normal_distribution<double>(610., 10.));
  distrs.push_back(std::normal_distribution<double>(410., 10.));
  distrs.push_back(std::normal_distribution<double>(660., 10.));
  distrs.push_back(std::normal_distribution<double>(400., 10.));

  WORD bitStream = 0;

  //first the BOE
  unsigned int eventNr = 1;
  bitStream = bitStream | 10<<28;
  data.push_back(bitStream);
  
  //generate the channel information
  for (unsigned int channel=0; channel<8; channel++) {
    unsigned int readout;
    readout=distrs[channel](generator);
    

    for (int N=0; N<20; N++) {
      bitStream = 0;
      bitStream = bitStream | 0<<28;
      bitStream = bitStream | channel<<21;
      bitStream = bitStream | (readout+N);
      data.push_back(bitStream);
    }
  }
  
  //last the BOE
  bitStream = 0;
  bitStream = bitStream | 0x08<<28;
  std::cout<<" 0x10 = "<<0x10<<std::endl;
  std::cout<<" 0x10<<2 = "<<(0x10<<2)<<std::endl;
  std::cout<<" 0x10<<10 = "<<(0x10<<10)<<std::endl;
  std::cout<<" 0x10<<20 = "<<(0x10<<20)<<std::endl;
  std::cout<<" 0x10<<24 = "<<(0x10<<24)<<std::endl;
  std::cout<<" 0x10<<28 = "<<(0x10<<28)<<std::endl;
  std::cout<<" sizeof(0x10<<28) = "<<sizeof(0x10<<28)<<std::endl;
  std::cout<<"Last bitstream: "<<bitStream<<std::endl;
  data.push_back(bitStream);
}
