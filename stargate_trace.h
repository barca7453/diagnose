/*
 * Copyright (c) 2015 Nutanix Inc. All rights reserved.
 *
 * Author: jayendr.gowrish@nutanix.com
 * Purpose: Log parser, general error diagnoser on customer 
 * clusters
*/
#ifndef _CEREBRO_DIAGNOSE_STARGATE_TRACE_
#define _CEREBRO_DIAGNOSE_STARGATE_TRACE_

#include "cerebro/diagnose/infolog_parser.h"
#include "cerebro/diagnose/file_parser.h"

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
// Parses both stargate and cerebro dumps on a hung OP
// Makes looking up Opcodes and Attributes easier
// given a known opid
//
// Key:   opid
// Value:
// [opcode:<opcode>],[file:<filename>],[work_id:<work_id>],[remote:<remote>]
// Lines to parse
//<td align="left"><pre><b>CerebroSlaveVDiskDiffOp
//latest_considered_vblock=-1 reference_file_vdisk_id=30855358 max_vblock=406 file_vdisk_id=62740970 opid=178 </b></pre></td>
//
//<td align="left"><pre><b>CerebroSlaveReplicateFileOp
//skip_vdisk_data=1 skip_snapshot_prefix=0 file=/Stretch-PD27_sabine08_ctr/.snapshot/70/33946-1453238923649053-4348370/.vSphere-HA/FDM-A5A4D38B-744B-4BAD-B13B-DD160133BB2B-47025-5d273f4-SV2-ENG-VCSRM55B/.lck-2306000000000000 remote=sabine09 work_id=4348788 opid=170 </b></pre></td>

struct TraceDump {
  typedef boost::char_separator<char> CHAR_SEP;
  typedef map<string,string> ATTRIB_MAP;

  TraceDump(){}

  ~TraceDump(){}

  void ParseAndStore(const std::string& line, TablenameClassMap::Ptr table_map);
 
  const string GetTableName() const {
      return "TRACE_DUMP";
  }

private:
 
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

