

const t = require('tap')
const LNSocket = require('../')

t.test('connection works', async (t) => {
	const ln = await LNSocket()

	ln.genkey()
	await ln.connect_and_init("03f3c108ccd536b8526841f0a5c58212bb9e6584a1eb493080e7c1cc34f82dad71", "24.84.152.187")

	t.ok(ln.connected, "connection works")

	ln.destroy()
	t.end()
})
