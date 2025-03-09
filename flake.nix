{
  description = "A modern terminal multiplexer";

  inputs = {
    nixpkgs = {
      url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    };

    di = {
      url = "github:coletrammer/di/mac-support";
      inputs.nixpkgs.follows = "nixpkgs";
      inputs.flake-parts.follows = "flake-parts";
    };

    dius = {
      url = "github:coletrammer/dius/mac-support";
      inputs.nixpkgs.follows = "nixpkgs";
      inputs.di.follows = "di";
      inputs.flake-parts.follows = "flake-parts";
    };

    flake-parts = {
      url = "github:hercules-ci/flake-parts";
      inputs.nixpkgs-lib.follows = "nixpkgs";
    };
  };

  outputs =
    inputs:
    inputs.flake-parts.lib.mkFlake
      {
        inherit inputs;
      }
      {
        systems = [
          "x86_64-linux"
          "aarch64-linux"
          "x86_64-darwin"
          "aarch64-darwin"
        ];
        imports = [ ./meta/nix ];
      };
}
