/*
 * Copyright (c) 2015 Nutanix Inc. All rights reserved.
 *
 * Author: jayendr.gowrish@nutanix.com
 * Purpose: Log parser, general error diagnoser on customer 
 * clusters
*/

#include "cerebro/diagnose/failfast_fatal.h"
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
// Fail-fast after detecting hung stargate ops: Operation with id 172904535 hung for 60secs
namespace nutanix { namespace cerebro { namespace diagnose {

void FailFastFatal::ParseAndStore(const string& line,
    TablenameClassMap::Ptr table_map){
  std::regex expr{"Fail-fast"};
  std::smatch tokens;
  if (std::regex_search(line, tokens, expr)){
    std::regex component{"hung\\s[A-Za-z]+\\sops:"};
    auto component_begin = std::sregex_iterator(line.begin(), line.end(), component);
    auto component_end = std::sregex_iterator();

    for (std::sregex_iterator i = component_begin; i != component_end; ++i) {
        std::smatch match = *i;                                                 
        std::string match_str = match.str(); 

        // Tokenize on  "\s" 
        CHAR_SEP space_sep(" ");
        boost::tokenizer<CHAR_SEP> elem_tok(match_str, space_sep);
        boost::tokenizer<CHAR_SEP>::iterator component_iter = elem_tok.begin();
        boost::tokenizer<CHAR_SEP>::iterator component_end = elem_tok.end();

        string component = *(++component_iter);
 
        std::regex opid_expr{"id [0-9]+ hung"};
        auto opid_begin = std::sregex_iterator(line.begin(), line.end(), opid_expr);
        auto opid_end = std::sregex_iterator();
        if (opid_begin == opid_end)
            continue;
        string opid_str = (*opid_begin).str();
        boost::tokenizer<CHAR_SEP> opid_tok(opid_str, space_sep);
        boost::tokenizer<CHAR_SEP>::iterator opid_iter = opid_tok.begin();
        boost::tokenizer<CHAR_SEP>::iterator opid_iter_end = opid_tok.end();
        string opid = *(++opid_iter);


        shared_ptr<ATTRIB_MAP> attrib_map  = make_shared<ATTRIB_MAP>();;
        (*attrib_map)["COMPONENT"] = component;
        (*attrib_map)["OPID"] = opid;

        AddToMap(opid, attrib_map, table_map);
    }
  }
}

} } } //namespace


