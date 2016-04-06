/*
 * Copyright (c) 2015 Nutanix Inc. All rights reserved.
 *
 * Author: jayendr.gowrish@nutanix.com
 * Purpose: Log parser, general error diagnoser on customer 
 * clusters
*/

#ifndef _CEREBRO_MASTER_IP_TIMESTAMP
#define _CEREBRO_MASTER_IP_TIMESTAMP

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
struct MasterIpTimestamp{
    typedef map<string,string> ATTRIB_MAP;
    MasterIpTimestamp(){}

    ~MasterIpTimestamp(){}

    void ParseAndStore(const string& line,
        TablenameClassMap::Ptr table_map) {
        boost::regex master_changed_expr{"Cerebro master changed to"};
        boost::smatch tokens;
        if (boost::regex_search(line, tokens, master_changed_expr)){
            string ip_address;
            // Add the time stamp
            //boost::regex timestamp_expr{"I[0-9]+\\s[0-9]+\\:[0-9]+\\:[0-9]+\\.[0-9]+"};
            CHAR_SEP space_sep(" ");     
            boost::tokenizer<CHAR_SEP> elem_tok(line, space_sep);
            boost::tokenizer<CHAR_SEP>::iterator iter = elem_tok.begin();
            boost::tokenizer<CHAR_SEP>::iterator end = elem_tok.end();
            const string date_str = *iter;
            const string mm = date_str.substr(1,2);
            const string dd = date_str.substr(3,2);
            const string yy = "9999"; // Dummy to make it compatible and flat need 
                                      // to handle dec-Jan rollover somehow
            const string timestamp = *(++iter);
            // Create a boost compatible string 
            const string boost_ts = yy + "-" + mm + "-" + dd
                + " " + timestamp;
            AddToMap(boost_ts, "TIMESTAMP", boost_ts,  table_map);

            string delimiter = "Cerebro master changed to ";
            size_t pos = 0, pos_plus = 0;
            pos = line.find(delimiter) + 26;
            pos_plus = line.find("+") - 5;

            ip_address = line.substr(pos,(pos_plus - pos)); 
            boost::trim(ip_address);
            AddToMap(boost_ts, "MASTER_IP_ADDRESS", ip_address, table_map);
        }
    }

    const string GetTableName() const {
        return "MASTER_IP_TIMESTAMP";
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
                make_shared<MasterIpTimestampTable>());
        }

        if (!table->Exists(key)){
            shared_ptr<ATTRIB_MAP> attrib_map = make_shared<ATTRIB_MAP>();
            (*attrib_map)[attr] = message;
            table->InsertRecord(key,attrib_map);
        } else {
            table->UpdateRecord (key, attr, message, false);
        }
    }

};

} } } // namespace

#endif
