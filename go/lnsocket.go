package lnsocket

import (
	"bytes"
	"encoding/hex"
	"fmt"
	"io"
	"net"

	"github.com/btcsuite/btcd/btcec/v2"
	"github.com/lightningnetwork/lnd/brontide"
	"github.com/lightningnetwork/lnd/keychain"
	"github.com/lightningnetwork/lnd/lnwire"
	"github.com/lightningnetwork/lnd/tor"
)

const (
	COMMANDO_CMD             = 0x4c4f
	COMMANDO_REPLY_CONTINUES = 0x594b
	COMMANDO_REPLY_TERM      = 0x594d
)

type CommandoMsg struct {
	Rune      string
	Method    string
	Params    string
	RequestId uint64
}

func NewCommandoMsg(token string, method string, params string) CommandoMsg {
	return CommandoMsg{
		Rune:   token,
		Method: method,
		Params: params,
	}
}

// A compile time check to ensure Init implements the lnwire.Message
// interface.

func (msg *CommandoMsg) MsgType() lnwire.MessageType {
	return COMMANDO_CMD
}

func (msg *CommandoMsg) Decode(reader io.Reader, size uint32) error {
	return fmt.Errorf("implememt commando decode?")
}

func (msg *CommandoMsg) Encode(buf *bytes.Buffer, pver uint32) error {
	if err := lnwire.WriteUint64(buf, msg.RequestId); err != nil {
		return err
	}

	buf.WriteString("{\"method\": \"")
	buf.WriteString(msg.Method)
	buf.WriteString("\",\"params\":")
	buf.WriteString(msg.Params)
	buf.WriteString(",\"rune\":\"")
	buf.WriteString(msg.Rune)
	buf.WriteString("\",\"id\":\"d0\",\"jsonrpc\":\"2.0\"}")

	return nil
}

type LNSocket struct {
	Conn        net.Conn
	PrivKeyECDH *keychain.PrivKeyECDH
}

func (ln *LNSocket) GenKey() {
	remotePriv, _ := btcec.NewPrivateKey()
	ln.PrivKeyECDH = &keychain.PrivKeyECDH{PrivKey: remotePriv}
}

func (ln *LNSocket) ConnectWith(netAddr *lnwire.NetAddress) error {
	conn, err := brontide.Dial(ln.PrivKeyECDH, netAddr, tor.DefaultConnTimeout, net.DialTimeout)
	ln.Conn = conn
	return err
}

func (ln *LNSocket) Connect(hostname string, pubkey string) error {
	addr, err := net.ResolveTCPAddr("tcp", hostname)
	if err != nil {
		return err
	}
	bytes, err := hex.DecodeString(pubkey)
	if err != nil {
		return err
	}
	key, err := btcec.ParsePubKey(bytes)
	if err != nil {
		return err
	}

	netAddr := &lnwire.NetAddress{
		IdentityKey: key,
		Address:     addr,
	}

	return ln.ConnectWith(netAddr)
}

func (ln *LNSocket) PerformInit() error {
	no_features := lnwire.NewRawFeatureVector()
	init_reply_msg := lnwire.NewInitMessage(no_features, no_features)

	var b bytes.Buffer
	_, err := lnwire.WriteMessage(&b, init_reply_msg, 0)
	if err != nil {
		return err
	}
	_, err = ln.Conn.Write(b.Bytes())

	// receive the first init msg
	_, _, err = ln.Recv()
	if err != nil {
		return err
	}

	return nil
}

func (ln *LNSocket) Rpc(token string, method string, params string) (string, error) {
	commando_msg := NewCommandoMsg(token, method, params)

	var b bytes.Buffer
	_, err := lnwire.WriteMessage(&b, &commando_msg, 0)
	if err != nil {
		return "", err
	}

	bs := b.Bytes()
	_, err = ln.Conn.Write(bs)
	if err != nil {
		return "", err
	}

	return ln.rpcReadAll()
}

func ParseMsgType(bytes []byte) uint16 {
	return uint16(bytes[0])<<8 | uint16(bytes[1])
}

func (ln *LNSocket) Recv() (uint16, []byte, error) {
	res := make([]byte, 65535)
	n, err := ln.Conn.Read(res)
	if err != nil {
		return 0, nil, err
	}
	if n < 2 {
		return 0, nil, fmt.Errorf("read too small")
	}
	res = res[:n]
	msgtype := ParseMsgType(res)
	return msgtype, res[2:], nil
}

func (ln *LNSocket) rpcReadAll() (string, error) {
	all := []byte{}
	for {
		msgtype, res, err := ln.Recv()
		if err != nil {
			return "", err
		}
		switch msgtype {
		case COMMANDO_REPLY_CONTINUES:
			all = append(all, res[8:]...)
			continue
		case COMMANDO_REPLY_TERM:
			all = append(all, res[8:]...)
			return string(all), nil
		default:
			continue
		}
	}
}

func (ln *LNSocket) Disconnect() {
	ln.Conn.Close()
}

func (ln *LNSocket) ConnectAndInit(hostname string, pubkey string) error {
	err := ln.Connect(hostname, pubkey)
	if err != nil {
		return err
	}

	return ln.PerformInit()
}
