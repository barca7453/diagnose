/*
 * Copyright (c) 2015 Nutanix Inc. All rights reserved.
 *
 * Author: jayendr.gowrish@nutanix.com
 * Purpose: Log parser, general error diagnoser on customer 
 * clusters
*/

#ifndef _CEREBRO_DIAGNOSE_FILE_PARSER_
#define _CEREBRO_DIAGNOSE_FILE_PARSER_
#include <gflags/gflags.h>
#include "cerebro/diagnose/infolog_parser.h"
#include "cerebro/diagnose/mop_history.h"
#include "cerebro/diagnose/completed_mop.h"
#include "cerebro/diagnose/failfast_fatal.h"
#include <boost/filesystem.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>
#include <fstream>
#include <sstream> 


using namespace std;
using namespace boost::filesystem;
namespace nutanix { namespace cerebro { namespace diagnose {

// Hacky element cache that is used to parse HTML that can span
// multiple lines
extern vector <string> g_parse_cache_;

// Policy based log analyzer
//   * Policy to parse file
//      * FileParser class (log file, trace dump file)
//        * Policy to parse 
//        * Stream policy class
//        * Regular expressions to parse (in a map)
//   * Policy to connect to other nodes (Networker)
//   * Policy for Threading
//   * Policy for memory allocation
//   * Policy to make other decisions (added as thought of) 
//
template <class ParserType>
struct FileParser : public ParserType {
  FileParser() {};
  FileParser(const string& file_path)
  {
    // Verify the log file exists.
    path p(file_path); 
    // Initialize the file vector with a regex
    const std::regex cerebro_filter("cerebro.[A-Za-z0-9\\-\\.\\&\\_]+");
    const std::regex stargate_filter("stargate.[A-Za-z0-9\\-\\.\\&\\_]+");
    const std::regex tgz_filter("[A-Za-z0-9\\-\\.\\&\\_]+.gz");
    boost::filesystem::directory_iterator end_itr; // Default ctor yields past-the-end
    for( boost::filesystem::directory_iterator i(file_path ); i != end_itr; ++i ) {
      // Skip if not a file
      if(!boost::filesystem::is_regular_file(i->status())) {
          cout << "No log file found by that name..";
          continue;
      }

      std::smatch what;
      // skip tzg files
      if (std::regex_match(i->path().filename().string(), what, tgz_filter)) {
          cout << "Skipping file " << i->path().filename().string() << endl;
          continue;
      }

      if(!std::regex_match(i->path().filename().string(), what, cerebro_filter) &&
         !std::regex_match(i->path().filename().string(), what, stargate_filter)) {
          cout << "Skipping file " << i->path().filename().string() << endl;
        continue;
      }

      cout << "Adding to file list " << absolute(i->path().filename(),path(file_path)).string() << endl;
      log_file_names_.push_back(absolute(i->path().filename(),path(file_path)).string());
    }
  }

  ~FileParser() {};

  // TODO more elagant way to go though the file
  void ParseFile(){
    // Parse the parser_spec file
    boost::property_tree::ptree pt;
    boost::property_tree::read_json("parser_spec.json", pt);

    BOOST_FOREACH(boost::property_tree::ptree::value_type& rowPair, pt.get_child( "" ) ) 
    {
        std::cout << rowPair.first << ": " << std::endl;
        BOOST_FOREACH( boost::property_tree::ptree::value_type& itemPair, rowPair.second ) 
        {
            std::cout << "\t" << itemPair.first << " ";
            std::cout << "\t" << itemPair.second.get_value<std::string>() << " ";
            BOOST_FOREACH( boost::property_tree::ptree::value_type& node, itemPair.second ) 
            {
                std::cout << node.second.get_value<std::string>() << " ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;

    for (auto f: log_file_names_) {
      ifstream log_file(f);
      if (log_file.is_open()) {
          cout << "Parsing file " << f << endl;
          string line;
          size_t line_number = 0;
          g_parse_cache_.clear();
          g_parse_cache_.push_back("");
          g_parse_cache_.push_back("");

          while (getline(log_file,line)) {
              this->ParseLine(line);
              g_parse_cache_[line_number % 1] = line; 
              ++line_number;
          }

          cout << "Done loading file " << f << endl;
      }
      log_file.close();
    }
  }
  
  // Channel the query out to the parser
  void ExecQuery(stringstream& ss, const string& table_name, const string& col_list, const string& filter_str) {
    this->Query(ss, table_name, col_list, filter_str);
  }

  void ExecJoin (stringstream& ss,
    const string& l_table_name,
    const string& r_table_name,
    const string& col_to_join,
    const string& col_list,
    const string& filter_str) {
      this->Join(ss, l_table_name, r_table_name, col_to_join, col_list, filter_str);
  }

protected:

private:
  vector<string> log_file_names_;

  ifstream log_file_stream_; // make this unique_ptr with move 
                             // semantics
                             // Or maybe, make this a policy
};


} } } // namespace

#endif
