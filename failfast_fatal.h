/*
 * Copyright (c) 2015 Nutanix Inc. All rights reserved.
 *
 * Author: jayendr.gowrish@nutanix.com
 * Purpose: Log parser, general error diagnoser on customer 
 * clusters
*/
#ifndef _CEREBRO_DIAGNOSE_STARGATE_FATAL_
#define _CEREBRO_DIAGNOSE_STARGATE_FATAL_

#include "cerebro/diagnose/infolog_parser.h"

#include <stdlib.h>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <memory>
#include <regex>
#include <iostream>
#include <vector>
#include <map>
#include <fstream>

using namespace std;
namespace nutanix { namespace cerebro { namespace diagnose {
// Key:   opid
// Value: OpCode, DumpedMessage
struct FailFastFatal {
  typedef boost::char_separator<char> CHAR_SEP;
  typedef map<string,string> ATTRIB_MAP;

  FailFastFatal(){}

  ~FailFastFatal(){}

  void ParseAndStore(const std::string& line, TablenameClassMap::Ptr table_map);
 
  const string GetTableName() const {
      return "FATAL_COMPONENT_OPID";
  }

private:
 
  void AddToMap(const std::string& key,
    shared_ptr<ATTRIB_MAP> attrib_map,
    TablenameClassMap::Ptr table_map) {
    shared_ptr<Table> table =
        table_map->GetTableObject(GetTableName());
    if (table == nullptr) {
      table = table_map->RegisterClass(GetTableName(),
                               make_shared<FileVDiskTable>());
    }
    table->InsertRecord(key, attrib_map);
  }

};
} } } // end



#endif
