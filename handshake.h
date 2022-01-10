
#ifndef LNLINK_HANDSHAKE_H
#define LNLINK_HANDSHAKE_H

#include <secp256k1/>

struct pubkey {
	/* Unpacked pubkey (as used by libsecp256k1 internally) */
	secp256k1_pubkey pubkey;
};

/* BOLT #8:
 *
 * Act One is sent from initiator to responder. During Act One, the
 * initiator attempts to satisfy an implicit challenge by the responder. To
 * complete this challenge, the initiator must know the static public key of
 * the responder.
 */
struct act_one {
	u8 v;
	u8 pubkey[PUBKEY_CMPR_LEN];
	u8 tag[crypto_aead_chacha20poly1305_ietf_ABYTES];
};

/* BOLT #8:
 *
 * Act Two is sent from the responder to the initiator. Act Two will
 * _only_ take place if Act One was successful. Act One was successful if
 * the responder was able to properly decrypt and check the MAC of the tag
 * sent at the end of Act One.
 */
struct act_two {
	u8 v;
	u8 pubkey[PUBKEY_CMPR_LEN];
	u8 tag[crypto_aead_chacha20poly1305_ietf_ABYTES];
};

/* BOLT #8:
 *
 * Act Three is the final phase in the authenticated key agreement described
 * in this section. This act is sent from the initiator to the responder as a
 * concluding step. Act Three is executed  _if and only if_  Act Two was
 * successful.  During Act Three, the initiator transports its static public
 * key to the responder encrypted with _strong_ forward secrecy, using the
 * accumulated `HKDF` derived secret key at this point of the handshake.
 */
struct act_three {
	u8 v;
	u8 ciphertext[PUBKEY_CMPR_LEN + crypto_aead_chacha20poly1305_ietf_ABYTES];
	u8 tag[crypto_aead_chacha20poly1305_ietf_ABYTES];
};

/* BOLT #8:
 *
 * Throughout the handshake process, each side maintains these variables:
 *
 *  * `ck`: the **chaining key**. This value is the accumulated hash of all
 *    previous ECDH outputs. At the end of the handshake, `ck` is used to derive
 *    the encryption keys for Lightning messages.
 *
 *  * `h`: the **handshake hash**. This value is the accumulated hash of _all_
 *    handshake data that has been sent and received so far during the handshake
 *    process.
 *
 *  * `temp_k1`, `temp_k2`, `temp_k3`: the **intermediate keys**. These are used to
 *    encrypt and decrypt the zero-length AEAD payloads at the end of each handshake
 *    message.
 *
 *  * `e`: a party's **ephemeral keypair**. For each session, a node MUST generate a
 *    new ephemeral key with strong cryptographic randomness.
 *
 *  * `s`: a party's **static keypair** (`ls` for local, `rs` for remote)
 */
struct handshake {
	struct secret ck;
	struct secret temp_k;
	struct sha256 h;
	struct keypair e;
	struct secret *ss;

	/* Used between the Acts */
	struct pubkey re;
	struct act_one act1;
	struct act_two act2;
	struct act_three act3;

	/* Where is connection from/to */
	struct wireaddr_internal addr;

	/* Who we are */
	struct pubkey my_id;
	/* Who they are: set already if we're initiator. */
	struct pubkey their_id;

	/* Are we initiator or responder. */
	enum bolt8_side side;

	/* Function to call once handshake complete. */
	struct io_plan *(*cb)(struct io_conn *conn,
			      const struct pubkey *their_id,
			      const struct wireaddr_internal *wireaddr,
			      struct crypto_state *cs,
			      void *cbarg);
	void *cbarg;
};

#endif /* LNLINK_HANDSHAKE_H */
