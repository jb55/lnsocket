const LNSocket = require('../')

async function go() {
	const ln = await LNSocket()
	
	ln.genkey()
	await ln.connect_and_init("03f3c108ccd536b8526841f0a5c58212bb9e6584a1eb493080e7c1cc34f82dad71", "24.84.152.187")
	
	const rune = "uQux-hID66AX5rFUpkt1p9CU_7DsTMyUC4G5yq7-dcw9MTMmbWV0aG9kPWdldGluZm8="
	const res = await ln.rpc({ method: "getinfo", rune })
	
	ln.destroy()
	console.log(res)
	return res
}

go()
