#define app_copyright "Copyright 1999, app corporation"

; comment

[Setup]
AppName=MyApp
AppCopyright={#app_copyright}
WizardSmallImageFile=WizardSmallImageFile.bmp
OnlyBelowVersion=6.01

[Files]
Source: "app.exe"; DestDir: "{tmp}"; OnlyBelowVersion: 6.01

[INI]
Key: "keyname1"; String: "unterminated
Key: 'keyname2'; String: 'unterminated

[Registry]
Root: HKLM; ValueType: string

[CustomMessages]
keyname     =Other tasks:'not string

[Messages]
keyname="{#app_copyright}"not string

[Code]

// comment

(* comment *)

{ comment }

{ *) comment }

(* } comment *)

(*
comment *)

{
comment }

function ShouldInstallComCtlUpdate: Boolean;
begin
  Result := False;
  Log('string');
  IsEscaped('''good''', ''bad');
end;
