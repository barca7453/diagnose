/*
 * Copyright (c) 2015 Nutanix Inc. All rights reserved.
 *
 * Author: jayendr.gowrish@nutanix.com
 * Purpose: Log parser, general error diagnoser on customer 
 * clusters
*/
#ifndef _CEREBRO_DIAGNOSE_COMPLETED_METAOP_
#define _CEREBRO_DIAGNOSE_COMPLETED_METAOP_

#include "cerebro/diagnose/infolog_parser.h"

#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

#include <memory>
#include <regex>
#include <iostream>
#include <vector>
#include <map>
#include <sstream>

using namespace std;
namespace nutanix { namespace cerebro { namespace diagnose {

struct TablenameClassMap;
struct Table;

// Parse the completed meta op statement
// Store all the attriubutes in the table
// Key: MetaOPID
struct CompletedMetaopMap{
  typedef boost::char_separator<char> CHAR_SEP;
  typedef map<string,string> ATTRIB_MAP;

  CompletedMetaopMap(){}

  ~CompletedMetaopMap(){}

  void ParseAndStore(const std::string& line, TablenameClassMap::Ptr table_map);

  const string GetTableName() const {
    return "COMPLETED_MOPS";
  }

private:

  void AddToMap(const std::string& key,
    shared_ptr<ATTRIB_MAP> attrib_map,
    TablenameClassMap::Ptr table_map) {
    shared_ptr<Table> table =
        table_map->GetTableObject(GetTableName());
    if (table == nullptr) {
      table = table_map->RegisterClass(GetTableName(),
                               make_shared<CompletedMetaopsTable>());
    }
    table->InsertRecord(key, attrib_map);
  }
};
} } } // namespace
#endif
