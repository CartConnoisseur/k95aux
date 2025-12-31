{ stdenv }:

stdenv.mkDerivation {
  pname = "k95aux";
  version = "1.0.0";

  src = ./main.c;
  dontUnpack = true;

  buildPhase = ''
    gcc $src -o k95aux
  '';

  installPhase = ''
    mkdir -p $out/bin
    install -Dm755 k95aux $out/bin/k95aux
  '';
}
