/*
 * Copyright (c) 2015 Nutanix Inc. All rights reserved.
 *
 * Author: jayendr.gowrish@nutanix.com
 * Purpose: Log parser, general error diagnosis tool on customer 
 * clusters
*/

#include "cerebro/diagnose/stargate_trace.h"
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

vector <string> g_parse_cache_;
void TraceDump::ParseAndStore(const string& line,
  TablenameClassMap::Ptr table_map){
  //<td align="left"><pre><b>ExtentGroupWriteOp
  //disk=32 primary=1 intent_sequence=2 egroup=59590628 opid=173080350 </b>
  // Regex match for opid=123123 </b>
  std::regex opid_expr{"opid=[0-9]+\\s</b>"};
  std::smatch tokens;
  if (std::regex_search(line, tokens, opid_expr)){
      string opid;

      std::regex attribute{"[A-Za-z]+=[A-Za-z0-9\\-\\.\\_\\/]+"};

      auto attribute_begin = std::sregex_iterator(line.begin(), line.end(), attribute);
      auto attribute_end   = std::sregex_iterator();

      shared_ptr<ATTRIB_MAP> attrib_map  = make_shared<ATTRIB_MAP>();
      for (std::sregex_iterator i = attribute_begin; i != attribute_end; ++i) {
          std::smatch match = *i;                                                 
          std::string match_str = match.str(); 

          CHAR_SEP eq_sep("=");
          boost::tokenizer<CHAR_SEP> attribute_tok(match_str, eq_sep);
          boost::tokenizer<CHAR_SEP>::iterator attribute_iter = attribute_tok.begin();
          boost::tokenizer<CHAR_SEP>::iterator end = attribute_tok.end();

          if (std::distance(attribute_iter,end) >= 2){
            string key = *(attribute_iter);
            boost::trim(key);
            string value =  *(++attribute_iter);
            boost::trim(value);
            (*attrib_map)[key] = value;
            if (key == "opid"){
                opid = value;
            }
          }

      }

      // Hacky parse of th cached line which has the opcode
      // separator <b>
      std::regex opcode_expr{"<b>[A-Za-z]+"};
      std::smatch tokens;
      string opcode;
      if (std::regex_search(g_parse_cache_[0], tokens, opcode_expr)){
           opcode = tokens[0].str().substr(3,tokens[0].str().size());
      }


      (*attrib_map)["OPCODE"] = opcode; 
      CHECK(!opid.empty());
      AddToMap(opid, attrib_map, table_map);

  }
}

} } } //namespace
 
