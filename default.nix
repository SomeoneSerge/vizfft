{ pkgs ? import <nixpkgs> { } }:

with pkgs;

callPackage ./vizfft.nix { }
