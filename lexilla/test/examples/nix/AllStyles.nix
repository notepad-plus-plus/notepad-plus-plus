# coding:utf-8

1 + 2

let
 x = 1;
 y = 2;
in x + y

let x=1;y=2;in x+y

{
  string = "hello";
  integer = 1;
  float = 3.141;
  bool = true;
  null = null;
  list = [ 1 "two" false ];
  attribute-set = {
    a = "hello";
    b = 2;
    c = 2.718;
    d = false;
  }; # comments are supported
}

rec {
  one = 1;
  two = one + 1;
  three = two + 1;
}

{ one = 1; three = 3; two = 2; }

let
  a = 1;
in
a + a

let
  b = a + 1;
  a = 1;
in
a + b

{
  a = let x = 1; in x;
  b = x;
}

let
  attrset = { x = 1; };
in
attrset.x

let
  attrset = { a = { b = { c = 1; }; }; };
in
attrset.a.b.c

{ a.b.c = 1; }

let
  a = {
    x = 1;
    y = 2;
    z = 3;
  };
in
with a; [ x y z ]

let
  x = 1;
  y = 2;
in
{
  inherit x y;
}

let
  a = { x = 1; y = 2; };
in
{
  inherit (a) x y;
}

let
  inherit ({ x = 1; y = 2; }) x y;
in [ x y ]

let
  name = "Nix${}";
in
"hello ${name}"

graphviz = (import ../tools/graphics/graphviz) {
  inherit fetchurl stdenv libpng libjpeg expat x11 yacc;
  inherit (xorg) libXaw;
};

let negate = x: !x;
    concat = x: y: x + y;
in if negate true then concat "foo" "bar" else ""

# A number
# Equals 1 + 1
# asd

/* /*
Block comments
can span multiple lines.
*/ "hello"
"hello"

/* /* nope */ 1

map (concat "foo") [ "bar" "bla" "abc" ]

{ localServer ? false
, httpServer ? false
, sslSupport ? false
, pythonBindings ? false
, javaSwigBindings ? false
, javahlBindings ? false
, stdenv, fetchurl
, openssl ? null, httpd ? null, db4 ? null, expat, swig ? null, j2sdk ? null
}:

assert localServer -> db4 != null;
assert httpServer -> httpd != null && httpd.expat == expat;
assert sslSupport -> openssl != null && (httpServer -> httpd.openssl == openssl);
assert pythonBindings -> swig != null && swig.pythonSupport;
assert javaSwigBindings -> swig != null && swig.javaSupport;
assert javahlBindings -> j2sdk != null;

stdenv.mkDerivation {
  name = "subversion-1.1.1";
  openssl = if sslSupport then openssl else null;
}

configureFlags = ''
  -system-zlib -system-libpng -system-libjpeg
  ${if openglSupport then ''-dlopen-opengl
    -L${mesa}/lib -I${mesa}/include
    -L${libXmu}/lib -I${libXmu}/include'' else ""}
  ${if threadSupport then "-thread" else "-no-thread"}
'';

let
  a = "no";
  a.b.c.d = "foo"
in
"${a + " ${a + " ${a}"}"}"

let
  out = "Nix";
in
"echo ${out} > $out"

<nixpkgs/lib>

''
multi
''${}
'''
line
''\n
string
''

''
  one
   two
    three
''

x: x + 1

x: y: x + y

{ a, b }: a + b

{ a, b ? 0 }: a + b

{ a, b, ...}: a + b

args@{ a, b, ... }: a + b + args.c

{ a, b, ... }@args: a + b + args.c

let
  f = x: x + 1;
in f

let
  f = x: x + 1;
in f 1

let
  f = x: x.a;
  v = { a = 1; };
in
f v

(x: x + 1) 1

let
 f = x: x + 1;
 a = 1;
in [ (f a) ]

let
 f = x: x + 1;
 a = 1;
in [ f a ]

let
  f = x: y: x + y;
in
f 1 2

{a, b}: a + b

let
  f = {a, b}: a + b;
in
f { a = 1; b = 2; }

let
  f = {a, b}: a + b;
in
f { a = 1; b = 2; c = 3; }

let
  f = {a, b ? 0}: a + b;
in
f { a = 1; }

let
  f = {a ? 0, b ? 0}: a + b;
in
f { } # empty attribute set

let
  f = {a, b, ...}: a + b;
in
f { a = 1; b = 2; c = 3; }

{a, b, ...}@args: a + b + args.c

args@{a, b, ...}: a + b + args.c

let
  f = {a, b, ...}@args: a + b + args.c;
in
f { a = 1; b = 2; c = 3; }

{ pkgs ? import <nixpkgs> {} }:
let
  message = "hello world";
in
pkgs.mkShellNoCC {
  packages = with pkgs; [ cowsay ];
  shellHook = ''
    cowsay ${message}
  '';
}

{ config, pkgs, ... }: {

  imports = [ ./hardware-configuration.nix ];

  environment.systemPackages = with pkgs; [ git ];

  # ...
}

{ lib, stdenv, fetchurl }:

stdenv.mkDerivation rec {

  pname = "hello";

  version = "2.12";

  src = fetchurl {
    url = "mirror://gnu/${pname}/${pname}-${version}.tar.gz";
    sha256 = "1ayhp9v4m4rdhjmnl2bq3cibrbqqkgjbl3s7yk2nhlh8vj3ay16g";
  };

  meta = with lib; {
    license = licenses.gpl3Plus;
  };

}

{
  baseName = baseNameOf name;

  pullImage =
    let
      fixName = name: builtins.replaceStrings [ "/" ":" ] [ "-" "-" ] name;
    in
    { imageName
      # To find the digest of an image, you can use skopeo:
      # see doc/functions.xml
    , imageDigest
    , sha256
    , os ? "linux"
    , # Image architecture, defaults to the architecture of the `hostPlatform` when unset
      arch ? defaultArchitecture
      # This is used to set name to the pulled image
    , finalImageName ? imageName
      # This used to set a tag to the pulled image
    , finalImageTag ? "latest"
      # This is used to disable TLS certificate verification, allowing access to http registries on (hopefully) trusted networks
    , tlsVerify ? true

    , name ? fixName "docker-image-${finalImageName}-${finalImageTag}.tar"
    }:

    runCommand name
      {
        inherit imageDigest;
        imageName = finalImageName;
        imageTag = finalImageTag;
        impureEnvVars = lib.fetchers.proxyImpureEnvVars;
        outputHashMode = "flat";
        outputHashAlgo = "sha256";
        outputHash = sha256;

        nativeBuildInputs = [ skopeo ];
        SSL_CERT_FILE = "${cacert.out}/etc/ssl/certs/ca-bundle.crt";

        sourceURL = "docker://${imageName}@${imageDigest}";
        destNameTag = "${finalImageName}:${finalImageTag}";
      } ''
      skopeo \
        --insecure-policy \
        --tmpdir=$TMPDIR \
        --override-os ${os} \
        --override-arch ${arch} \
        copy \
        --src-tls-verify=${lib.boolToString tlsVerify} \
        "$sourceURL" "docker-archive://$out:$destNameTag" \
        | cat  # pipe through cat to force-disable progress bar
    '';

}

message = "unterminated string;
message2 = "unterminated string;
