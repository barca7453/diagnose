/*
 * Copyright (c) 2015 Nutanix Inc. All rights reserved.
 *
 * Author: jayendr.gowrish@nutanix.com
 * Purpose: Log parser, general error diagnoser on customer 
 * clusters
*/
#include "cerebro/diagnose/file_parser.h"
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
#include <fstream>


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

using namespace boost::filesystem;
namespace io = boost::iostreams;

namespace nutanix { namespace cerebro { namespace diagnose {
 // Nothing here yet
} } } // namespace


