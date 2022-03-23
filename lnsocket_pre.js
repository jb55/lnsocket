Module.getRandomValue = (function() {
	const window_ = "object" === typeof window ? window : this
	const crypto_ = typeof window_.crypto !== "undefined" ? window_.crypto : window_.msCrypto;

	let randomBytesNode
	let fn
	if (!crypto_) {
		randomBytesNode = require('crypto').randomBytes
		fn = randomValuesNode 
	} else {
		fn = randomValuesStandard
	}

	function randomValuesNode() {
		return randomBytesNode(1)[0] >>> 0
	}

	function randomValuesStandard() {
		var buf = new Uint32Array(1);
		crypto_.getRandomValues(buf);
		return buf[0] >>> 0;
	};

	return fn
})()
