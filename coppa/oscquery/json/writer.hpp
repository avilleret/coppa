#pragma once
#include <coppa/oscquery/parameter.hpp>
#include <coppa/oscquery/json/keys.hpp>

#include <jeayeson/jeayeson.hpp>
#include <boost/tokenizer.hpp>
#include <boost/bimap.hpp>
#include <boost/assign.hpp>
namespace coppa
{
namespace oscquery
{
namespace JSONFormat
{
namespace detail
{
static void addValueToJsonArray(
    json_array& array,
    const Variant& val)
{
  using namespace eggs::variants;
  switch(val.which())
  {
    case 0: array.add(get<int>(val)); break;
    case 1: array.add(get<float>(val)); break;
      //case 2: array.add(get<bool>(val)); break;
    case 3: array.add(get<std::string>(val)); break;
    case 4: array.add(get<const char*>(val)); break;
  }
}

static json_array getJsonValueArray(
    const Values& values)
{
  json_array value_arr;

  for(const auto& value : values.values)
  {
    addValueToJsonArray(value_arr, value);
  }

  return value_arr;
}

static json_array getJsonClipModeArray(
    const ClipModes& clipmodes)
{
  static const boost::bimap<std::string, ClipMode> clipmodeMap =
      boost::assign::list_of<boost::bimap<std::string, ClipMode>::relation>
      ("None", ClipMode::None)
      ("Low",  ClipMode::Low)
      ("High", ClipMode::High)
      ("Both", ClipMode::Both);

  json_array clip_arr;
  for(const auto& clipmode : clipmodes.clipmodes)
  {
    clip_arr.add(clipmodeMap.right.at(clipmode));
  }

  return clip_arr;
}

static json_array getJsonRangeArray(
    const Ranges& ranges)
{
  json_array range_arr;
  for(const auto& range : ranges.ranges)
  {
    json_array range_subarray;
    if(!range.min)
    { range_subarray.add(json_value::null_t{}); }
    else
    { addValueToJsonArray(range_subarray, *range.min); }

    if(!range.max)
    { range_subarray.add(json_value::null_t{}); }
    else
    { addValueToJsonArray(range_subarray, *range.max); }

    if(range.values.empty())
    { range_subarray.add(json_value::null_t{}); }
    else
    {
      json_array range_values_array;
      for(auto& elt : range.values)
      {
        addValueToJsonArray(range_values_array, elt);
      }
      range_subarray.add(range_values_array);
    }

    range_arr.add(range_subarray);
  }

  return range_arr;
}

static json_array getJsonTags(
    const Tags& tags)
{
  json_array arr;
  for(const auto& tag : tags.tags)
  {
    arr.add(tag);
  }

  return arr;
}


inline auto attributeToJson(const Values& val) { return getJsonValueArray(val); }
inline auto attributeToJson(const Ranges& val) { return getJsonRangeArray(val); }
inline auto attributeToJson(const ClipModes& val) { return getJsonClipModeArray(val); }
inline auto attributeToJson(const Access& val) { return static_cast<int>(val.accessmode); }
inline auto attributeToJson(const Description& val) { return val.description; }
inline auto attributeToJson(const Tags& val) { return getJsonTags(val); }

static std::string getJsonTypeString(
    const Parameter& parameter)
{
  std::string str_type;
  for(const auto& value : parameter.values)
  {
    switch(value.which())
    {
      case 0: str_type += "i"; break;
      case 1: str_type += "f"; break;
        // case 2: str_type += "B"; break; -> no bool
      case 3: str_type += "s"; break;
      case 4: str_type += "b"; break;
    }
  }

  return str_type;
}


static json_map attributeToJson(
    const Parameter& parameter,
    const std::string& method)
{
  json_map map;

  if(method == Key::attribute<Values>())
  { map.set(method, getJsonValueArray(parameter)); }
  else if(method == Key::attribute<Ranges>())
  { map.set(method, getJsonRangeArray(parameter)); }
  else if(method == Key::attribute<ClipModes>())
  { map.set(method, getJsonClipModeArray(parameter)); }
  else if(method == Key::attribute<Access>())
  { map.set(method, static_cast<int>(parameter.accessmode)); }
  else if(method == Key::type())
  { map.set(method, getJsonTypeString(parameter)); }
  else if(method == Key::attribute<Description>())
  { map.set(method, parameter.description); }
  else if(method == Key::attribute<Tags>())
  { map.set(method, getJsonTags(parameter)); }
  else if(method == Key::full_path())
  { map.set(method, parameter.destination); }

  return map;
}

static void parameterToJson(
    const Parameter& parameter,
    json_map& obj)
{
  using namespace std;
  using namespace boost;
  using namespace eggs::variants;

  // These attributes are always here
  obj.set(Key::full_path(), parameter.destination);
  obj.set(Key::attribute<Access>(), static_cast<int>(parameter.accessmode));

  // Potentially empty attributes :
  // Description
  if(!parameter.description.empty())
  {
    obj.set(Key::attribute<Description>(), parameter.description);
  }

  // Tags
  if(!parameter.tags.empty())
  {
    obj.set(Key::attribute<Tags>(), getJsonTags(parameter));
  }

  // Handling of the types / values
  if(!parameter.values.empty())
  {
    obj.set(Key::type(), getJsonTypeString(parameter));
    obj.set(Key::attribute<Values>(), getJsonValueArray(parameter));
    obj.set(Key::attribute<Ranges>(), getJsonRangeArray(parameter));
    obj.set(Key::attribute<ClipModes>(), getJsonClipModeArray(parameter));
  }
}

// A ParameterMap can be JSON'd
template<typename Map>
static json_map mapToJson(
    const Map& theMap,
    const std::string& root)
{
  using namespace std;
  using namespace boost;
  using namespace eggs::variants;
  // Root node
  json_map localroot;

  // Create a tree with the parameters
  for(const auto& parameter : filter(theMap, root))
  {
    // Trunk the given root from the parameters
    auto trunked_dest = parameter.destination;
    if(root != "/")
      trunked_dest.erase(0, root.length());

    char_separator<char> sep("/");
    tokenizer<char_separator<char>> tokens(trunked_dest, sep);

    // Create the required parts of the tree and navigate to the corresponding node
    auto current_map = &localroot;
    for(const auto& token : tokens)
    {
      // Note : see this in relation the osc method part of the spec
      if(!current_map->has(Key::contents()))
      { current_map->set(Key::contents(), json_map{}); }

      current_map = &current_map->get_for_path<json_map>(Key::contents());

      if(!current_map->has(token))
      { current_map->set(token, json_map{}); }

      current_map = &current_map->get_for_path<json_map>(token);
    }

    parameterToJson(parameter, *current_map);
  }

  return localroot;
}
} // End of namespace detail

static std::string deviceInfo(int port)
{
  json_map map;
  map[Key::osc_port()] = port;

  return map.to_string();
}

// Format interface
template<typename... Args>
static std::string marshallParameterMap(
    Args&&... args)
{
  using namespace detail;
  return mapToJson(std::forward<Args>(args)...).to_string();
}

template<typename... Args>
static std::string marshallAttribute(
    Args&&... args)
{
  using namespace detail;
  return attributeToJson(std::forward<Args>(args)...).to_string();
}


template<typename... Args>
static std::string path_added(
    Args&&... args)
{
  using namespace detail;
  json_map map;
  map[Key::path_added()] = mapToJson(std::forward<Args>(args)...);
  return map.to_string();
}

template<typename... Args>
static std::string change_path(
    Args&&... args)
{
  using namespace detail;
  json_map map;
  map[Key::path_changed()] = mapToJson(std::forward<Args>(args)...);
  return map.to_string();
}

static std::string path_removed(
    const std::string& path)
{
  using namespace detail;
  json_map map;
  map[Key::path_removed()] = path;
  return map.to_string();
}

namespace detail {
// The following three methods are here
// to make the attributes_changed message.
// End of recursion
template<typename Attribute>
static void addAttributes(
    json_map& map,
    const Attribute& attr)
{
  using namespace detail;
  map[Key::attribute(attr)] = attributeToJson(attr);
}

template<typename Attribute, typename... Attributes>
static void addAttributes(
    json_map& map,
    const Attribute& attr,
    Attributes&&... attrs)
{
  using namespace detail;
  map[attributeToKey(attr)] = attributeToJson(attr);
  addAttributes(std::forward<Attributes>(attrs)...);
}
}

template<typename... Attributes>
static std::string attributes_changed(
    const std::string& path,
    Attributes&&... attrs)
{
  using namespace detail;
  // TODO what if type changed?
  json_map objmap;
  objmap[Key::full_path()] = path;

  addAttributes(objmap, std::forward<Attributes>(attrs)...);

  json_map map;
  map[Key::attributes_changed()] = objmap;
  return map.to_string();
}



template<typename Map, typename Vector>
static std::string paths_added(
    const Map& theMap,
    const Vector& vec)
{
  using namespace detail;
  json_array arr;
  for(const auto& elt : vec)
  {
    arr.push_back(mapToJson(theMap, elt));
  }

  json_map map;
  map[Key::paths_added()] = arr;
  return map.to_string();
}

template<typename Map, typename Vector>
static std::string pahs_changed(
    const Map& theMap,
    const Vector& vec)
{
  using namespace detail;
  json_array arr;
  for(const auto& elt : vec)
  {
    arr.push_back(mapToJson(theMap, elt));
  }

  json_map map;
  map[Key::paths_changed()] = arr;
  return map.to_string();
}

template<typename Vector>
static std::string paths_removed(
    const Vector& vec)
{
  using namespace detail;
  json_array arr;
  for(const auto& elt : vec)
  {
    arr.push_back(elt);
  }

  json_map map;
  map[Key::paths_removed()] = arr;
  return map.to_string();
}

template<typename... Attributes>
static std::string attributes_changed_array(
    const std::string& path,
    Attributes&&... attrs)
{
  using namespace detail;
  // TODO what if type changed?
  json_map objmap;
  objmap[Key::full_path()] = path;

  addAttributes(objmap, std::forward<Attributes>(attrs)...);

  json_map map;
  map[Key::attributes_changed()] = objmap;
  return map.to_string();
}


}
}
}

