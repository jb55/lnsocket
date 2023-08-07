package main

import (
  "fmt"
  "os"
	"github.com/tidwall/gjson"
  lnsocket "github.com/jb55/lnsocket/go"
)

// Print the node id of the CLN node given as input using
// commando plugin and lnsocket/go.
// Usage:
//
//    $ go run getinfo.go node_id hostname rune
//
// with for instance
//    node_id:  034fa9ed201192c7e865f0fc67001725620e79422347186f0ae4578aac08f4df3d
//    hostname: 127.0.0.1:7171
//    rune:     sxKlx_DW_LtN1kxbTEEfvJN4ytGLMNfyKH_8AjK9yx89MA==

func main() {
  nodeid := os.Args[1]
  hostname := os.Args[2]
  rune := os.Args[3]
	ln := lnsocket.LNSocket{}
  ln.GenKey()
  err := ln.ConnectAndInit(hostname, nodeid)
  if err != nil {
		panic(err)
	}
	body, err := ln.Rpc(rune, "getinfo", "[]")
	if err != nil {
		panic(err)
	}
	getinfoNodeid := gjson.Get(body, "result.id").String()
	fmt.Printf(getinfoNodeid)
}
