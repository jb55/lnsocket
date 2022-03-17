const lnsocket_init = require('lnsocket')

async function go() {
	const ln = await LNSocket()
	
	ln.genkey()
	await ln.connect_and_init("03f3c108ccd536b8526841f0a5c58212bb9e6584a1eb493080e7c1cc34f82dad71", "24.84.152.187")
	
	const rune = "b3Xsg2AS2cknHYa6H94so7FAVQVdnRSP6Pv-1WOQEBc9NCZtZXRob2Q9b2ZmZXItc3VtbWFyeQ=="
	const summary = await ln.rpc({ 
	    rune,
	    method: "offer-summary",
	    params: {
	        offerid: "22db2cbdb2d6e1f4d727d099e2ea987c05212d6b4af56d92497e093b82360db7",
	        limit: 10
	    }
	})
	
	ln.destroy()
	console.log(summary.result)
	return summary.result
}

go()
