
async function go() {
	const LNSocket = await lnsocket_init()
	const ln = LNSocket()

	ln.genkey()
	//const their_init = await ln.connect_and_init("03f3c108ccd536b8526841f0a5c58212bb9e6584a1eb493080e7c1cc34f82dad71", "cln.jb55.com:443")
	await ln.connect_and_init("03f3c108ccd536b8526841f0a5c58212bb9e6584a1eb493080e7c1cc34f82dad71", "ws://24.84.152.187:8324")

	const rune = "APaeUhcGPAMQwgV1Kn-hRRs5Bi4-D1nrfsHfCoTLl749MTAmbWV0aG9kPWdldGluZm8="
	const res = await ln.rpc({ method: "getinfo", rune })

	document.body.innerHTML = `<pre>${JSON.stringify(res.result, undefined, 2)}</pre>`
}

go()
