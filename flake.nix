{
  description = "Run real programs in the discrete time simulator ns3";

  inputs = {
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachSystem ["x86_64-linux"] (system: let
      pkgs = import nixpkgs { inherit system; };

    in {

    packages.dce = pkgs.callPackage ./default.nix {};

    defaultPackage = self.packages."${system}".dce;

  });
}
