#include "eudaq/DataCollector.hh"
#include "eudaq/Event.hh"

#include <boost/date_time/posix_time/posix_time.hpp>

#include <deque>
#include <map>
#include <string>

namespace eudaq {
  class EventnumberSyncDataCollector :public DataCollector{
  public:
    using DataCollector::DataCollector;
    void DoStartRun() override;
    void DoTerminate() override;
    void DoConnect(ConnectionSPC /*id*/) override;
    void DoDisconnect(ConnectionSPC /*id*/) override;
    void DoReceive(ConnectionSPC id, EventSP ev) override;
    static const uint32_t m_id_factory = eudaq::cstr2hash("EventnumberSyncDataCollector");
  private:
    std::map<std::string, std::deque<EventSPC>> m_que_event;
    std::mutex m_mtx_map;
  };

  namespace{
    auto dummy0 = Factory<DataCollector>::
      Register<EventnumberSyncDataCollector, const std::string&, const std::string&>
      (EventnumberSyncDataCollector::m_id_factory);
  }

  void EventnumberSyncDataCollector::DoStartRun(){
    std::unique_lock<std::mutex> lk(m_mtx_map);
    for(auto &que :m_que_event){
      que.second.clear();
    }
  }
  
  void EventnumberSyncDataCollector::DoConnect(ConnectionSPC id){
    std::unique_lock<std::mutex> lk(m_mtx_map);
    std::string pdc_name = id->GetName();
    EUDAQ_INFO("Producer."+pdc_name+" is connecting");
    if(m_que_event.find(pdc_name) == m_que_event.end())
      //EUDAQ_THROW("DataCollector::Doconnect, multiple producers are sharing a same name");
      m_que_event[pdc_name];
  }
  
  void EventnumberSyncDataCollector::DoDisconnect(ConnectionSPC id){
    std::unique_lock<std::mutex> lk(m_mtx_map);
    std::string pdc_name = id->GetName();
    if(m_que_event.find(pdc_name) != m_que_event.end()){
      //EUDAQ_THROW("DataCollector::DisDoconnect, the disconnecting producer was not existing in list");
      EUDAQ_WARN("Producer."+pdc_name+" is disconnected, the remaining events are erased. ("+std::to_string(m_que_event[pdc_name].size())+ " Events)");
      m_que_event.erase(pdc_name);
    }
  }
  
  void EventnumberSyncDataCollector::DoReceive(ConnectionSPC id, EventSP ev){
    std::unique_lock<std::mutex> lk(m_mtx_map);
    std::string pdc_name = id->GetName();
    m_que_event[pdc_name].push_back(std::move(ev));
    uint32_t n = 0;
    for(auto &que :m_que_event){
      if(!que.second.empty())
	n++;
    }
    if(n==m_que_event.size()){
      auto ev_wrap = Event::MakeUnique(GetFullName());
      ev_wrap->SetFlagPacket();
      uint32_t ev_c = m_que_event.begin()->second.front()->GetEventN();
      bool match = true;
      for(auto &que :m_que_event){
	if(ev_c != que.second.front()->GetEventN())
	  match = false;
	ev_wrap->AddSubEvent(que.second.front());
	que.second.pop_front();
      }
      if(!match){
	EUDAQ_WARN("EventNumbers are Mismatched");
      }
      boost::posix_time::ptime epoch = boost::posix_time::time_from_string("1970-01-01 00:00:00.000");
      boost::posix_time::ptime current_time = boost::posix_time::microsec_clock::local_time();
      uint64_t t0=(current_time-epoch).total_milliseconds() & 0xffffffffffffffff;
      uint64_t t1=0;//no need for more than 64 bits a priori
      ev_wrap->SetTimestamp( t0,t1 ) ;
      WriteEvent(std::move(ev_wrap));
    }
  }

  void EventnumberSyncDataCollector::DoTerminate(){
    m_que_event.erase( m_que_event.begin(),m_que_event.end() );
  }
}
