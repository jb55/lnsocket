
# lnsocket

A simple C, JS and Go library for sending messages over the lightning network.

Thanks to Rusty and the clightning project for much of this code, I have
adapted it to be more library friendly.


## Motivation

I wanted a way to send custom messages to my lightning node, such as RPC.
Building this as a simple C library will allow you to speak the lightning
network in native applications, like on mobile.


## Dependencies

You'll need `libtool`, `autoconf`, and `automake` for the `libsodium` &
`secp256k1` submodules, but otherwise there are no dependencies.

You'll need `emscripten` for the `wasm` build.

## Building

    $ make

### iOS

    $ make ios

This will build `lnsocket.a`, `libsodium.a` and `libsecp256k1.a` under
`target/ios` for `arm64` and `ios-sim-x86`.


### WASM/JS/Web

Building manually:

    $ make js

This will build `lnsocket.js` and `lnsocket.wasm` in `target/js` so that you
can connect to the lightning network from your browser via websockets. 

There are packaged versions of the js build under [dist/js](dist/js)

If you are in a web environment that supports npm modules, you can import
lnsocket using npm:

```js
const LNSocket = require('lnsocket')

async function makeRequest(method, params, rune) {
  const ln = await LNSocket()
  ln.genkey()
  await ln.connect_and_init(node_id, host)
  // ... etc
}
```

The plain js file under [dist/js](dist/js) declares an `lnsocket_init()`
function like so:

```js
const LNSocket = await lnsocket_init()
const ln = LNSocket()

ln.genkey()
```

See [examples/websockets.js](examples/websockets.js) for a demo.

### NodeJS

    $ npm install --save lnsocket

See [examples/node.js](examples/node.js)

### Go

There is a Go version of lnsocket written using lnd's brontide[^3].

You can import it via:

    import "github.com/jb55/lnsocket/go"

It is currently used in fiatjaf's makeinvoice go library[^4] if you want an
example of its usage.

### Rust

There are some initial rust bindings which you can build via: `make rust`

## C Examples

* See [test.c](test.c) for a ping/pong example

* See [lnrpc.c](lnrpc.c) for an RPC example

## Contributing

Send patches to `jb55@jb55.com`

    $ git config format.subjectPrefix 'PATCH lnsocket'
    $ git config sendemail.to 'William Casarin <jb55@jb55.com>'
    $ git send-email --annotate HEAD^

See git-send-email.io[^1] for configuring your mailer

You can open a PR on github[^2] as well

[^1]: https://git-send-email.io/
[^2]: https://github.com/jb55/lnsocket
[^3]: https://github.com/lightningnetwork/lnd/tree/master/brontide
[^4]: https://github.com/fiatjaf/makeinvoice/blob/d523b35084af04883f94323dc11a50c2a99d253d/makeinvoice.go#L366
