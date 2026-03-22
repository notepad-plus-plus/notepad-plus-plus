// lexer_styles.mm — Per-lexer style tables, language switching
// Part of the Notepad++ macOS port modular refactor.

#include "lexer_styles.h"
#include "npp_constants.h"
#include "app_state.h"
#include "language_defs.h"
#include "scintilla_bridge.h"
#include "ILexer.h"
#include "Lexilla.h"
#include <cstring>

// Forward declaration
void applyAppearance();

// C/C++ lexer styles (SCE_C_*)
static const StyleDef s_cppStyles[] = {
	{1,  0x008000, 0x6A9955, false, false},
	{2,  0x008000, 0x6A9955, false, false},
	{3,  0x008000, 0x6A9955, false, true},
	{4,  0x008080, 0xB5CEA8, false, false},
	{5,  0xFF0000, 0x569CD6, true,  false},
	{6,  0x800080, 0xCE9178, false, false},
	{7,  0x800080, 0xCE9178, false, false},
	{9,  0x808080, 0xC586C0, false, false},
	{10, 0x000000, 0xD4D4D4, false, false},
	{16, 0x990000, 0x4EC9B0, false, false},
};

static const StyleDef s_objcStyles[] = {
	{1,  0x008000, 0x6A9955, false, false},
	{2,  0x008000, 0x6A9955, false, false},
	{3,  0x008000, 0x6A9955, false, true},
	{4,  0x008080, 0xB5CEA8, false, false},
	{5,  0xFF0000, 0x569CD6, true,  false},
	{6,  0x800080, 0xCE9178, false, false},
	{7,  0x800080, 0xCE9178, false, false},
	{9,  0x808080, 0xC586C0, false, false},
	{10, 0x000000, 0xD4D4D4, false, false},
	{16, 0x990000, 0x4EC9B0, false, false},
};

static const StyleDef s_pythonStyles[] = {
	{1,  0x008000, 0x6A9955, false, false},
	{2,  0x008080, 0xB5CEA8, false, false},
	{3,  0x800080, 0xCE9178, false, false},
	{4,  0x800080, 0xCE9178, false, false},
	{5,  0xFF0000, 0x569CD6, true,  false},
	{6,  0x800080, 0xCE9178, false, false},
	{7,  0x800080, 0xCE9178, false, false},
	{8,  0x990000, 0x4EC9B0, false, false},
	{9,  0x880000, 0xDCDCAA, false, false},
	{10, 0x000000, 0xD4D4D4, false, false},
	{14, 0x990000, 0x4EC9B0, false, false},
	{15, 0xCC6600, 0xDCDCAA, false, true},
};

static const StyleDef s_htmlStyles[] = {
	{1,  0x800000, 0x569CD6, true,  false},
	{2,  0x800000, 0x569CD6, false, false},
	{3,  0xFF0000, 0x9CDCFE, false, false},
	{5,  0x008080, 0xB5CEA8, false, false},
	{6,  0x800080, 0xCE9178, false, false},
	{7,  0x800080, 0xCE9178, false, false},
	{9,  0x008000, 0x6A9955, false, false},
};

static const StyleDef s_cssStyles[] = {
	{1,  0x800000, 0x569CD6, true,  false},
	{2,  0x990000, 0xD7BA7D, false, false},
	{4,  0x990000, 0xD7BA7D, false, false},
	{5,  0x000000, 0xD4D4D4, false, false},
	{7,  0x008080, 0xCE9178, false, false},
	{9,  0x008000, 0x6A9955, false, false},
	{13, 0xFF0000, 0x569CD6, true,  false},
};

static const StyleDef s_jsonStyles[] = {
	{1,  0x008080, 0xB5CEA8, false, false},
	{2,  0x800080, 0xCE9178, false, false},
	{4,  0x990000, 0x9CDCFE, false, false},
	{8,  0xFF0000, 0x569CD6, true,  false},
	{9,  0x008080, 0xB5CEA8, false, false},
	{5,  0x000000, 0xD4D4D4, false, false},
};

static const StyleDef s_sqlStyles[] = {
	{1,  0x008000, 0x6A9955, false, false},
	{2,  0x008000, 0x6A9955, false, false},
	{4,  0x008080, 0xB5CEA8, false, false},
	{5,  0xFF0000, 0x569CD6, true,  false},
	{6,  0x800080, 0xCE9178, false, false},
	{7,  0x000000, 0xD4D4D4, false, false},
	{8,  0x990000, 0x4EC9B0, false, false},
};

static const StyleDef s_bashStyles[] = {
	{2,  0x008000, 0x6A9955, false, false},
	{3,  0x008080, 0xB5CEA8, false, false},
	{4,  0xFF0000, 0x569CD6, true,  false},
	{5,  0x800080, 0xCE9178, false, false},
	{6,  0x800080, 0xCE9178, false, false},
	{7,  0x000000, 0xD4D4D4, false, false},
	{8,  0x990000, 0x9CDCFE, false, false},
	{9,  0xCC6600, 0xDCDCAA, false, false},
};

static const StyleDef s_rustStyles[] = {
	{1,  0x008000, 0x6A9955, false, false},
	{2,  0x008000, 0x6A9955, false, false},
	{3,  0x008000, 0x6A9955, false, true},
	{4,  0x008000, 0x6A9955, false, true},
	{5,  0xFF0000, 0x569CD6, true,  false},
	{6,  0x800080, 0xCE9178, false, false},
	{8,  0x008080, 0xB5CEA8, false, false},
	{10, 0x000000, 0xD4D4D4, false, false},
	{12, 0x800080, 0xCE9178, false, false},
	{14, 0x990000, 0x4EC9B0, false, false},
	{16, 0xCC6600, 0xDCDCAA, false, false},
};

static const StyleDef s_markdownStyles[] = {
	{4,  0x000000, 0xD4D4D4, true,  false},
	{5,  0x000000, 0xD4D4D4, true,  false},
	{6,  0x000000, 0xD4D4D4, false, true},
	{7,  0x000000, 0xD4D4D4, false, true},
	{8,  0x800000, 0x569CD6, true,  false},
	{9,  0x800000, 0x569CD6, true,  false},
	{10, 0x800000, 0x569CD6, true,  false},
	{11, 0x800000, 0x569CD6, true,  false},
	{12, 0x800000, 0x569CD6, true,  false},
	{19, 0x808080, 0xCE9178, false, false},
	{20, 0x808080, 0xCE9178, false, false},
	{21, 0x0000FF, 0x569CD6, false, false},
};

static const StyleDef s_xmlStyles[] = {
	{1,  0x800000, 0x569CD6, true,  false},
	{3,  0xFF0000, 0x9CDCFE, false, false},
	{5,  0x008080, 0xB5CEA8, false, false},
	{6,  0x800080, 0xCE9178, false, false},
	{7,  0x800080, 0xCE9178, false, false},
	{9,  0x008000, 0x6A9955, false, false},
};

// Ruby lexer styles (SCE_RB_*)
static const StyleDef s_rubyStyles[] = {
	{2,  0x008000, 0x6A9955, false, false},   // comment
	{3,  0x008000, 0x6A9955, false, true},    // POD
	{4,  0x008080, 0xB5CEA8, false, false},   // number
	{5,  0xFF0000, 0x569CD6, true,  false},   // keyword
	{6,  0x800080, 0xCE9178, false, false},   // string double
	{7,  0x800080, 0xCE9178, false, false},   // string single/char
	{8,  0x990000, 0x4EC9B0, false, false},   // class name
	{9,  0x880000, 0xDCDCAA, false, false},   // def name
	{12, 0xCC6600, 0xDCDCAA, false, false},   // regex
	{14, 0x808080, 0xC586C0, false, false},   // symbol
};

// Perl lexer styles (SCE_PL_*)
static const StyleDef s_perlStyles[] = {
	{2,  0x008000, 0x6A9955, false, false},   // comment
	{3,  0x008000, 0x6A9955, false, true},    // POD
	{4,  0x008080, 0xB5CEA8, false, false},   // number
	{5,  0xFF0000, 0x569CD6, true,  false},   // keyword
	{6,  0x800080, 0xCE9178, false, false},   // string double
	{7,  0x800080, 0xCE9178, false, false},   // character
	{10, 0x000000, 0xD4D4D4, false, false},   // operator
	{12, 0x990000, 0x9CDCFE, false, false},   // scalar $
	{13, 0x990000, 0x9CDCFE, false, false},   // array @
	{14, 0x990000, 0x9CDCFE, false, false},   // hash %
	{17, 0xCC6600, 0xDCDCAA, false, false},   // regex
};

// Lua lexer styles (SCE_LUA_*)
static const StyleDef s_luaStyles[] = {
	{1,  0x008000, 0x6A9955, false, false},   // comment
	{2,  0x008000, 0x6A9955, false, false},   // comment line
	{3,  0x008000, 0x6A9955, false, true},    // comment doc
	{4,  0x008080, 0xB5CEA8, false, false},   // number
	{5,  0xFF0000, 0x569CD6, true,  false},   // keyword
	{6,  0x800080, 0xCE9178, false, false},   // string
	{7,  0x800080, 0xCE9178, false, false},   // character
	{8,  0x800080, 0xCE9178, false, false},   // literal string
	{9,  0x808080, 0xC586C0, false, false},   // preprocessor
	{13, 0x990000, 0x4EC9B0, false, false},   // keyword2 (builtins)
};

// YAML lexer styles (SCE_YAML_*)
static const StyleDef s_yamlStyles[] = {
	{1,  0x008000, 0x6A9955, false, false},   // comment
	{2,  0xFF0000, 0x569CD6, true,  false},   // identifier/key
	{3,  0xFF0000, 0x569CD6, true,  false},   // keyword
	{4,  0x008080, 0xB5CEA8, false, false},   // number
	{5,  0x990000, 0x9CDCFE, false, false},   // reference
	{6,  0x808080, 0xC586C0, false, false},   // document
	{7,  0x800080, 0xCE9178, false, false},   // text/value
};

// TOML lexer styles (SCE_TOML_*)
static const StyleDef s_tomlStyles[] = {
	{1,  0x008000, 0x6A9955, false, false},   // comment
	{3,  0xFF0000, 0x569CD6, true,  false},   // keyword (true/false)
	{4,  0x008080, 0xB5CEA8, false, false},   // number
	{5,  0x990000, 0x4EC9B0, true,  false},   // table header
	{6,  0xFF0000, 0x9CDCFE, false, false},   // key
	{9,  0x800080, 0xCE9178, false, false},   // string single
	{10, 0x800080, 0xCE9178, false, false},   // string double
	{11, 0x800080, 0xCE9178, false, false},   // triple string single
	{12, 0x800080, 0xCE9178, false, false},   // triple string double
	{14, 0xCC6600, 0xDCDCAA, false, false},   // datetime
};

// Properties/INI lexer styles (SCE_PROPS_*)
static const StyleDef s_propsStyles[] = {
	{1,  0x008000, 0x6A9955, false, false},   // comment
	{2,  0x990000, 0x4EC9B0, true,  false},   // section
	{3,  0x808080, 0xC586C0, false, false},   // assignment
	{5,  0xFF0000, 0x569CD6, false, false},   // key
};

// Makefile lexer styles (SCE_MAKE_*)
static const StyleDef s_makefileStyles[] = {
	{1,  0x008000, 0x6A9955, false, false},   // comment
	{2,  0x808080, 0xC586C0, false, false},   // preprocessor
	{3,  0xFF0000, 0x569CD6, false, false},   // identifier
	{5,  0x990000, 0x4EC9B0, true,  false},   // target
};

// Diff lexer styles (SCE_DIFF_*)
static const StyleDef s_diffStyles[] = {
	{1,  0x008000, 0x6A9955, false, false},   // comment
	{2,  0x808080, 0xC586C0, false, false},   // command
	{3,  0xFF0000, 0x569CD6, true,  false},   // header
	{4,  0x990000, 0x9CDCFE, false, false},   // position
	{5,  0x0000CC, 0xFF6060, false, false},   // deleted (red in BGR)
	{6,  0x00CC00, 0x60FF60, false, false},   // added (green in BGR)
	{7,  0xCC6600, 0xDCDCAA, false, false},   // changed
};

// CMake lexer styles (SCE_CMAKE_*)
static const StyleDef s_cmakeStyles[] = {
	{1,  0x008000, 0x6A9955, false, false},   // comment
	{2,  0x800080, 0xCE9178, false, false},   // string DQ
	{5,  0xFF0000, 0x569CD6, true,  false},   // commands/keywords
	{6,  0x990000, 0x4EC9B0, false, false},   // parameters
	{7,  0x808080, 0x9CDCFE, false, false},   // variable
	{14, 0x008080, 0xB5CEA8, false, false},   // number
};

// PowerShell lexer styles (SCE_POWERSHELL_*)
static const StyleDef s_powershellStyles[] = {
	{1,  0x008000, 0x6A9955, false, false},   // comment
	{2,  0x800080, 0xCE9178, false, false},   // string
	{3,  0x800080, 0xCE9178, false, false},   // character
	{4,  0x008080, 0xB5CEA8, false, false},   // number
	{5,  0x990000, 0x9CDCFE, false, false},   // variable
	{8,  0xFF0000, 0x569CD6, true,  false},   // keyword
	{9,  0x880000, 0xDCDCAA, false, false},   // cmdlet
	{13, 0x008000, 0x6A9955, false, false},   // comment stream
};

// R lexer styles (SCE_R_*)
static const StyleDef s_rStyles[] = {
	{1,  0x008000, 0x6A9955, false, false},   // comment
	{2,  0xFF0000, 0x569CD6, true,  false},   // keyword
	{3,  0x990000, 0x4EC9B0, false, false},   // base keyword
	{5,  0x008080, 0xB5CEA8, false, false},   // number
	{6,  0x800080, 0xCE9178, false, false},   // string
	{7,  0x800080, 0xCE9178, false, false},   // string2
	{8,  0x000000, 0xD4D4D4, false, false},   // operator
};

// LaTeX lexer styles (SCE_L_*)
static const StyleDef s_latexStyles[] = {
	{1,  0xFF0000, 0x569CD6, true,  false},   // command
	{2,  0x990000, 0x4EC9B0, false, false},   // tag
	{3,  0x808080, 0xC586C0, false, false},   // math
	{4,  0x008000, 0x6A9955, false, false},   // comment
	{5,  0x990000, 0x4EC9B0, false, false},   // tag2
	{6,  0x808080, 0xC586C0, false, false},   // math2
	{9,  0xCC6600, 0xDCDCAA, false, false},   // short command
};

static const LexerStyles s_allLexerStyles[] = {
	{"cpp",        s_cppStyles,      sizeof(s_cppStyles) / sizeof(s_cppStyles[0])},
	{"objc",       s_objcStyles,     sizeof(s_objcStyles) / sizeof(s_objcStyles[0])},
	{"python",     s_pythonStyles,   sizeof(s_pythonStyles) / sizeof(s_pythonStyles[0])},
	{"hypertext",  s_htmlStyles,     sizeof(s_htmlStyles) / sizeof(s_htmlStyles[0])},
	{"css",        s_cssStyles,      sizeof(s_cssStyles) / sizeof(s_cssStyles[0])},
	{"json",       s_jsonStyles,     sizeof(s_jsonStyles) / sizeof(s_jsonStyles[0])},
	{"sql",        s_sqlStyles,      sizeof(s_sqlStyles) / sizeof(s_sqlStyles[0])},
	{"bash",       s_bashStyles,     sizeof(s_bashStyles) / sizeof(s_bashStyles[0])},
	{"rust",       s_rustStyles,     sizeof(s_rustStyles) / sizeof(s_rustStyles[0])},
	{"markdown",   s_markdownStyles, sizeof(s_markdownStyles) / sizeof(s_markdownStyles[0])},
	{"xml",        s_xmlStyles,      sizeof(s_xmlStyles) / sizeof(s_xmlStyles[0])},
	{"ruby",       s_rubyStyles,     sizeof(s_rubyStyles) / sizeof(s_rubyStyles[0])},
	{"perl",       s_perlStyles,     sizeof(s_perlStyles) / sizeof(s_perlStyles[0])},
	{"lua",        s_luaStyles,      sizeof(s_luaStyles) / sizeof(s_luaStyles[0])},
	{"yaml",       s_yamlStyles,     sizeof(s_yamlStyles) / sizeof(s_yamlStyles[0])},
	{"toml",       s_tomlStyles,     sizeof(s_tomlStyles) / sizeof(s_tomlStyles[0])},
	{"props",      s_propsStyles,    sizeof(s_propsStyles) / sizeof(s_propsStyles[0])},
	{"makefile",   s_makefileStyles, sizeof(s_makefileStyles) / sizeof(s_makefileStyles[0])},
	{"diff",       s_diffStyles,     sizeof(s_diffStyles) / sizeof(s_diffStyles[0])},
	{"cmake",      s_cmakeStyles,    sizeof(s_cmakeStyles) / sizeof(s_cmakeStyles[0])},
	{"powershell", s_powershellStyles, sizeof(s_powershellStyles) / sizeof(s_powershellStyles[0])},
	{"r",          s_rStyles,        sizeof(s_rStyles) / sizeof(s_rStyles[0])},
	{"latex",      s_latexStyles,    sizeof(s_latexStyles) / sizeof(s_latexStyles[0])},
	{"phpscript",  s_cppStyles,      sizeof(s_cppStyles) / sizeof(s_cppStyles[0])},
};

const LexerStyles* findLexerStyles(const char* lexerName)
{
	if (!lexerName) return nullptr;
	for (const auto& ls : s_allLexerStyles)
	{
		if (strcmp(ls.lexerName, lexerName) == 0)
			return &ls;
	}
	return nullptr;
}

void applyLanguageToView(void* sci, int langIndex)
{
	if (!sci) return;
	if (langIndex < 0 || langIndex >= g_numLanguages) return;

	const auto& lang = g_languages[langIndex];

	ILexer5* lexer = CreateLexer(lang.lexerName);
	ScintillaBridge_sendMessage(sci, SCI_SETILEXER, 0, (intptr_t)lexer);

	ScintillaBridge_sendMessage(sci, SCI_SETKEYWORDS, 0, (intptr_t)lang.keywords);
	if (lang.keywords2 && lang.keywords2[0])
		ScintillaBridge_sendMessage(sci, SCI_SETKEYWORDS, 1, (intptr_t)lang.keywords2);

	ScintillaBridge_sendMessage(sci, SCI_SETPROPERTY, (uintptr_t)"fold", (intptr_t)"1");
	ScintillaBridge_sendMessage(sci, SCI_SETPROPERTY, (uintptr_t)"fold.compact", (intptr_t)"0");
	ScintillaBridge_sendMessage(sci, SCI_SETPROPERTY, (uintptr_t)"fold.comment", (intptr_t)"1");
	ScintillaBridge_sendMessage(sci, SCI_SETPROPERTY, (uintptr_t)"fold.preprocessor", (intptr_t)"1");

	ScintillaBridge_sendMessage(sci, SCI_STYLESETFONT, 32, (intptr_t)ctx().fontName.c_str());
	ScintillaBridge_sendMessage(sci, SCI_STYLESETSIZE, 32, ctx().fontSize);

	applyAppearance();
	ScintillaBridge_sendMessage(sci, SCI_COLOURISE, 0, -1);
}

void applyLanguage(int langIndex)
{
	applyLanguageToView(ctx().activeScintillaView(), langIndex);
}
