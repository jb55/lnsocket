

const t = require('tap')
const LNSocket = require('../')

t.test('connection works', async (t) => {
	const ln = await LNSocket()

	ln.genkey()

	const badkey = "0000000000000000000000000000000000000000000000000000000000000000"
	if (ln.setkey(badkey))
		t.ok(false, "expected zero setkey to fail")

	const goodkey = "9a5419eacf204a917cf06d5f830e48bc1fcb3e33bb60932e837697c8b7718f6f"
	if (!ln.setkey(goodkey))
		t.ok(false, "expected goodkey to succeed")

	const nodeid = "03f3c108ccd536b8526841f0a5c58212bb9e6584a1eb493080e7c1cc34f82dad71"
	await ln.connect_and_init(nodeid, "24.84.152.187")

	t.ok(ln.connected, "connection works")

	const rune = "APaeUhcGPAMQwgV1Kn-hRRs5Bi4-D1nrfsHfCoTLl749MTAmbWV0aG9kPWdldGluZm8="
	const res = await ln.rpc({ method: "getinfo", rune })

	t.ok(res.result, "didn't get result")
	t.ok(res.result.id === nodeid, "nodeid is wrong!?")

	ln.destroy()
	t.end()
})
