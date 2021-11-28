{ pkgs ? import <nixpkgs> { crossSystem.config = "x86_64-elf"; } }:
pkgs.mkShell {
    nativeBuildInputs = with pkgs.buildPackages; [cmake gcc];
    buildInputs = with pkgs; [];
}
