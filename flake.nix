{
  description = "term-se flake";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    utils.url = "github:numtide/flake-utils";
  };

  outputs = {
    self,
    nixpkgs,
    utils,
  }:
    utils.lib.eachDefaultSystem
    (system: let
      pkgs = import nixpkgs {
        inherit system;
      };
    in {
      devShells.default = pkgs.mkShell {
        packages = with pkgs; [
          boost
          cmake
          clang-tools
          libcxx
          cmake-language-server
          clang
          clangStdenv
        ];
      };

      # apps = rec {
      #   dev = utils.lib.mkApp {
      #     drv = pkgs.writeShellScriptBin "composer-dev" ''
      #         ${pkgs.phpPackages.composer}/bin/composer run dev
      #     '';
      #   };

      #   default = dev;
      # };
    });
}
