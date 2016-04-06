/*
 * Copyright (c) 2015 Nutanix Inc. All rights reserved.
 *
 * Author: jayendr.gowrish@nutanix.com
 * Purpose: Log parser, general error diagnoser on customer 
 * clusters
*/

#ifndef _CEREBRO_METAOP_HISTORY_
#define _CEREBRO_METAOP_HISTORY_

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
struct MetaopHistoryStorageMap{
    typedef map<string,string> ATTRIB_MAP;
    MetaopHistoryStorageMap(){}

    ~MetaopHistoryStorageMap(){}

    void ParseAndStore(const string& line, TablenameClassMap::Ptr table_map);

    const string GetTableName() const {
        return "MOPS_HISTORY";
    }

private:

    void AddToMap(const string& key,
       const string& attr,
       const string& message,
       TablenameClassMap::Ptr table_map) {
        shared_ptr<Table> table =
            table_map->GetTableObject(GetTableName());
        if (table == nullptr) {
            table = table_map->RegisterClass(GetTableName(),
                make_shared<MopsHistoryTable>());
        }

        if (!table->Exists(key)){
            shared_ptr<ATTRIB_MAP> attrib_map = make_shared<ATTRIB_MAP>();
            (*attrib_map)[attr] = message;
            table->InsertRecord(key,attrib_map);
        } else if (table->Exists(key) && (attr == "HISTORY")){
            table->UpdateRecord (key, attr, message, true);
        }
    }

    void static PrintEachMetaop(std::pair<const std::string, std::vector<std::string>>& elem, 
                                const string& filter = "") {
      cout << "\n\nMetaop ID: " << elem.first << endl ;
      for (auto& e: elem.second) {
          cout <<"     " << e << endl;
      }
    }
};

} } } // namespace

#endif
