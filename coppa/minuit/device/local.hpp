#pragma once
#include <coppa/minuit/parameter.hpp>
#include <coppa/device/local.hpp>
#include <coppa/minuit/device/message_handler.hpp>
#include <coppa/map.hpp>

#include <coppa/protocol/osc/oscreceiver.hpp>
#include <coppa/protocol/osc/oscsender.hpp>
#include <coppa/protocol/osc/oscmessagegenerator.hpp>
#include <coppa/string_view.hpp>
#include <nano-signal-slot/nano_signal_slot.hpp>
namespace coppa
{
namespace ossia
{
// Servers to do : 
// Local OSC device : 
// Can just be set from the outside.

// callbacks : try with callback on each member, 
// and global "value changed" callback


// Shmem callbacks ? 
// IPC ?
template<typename Map,
         typename DataProtocolServer,
         typename DataProtocolHandler>
class osc_local_device : public Map
{
    
  public:
    using map_type = Map;
    using parent_t = osc_local_device<Map, DataProtocolServer, DataProtocolHandler>;
    
    osc_local_device( 
        Map& map,
        int data_port):
      m_map{map},
      m_data_server{data_port, [&] (const auto& m)
      { DataProtocolHandler::on_messageReceived(*this, m); }}
    {
      m_data_server.run();
    }
    
    template<typename String, typename Arg>
    void update(param_t<String> path, Arg&& val)
    {
      if(bool ok = m_map.update(path, std::forward<Arg>(val)))
        on_value_changed.emit(m_map.get(path));
    }
    
    Nano::Signal<void(const Parameter&)> on_value_changed;
  
  private:
    Map& m_map;
    DataProtocolServer m_data_server;
};

class osc_local_impl : public osc_local_device<
    coppa::locked_map<coppa::basic_map<ParameterMapType<coppa::ossia::Parameter>>>,
    coppa::osc::receiver,
    coppa::ossia::message_handler>
{
    using parent_t::osc_local_device;
};
    

// Remote OSC device : 
// Can just send data to the outside

// Updating OSC device
// Can be set from t
// Local Minuit device, connected to a remote device (fixed)
// Local Minuit device, that will reply to every client (no registering)
// Local Minuit device, that will reply to every client (registering & garbage collection)
// Updating Minuit device
}
}
