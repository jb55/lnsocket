from pyln.testing.fixtures import *
import os

def test_getinfo(node_factory):
    node = node_factory.get_node()
    node_info = node.rpc.getinfo()
    node_id = node_info["id"]
    node_address = node_info["binding"][0]["address"]
    node_port = node_info["binding"][0]["port"]
    node_hostname = node_address + ":" + str(node_port)
    # when `commando-rune` deprecated, we'll use `createrune` instead
    # rune = node.rpc.createrune()["rune"]
    rune = node.rpc.commando_rune()["rune"]
    getinfo_cmd = f"go run getinfo.go {node_id} {node_hostname} {rune}"
    assert os.popen(getinfo_cmd).read() == node_id
