Module.getRandomValue = (function() {
	const window_ = "object" === typeof window ? window : self
	const crypto_ = typeof window_.crypto !== "undefined" ? window_.crypto : window_.msCrypto;

	function randomValuesStandard() {
		var buf = new Uint32Array(1);
		crypto_.getRandomValues(buf);
		return buf[0] >>> 0;
	};

	return randomValuesStandard
})()
