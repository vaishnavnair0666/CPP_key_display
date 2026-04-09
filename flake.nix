{
  description = "key-display dev environment";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs";

  outputs =
    { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        packages = [
          pkgs.gcc
          pkgs.cmake
          pkgs.pkg-config

          pkgs.libinput
          pkgs.libxkbcommon
          pkgs.wayland
          pkgs.wayland-protocols
        ];

        shellHook = ''
          export Nix_SHELL=key-display
          echo "Key-display dev shell (GC-rootable)"
        '';
      };
    };
}
