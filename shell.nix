{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
  buildInputs = [ ];
  nativeBuildInputs = with pkgs; [ gdb autoreconfHook emscripten wabt go ];
}
