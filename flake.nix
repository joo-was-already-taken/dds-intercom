{
  inputs = {
    nixpkgs.url = "nixpkgs/nixos-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
  };

  outputs = { nixpkgs, flake-parts, ... }@inputs:
    flake-parts.lib.mkFlake { inherit inputs; } ({ ... }: {
      systems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];

      perSystem = { pkgs, ... }: let
        pkgName = "dds-intercom";

        stdenv = pkgs.gcc16Stdenv;
        buildInputs = with pkgs; [
          cli11
          miniaudio
          fastdds
          fastcdr
          foonathan-memory
          libpulseaudio
          alsa-lib
          tinyxml-2
          openssl
          libjack2
          sndio
        ];
        nativeBuildInputs = with pkgs; [
          cmake
          pkg-config
          ninja
          fastddsgen
        ];
      in rec {
        packages.default = packages.${pkgName};
        packages.${pkgName} = stdenv.mkDerivation {
          name = pkgName;
          version = "0.1.0";
          src = ./.;

          inherit buildInputs nativeBuildInputs;

          cmakeGenerator = "Ninja";
          cmakeBuildType = "RelWithDebInfo";
          dontStrip = true;
        };

        devShells.default = pkgs.mkShell {
          inherit buildInputs nativeBuildInputs;

          packages = with pkgs; [
            clang-tools
            gdb
            gcc16
          ];
        };
      };
    });
}
