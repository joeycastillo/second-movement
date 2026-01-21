{
  description = "second-movement";
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    systems.url = "github:nix-systems/default";
    flake-utils = {
      url = "github:numtide/flake-utils";
      inputs.systems.follows = "systems";
    };
    firmware = {
      url = "git+file:./.?submodules=true";
      flake = false;
    };
  };

  outputs =
    {
      nixpkgs,
      flake-utils,
      firmware,
      ...
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        packages = with pkgs; [
          bashInteractive
          gnumake
          emscripten
          gcc-arm-embedded
          python3
        ];
      in
      {
        packages =
          with pkgs;
          let
            mkFW =
              board: display:
              stdenv.mkDerivation {
                name = "${board}_${display}.uf2";

                src = firmware;

                nativeBuildInputs = packages;

                buildPhase = ''
                  make BOARD=${board} DISPLAY=${display}
                '';

                installPhase = ''
                  mkdir -p $out
                  cp build/firmware.uf2 $out/${board}_${display}.uf2
                '';
              };
          in
          {
            sensorwatch_pro_custom = mkFW "sensorwatch_pro" "custom";
            sensorwatch_pro_classic = mkFW "sensorwatch_pro" "classic";
            sensorwatch_green_classic = mkFW "sensorwatch_green" "classic";
            sensorwatch_red_classic = mkFW "sensorwatch_red" "classic";
            sensorwatch_blue_classic = mkFW "sensorwatch_blue" "classic";
          };
        devShells.default =
          with pkgs;
          mkShell {
            inherit packages;
            shellHook = ''
              export EM_CACHE=$(pwd)/.emscripten_cache/
            '';
          };
      }
    );
}
