/*
 * Copyright (c) 2015 Nutanix Inc. All rights reserved.
 *
 * Author: jayendr.gowrish@nutanix.com
 * Purpose: Log parser, general error diagnoser on customer 
 * clusters
*/
#ifndef _CEREBRO_DIAGNOSE_FILE_VDISK_
#define _CEREBRO_DIAGNOSE_FILE_VDISK_

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
// Key:   file_name
// Value: vdisk_id
struct FileVdiskMap {
  typedef boost::char_separator<char> CHAR_SEP;
  typedef map<string,string> ATTRIB_MAP;

  FileVdiskMap(){}

  ~FileVdiskMap(){}

  void ParseAndStore(const std::string& line, TablenameClassMap::Ptr table_map);
 
  const string GetTableName() const {
      return "FILENAME_VDISKID";
  }

private:
 
  // Gets the node IPs, for each IP
  // grep for the file name in stargate logs
  // A hit shows that the file was hosted
  // on the node at that time
  string GetReplicatingNode(const string& filename) {
      if (filename.empty()){
          return "";
      }

      typedef boost::char_separator<char> CHAR_SEP;
      system ("svmips &> /tmp/svmips_out");
      // Parse IP address file
      ifstream log_file("/tmp/svmips_out");
      string replicating_node = "";
      if (log_file.is_open()) {
        string line;
        if (getline(log_file,line)) {
          CHAR_SEP space_sep(" ");
          boost::tokenizer<CHAR_SEP> tok(line, space_sep);
          for_each(begin(tok), end(tok), [&](const string& ip_address) { 
              std::stringstream command;
              command << "ssh nutanix@" << ip_address << " \"cd data/logs;grep \\\"" << filename
                      << "\\\" * --include stargate\\* \" &> /tmp/node_grep_output.tmp" << endl;
              // cout << "COMMAND is " << command.str() << endl; 
              system (command.str().c_str());
              // Parse grep_output
              // Look for the file name in each line
              // First hit - ABORT, you found the node
              ifstream grep_file("/tmp/node_grep_output.tmp");
              if (grep_file.is_open()) {
                string grep_line;
                while (getline(grep_file,grep_line)) {
                  boost::regex expr{filename};
                  boost::smatch tokens;
                  if (boost::regex_search(grep_line, tokens, expr)){
                     replicating_node = ip_address;
                     break;
                  }
                }
                grep_file.close();
              }
          });
        }
      }
      log_file.close();
      
      return replicating_node;
  }

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

} } } //namespace

#endif

