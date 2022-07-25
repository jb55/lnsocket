
var Module = (() => {
  var _scriptDir = typeof document !== 'undefined' && document.currentScript ? document.currentScript.src : undefined;
  if (typeof __filename !== 'undefined') _scriptDir = _scriptDir || __filename;
  return (
function(Module) {
  Module = Module || {};

var Module=typeof Module!="undefined"?Module:{};var readyPromiseResolve,readyPromiseReject;Module["ready"]=new Promise(function(resolve,reject){readyPromiseResolve=resolve;readyPromiseReject=reject});Module.getRandomValue=function(){const window_="object"===typeof window?window:this;const crypto_=typeof window_.crypto!=="undefined"?window_.crypto:window_.msCrypto;let randomBytesNode;let fn;if(!crypto_){randomBytesNode=require("crypto").randomBytes;fn=randomValuesNode}else{fn=randomValuesStandard}function randomValuesNode(){return randomBytesNode(1)[0]>>>0}function randomValuesStandard(){var buf=new Uint32Array(1);crypto_.getRandomValues(buf);return buf[0]>>>0}return fn}();var moduleOverrides=Object.assign({},Module);var arguments_=[];var thisProgram="./this.program";var quit_=(status,toThrow)=>{throw toThrow};var ENVIRONMENT_IS_WEB=typeof window=="object";var ENVIRONMENT_IS_WORKER=typeof importScripts=="function";var ENVIRONMENT_IS_NODE=typeof process=="object"&&typeof process.versions=="object"&&typeof process.versions.node=="string";var scriptDirectory="";function locateFile(path){if(Module["locateFile"]){return Module["locateFile"](path,scriptDirectory)}return scriptDirectory+path}var read_,readAsync,readBinary,setWindowTitle;function logExceptionOnExit(e){if(e instanceof ExitStatus)return;let toLog=e;err("exiting due to exception: "+toLog)}var fs;var nodePath;var requireNodeFS;if(ENVIRONMENT_IS_NODE){if(ENVIRONMENT_IS_WORKER){scriptDirectory=require("path").dirname(scriptDirectory)+"/"}else{scriptDirectory=__dirname+"/"}requireNodeFS=()=>{if(!nodePath){fs=require("fs");nodePath=require("path")}};read_=function shell_read(filename,binary){requireNodeFS();filename=nodePath["normalize"](filename);return fs.readFileSync(filename,binary?undefined:"utf8")};readBinary=filename=>{var ret=read_(filename,true);if(!ret.buffer){ret=new Uint8Array(ret)}return ret};readAsync=(filename,onload,onerror)=>{requireNodeFS();filename=nodePath["normalize"](filename);fs.readFile(filename,function(err,data){if(err)onerror(err);else onload(data.buffer)})};if(process["argv"].length>1){thisProgram=process["argv"][1].replace(/\\/g,"/")}arguments_=process["argv"].slice(2);process["on"]("uncaughtException",function(ex){if(!(ex instanceof ExitStatus)){throw ex}});process["on"]("unhandledRejection",function(reason){throw reason});quit_=(status,toThrow)=>{if(keepRuntimeAlive()){process["exitCode"]=status;throw toThrow}logExceptionOnExit(toThrow);process["exit"](status)};Module["inspect"]=function(){return"[Emscripten Module object]"}}else if(ENVIRONMENT_IS_WEB||ENVIRONMENT_IS_WORKER){if(ENVIRONMENT_IS_WORKER){scriptDirectory=self.location.href}else if(typeof document!="undefined"&&document.currentScript){scriptDirectory=document.currentScript.src}if(_scriptDir){scriptDirectory=_scriptDir}if(scriptDirectory.indexOf("blob:")!==0){scriptDirectory=scriptDirectory.substr(0,scriptDirectory.replace(/[?#].*/,"").lastIndexOf("/")+1)}else{scriptDirectory=""}{read_=url=>{var xhr=new XMLHttpRequest;xhr.open("GET",url,false);xhr.send(null);return xhr.responseText};if(ENVIRONMENT_IS_WORKER){readBinary=url=>{var xhr=new XMLHttpRequest;xhr.open("GET",url,false);xhr.responseType="arraybuffer";xhr.send(null);return new Uint8Array(xhr.response)}}readAsync=(url,onload,onerror)=>{var xhr=new XMLHttpRequest;xhr.open("GET",url,true);xhr.responseType="arraybuffer";xhr.onload=()=>{if(xhr.status==200||xhr.status==0&&xhr.response){onload(xhr.response);return}onerror()};xhr.onerror=onerror;xhr.send(null)}}setWindowTitle=title=>document.title=title}else{}var out=Module["print"]||console.log.bind(console);var err=Module["printErr"]||console.warn.bind(console);Object.assign(Module,moduleOverrides);moduleOverrides=null;if(Module["arguments"])arguments_=Module["arguments"];if(Module["thisProgram"])thisProgram=Module["thisProgram"];if(Module["quit"])quit_=Module["quit"];var wasmBinary;if(Module["wasmBinary"])wasmBinary=Module["wasmBinary"];var noExitRuntime=Module["noExitRuntime"]||true;if(typeof WebAssembly!="object"){abort("no native wasm support detected")}var wasmMemory;var ABORT=false;var EXITSTATUS;var UTF8Decoder=typeof TextDecoder!="undefined"?new TextDecoder("utf8"):undefined;function UTF8ArrayToString(heapOrArray,idx,maxBytesToRead){var endIdx=idx+maxBytesToRead;var endPtr=idx;while(heapOrArray[endPtr]&&!(endPtr>=endIdx))++endPtr;if(endPtr-idx>16&&heapOrArray.buffer&&UTF8Decoder){return UTF8Decoder.decode(heapOrArray.subarray(idx,endPtr))}else{var str="";while(idx<endPtr){var u0=heapOrArray[idx++];if(!(u0&128)){str+=String.fromCharCode(u0);continue}var u1=heapOrArray[idx++]&63;if((u0&224)==192){str+=String.fromCharCode((u0&31)<<6|u1);continue}var u2=heapOrArray[idx++]&63;if((u0&240)==224){u0=(u0&15)<<12|u1<<6|u2}else{u0=(u0&7)<<18|u1<<12|u2<<6|heapOrArray[idx++]&63}if(u0<65536){str+=String.fromCharCode(u0)}else{var ch=u0-65536;str+=String.fromCharCode(55296|ch>>10,56320|ch&1023)}}}return str}function UTF8ToString(ptr,maxBytesToRead){return ptr?UTF8ArrayToString(HEAPU8,ptr,maxBytesToRead):""}function stringToUTF8Array(str,heap,outIdx,maxBytesToWrite){if(!(maxBytesToWrite>0))return 0;var startIdx=outIdx;var endIdx=outIdx+maxBytesToWrite-1;for(var i=0;i<str.length;++i){var u=str.charCodeAt(i);if(u>=55296&&u<=57343){var u1=str.charCodeAt(++i);u=65536+((u&1023)<<10)|u1&1023}if(u<=127){if(outIdx>=endIdx)break;heap[outIdx++]=u}else if(u<=2047){if(outIdx+1>=endIdx)break;heap[outIdx++]=192|u>>6;heap[outIdx++]=128|u&63}else if(u<=65535){if(outIdx+2>=endIdx)break;heap[outIdx++]=224|u>>12;heap[outIdx++]=128|u>>6&63;heap[outIdx++]=128|u&63}else{if(outIdx+3>=endIdx)break;heap[outIdx++]=240|u>>18;heap[outIdx++]=128|u>>12&63;heap[outIdx++]=128|u>>6&63;heap[outIdx++]=128|u&63}}heap[outIdx]=0;return outIdx-startIdx}function stringToUTF8(str,outPtr,maxBytesToWrite){return stringToUTF8Array(str,HEAPU8,outPtr,maxBytesToWrite)}var buffer,HEAP8,HEAPU8,HEAP16,HEAPU16,HEAP32,HEAPU32,HEAPF32,HEAPF64;function updateGlobalBufferAndViews(buf){buffer=buf;Module["HEAP8"]=HEAP8=new Int8Array(buf);Module["HEAP16"]=HEAP16=new Int16Array(buf);Module["HEAP32"]=HEAP32=new Int32Array(buf);Module["HEAPU8"]=HEAPU8=new Uint8Array(buf);Module["HEAPU16"]=HEAPU16=new Uint16Array(buf);Module["HEAPU32"]=HEAPU32=new Uint32Array(buf);Module["HEAPF32"]=HEAPF32=new Float32Array(buf);Module["HEAPF64"]=HEAPF64=new Float64Array(buf)}var INITIAL_MEMORY=Module["INITIAL_MEMORY"]||16777216;var wasmTable;var __ATPRERUN__=[];var __ATINIT__=[];var __ATPOSTRUN__=[];var runtimeInitialized=false;function keepRuntimeAlive(){return noExitRuntime}function preRun(){if(Module["preRun"]){if(typeof Module["preRun"]=="function")Module["preRun"]=[Module["preRun"]];while(Module["preRun"].length){addOnPreRun(Module["preRun"].shift())}}callRuntimeCallbacks(__ATPRERUN__)}function initRuntime(){runtimeInitialized=true;callRuntimeCallbacks(__ATINIT__)}function postRun(){if(Module["postRun"]){if(typeof Module["postRun"]=="function")Module["postRun"]=[Module["postRun"]];while(Module["postRun"].length){addOnPostRun(Module["postRun"].shift())}}callRuntimeCallbacks(__ATPOSTRUN__)}function addOnPreRun(cb){__ATPRERUN__.unshift(cb)}function addOnInit(cb){__ATINIT__.unshift(cb)}function addOnPostRun(cb){__ATPOSTRUN__.unshift(cb)}var runDependencies=0;var runDependencyWatcher=null;var dependenciesFulfilled=null;function addRunDependency(id){runDependencies++;if(Module["monitorRunDependencies"]){Module["monitorRunDependencies"](runDependencies)}}function removeRunDependency(id){runDependencies--;if(Module["monitorRunDependencies"]){Module["monitorRunDependencies"](runDependencies)}if(runDependencies==0){if(runDependencyWatcher!==null){clearInterval(runDependencyWatcher);runDependencyWatcher=null}if(dependenciesFulfilled){var callback=dependenciesFulfilled;dependenciesFulfilled=null;callback()}}}function abort(what){{if(Module["onAbort"]){Module["onAbort"](what)}}what="Aborted("+what+")";err(what);ABORT=true;EXITSTATUS=1;what+=". Build with -sASSERTIONS for more info.";var e=new WebAssembly.RuntimeError(what);readyPromiseReject(e);throw e}var dataURIPrefix="data:application/octet-stream;base64,";function isDataURI(filename){return filename.startsWith(dataURIPrefix)}function isFileURI(filename){return filename.startsWith("file://")}var wasmBinaryFile;wasmBinaryFile="lnsocket.wasm";if(!isDataURI(wasmBinaryFile)){wasmBinaryFile=locateFile(wasmBinaryFile)}function getBinary(file){try{if(file==wasmBinaryFile&&wasmBinary){return new Uint8Array(wasmBinary)}if(readBinary){return readBinary(file)}else{throw"both async and sync fetching of the wasm failed"}}catch(err){abort(err)}}function getBinaryPromise(){if(!wasmBinary&&(ENVIRONMENT_IS_WEB||ENVIRONMENT_IS_WORKER)){if(typeof fetch=="function"&&!isFileURI(wasmBinaryFile)){return fetch(wasmBinaryFile,{credentials:"same-origin"}).then(function(response){if(!response["ok"]){throw"failed to load wasm binary file at '"+wasmBinaryFile+"'"}return response["arrayBuffer"]()}).catch(function(){return getBinary(wasmBinaryFile)})}else{if(readAsync){return new Promise(function(resolve,reject){readAsync(wasmBinaryFile,function(response){resolve(new Uint8Array(response))},reject)})}}}return Promise.resolve().then(function(){return getBinary(wasmBinaryFile)})}function createWasm(){var info={"a":asmLibraryArg};function receiveInstance(instance,module){var exports=instance.exports;Module["asm"]=exports;wasmMemory=Module["asm"]["i"];updateGlobalBufferAndViews(wasmMemory.buffer);wasmTable=Module["asm"]["m"];addOnInit(Module["asm"]["j"]);removeRunDependency("wasm-instantiate")}addRunDependency("wasm-instantiate");function receiveInstantiationResult(result){receiveInstance(result["instance"])}function instantiateArrayBuffer(receiver){return getBinaryPromise().then(function(binary){return WebAssembly.instantiate(binary,info)}).then(function(instance){return instance}).then(receiver,function(reason){err("failed to asynchronously prepare wasm: "+reason);abort(reason)})}function instantiateAsync(){if(!wasmBinary&&typeof WebAssembly.instantiateStreaming=="function"&&!isDataURI(wasmBinaryFile)&&!isFileURI(wasmBinaryFile)&&!ENVIRONMENT_IS_NODE&&typeof fetch=="function"){return fetch(wasmBinaryFile,{credentials:"same-origin"}).then(function(response){var result=WebAssembly.instantiateStreaming(response,info);return result.then(receiveInstantiationResult,function(reason){err("wasm streaming compile failed: "+reason);err("falling back to ArrayBuffer instantiation");return instantiateArrayBuffer(receiveInstantiationResult)})})}else{return instantiateArrayBuffer(receiveInstantiationResult)}}if(Module["instantiateWasm"]){try{var exports=Module["instantiateWasm"](info,receiveInstance);return exports}catch(e){err("Module.instantiateWasm callback failed with error: "+e);return false}}instantiateAsync().catch(readyPromiseReject);return{}}var ASM_CONSTS={70236:()=>{return Module.getRandomValue()},70272:()=>{if(Module.getRandomValue===undefined){try{var window_="object"===typeof window?window:self;var crypto_=typeof window_.crypto!=="undefined"?window_.crypto:window_.msCrypto;var randomValuesStandard=function(){var buf=new Uint32Array(1);crypto_.getRandomValues(buf);return buf[0]>>>0};randomValuesStandard();Module.getRandomValue=randomValuesStandard}catch(e){try{var crypto=require("crypto");var randomValueNodeJS=function(){var buf=crypto["randomBytes"](4);return(buf[0]<<24|buf[1]<<16|buf[2]<<8|buf[3])>>>0};randomValueNodeJS();Module.getRandomValue=randomValueNodeJS}catch(e){throw"No secure random number generator found"}}}}};function ExitStatus(status){this.name="ExitStatus";this.message="Program terminated with exit("+status+")";this.status=status}function callRuntimeCallbacks(callbacks){while(callbacks.length>0){callbacks.shift()(Module)}}function writeArrayToMemory(array,buffer){HEAP8.set(array,buffer)}function ___assert_fail(condition,filename,line,func){abort("Assertion failed: "+UTF8ToString(condition)+", at: "+[filename?UTF8ToString(filename):"unknown filename",line,func?UTF8ToString(func):"unknown function"])}function _abort(){abort("")}var readAsmConstArgsArray=[];function readAsmConstArgs(sigPtr,buf){readAsmConstArgsArray.length=0;var ch;buf>>=2;while(ch=HEAPU8[sigPtr++]){buf+=ch!=105&buf;readAsmConstArgsArray.push(ch==105?HEAP32[buf]:HEAPF64[buf++>>1]);++buf}return readAsmConstArgsArray}function _emscripten_asm_const_int(code,sigPtr,argbuf){var args=readAsmConstArgs(sigPtr,argbuf);return ASM_CONSTS[code].apply(null,args)}function _emscripten_memcpy_big(dest,src,num){HEAPU8.copyWithin(dest,src,src+num)}function abortOnCannotGrowMemory(requestedSize){abort("OOM")}function _emscripten_resize_heap(requestedSize){var oldSize=HEAPU8.length;requestedSize=requestedSize>>>0;abortOnCannotGrowMemory(requestedSize)}var SYSCALLS={varargs:undefined,get:function(){SYSCALLS.varargs+=4;var ret=HEAP32[SYSCALLS.varargs-4>>2];return ret},getStr:function(ptr){var ret=UTF8ToString(ptr);return ret}};function _fd_close(fd){return 52}function _fd_seek(fd,offset_low,offset_high,whence,newOffset){return 70}var printCharBuffers=[null,[],[]];function printChar(stream,curr){var buffer=printCharBuffers[stream];if(curr===0||curr===10){(stream===1?out:err)(UTF8ArrayToString(buffer,0));buffer.length=0}else{buffer.push(curr)}}function _fd_write(fd,iov,iovcnt,pnum){var num=0;for(var i=0;i<iovcnt;i++){var ptr=HEAPU32[iov>>2];var len=HEAPU32[iov+4>>2];iov+=8;for(var j=0;j<len;j++){printChar(fd,HEAPU8[ptr+j])}num+=len}HEAPU32[pnum>>2]=num;return 0}function getCFunc(ident){var func=Module["_"+ident];return func}function ccall(ident,returnType,argTypes,args,opts){var toC={"string":function(str){var ret=0;if(str!==null&&str!==undefined&&str!==0){var len=(str.length<<2)+1;ret=stackAlloc(len);stringToUTF8(str,ret,len)}return ret},"array":function(arr){var ret=stackAlloc(arr.length);writeArrayToMemory(arr,ret);return ret}};function convertReturnValue(ret){if(returnType==="string"){return UTF8ToString(ret)}if(returnType==="boolean")return Boolean(ret);return ret}var func=getCFunc(ident);var cArgs=[];var stack=0;if(args){for(var i=0;i<args.length;i++){var converter=toC[argTypes[i]];if(converter){if(stack===0)stack=stackSave();cArgs[i]=converter(args[i])}else{cArgs[i]=args[i]}}}var ret=func.apply(null,cArgs);function onDone(ret){if(stack!==0)stackRestore(stack);return convertReturnValue(ret)}ret=onDone(ret);return ret}function cwrap(ident,returnType,argTypes,opts){argTypes=argTypes||[];var numericArgs=argTypes.every(type=>type==="number");var numericRet=returnType!=="string";if(numericRet&&numericArgs&&!opts){return getCFunc(ident)}return function(){return ccall(ident,returnType,argTypes,arguments,opts)}}var asmLibraryArg={"a":___assert_fail,"b":_abort,"h":_emscripten_asm_const_int,"g":_emscripten_memcpy_big,"c":_emscripten_resize_heap,"f":_fd_close,"d":_fd_seek,"e":_fd_write};var asm=createWasm();var ___wasm_call_ctors=Module["___wasm_call_ctors"]=function(){return(___wasm_call_ctors=Module["___wasm_call_ctors"]=Module["asm"]["j"]).apply(null,arguments)};var _malloc=Module["_malloc"]=function(){return(_malloc=Module["_malloc"]=Module["asm"]["k"]).apply(null,arguments)};var _free=Module["_free"]=function(){return(_free=Module["_free"]=Module["asm"]["l"]).apply(null,arguments)};var _lnsocket_make_default_initmsg=Module["_lnsocket_make_default_initmsg"]=function(){return(_lnsocket_make_default_initmsg=Module["_lnsocket_make_default_initmsg"]=Module["asm"]["n"]).apply(null,arguments)};var _lnsocket_encrypt=Module["_lnsocket_encrypt"]=function(){return(_lnsocket_encrypt=Module["_lnsocket_encrypt"]=Module["asm"]["o"]).apply(null,arguments)};var _lnsocket_decrypt=Module["_lnsocket_decrypt"]=function(){return(_lnsocket_decrypt=Module["_lnsocket_decrypt"]=Module["asm"]["p"]).apply(null,arguments)};var _lnsocket_decrypt_header=Module["_lnsocket_decrypt_header"]=function(){return(_lnsocket_decrypt_header=Module["_lnsocket_decrypt_header"]=Module["asm"]["q"]).apply(null,arguments)};var _lnsocket_make_pong_msg=Module["_lnsocket_make_pong_msg"]=function(){return(_lnsocket_make_pong_msg=Module["_lnsocket_make_pong_msg"]=Module["asm"]["r"]).apply(null,arguments)};var _lnsocket_make_pong_from_ping=Module["_lnsocket_make_pong_from_ping"]=function(){return(_lnsocket_make_pong_from_ping=Module["_lnsocket_make_pong_from_ping"]=Module["asm"]["s"]).apply(null,arguments)};var _lnsocket_make_ping_msg=Module["_lnsocket_make_ping_msg"]=function(){return(_lnsocket_make_ping_msg=Module["_lnsocket_make_ping_msg"]=Module["asm"]["t"]).apply(null,arguments)};var _lnsocket_msgbuf=Module["_lnsocket_msgbuf"]=function(){return(_lnsocket_msgbuf=Module["_lnsocket_msgbuf"]=Module["asm"]["u"]).apply(null,arguments)};var _lnsocket_create=Module["_lnsocket_create"]=function(){return(_lnsocket_create=Module["_lnsocket_create"]=Module["asm"]["v"]).apply(null,arguments)};var _lnsocket_destroy=Module["_lnsocket_destroy"]=function(){return(_lnsocket_destroy=Module["_lnsocket_destroy"]=Module["asm"]["w"]).apply(null,arguments)};var _lnsocket_secp=Module["_lnsocket_secp"]=function(){return(_lnsocket_secp=Module["_lnsocket_secp"]=Module["asm"]["x"]).apply(null,arguments)};var _lnsocket_genkey=Module["_lnsocket_genkey"]=function(){return(_lnsocket_genkey=Module["_lnsocket_genkey"]=Module["asm"]["y"]).apply(null,arguments)};var _lnsocket_setkey=Module["_lnsocket_setkey"]=function(){return(_lnsocket_setkey=Module["_lnsocket_setkey"]=Module["asm"]["z"]).apply(null,arguments)};var _lnsocket_print_errors=Module["_lnsocket_print_errors"]=function(){return(_lnsocket_print_errors=Module["_lnsocket_print_errors"]=Module["asm"]["A"]).apply(null,arguments)};var _lnsocket_act_two=Module["_lnsocket_act_two"]=function(){return(_lnsocket_act_two=Module["_lnsocket_act_two"]=Module["asm"]["B"]).apply(null,arguments)};var _commando_make_rpc_msg=Module["_commando_make_rpc_msg"]=function(){return(_commando_make_rpc_msg=Module["_commando_make_rpc_msg"]=Module["asm"]["C"]).apply(null,arguments)};var _lnsocket_act_one=Module["_lnsocket_act_one"]=function(){return(_lnsocket_act_one=Module["_lnsocket_act_one"]=Module["asm"]["D"]).apply(null,arguments)};var stackSave=Module["stackSave"]=function(){return(stackSave=Module["stackSave"]=Module["asm"]["E"]).apply(null,arguments)};var stackRestore=Module["stackRestore"]=function(){return(stackRestore=Module["stackRestore"]=Module["asm"]["F"]).apply(null,arguments)};var stackAlloc=Module["stackAlloc"]=function(){return(stackAlloc=Module["stackAlloc"]=Module["asm"]["G"]).apply(null,arguments)};Module["ccall"]=ccall;Module["cwrap"]=cwrap;var calledRun;dependenciesFulfilled=function runCaller(){if(!calledRun)run();if(!calledRun)dependenciesFulfilled=runCaller};function run(args){args=args||arguments_;if(runDependencies>0){return}preRun();if(runDependencies>0){return}function doRun(){if(calledRun)return;calledRun=true;Module["calledRun"]=true;if(ABORT)return;initRuntime();readyPromiseResolve(Module);if(Module["onRuntimeInitialized"])Module["onRuntimeInitialized"]();postRun()}if(Module["setStatus"]){Module["setStatus"]("Running...");setTimeout(function(){setTimeout(function(){Module["setStatus"]("")},1);doRun()},1)}else{doRun()}}if(Module["preInit"]){if(typeof Module["preInit"]=="function")Module["preInit"]=[Module["preInit"]];while(Module["preInit"].length>0){Module["preInit"].pop()()}}run();


  return Module.ready
}
);
})();
if (typeof exports === 'object' && typeof module === 'object')
  module.exports = Module;
else if (typeof define === 'function' && define['amd'])
  define([], function() { return Module; });
else if (typeof exports === 'object')
  exports["Module"] = Module;

async function lnsocket_init() {
	const module = await Module()

	function SocketImpl(host) {
		if (!(this instanceof SocketImpl))
			return new SocketImpl(host)

		if (typeof WebSocket !== 'undefined') {
			console.log("WebSocket", typeof WebSocket)
			const ok = host.startsWith("ws://") || host.startsWith("wss://")
			if (!ok)
				throw new Error("host must start with ws:// or wss://")
			const ws = new WebSocket(host)
			ws.ondata = function(fn) {
				ws.onmessage = (v) => {
					const data = v.data.arrayBuffer()
					fn(data)
				}
			}
			return ws
		}

		//
		// we're in nodejs
		//
		const net = require('net')
		let [hostname,port] = host.split(":")
		port = +port || 9735
		const socket = net.createConnection(port, hostname, () => {
			socket.emit("open")
		})
		socket.addEventListener = socket.on.bind(socket)

		if (socket.onmessage)
			throw new Error("socket already has onmessage?")

		socket.ondata = (fn) => {
			socket.on('data', fn)
		}

		socket.close = () => {
			socket.destroy()
		}

		if (socket.send)
			throw new Error("socket already has send?")

		socket.send = function socket_send(data) {
			return new Promise((resolve, reject) => {
				socket.write(data, resolve)
			});
		}

		return socket
	}

	const ACT_ONE_SIZE = 50
	const ACT_TWO_SIZE = 50
	const ACT_THREE_SIZE = 66
	const DEFAULT_TIMEOUT = 15000

	const COMMANDO_REPLY_CONTINUES = 0x594b
	const COMMANDO_REPLY_TERM = 0x594d
	const WIRE_PING = 18

	const lnsocket_create = module.cwrap("lnsocket_create", "number")
	const lnsocket_destroy = module.cwrap("lnsocket_destroy", "number")
	const lnsocket_encrypt = module.cwrap("lnsocket_encrypt", "number", ["int", "array", "int", "int"])
	const lnsocket_decrypt = module.cwrap("lnsocket_decrypt", "number", ["int", "array", "int"])
	const lnsocket_decrypt_header = module.cwrap("lnsocket_decrypt_header", "number", ["number", "array"])
	const lnsocket_msgbuf = module.cwrap("lnsocket_msgbuf", "number", ["int"])
	const lnsocket_act_one = module.cwrap("lnsocket_act_one", "number", ["number", "string"])
	const lnsocket_act_two = module.cwrap("lnsocket_act_two", "number", ["number", "array"])
	const lnsocket_print_errors = module.cwrap("lnsocket_print_errors", "int")
	const lnsocket_genkey = module.cwrap("lnsocket_genkey", null, ["number"])
	const lnsocket_setkey = module.cwrap("lnsocket_setkey", "number", ["number", "array"])
	const lnsocket_make_default_initmsg = module.cwrap("lnsocket_make_default_initmsg", "int", ["int", "int"])
	const lnsocket_make_ping_msg = module.cwrap("lnsocket_make_ping_msg", "int", ["int", "int", "int", "int"])
	const lnsocket_make_pong_from_ping = module.cwrap("lnsocket_make_pong_from_ping", "int", ["int", "int", "array", "int"])
	const commando_make_rpc_msg = module.cwrap("commando_make_rpc_msg", "int", ["string", "string", "string", "number", "int", "int"])

	function concat_u8_arrays(arrays) {
		// sum of individual array lengths
		let totalLength = arrays.reduce((acc, value) =>
			acc + (value.length || value.byteLength)
		, 0);

		if (!arrays.length) return null;

		let result = new Uint8Array(totalLength);

		let length = 0;
		for (let array of arrays) {
			if (array instanceof ArrayBuffer)
				result.set(new Uint8Array(array), length);
			else
				result.set(array, length);

			length += (array.length || array.byteLength);
		}

		return result;
	}

	function parse_msgtype(buf) {
		return buf[0] << 8 | buf[1]
	}

	function wasm_free(buf) {
		module._free(buf);
	}

	function char_to_hex(cstr) {
		const c = cstr.charCodeAt(0)
		// c >= 0 && c <= 9
		if (c >= 48 && c <= 57) {
			return c - 48;
		}
		// c >= a && c <= f
		if (c >= 97 && c <= 102) {
			return c - 97 + 10;
		}
		// c >= A && c <= F
		if (c >= 65 && c <= 70) {
			return c - 65 + 10;
		}
		return -1;
	}


	function hex_decode(str, buflen)
	{
		let bufsize = buflen || 33
		let c1, c2
		let i = 0
		let j = 0
		let buf = new Uint8Array(bufsize)
		let slen = str.length
		while (slen > 1) {
			if (-1==(c1 = char_to_hex(str[j])) || -1==(c2 = char_to_hex(str[j+1])))
				return null;
			if (!bufsize)
				return null;
			j += 2
			slen -= 2
			buf[i++] = (c1 << 4) | c2
			bufsize--;
		}

		return buf
	}

	function wasm_alloc(len) {
		const buf = module._malloc(len);
		module.HEAPU8.set(Uint8Array, buf);
		return buf
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

	LNSocket.prototype.queue_recv = function() {
		let self = this
		return new Promise((resolve, reject) => {
			const checker = setInterval(() => {
				const val = self.queue.shift()
				if (val) {
					clearInterval(checker)
					resolve(val)
				} else if (!self.connected) {
					clearInterval(checker)
					reject()
				}
			}, 5);
		})
	}

	LNSocket.prototype.print_errors = function _lnsocket_print_errors() {
		lnsocket_print_errors(this.ln)
	}

	LNSocket.prototype.genkey = function _lnsocket_genkey() {
		lnsocket_genkey(this.ln)
	}

	LNSocket.prototype.setkeyraw = function lnsocket_setkeyraw(rawkey) {
		return lnsocket_setkey(this.ln, rawkey)
	}

	LNSocket.prototype.setkey = function _lnsocket_setkey(key) {
		const rawkey = hex_decode(key)
		return this.setkeyraw(rawkey)
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
		const act2 = await this.read_all(ACT_TWO_SIZE)
		if (act2.length != ACT_TWO_SIZE) {
			throw new Error(`expected act2 to be ${ACT_TWO_SIZE} long, got ${act2.length}`)
		}
		const act3 = this.act_two(act2)
		this.ws.send(act3)
	}

	LNSocket.prototype.connect_and_init = async function _connect_and_init(node_id, host) {
		await this.connect(node_id, host)
		await this.perform_init()
	}

	LNSocket.prototype.read_all = async function read_all(n) {
		let count = 0
		let chunks = []
		if (!this.connected)
			throw new Error("read_all: not connected")
		while (true) {
			let res = await this.queue_recv()

			const remaining = n - count

			if (res.byteLength > remaining) {
				chunks.push(res.slice(0, remaining))
				this.queue.unshift(res.slice(remaining))
				break
			} else if (res.byteLength === remaining) {
				chunks.push(res)
				break
			}

			chunks.push(res)
			count += res.byteLength
		}

		return concat_u8_arrays(chunks)
	}

	LNSocket.prototype.read_header = async function read_header() {
		const header = await this.read_all(18)
		if (header.length != 18)
			throw new Error("Failed to read header")
		return lnsocket_decrypt_header(this.ln, header)
	}

	LNSocket.prototype.rpc = async function lnsocket_rpc(opts) {
		const msg = this.make_commando_msg(opts)
		this.write(msg)
		const res = await this.read_all_rpc()
		return JSON.parse(res)
	}

	LNSocket.prototype.recv = async function lnsocket_recv() {
		const msg = await this.read()
		const msgtype = parse_msgtype(msg.slice(0,2))
		const res = [msgtype, msg.slice(2)]
		return res
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
			case WIRE_PING:
				// cln will disconnect us eventually if we don't pong
				const pong = this.make_pong(msg, msg.length)
				if (pong) {
					console.log("lnsocket: got ping -> sent pong", typ)
					this.write(pong)
				} else {
					console.log("lnsocket: failed to create pong :(")
				}
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
		const buf = wasm_alloc(buflen);

		const params = JSON.stringify(opts.params||{})
		if (!(len = commando_make_rpc_msg(opts.method, params, opts.rune,
			0, buf, buflen))) {
			throw new Error("couldn't make commando msg");
		}

		const dat = wasm_mem(buf, len)
		wasm_free(buf);
		return dat
	}

	LNSocket.prototype.make_ping_msg = function _lnsocket_make_ping_msg(num_pong_bytes=1, ignored_bytes=1)  {
		const buflen = 32
		let len = 0;
		const buf = wasm_alloc(buflen)

		if (!(len = lnsocket_make_ping_msg(buf, buflen, num_pong_bytes, ignored_bytes)))
			throw new Error("couldn't make ping msg");

		const dat = wasm_mem(buf, len)
		wasm_free(buf);
		return dat
	}

	LNSocket.prototype.make_pong = function _lnsocket_make_pong_msg(ping, ping_len) {
		const buflen = 0xFFFF
		const buf = wasm_alloc(buflen)
		if (!lnsocket_make_pong_from_ping(buf, buflen, ping, ping.length))
			return 0
		const dat = wasm_mem(buf, 0xFFFF)
		wasm_free(buf)
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

	LNSocket.prototype.read = async function _lnsocket_read() {
		const size = await this.read_header()
		const enc = await this.read_all(size+16)
		return this.decrypt(enc)
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
		this.write(our_init)
	}

	LNSocket.prototype.ping_pong = async function lnsocket_ping_pong() {
		const pingmsg = this.make_ping_msg()
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

	LNSocket.prototype.destroy = function _lnsocket_destroy() {
		this.disconnect()
		lnsocket_destroy(this.ln)
	}

	function handle_connect(ln, node_id, host) {
		const ws = new SocketImpl(host)
		return new Promise((resolve, reject) => {
			const timeout = ln.opts.timeout || DEFAULT_TIMEOUT
			const timer = setTimeout(reject, timeout);

			ws.ondata((v) => {
				ln.queue.push(v)
			});

			ws.addEventListener('open', function(ev) {
				ln.ws = ws
				ln.connected = true
				clearTimeout(timer)
				resolve(ws)
			});

			ws.addEventListener('close', function(ev) {
				ln.connected = false
			});
		})
	}

	return LNSocket
}

Module.init = Module.lnsocket_init = lnsocket_init
