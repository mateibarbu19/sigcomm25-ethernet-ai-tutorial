{
  description = "Simple C/C++ dev environment";

  inputs = {
    nixpkgs.url = "nixpkgs/25.05";
  };

  outputs =
    { self, nixpkgs }:
    let
      pkgs = nixpkgs.legacyPackages.x86_64-linux;
      # Import the pip2nix-generated python-packages.nix
      # packageOverrides = pkgs.callPackage ./python-packages.nix { };
      # Create the Python interpreter with the overrides
      python = pkgs.python313.override { };
      pythonWithPackages = python.withPackages (
        ps: with ps; [
          matplotlib
          pandas
          seaborn
        ]
      );

    in
    {
      packages.x86_64-linux.default = pkgs.hello;

      devShells.x86_64-linux.default = pkgs.mkShell {
        buildInputs = with pkgs; [
          llvmPackages_latest.clang-tools # Order
          gnumake
          cmake
          gdb

          pythonWithPackages

          ruff
          gnuplot
        ];

        # env.LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath [
        #   pkgs.stdenv.cc.cc.lib
        #   pkgs.libz
        # ];
      };
    };
}
