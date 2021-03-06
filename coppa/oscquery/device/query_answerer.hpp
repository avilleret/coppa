#pragma once
#include <string>
#include <map>
#include <vector>
#include <coppa/exceptions/BadRequest.hpp>
#include <coppa/string_view.hpp>
namespace coppa {
namespace oscquery {
/**
 * @brief The Answerer class
 *
 * OSCQuery query-answering logic.
 */
class answerer
{
  public:
    template<typename Device>
    static auto answer (Device& dev, typename Device::query_server_type::connection_handler& hdl)
    {
      return [&] (
          const std::string& path,
          const std::map<std::string, std::string>& parameters)
      {
        // Here we handle the url elements relative to oscquery
        if(parameters.size() == 0)
        {
          auto&& lock = dev.map().acquire_read_lock();
          return json::writer::query_namespace(dev.map(), path);
        }
        else
        {
          // First check if we have the path
          if(!dev.map().has(path))
            throw PathNotFoundException{path};

          // Listen
          auto listen_it = parameters.find("listen");
          if(listen_it != end(parameters))
          {
            // First we find for a corresponding client
            auto it = find(begin(dev.clients()), end(dev.clients()), hdl);
            if(it == end(dev.clients()))
              throw BadRequestException{"Client not found"};

            // Then we enable / disable listening
            if(listen_it->second == "true")
            {
              it->addListenedPath(path);
            }
            else if(listen_it->second == "false")
            {
              it->removeListenedPath(path);
            }
            else
            {
              throw BadRequestException{"Wrong arguments to listen query"};
            }
          }

          // All the value-less parameters
          std::vector<std::string> attributes;
          attributes.reserve(5);
          for(const auto& elt : parameters)
          {
            if(elt.second.empty())
            {
              attributes.push_back(elt.first);
            }
          }

          if(!attributes.empty())
          {
            return json::writer::query_attributes(
                  dev.map().get(path),
                  attributes);
          }
        }

        return std::string{};
      };
    }

};
}}
