extern crate bindgen;

use std::env;
use std::path::PathBuf;
use std::process::Command;

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
    // fetch deps
    std::process::Command::new("git")
        .args([
            "submodule",
            "update",
            "--init",
            "--depth 1",
            "--recommend-shallow",
    ])
    .output()
    .expect("Failed to fetch git submodules!");

    // Build the library
    Command::new("make").status()
    .expect("Failed to build library");

    // Copy library
    let lib_path = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let input_path = lib_path.join("lnsocket.a");
    let output_path = lib_path.join("liblnsocket.a");
    let res = std::fs::copy(input_path, output_path);
    println!("cargo:warning={:#?}",res);

    // Tell cargo to look for shared libraries in the specified directory
    println!("cargo:rustc-link-search={}", lib_path.display());

    // Tell cargo to tell rustc to link the shared library.
    println!("cargo:rustc-link-lib=static=lnsocket");
    println!("cargo:rustc-link-lib=static=secp256k1");
    println!("cargo:rustc-link-lib=static=sodium");

    let ignored_macros = IgnoreMacros("IPPORT_RESERVED".to_string());

    // The bindgen::Builder is the main entry point
    // to bindgen, and lets you build up options for
    // the resulting bindings.
    let bindings = bindgen::Builder::default()
        // The input header we would like to generate
        // bindings for.
        .header("lnsocket.h")
        .header("lnsocket_internal.h")
        .clang_arg(format!("-I{}", lib_path.join("deps/secp256k1/include").display()))
        .clang_arg(format!("-I{}", lib_path.join("deps/libsodium/src/libsodium/include").display()))
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
