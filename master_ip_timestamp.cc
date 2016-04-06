/*
 * Copyright (c) 2015 Nutanix Inc. All rights reserved.
 *
 * Author: jayendr.gowrish@nutanix.com
 * Purpose: Log parser, general error diagnoser on customer 
 * clusters
*/

#include "cerebro/diagnose/master_ip_timestamp.h"
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
// Cerebro master changed to 10.4.161.68:2020+209
namespace nutanix { namespace cerebro { namespace diagnose {

} } }
