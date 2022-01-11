{ pkgs ? import <nixpkgs> {} }:
with pkgs;
mkShell {
  buildInputs = [ ];
  nativeBuildInputs = [ gdb autoreconfHook clib ];
}
