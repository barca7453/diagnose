/*
 * Copyright (c) 2015 Nutanix Inc. All rights reserved.
 *
 * Author: jayendr.gowrish@nutanix.com
 * Purpose: Log parser, general error diagnoser on customer 
 * clusters
*/

#include "cerebro/diagnose/file_vdisk.h"
#include "cerebro/base/cerebro_util.h"
#include "cerebro/cerebro_net.h"
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

// Line to be parsed:
// I1216 19:28:02.553179  5620 replicate_meta_op.cc:3629] File: /test-ctr/.acropolis/vmdisk/c61fa50c-c61f-48ad-81cc-7f3c8b7c15ea resolved to vdisk_id: 804020
namespace nutanix { namespace cerebro { namespace diagnose {

void FileVdiskMap::ParseAndStore(const string& line,
    TablenameClassMap::Ptr table_map){
  boost::regex expr{"resolved to vdisk_id"};
  boost::smatch tokens;
  if (boost::regex_search(line, tokens, expr)){
    // Lambda function to populate the element vec
    auto parse_attribs = [] (const string& str, std::regex& re, CHAR_SEP& sep, string& filename) {
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
            filename =  *(++attribute_iter);
            boost::trim(filename);
          }  
        }
      }
    };

    CHAR_SEP colon_sep(":");
    std::regex filename_regex("\\sFile:\\s[A-Za-z0-9\\-\\.\\&\\_\\/]+\\s");
    string filename;
    parse_attribs(line, filename_regex, colon_sep, filename);

    std::regex vdisk_regex("\\svdisk_id:\\s[A-Za-z0-9\\-\\.\\&\\_\\/]+");
    string vdisk_id;
    parse_attribs(line, vdisk_regex, colon_sep, vdisk_id);
    shared_ptr<ATTRIB_MAP> attrib_map  = make_shared<ATTRIB_MAP>();;
    (*attrib_map)["VDISK_ID"] = vdisk_id;
    (*attrib_map)["HOSTED_NODE"] = GetReplicatingNode(filename);
    (*attrib_map)["FILE_PATH"] = filename;
    
    AddToMap(filename, attrib_map, table_map);
  }
}

} } } //namespace


