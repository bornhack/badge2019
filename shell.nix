{ pkgs ? import <nixpkgs> {} }:

with pkgs;

stdenv.mkDerivation {
  name = "bornhack-badge-2019";

  buildInputs = [ gcc-arm-embedded newlib dfu-util ];
}
