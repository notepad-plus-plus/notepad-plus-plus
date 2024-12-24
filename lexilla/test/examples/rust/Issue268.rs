// coding: utf-8

b"foo"; br"foo"            // foo
b"\"foo\""; br#""foo""#;   // "foo"

b"foo #\"# bar";
br##"foo #"# bar"##;       // foo #"# bar

b"\x52"; b"R"; br"R"       // R
b"\\x52"; br"\x52"         // \x52

c"æ"                       // LATIN SMALL LETTER AE (U+00E6)
c"\u{00E6}";
c"\xC3\xA6";

c"foo"; cr"foo"           // foo
c"\"foo\""; cr#""foo""#;  // "foo"

c"foo #\"# bar";
cr##"foo #"# bar"##;      // foo #"# bar

c"\x52"; c"R"; cr"R"      // R
c"\\x52"; cr"\x52"        // \x52

"foo"; r"foo"             // foo
"\"foo\""; r#""foo""#;    // "foo"

"foo #\"# bar";
r##"foo #"# bar"##;       // foo #"# bar

"\x52"; "R"; r"R"         // R
"\\x52"; r"\x52"          // \x52

"æ"                       // LATIN SMALL LETTER AE (U+00E6)
"\u{00E6}";
"\xC3\xA6";