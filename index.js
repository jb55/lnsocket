
const is_browser = new Function("try {return this===window;}catch(e){ return false;}");

let Module
if (is_browser()) {
	Module = require("./dist/js/lnsocket.js")
} else {
	Module = require("./dist/node/lnsocket.js")
}

const LNSocketReady = Module.lnsocket_init()

async function load_lnsocket(opts)
{
	const LNSocket = await LNSocketReady
	return new LNSocket(opts)
}

module.exports = load_lnsocket

