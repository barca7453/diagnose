/*
 * Copyright (c) 2015 Nutanix Inc. All rights reserved.
 *
 * Author: jayendr.gowrish@nutanix.com
 * Purpose: Log parser, general error diagnoser on customer 
 * clusters
*/

#include "cerebro/diagnose/completed_mop.h"
#include "cerebro/base/cerebro_util.h"
#include "cerebro/cerebro_net.h"
#include "stats/progress_monitor/progress_monitor_backend.h"
#include "util/base/walltime.h"
#include "util/misc/init_nutanix.h"
#include "util/net/onc_rpc.h"
#include "util/nfs/nfs_client.h"
#include "util/nfs/nfs_util.h"

using namespace nutanix;
using namespace nutanix::cerebro;
using namespace nutanix::cerebro::interface;
using namespace nutanix::cerebro::master;
using namespace nutanix::net;
using namespace nutanix::nfs;
using namespace nutanix::progress_monitor;
using namespace nutanix::thread;
using namespace nutanix::zeus;

using namespace std;
using namespace std::placeholders;

namespace nutanix { namespace cerebro { namespace diagnose {

void CompletedMetaopMap::ParseAndStore(const string& line,
  TablenameClassMap::Ptr table_map){
  
  // Parse out "Top level meta op completed line
  boost::regex expr{"Top level"};
  boost::smatch tokens;
  if (boost::regex_search(line, tokens, expr)){
    // Create attribute map
    shared_ptr<ATTRIB_MAP> elem_map = make_shared<ATTRIB_MAP>();
    
    // Tokenize by "," 
    string meta_opid;
    CHAR_SEP comma_sep(",");
    boost::tokenizer<CHAR_SEP> tok(line, comma_sep);
    for_each(begin(tok), end(tok), [&](const string& token) { 
        // Tokenize by "="
        // Get the individual attrs
        CHAR_SEP eq_sep("=");
        boost::tokenizer<CHAR_SEP> elem_tok(token, eq_sep);
        boost::tokenizer<CHAR_SEP>::iterator iter = elem_tok.begin();
        boost::tokenizer<CHAR_SEP>::iterator end = elem_tok.end();
        if (std::distance(iter,end) >= 2){
          string key = *(iter);
          boost::trim(key);
          string value =  *(++iter);
          boost::trim(value);

          // Lambda function to populate the element vec
          // Takes a string that has attributes and creates
          // individual attr elemes
          // snap=(a,b,c), ref_snap=(e,f,r) remote=rem_1
          // becomes
          // [snap]:[(a,b,c)] [ref_snap]:[(e,f,r)]..etc in the attr map 
          auto parse_attribs = [] (const string& str, std::regex& re, CHAR_SEP& sep, shared_ptr<ATTRIB_MAP> up) {
            std::smatch attrs_match;
            if (std::regex_search(str, attrs_match, re)) {
              for (size_t i = 0; i < attrs_match.size(); ++i) {
                std::ssub_match sub_match = attrs_match[i];
                std::string attribute = sub_match.str();
                // Add this attribute as a first class citizen
                boost::tokenizer<CHAR_SEP> attribute_tok(attribute, sep);
                boost::tokenizer<CHAR_SEP>::iterator attribute_iter = attribute_tok.begin();
                boost::tokenizer<CHAR_SEP>::iterator attribute_end = attribute_tok.end();
                if (std::distance(attribute_iter,attribute_end) >= 2){
                  string attribute_key = *(attribute_iter);
                  boost::trim(attribute_key);
                  string attribute_value =  *(++attribute_iter);
                  boost::trim(attribute_value);
                  up->insert(make_pair(attribute_key,attribute_value));
                }  
              }
            }
          };

          // Record the Uniq key which is the meta op id
          // for this table
          if (key == "Meta opid"){
            meta_opid = value;
            elem_map->insert(make_pair(key, value));

            // Add the time stamp
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
            elem_map->insert(make_pair("TIMESTAMP", boost_ts));
          } else if (key == "Attributes") {
            // Attributes of the meta op to be grouped separately
            // Re-parse the line capture substring
            // Attributes =  ..... to .... Aborted
            int pos_beg = line.find(key);
            int pos_end = line.find(", Aborted");
            string attribute_set = line.substr(pos_beg, pos_end - pos_beg);
            attribute_set.erase(0,13); // remove "Attributes = "

            std::regex attrs_regex("entity=\\([0-9]+,\\s[0-9]+,\\s[0-9]+\\)");
            parse_attribs(attribute_set, attrs_regex, eq_sep, elem_map);

            attrs_regex = ("snapshot=\\([0-9]+,\\s[0-9]+,\\s[0-9]+\\)");
            parse_attribs(attribute_set, attrs_regex, eq_sep, elem_map);

            attrs_regex = ("reference_snapshot=\\([0-9]+,\\s[0-9]+,\\s[0-9]+\\)");
            parse_attribs(attribute_set, attrs_regex, eq_sep, elem_map);

            attrs_regex = ("remote=[A-Za-z0-9\\-\\.\\&\\_]+\\s");
            parse_attribs(attribute_set, attrs_regex, eq_sep, elem_map);

            attrs_regex = ("replicated_bytes=[0-9]+\\s");
            parse_attribs(attribute_set, attrs_regex, eq_sep, elem_map);

            attrs_regex = ("tx_bytes=[0-9]+");
            parse_attribs(attribute_set, attrs_regex, eq_sep, elem_map);

            // path prefix
            attrs_regex = ("path_prefix=[A-Za-z0-9\\-\\.\\_\\/]+\\s");
            parse_attribs(attribute_set, attrs_regex, eq_sep, elem_map);
            
            // replace entities
            attrs_regex = ("replace_entities=[A-Za-z]+");
            parse_attribs(attribute_set, attrs_regex, eq_sep, elem_map);

            // target hyper_type
            attrs_regex = ("target_hypervisor_type=[A-Za-z]+");
            parse_attribs(attribute_set, attrs_regex, eq_sep, elem_map);
         } else {
            // Bunch of spurious entries that ALREADY 
            // get handled under the previous Attrtibutes
            // section show up here, skip adding them
            if ((key.find("entity") == string::npos) &&
                (key.find("snapshot") == string::npos) &&
                (key.find("path_prefix") == string::npos) &&
                (key.find("replicated_bytes") == string::npos) &&
                (key.find("reference_snapshot") == string::npos)) {
                elem_map->insert(make_pair(key,value));
            }
         }
       }
    });

    // Add the attribute vector to the meta op map
    if (!meta_opid.empty()){
      AddToMap(meta_opid, elem_map, table_map);
    }
 }
}

} } } // namespace
