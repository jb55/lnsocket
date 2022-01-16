{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
  buildInputs = [ ];
  nativeBuildInputs = with pkgs; [ gdb autoreconfHook ];
}
