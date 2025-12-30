{
  description = "k95aux flake";
  
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs, ... }: let
    pkgs = nixpkgs.legacyPackages.x86_64-linux;
  in {
    devShells.x86_64-linux.default = pkgs.mkShell {
      packages = with pkgs; [
        bashInteractive
        gcc
      ];
    };

    packages.x86_64-linux.default = pkgs.callPackage ./package.nix {};
  };
}
