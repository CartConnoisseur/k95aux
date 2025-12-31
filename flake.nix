{
  description = "k95aux flake";
  
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }@inputs: let
    system = "x86_64-linux";
    pkgs = nixpkgs.legacyPackages.${system};
  in {
    devShells.${system}.default = pkgs.mkShell {
      packages = with pkgs; [
        bashInteractive
        gcc
      ];
    };

    packages.${system} = rec {
      k95aux = pkgs.callPackage ./package.nix {};
      default = k95aux;
    };

    nixosModules = rec {
      k95aux = import ./module.nix inputs;
      default = k95aux;
    };
  };
}
