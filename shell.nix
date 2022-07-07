{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
  buildInputs = [ ];
  nativeBuildInputs = with pkgs; [ 
    gdb
    autoreconfHook
    emscripten
    go
    pkgconfig
    rustup
    llvmPackages_latest.llvm
    llvmPackages_latest.bintools
    zlib.out
    llvmPackages_latest.lld
    python3
  ];

  RUSTC_VERSION = "nightly-2021-09-19";
  LIBCLANG_PATH= pkgs.lib.makeLibraryPath [ pkgs.llvmPackages_latest.libclang.lib ];
  HISTFILE=toString ./.history;

  shellHook = ''
    export PATH=$PATH:~/.cargo/bin
    export PATH=$PATH:~/.rustup/toolchains/$RUSTC_VERSION-x86_64-unknown-linux-gnu/bin/
  '';

  BINDGEN_EXTRA_CLANG_ARGS = 
    # Includes with normal include path
    (builtins.map (a: ''-I"${a}/include"'') [
      pkgs.glibc.dev 
    ])
    # Includes with special directory paths
    ++ [
      ''-I"${pkgs.llvmPackages_latest.libclang.lib}/lib/clang/${pkgs.llvmPackages_latest.libclang.version}/include"''
      ''-I"${pkgs.glib.dev}/include/glib-2.0"''
      ''-I${pkgs.glib.out}/lib/glib-2.0/include/''
    ];
}
