
async function lnsocket_init() {
	const module = await Module()

	const ACT_ONE_SIZE = 50
	const ACT_TWO_SIZE = 50
	const ACT_THREE_SIZE = 66
	const DEFAULT_TIMEOUT = 15000

	const COMMANDO_REPLY_CONTINUES = 0x594b
	const COMMANDO_REPLY_TERM = 0x594d

	const lnsocket_create = module.cwrap("lnsocket_create", "number")
	const lnsocket_encrypt = module.cwrap("lnsocket_encrypt", "number", ["int", "array", "int", "int"])
	const lnsocket_decrypt = module.cwrap("lnsocket_decrypt", "number", ["int", "array", "int"])
	const lnsocket_msgbuf = module.cwrap("lnsocket_msgbuf", "number", ["int"])
	const lnsocket_act_one = module.cwrap("lnsocket_act_one", "number", ["number", "string"])
	const lnsocket_act_two = module.cwrap("lnsocket_act_two", "number", ["number", "array"])
	const lnsocket_print_errors = module.cwrap("lnsocket_print_errors", "int")
	const lnsocket_genkey = module.cwrap("lnsocket_genkey", "int")
	const lnsocket_make_default_initmsg = module.cwrap("lnsocket_make_default_initmsg", "int", ["int", "int"])
	const lnsocket_make_ping_msg = module.cwrap("lnsocket_make_ping_msg", "int", ["int", "int", "int", "int"])
	const commando_make_rpc_msg = module.cwrap("commando_make_rpc_msg", "int", ["string", "string", "string", "number", "int", "int"])

	function concat_u8_arrays(arrays) {
		// sum of individual array lengths
		let totalLength = arrays.reduce((acc, value) => acc + value.length, 0);

		if (!arrays.length) return null;

		let result = new Uint8Array(totalLength);

		let length = 0;
		for (let array of arrays) {
			result.set(array, length);
			length += array.length;
		}

		return result;
	}

	function queue_recv(queue) {
		return new Promise((resolve, reject) => {
			const checker = setInterval(() => {
				const val = queue.shift()
				if (val) {
					clearInterval(checker)
					resolve(val)
				}
			}, 5);
		}) 
	}

	function parse_msgtype(buf) {
		return buf[0] << 8 | buf[1]
	}

	function wasm_mem(ptr, size) {
		return new Uint8Array(module.HEAPU8.buffer, ptr, size);
	}

	function LNSocket(opts) {
		if (!(this instanceof LNSocket))
			return new LNSocket(opts)

		this.opts = opts || {
			timeout: DEFAULT_TIMEOUT
		}
		this.queue = []
		this.ln = lnsocket_create()
	}

	LNSocket.prototype.print_errors = function _lnsocket_print_errors() {
		lnsocket_print_errors(this.ln)
	}

	LNSocket.prototype.genkey = function _lnsocket_genkey() {
		lnsocket_genkey(this.ln)
	}

	LNSocket.prototype.act_one_data = function _lnsocket_act_one(node_id) {
		const act_one_ptr = lnsocket_act_one(this.ln, node_id)
		if (act_one_ptr === 0)
			return null
		return wasm_mem(act_one_ptr, ACT_ONE_SIZE)
	}

	LNSocket.prototype.act_two = function _lnsocket_act_two(act2) {
		const act_three_ptr = lnsocket_act_two(this.ln, new Uint8Array(act2))
		if (act_three_ptr === 0) {
			this.print_errors()
			return null
		}
		return wasm_mem(act_three_ptr, ACT_THREE_SIZE)
	}

	LNSocket.prototype.connect = async function lnsocket_connect(node_id, host) {
		await handle_connect(this, node_id, host)

		const act1 = this.act_one_data(node_id)
		this.ws.send(act1)
		const act2 = await this.read_clear()
		if (act2.byteLength != ACT_TWO_SIZE) {
			throw new Error(`expected act2 to be ${ACT_TWO_SIZE} long, got ${act2.length}`)
		}
		const act3 = this.act_two(act2)
		this.ws.send(act3)
	}

	LNSocket.prototype.connect_and_init = async function _connect_and_init(node_id, host) {
		await this.connect(node_id, host)
		await this.perform_init()
	}

	LNSocket.prototype.rpc = async function lnsocket_rpc(opts) {
		const msg = this.make_commando_msg(opts)
		this.write(msg)
		return JSON.parse(await this.read_all_rpc())
	}

	LNSocket.prototype.recv = async function lnsocket_recv() {
		const msg = await this.read()
		console.log("recv", msg)
		const msgtype = parse_msgtype(msg.slice(0,2))
		return [msgtype, msg.slice(2)]
	}

	LNSocket.prototype.read_all_rpc = async function read_all_rpc() {
		let chunks = []
		while (true) {
			const [typ, msg] = await this.recv()
			switch (typ) {
			case COMMANDO_REPLY_TERM:
				chunks.push(msg.slice(8))
				return new TextDecoder().decode(concat_u8_arrays(chunks));
			case COMMANDO_REPLY_CONTINUES:
				chunks.push(msg.slice(8))
				break
			default:
				console.log("got unknown type", typ)
				continue
			}
		}
	}

	LNSocket.prototype.make_commando_msg = function _lnsocket_make_commando_msg(opts) {
		const buflen = 4096
		let len = 0;
		const buf = module._malloc(buflen);
		module.HEAPU8.set(Uint8Array, buf);

		const params = JSON.stringify(opts.params||{})
		if (!(len = commando_make_rpc_msg(opts.method, params, opts.rune,
			0, buf, buflen))) {
			throw new Error("couldn't make commando msg");
		}

		const dat = wasm_mem(buf, len)
		module._free(buf);
		return dat
	}

	LNSocket.prototype.make_ping_msg = function _lnsocket_make_ping_msg(num_pong_bytes=1, ignored_bytes=1)  {
		const buflen = 32
		let len = 0;
		const buf = module._malloc(buflen);
		module.HEAPU8.set(Uint8Array, buf);

		if (!(len = lnsocket_make_ping_msg(buf, buflen, num_pong_bytes, ignored_bytes)))
			throw new Error("couldn't make ping msg");

		const dat = wasm_mem(buf, len)
		module._free(buf);
		return dat
	}

	LNSocket.prototype.encrypt = function _lnsocket_encrypt(dat) {
		const len = lnsocket_encrypt(this.ln, dat, dat.length)
		if (len === 0) {
			this.print_errors()
			throw new Error("encrypt error")
		}
		const enc = wasm_mem(lnsocket_msgbuf(this.ln), len)
		return enc
	}

	LNSocket.prototype.decrypt = function _lnsocket_decrypt(dat) {
		const len = lnsocket_decrypt(this.ln, dat, dat.length)
		if (len === 0) {
			this.print_errors()
			throw new Error("decrypt error")
		}
		return wasm_mem(lnsocket_msgbuf(this.ln), len)
	}

	LNSocket.prototype.write = function _lnsocket_write(dat) {
		this.ws.send(this.encrypt(dat))
	}

	LNSocket.prototype.read_clear = async function _lnsocket_read() {
		return (await queue_recv(this.queue))
	}

	LNSocket.prototype.read = async function _lnsocket_read() {
		const enc = await this.read_clear()
		return this.decrypt(new Uint8Array(enc))
	}

	LNSocket.prototype.make_default_initmsg = function _lnsocket_make_default_initmsg() {
		const buflen = 1024
		let len = 0;
		const buf = module._malloc(buflen);
		module.HEAPU8.set(Uint8Array, buf);

		if (!(len = lnsocket_make_default_initmsg(buf, buflen)))
			throw new Error("couldn't make initmsg");

		const dat = wasm_mem(buf, len)
		module._free(buf);
		return dat
	}

	LNSocket.prototype.perform_init = async function lnsocket_connect() {
		await this.read()
		const our_init = this.make_default_initmsg()
		console.log("our_init", our_init)
		this.write(our_init)
	}

	LNSocket.prototype.ping_pong = async function lnsocket_ping_pong() {
		const pingmsg = this.make_ping_msg()
		console.log("ping", pingmsg)
		this.write(pingmsg)
		return await this.read()
	}

	LNSocket.prototype.disconnect = function lnsocket_disconnect() {
		if (this.connected === true && this.ws) {
			this.ws.close()
			return true
		}
		return false
	}

	function handle_connect(ln, node_id, host) {
		const isws = host.startsWith("ws://") || host.startsWith("wss://")
		if (!isws)
			throw new Error("host must start with ws:// or wss://")
		const ws = new WebSocket(host)
		return new Promise((resolve, reject) => {
			const timeout = ln.opts.timeout || DEFAULT_TIMEOUT
			const timer = setTimeout(reject, timeout);

			ws.onmessage = (v) => {
				ln.queue.push(v.data.arrayBuffer())
			}

			ws.addEventListener('open', function(ev) {
				ln.ws = ws
				ln.connected = true
				clearTimeout(timer)
				resolve(ws)
			});

			ws.addEventListener('close', function(ev) {
				ln.connected = false
				console.log("closed websocket connection")
			});
		})
	}

	return LNSocket
}

Module.init = lnsocket_init
