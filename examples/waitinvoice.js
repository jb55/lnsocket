
async function go() {
	const LNSocket = await lnsocket_init()
	const ln = LNSocket()

	ln.genkey()
	//const their_init = await ln.connect_and_init("03f3c108ccd536b8526841f0a5c58212bb9e6584a1eb493080e7c1cc34f82dad71", "cln.jb55.com:443")
	await ln.connect_and_init("03f3c108ccd536b8526841f0a5c58212bb9e6584a1eb493080e7c1cc34f82dad71", "ws://monad.endpoint.jb55.com:8324")

	const rune = "zBYBKd5PhxUzbVGwJwAsJaqQ3dhkL5Vf1RA3Ut9HRoM9NzAmbWV0aG9kPXdhaXRpbnZvaWNlJnBhcnIwPXRlc3QxMjMyMzU0"
	const res = await ln.rpc({ method: "waitinvoice", params: ["test1232354"], rune })

	document.body.innerHTML = `<pre>${JSON.stringify(res.result, undefined, 2)}</pre>`
}

go()
