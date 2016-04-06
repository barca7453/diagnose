import cmd, sys
import gflags
import re
import time
import util.base.log as log
import subprocess 
import os
import shlex

from cerebro_diagnose_pb2 import *
from util.net.http_protobuf_rpc import HttpProtobufRpcClient
from util.net.sync_rpc_client import *
import argparse

FLAGS = gflags.FLAGS
gflags.DEFINE_integer("cerebro_diagnose_port", 2222, "Port where cerebro diagnose listens.");

gflags.DEFINE_string("cerebro_diagnose_host",
                     "127.0.0.1",
                     "Host where cerebro diagnose resides")


class CerebroDiagnoseError(Exception):
  """
  Encapsulates any errors thrown by the RPC server into a Python Exception. The
  consumer of the CerebroInterface class can 'catch' these errors to find out
  if the RPC server encountered any errors while processing the request.
  """
  def __init__(self, message, error_code=None):
    Exception.__init__(self, message)
    if error_code is not None:
      self.error_code = error_code

CerebroDiagnoseMetaSyncRpcClient = sync_rpc_client_metaclass(CerebroDiagnoseSvc_Stub)

class CerebroDiagnoseInterfaceClient(SyncRpcClientMixin):
  __metaclass__ = CerebroDiagnoseMetaSyncRpcClient

  def __init__(self, cerebro_diagnose_host, cerebro_diagnose_port):
    """
    Instantiates state creating a SyncRpcClientMixin object.

    Args:
      cerebro_diagnose_host (string): IP Address of the Cerebro RPC Server.
      cerebro_diagnose_port (integer): Port number of the Cerebro RPC Server.
    """
    SyncRpcClientMixin.__init__(self, CerebroDiagnoseErrorProto)
    self.__cerebro_diagnose_adapter_ip = cerebro_diagnose_host
    self.__cerebro_diagnose_adapter_port = cerebro_diagnose_port

  def _create_stub(self):
    """
    Creates a new stub with backing transport.
    """
    rpc_client = HttpProtobufRpcClient(self.__cerebro_diagnose_adapter_ip,
                                       self.__cerebro_diagnose_adapter_port)
    return CerebroDiagnoseSvc_Stub(rpc_client)

  def _filter_rpc_result(self, rpc, err, ret):
    """
    Filters the return value for RPC method invocations.

    Errors are converted to exceptions, the RPC context is dropped entirely,
    and only the result is returned.

    Args:
      rpc (ProtobufRpc): The RPC context.
      err (CerebroDiagnoseErrorProto.Type): The application error sent by the RPC
                                     server.
      ret (Message): The protobuf result.
    Returns:
      ret
    Raises:
      CerebroDiagnoseError
    """
    if err != CerebroDiagnoseErrorProto.kNoError:
      log.WARNING("Cerebro RPC returned error %s." % (err))
      raise CerebroDiagnoseError(self.strerror(), error_code=err)
    return ret




class CerebroDiagnoseMetaClient(CerebroDiagnoseMetaSyncRpcClient):
  """
  Metaclass that makes requests to cerbro master. Retries for configured
  (a) follow_master_count times when the rpc fails due to kNotMaster
  (b) cerebro_application_retry_count times when the rpc returns kRetry.
  """
  def __new__(mcs, class_name, bases, dikt):
    for method in CerebroDiagnoseSvc_Stub.DESCRIPTOR.methods:
      name = method.name
      (wrapped_name, wrapped_func) = mcs.__reflect_master_method(name)
      dikt[wrapped_name] = wrapped_func

    return CerebroDiagnoseMetaSyncRpcClient.__new__(mcs, class_name, bases, dikt)

  @staticmethod
  def __reflect_master_method(name):
    def method_wrapper(self, arg):
      # Number of retries performed when rpc returns kNotMaster.
      follow_master_count = 0
      # Number of retries performed when rpc returns kRetry.
      cerebro_application_retry_count = 0

      while True:
        try:
          if self._master_client is None:
            ret = getattr(self, name)(arg)
          else:
            ret = getattr(self._master_client, name)(arg)
          return ret
        except CerebroDiagnoseError as e:
            print "EXCEPTION...while.."
            return 
        except RpcClientTransportError:
            print "RPC transport ex"
        except RpcClientError:
            print "RPC client ex"



    split_func_name = re.findall("[A-Z][a-z]*", name)
    new_name = ""
    for comp in split_func_name:
      new_name += comp.lower()
      new_name += "_"
    new_name = new_name[:-1]

    method_wrapper.__name__ = new_name
    method_wrapper.__doc__ = """
    Invokes<> , retries <> number of times when master moves and %d number of
    times when cerebro service requests for a retry"
    """ 
    return (new_name, method_wrapper)

class CerebroDiagnoseInterfaceTool(CerebroDiagnoseInterfaceClient):
  __metaclass__ = CerebroDiagnoseMetaClient

  def __init__(self, cerebro_diagnose_host=None, cerebro_diagnose_port=None):
    self._master_client = None
    if cerebro_diagnose_host is None:
      cerebro_diagnose_host = FLAGS.cerebro_diagnose_host
    if cerebro_diagnose_port is None:
      cerebro_diagnose_port = FLAGS.cerebro_diagnose_port
    CerebroDiagnoseInterfaceClient.__init__(self, cerebro_diagnose_host, cerebro_diagnose_port)

  def _create_master_client(self, cerebro_diagnose_host, cerebro_diagnose_port):
    self._master_client = CerebroDiagnoseInterfaceClient(cerebro_diagnose_host, cerebro_diagnose_port)
    
class DiagnoseShell(cmd.Cmd):
    intro = 'Hail Cthulhu, our lord and savior... Type help or ? if you dare.\n'
    prompt = '(what?) '
    file = None

    # ----- basic cerebro diagnose commands -----
    def do_load(self, arg):
        'List all the completed meta ops or filter by attribute "Attribute=<val>"'
        filepath_arg = arg
        if not arg:
            print 'No file provided to load, will load all cerebro logs in data/logs'
            # arg = * # everything to be loaded

        cerebro_diagnose_rpc_client = CerebroDiagnoseInterfaceTool()
        try:
          print_everything_ret = PrintEverythingRet()
          print_everything_arg = PrintEverythingArg()  
          print_everything_arg.filename = filepath_arg # "/home/jayendra/cerebro.INFO.latest"
          print_everything_ret = cerebro_diagnose_rpc_client.PrintEverything(print_everything_arg)
          print "RESULT IS :" + str (print_everything_ret.str)
        except CerebroDiagnoseError as e:
          return cls.warned("QueryProtectionDomain failed with error %s" % e)

    def do_mop_attributes(self, arg):
        'Display all meta ops and attributes or filter by meta op id.'
        # Join example
        # select * from mop_attr, mop_history
        # Use argparse to manage all the option parsing for the command
        parser = argparse.ArgumentParser(usage="mop_attributes [filter] [join] [rtable] [ltable] [key]")
        try:
            parser.add_argument("schema", nargs='?') # optional arg hence the nargs='?'
            parser.add_argument("filter", nargs='?') # optional arg hence the nargs='?'
            parser.add_argument("join", nargs='?') # optional arg hence the nargs='?'
            parser.add_argument("rtable", nargs='?') # optional arg hence the nargs='?'
            parser.add_argument("ltable", nargs='?') # optional arg hence the nargs='?'
            parser.add_argument("key", nargs='?') # optional arg hence the nargs='?'
            args = parser.parse_args(shlex.split(arg))

            if args.schema:
              print args.schema
            if args.filter:
              print args.filter
            if args.join:
              print args.join
            if args.ltable:
              print args.ltable
            if args.rtable:
              print args.rtable
        except:
            print "Incorrect options, give me something I can work with.."
    def do_mop_history(self, arg):
        'Display history of all meta ops or filter by meta op id.'
    def do_bye(self, arg):
        print('OK..going back to sleep..')
        subprocess.Popen.kill(proc)
        self.close()
        return True

    def close(self):
        if self.file:
            self.file.close()
            self.file = None

def parse(arg):
    'Convert a series of zero or more numbers to an argument tuple'
    return tuple(map(int, arg.split()))

if __name__ == '__main__':
    global proc
    try:
       proc = subprocess.Popen('/home/jayendra/work/main/build/cerebro/diagnose/cerebro_diagnose')
       DiagnoseShell().cmdloop()
    except:
       subprocess.Popen.kill(proc)
       raise

