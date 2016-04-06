#
# Copyright (c) 2016 Nutanix Inc. All rights reserved.
#
# Author: jayendr.gowrish@nutanix.com
#

generate_proto_py(cerebro_diagnose.proto)
generate_proto_cpp(cerebro_diagnose.proto)

add_executable(cerebro_diagnose
               mop_history.cc
               file_vdisk_metaop.cc
               failfast_fatal.cc
               stargate_trace.cc
               file_vdisk.cc
               completed_mop.cc
               master_ip_timestamp.cc
               infolog_parser.cc
               file_parser.cc
               file_parser.cc
               cerebro_diagnose.pb.cc
               cerebro_diagnose.cc)

target_link_libraries(cerebro_diagnose
                      cerebro_base
                      cerebro_interface
                      util_misc
                      util_net)
