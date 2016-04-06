/*
 * Copyright (c) 2015 Nutanix Inc. All rights reserved.
 *
 * Author: jayendr.gowrish@nutanix.com
 * Purpose: Log parser, general error diagnoser on customer 
 * clusters
*/

#ifndef _CEREBRO_FILE_VDISK_MOP_
#define _CEREBRO_FILE_VDISK_MOP_

#include "cerebro/diagnose/infolog_parser.h"

#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <memory>
#include <regex>
#include <iostream>

using namespace std;
namespace nutanix { namespace cerebro { namespace diagnose {

struct TablenameClassMap;
struct Table;

typedef boost::char_separator<char> CHAR_SEP;
// Parse all lines that match meta_opid: <id>.
// Store the message as part of the history.
// Might need to scan the messages to minie more data.
struct FileWorkIDMop{
    typedef map<string,string> ATTRIB_MAP;
    FileWorkIDMop(){}

    ~FileWorkIDMop(){}

    void ParseAndStore(const string& line, TablenameClassMap::Ptr table_map);

    const string GetTableName() const {
        return "FILE_WORKID_MOP";
    }

private:

  void AddToMap(const std::string& key,
    shared_ptr<ATTRIB_MAP> attrib_map,
    TablenameClassMap::Ptr table_map) {
    shared_ptr<Table> table =
        table_map->GetTableObject(GetTableName());
    if (table == nullptr) {
      table = table_map->RegisterClass(GetTableName(),
                               make_shared<FileWorkIDMopTable>());
    }
    table->InsertRecord(key, attrib_map);
  }

};

} } } // namespace

#endif
