

const t = require('tap')
const LNSocket = require('../')

t.test('connection works', async (t) => {
	t.plan(1)

	const ln = await LNSocket()

	ln.genkey()

	const badkey = "0000000000000000000000000000000000000000000000000000000000000000"
	if (ln.setkey(badkey))
		t.ok(false, "expected zero setkey to fail")

	const goodkey = "9a5419eacf204a917cf06d5f830e48bc1fcb3e33bb60932e837697c8b7718f6f"
	if (!ln.setkey(goodkey))
		t.ok(false, "expected goodkey to succeed")

	await ln.connect_and_init("03f3c108ccd536b8526841f0a5c58212bb9e6584a1eb493080e7c1cc34f82dad71", "24.84.152.187")

	t.ok(ln.connected, "connection works")

	ln.destroy()
	t.end()
})
