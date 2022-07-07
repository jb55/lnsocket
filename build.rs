extern crate bindgen;

use std::env;
use std::path::PathBuf;

#[derive(Debug)]
struct IgnoreMacros(String);

impl bindgen::callbacks::ParseCallbacks for IgnoreMacros {
    fn will_parse_macro(&self, name: &str) -> bindgen::callbacks::MacroParsingBehavior {
        if name == self.0 {
            bindgen::callbacks::MacroParsingBehavior::Ignore
        } else {
            bindgen::callbacks::MacroParsingBehavior::Default
        }
    }
}

fn main() {
    // Tell cargo to look for shared libraries in the specified directory
    let lib_path = PathBuf::from(env::current_dir().unwrap());
    println!("cargo:rustc-link-search={}", lib_path.display());

    // Tell cargo to tell rustc to link the shared library.
    println!("cargo:rustc-link-lib=lnsocket");
    println!("cargo:rustc-link-lib=secp256k1");
    println!("cargo:rustc-link-lib=sodium");

    let ignored_macros = IgnoreMacros("IPPORT_RESERVED".to_string());

    // The bindgen::Builder is the main entry point
    // to bindgen, and lets you build up options for
    // the resulting bindings.
    let bindings = bindgen::Builder::default()
        // The input header we would like to generate
        // bindings for.
        .header("lnsocket.h")
        .header("lnsocket_internal.h")
        .clang_arg("-Ideps/secp256k1/include")
        .clang_arg("-Ideps/libsodium/src/libsodium/include")
        .header("deps/secp256k1/include/secp256k1.h")
        .parse_callbacks(Box::new(ignored_macros))
        .trust_clang_mangling(false)
        // Finish the builder and generate the bindings.
        .generate()
        // Unwrap the Result and panic on failure.
        .expect("Unable to generate bindings");

    // Write the bindings to the $OUT_DIR/bindings.rs file.
    bindings
        .write_to_file("./rust/bindings.rs")
        .expect("Couldn't write bindings!");
}
