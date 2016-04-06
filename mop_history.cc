/*
 * Copyright (c) 2015 Nutanix Inc. All rights reserved.
 *
 * Author: jayendr.gowrish@nutanix.com
 * Purpose: Log parser, general error diagnoser on customer 
 * clusters
*/
#include "cerebro/diagnose/mop_history.h"
#include "cerebro/base/cerebro_util.h"
#include "cerebro/cerebro_net.h"
#include "cerebro/interface/cerebro_interface.pb.h"
#include "cerebro/diagnose/completed_mop.h"
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

void MetaopHistoryStorageMap::ParseAndStore(const string& line,
     TablenameClassMap::Ptr table_map) {
  boost::regex metaop_id_expr{"meta_opid:\\s[0-9]+"};
  boost::smatch tokens;
  if (boost::regex_search(line, tokens, metaop_id_expr)){
    //  Get the meta opid
    string meta_opid;

    string message;
    for_each(begin(tokens), end(tokens), [&](const string& token) { 
      // ":" delimited
      CHAR_SEP colon_sep(":");
      boost::tokenizer<CHAR_SEP> elem_tok(token, colon_sep);
      boost::tokenizer<CHAR_SEP>::iterator iter = elem_tok.begin();
      boost::tokenizer<CHAR_SEP>::iterator end = elem_tok.end();
      if (std::distance(iter,end) >= 2){
        meta_opid =  *(++iter);
        boost::trim(meta_opid);
        // Get the associated message 
        string delimiter = "meta_opid:";
        size_t pos = 0;
        pos = line.find(delimiter);
        message = line.substr(0, pos); 
        AddToMap(meta_opid, "meta_opid", meta_opid, table_map);
        AddToMap(meta_opid, "HISTORY", message,  table_map);
      }
    });
    // Add the time stamp
    // boost::regex timestamp_expr{"I[0-9]+\\s[0-9]+\\:[0-9]+\\:[0-9]+\\.[0-9]+"};
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
    AddToMap(meta_opid, "TIMESTAMP", boost_ts,  table_map);

 
  }
}

} } } // namespace 
