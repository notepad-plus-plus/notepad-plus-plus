// language_defs.mm — Language table and extension-based detection
// Part of the Notepad++ macOS port modular refactor.

#import <Foundation/Foundation.h>
#include "language_defs.h"
#include "npp_constants.h"
#include "string_utils.h"

const LangDef g_languages[] = {
	{"Normal Text", "null", "", "", IDM_LANG_BASE + 0},
	{"C", "cpp",
	 "auto break case char const continue default do double else enum extern float for goto "
	 "if int long register return short signed sizeof static struct switch typedef union "
	 "unsigned void volatile while",
	 "int long double float char void short unsigned signed size_t ptrdiff_t bool "
	 "int8_t int16_t int32_t int64_t uint8_t uint16_t uint32_t uint64_t FILE",
	 IDM_LANG_BASE + 1},
	{"C++", "cpp",
	 "alignas alignof and and_eq asm auto bitand bitor bool break case catch char char8_t "
	 "char16_t char32_t class compl concept const consteval constexpr constinit const_cast "
	 "continue co_await co_return co_yield decltype default delete do double dynamic_cast "
	 "else enum explicit export extern false float for friend goto if import inline int long "
	 "module mutable namespace new noexcept not not_eq nullptr operator or or_eq private "
	 "protected public register reinterpret_cast requires return short signed sizeof static "
	 "static_assert static_cast struct switch template this thread_local throw true try "
	 "typedef typeid typename union unsigned using virtual void volatile wchar_t while xor xor_eq "
	 "override final include define ifdef ifndef endif pragma",
	 "string wstring vector map set unordered_map unordered_set list deque array "
	 "unique_ptr shared_ptr weak_ptr optional variant any tuple pair size_t "
	 "int8_t int16_t int32_t int64_t uint8_t uint16_t uint32_t uint64_t ptrdiff_t",
	 IDM_LANG_BASE + 2},
	{"Java", "cpp",
	 "abstract assert boolean break byte case catch char class const continue default do double "
	 "else enum extends final finally float for goto if implements import instanceof int "
	 "interface long native new package private protected public return short static strictfp "
	 "super switch synchronized this throw throws transient try void volatile while",
	 "String Integer Long Double Float Boolean Byte Short Character Object Class "
	 "List ArrayList Map HashMap Set HashSet Array Collections Arrays",
	 IDM_LANG_BASE + 3},
	{"Python", "python",
	 "False None True and as assert async await break class continue def del elif else except "
	 "finally for from global if import in is lambda nonlocal not or pass raise return try "
	 "while with yield",
	 "print len range list dict str int float bool type input open isinstance issubclass "
	 "hasattr getattr setattr delattr callable super object enumerate zip map filter sorted "
	 "reversed min max sum abs any all dir vars repr format id hash iter next",
	 IDM_LANG_BASE + 4},
	{"JavaScript", "cpp",
	 "abstract arguments async await boolean break byte case catch char class const continue "
	 "debugger default delete do double else enum eval export extends false final finally float "
	 "for from function goto if implements import in instanceof int interface let long native "
	 "new null of package private protected public return short static super switch synchronized "
	 "this throw throws transient true try typeof undefined var void volatile while with yield",
	 "console document window Math Array Object Promise Map Set WeakMap WeakSet JSON "
	 "Date RegExp Error TypeError RangeError Symbol Proxy Reflect Number String Boolean",
	 IDM_LANG_BASE + 5},
	{"HTML", "hypertext",
	 "a abbr address area article aside audio b base bdi bdo blockquote body br button canvas "
	 "caption cite code col colgroup data datalist dd del details dfn dialog div dl dt em embed "
	 "fieldset figcaption figure footer form h1 h2 h3 h4 h5 h6 head header hr html i iframe "
	 "img input ins kbd label legend li link main map mark meta meter nav noscript object ol "
	 "optgroup option output p param picture pre progress q rp rt ruby s samp script section "
	 "select small source span strong style sub summary sup table tbody td template textarea "
	 "tfoot th thead time title tr track u ul var video wbr",
	 "",
	 IDM_LANG_BASE + 6},
	{"CSS", "css",
	 "color background font margin padding border width height display position top left right "
	 "bottom float clear overflow visibility z-index text-align text-decoration line-height "
	 "font-size font-weight font-family content cursor opacity flex grid transition transform "
	 "animation box-shadow border-radius min-width max-width min-height max-height",
	 "",
	 IDM_LANG_BASE + 7},
	{"XML", "xml",
	 "",
	 "",
	 IDM_LANG_BASE + 8},
	{"JSON", "json",
	 "true false null",
	 "",
	 IDM_LANG_BASE + 9},
	{"Markdown", "markdown",
	 "",
	 "",
	 IDM_LANG_BASE + 10},
	{"SQL", "sql",
	 "select from where insert into values update set delete create table alter drop index "
	 "join inner outer left right on and or not null is in between like as order by group "
	 "having count sum avg min max distinct union all exists case when then else end primary "
	 "key foreign references constraint default check unique grant revoke",
	 "integer varchar text boolean date timestamp numeric decimal char real serial "
	 "bigint smallint float double blob clob nvarchar nchar",
	 IDM_LANG_BASE + 11},
	{"Shell", "bash",
	 "if then else elif fi case esac for while until do done in function select time coproc "
	 "echo printf read declare local export readonly typeset shift exit return break continue "
	 "eval exec source test true false",
	 "",
	 IDM_LANG_BASE + 12},
	{"Rust", "rust",
	 "as async await break const continue crate dyn else enum extern false fn for if impl "
	 "in let loop match mod move mut pub ref return self Self static struct super trait true "
	 "type unsafe use where while",
	 "i8 i16 i32 i64 i128 isize u8 u16 u32 u64 u128 usize f32 f64 bool char "
	 "String Vec Box Rc Arc Option Result Some None Ok Err HashMap HashSet",
	 IDM_LANG_BASE + 13},
	{"Go", "cpp",
	 "break case chan const continue default defer else fallthrough for func go goto if import "
	 "interface map package range return select struct switch type var true false nil",
	 "int int8 int16 int32 int64 uint uint8 uint16 uint32 uint64 uintptr "
	 "float32 float64 complex64 complex128 byte rune string bool error",
	 IDM_LANG_BASE + 14},
	{"Objective-C", "objc",
	 "auto break case char const continue default do double else enum extern float for goto "
	 "if int long register return short signed sizeof static struct switch typedef union "
	 "unsigned void volatile while id self super nil Nil YES NO "
	 "@interface @implementation @end @protocol @class @selector @property @synthesize "
	 "@dynamic @try @catch @finally @throw @autoreleasepool @encode @synchronized "
	 "instancetype nullable nonnull",
	 "NSObject NSString NSArray NSDictionary NSMutableArray NSMutableDictionary NSNumber "
	 "NSData NSDate NSURL NSError NSSet NSMutableSet NSInteger NSUInteger CGFloat BOOL",
	 IDM_LANG_BASE + 15},
	{"Swift", "cpp",
	 "actor any associatedtype async await break case catch class continue convenience default "
	 "defer deinit do else enum extension fallthrough false fileprivate final for func get "
	 "guard if import in indirect infix init inout internal is lazy let mutating nil nonisolated "
	 "nonmutating open operator optional override postfix precedencegroup prefix private protocol "
	 "public repeat required rethrows return self Self set some static struct subscript super "
	 "switch Task throw throws true try typealias unowned var weak where while",
	 "Int Double Float String Bool Character Array Dictionary Set Optional Result "
	 "Any AnyObject Void Never Error Codable Equatable Hashable Comparable",
	 IDM_LANG_BASE + LANG_SWIFT},
	{"TypeScript", "cpp",
	 "abstract any as async await boolean break case catch class const constructor continue "
	 "debugger declare default delete do else enum export extends false finally for from function "
	 "get goto if implements import in infer instanceof interface is keyof let module namespace "
	 "never new null of package private protected public readonly require return set static "
	 "string super switch symbol this throw true try type typeof undefined unique unknown var "
	 "void while with yield",
	 "Array Boolean Date Error Function JSON Map Math Number Object Promise Proxy "
	 "Reflect RegExp Set String Symbol WeakMap WeakSet console document window",
	 IDM_LANG_BASE + LANG_TYPESCRIPT},
	{"PHP", "phpscript",
	 "abstract and array as break callable case catch class clone const continue declare default "
	 "die do echo else elseif empty enddeclare endfor endforeach endif endswitch endwhile enum "
	 "eval exit extends final finally fn for foreach function global goto if implements include "
	 "include_once instanceof insteadof interface isset list match namespace new or print private "
	 "protected public readonly require require_once return static switch throw trait try unset "
	 "use var while xor yield",
	 "int float string bool array object null void mixed never true false self parent",
	 IDM_LANG_BASE + LANG_PHP},
	{"Ruby", "ruby",
	 "alias and begin break case class def defined? do else elsif end ensure false for if in "
	 "module next nil not or redo rescue retry return self super then true undef unless until "
	 "when while yield __FILE__ __LINE__ __ENCODING__",
	 "puts print require require_relative include extend attr_reader attr_writer attr_accessor "
	 "raise public private protected",
	 IDM_LANG_BASE + LANG_RUBY},
	{"Perl", "perl",
	 "chomp chop chr crypt hex index lc lcfirst length oct ord pack reverse rindex sprintf "
	 "substr uc ucfirst pos quotemeta split study abs atan2 cos exp int log oct rand sin "
	 "sqrt srand my our local if elsif else unless while until for foreach do sub return last "
	 "next redo goto use no require package BEGIN END",
	 "STDIN STDOUT STDERR ARGV ENV SIG",
	 IDM_LANG_BASE + LANG_PERL},
	{"Lua", "lua",
	 "and break do else elseif end false for function goto if in local nil not or repeat return "
	 "then true until while",
	 "assert collectgarbage dofile error getmetatable ipairs load loadfile next pairs pcall "
	 "print rawequal rawget rawlen rawset require select setmetatable tonumber tostring type xpcall "
	 "string table math io os coroutine debug",
	 IDM_LANG_BASE + LANG_LUA},
	{"YAML", "yaml",
	 "true false null yes no on off",
	 "",
	 IDM_LANG_BASE + LANG_YAML},
	{"TOML", "toml",
	 "true false",
	 "",
	 IDM_LANG_BASE + LANG_TOML},
	{"INI/Properties", "props",
	 "",
	 "",
	 IDM_LANG_BASE + LANG_INI},
	{"Makefile", "makefile",
	 "define endef undefine ifdef ifndef ifeq ifneq else endif include sinclude override export "
	 "unexport private vpath",
	 "",
	 IDM_LANG_BASE + LANG_MAKEFILE},
	{"Diff", "diff",
	 "",
	 "",
	 IDM_LANG_BASE + LANG_DIFF},
	{"Dockerfile", "bash",
	 "FROM AS RUN CMD LABEL MAINTAINER EXPOSE ENV ADD COPY ENTRYPOINT VOLUME USER WORKDIR ARG "
	 "ONBUILD STOPSIGNAL HEALTHCHECK SHELL",
	 "",
	 IDM_LANG_BASE + LANG_DOCKERFILE},
	{"CMake", "cmake",
	 "add_compile_definitions add_compile_options add_custom_command add_custom_target "
	 "add_definitions add_dependencies add_executable add_library add_subdirectory add_test "
	 "cmake_minimum_required configure_file enable_testing execute_process file find_package "
	 "find_path find_library foreach function if include include_directories install list "
	 "macro message option project return set set_property set_target_properties string target_link_libraries",
	 "AND OR NOT STREQUAL MATCHES GREATER LESS EQUAL DEFINED COMMAND EXISTS IS_DIRECTORY",
	 IDM_LANG_BASE + LANG_CMAKE},
	{"PowerShell", "powershell",
	 "begin break catch class continue data define do dynamicparam else elseif end enum exit "
	 "filter finally for foreach from function if in inlinescript parallel param process return "
	 "switch throw trap try until using var while workflow",
	 "Write-Host Write-Output Write-Error Write-Warning Get-Content Set-Content "
	 "Get-ChildItem Get-Item Set-Item New-Item Remove-Item Invoke-Command",
	 IDM_LANG_BASE + LANG_POWERSHELL},
	{"R", "r",
	 "break else for function if in next repeat return while TRUE FALSE NULL NA NA_integer_ "
	 "NA_real_ NA_complex_ NA_character_ Inf NaN",
	 "library require source cat print paste paste0 sprintf format nchar substr "
	 "grep grepl sub gsub strsplit as.numeric as.character as.integer as.logical "
	 "c list vector matrix data.frame factor length names which is.na",
	 IDM_LANG_BASE + LANG_R},
	{"Kotlin", "cpp",
	 "abstract annotation as break by catch class companion const constructor continue "
	 "crossinline data delegate do dynamic else enum external false final finally for fun get "
	 "if import in infix init inline inner interface internal is lateinit noinline null object "
	 "open operator out override package private protected public reified return sealed set "
	 "super suspend this throw true try typealias val var vararg when where while",
	 "Int Long Short Byte Double Float Boolean Char String Array List Map Set Any Unit Nothing "
	 "Pair Triple MutableList MutableMap MutableSet Sequence",
	 IDM_LANG_BASE + LANG_KOTLIN},
	{"Scala", "cpp",
	 "abstract case catch class def do else enum export extends false final finally for forSome "
	 "given if implicit import lazy match new null object override package private protected "
	 "return sealed super then this throw trait true try type val var while with yield",
	 "Int Long Short Byte Double Float Boolean Char String Array List Map Set Any Unit Nothing "
	 "Option Some None Either Left Right Future Promise Vector Seq",
	 IDM_LANG_BASE + LANG_SCALA},
	{"LaTeX", "latex",
	 "documentclass usepackage begin end newcommand renewcommand newenvironment "
	 "section subsection subsubsection paragraph label ref cite bibliography "
	 "include input maketitle tableofcontents textbf textit emph underline",
	 "article report book letter beamer memoir standalone",
	 IDM_LANG_BASE + LANG_LATEX},
};
const int g_numLanguages = sizeof(g_languages) / sizeof(g_languages[0]);

int guessLanguage(const std::wstring& filePath)
{
	NSString* path = WideToNSString(filePath.c_str());
	NSString* filename = [path lastPathComponent];
	NSString* ext = [path.pathExtension lowercaseString];

	// Filename-based detection (for extensionless files) — check first
	if ([filename isEqualToString:@"Makefile"] || [filename isEqualToString:@"makefile"] ||
	    [filename isEqualToString:@"GNUmakefile"])
		return LANG_MAKEFILE;
	if ([filename isEqualToString:@"Dockerfile"] || [filename hasPrefix:@"Dockerfile."])
		return LANG_DOCKERFILE;
	if ([filename isEqualToString:@"CMakeLists.txt"])
		return LANG_CMAKE;

	// Extension-based detection
	if ([ext isEqualToString:@"c"] || [ext isEqualToString:@"h"])
		return LANG_C;
	if ([ext isEqualToString:@"cpp"] || [ext isEqualToString:@"cc"] ||
	    [ext isEqualToString:@"cxx"] || [ext isEqualToString:@"hpp"] ||
	    [ext isEqualToString:@"hh"] || [ext isEqualToString:@"hxx"] ||
	    [ext isEqualToString:@"mm"])
		return LANG_CPP;
	if ([ext isEqualToString:@"java"])
		return LANG_JAVA;
	if ([ext isEqualToString:@"py"] || [ext isEqualToString:@"pyw"])
		return LANG_PYTHON;
	if ([ext isEqualToString:@"js"] || [ext isEqualToString:@"jsx"])
		return LANG_JAVASCRIPT;
	if ([ext isEqualToString:@"ts"] || [ext isEqualToString:@"tsx"])
		return LANG_TYPESCRIPT;
	if ([ext isEqualToString:@"html"] || [ext isEqualToString:@"htm"])
		return LANG_HTML;
	if ([ext isEqualToString:@"css"] || [ext isEqualToString:@"scss"] ||
	    [ext isEqualToString:@"less"])
		return LANG_CSS;
	if ([ext isEqualToString:@"xml"] || [ext isEqualToString:@"xsl"] ||
	    [ext isEqualToString:@"xslt"] || [ext isEqualToString:@"plist"])
		return LANG_XML;
	if ([ext isEqualToString:@"json"])
		return LANG_JSON;
	if ([ext isEqualToString:@"md"] || [ext isEqualToString:@"markdown"])
		return LANG_MARKDOWN;
	if ([ext isEqualToString:@"sql"])
		return LANG_SQL;
	if ([ext isEqualToString:@"sh"] || [ext isEqualToString:@"bash"] ||
	    [ext isEqualToString:@"zsh"])
		return LANG_SHELL;
	if ([ext isEqualToString:@"rs"])
		return LANG_RUST;
	if ([ext isEqualToString:@"go"])
		return LANG_GO;
	if ([ext isEqualToString:@"m"])
		return LANG_OBJC;
	if ([ext isEqualToString:@"swift"])
		return LANG_SWIFT;
	if ([ext isEqualToString:@"php"] || [ext isEqualToString:@"phtml"])
		return LANG_PHP;
	if ([ext isEqualToString:@"rb"] || [ext isEqualToString:@"rake"] ||
	    [ext isEqualToString:@"gemspec"])
		return LANG_RUBY;
	if ([ext isEqualToString:@"pl"] || [ext isEqualToString:@"pm"] ||
	    [ext isEqualToString:@"t"])
		return LANG_PERL;
	if ([ext isEqualToString:@"lua"])
		return LANG_LUA;
	if ([ext isEqualToString:@"yml"] || [ext isEqualToString:@"yaml"])
		return LANG_YAML;
	if ([ext isEqualToString:@"toml"])
		return LANG_TOML;
	if ([ext isEqualToString:@"ini"] || [ext isEqualToString:@"cfg"] ||
	    [ext isEqualToString:@"properties"] || [ext isEqualToString:@"conf"])
		return LANG_INI;
	if ([ext isEqualToString:@"mk"] || [ext isEqualToString:@"mak"])
		return LANG_MAKEFILE;
	if ([ext isEqualToString:@"diff"] || [ext isEqualToString:@"patch"])
		return LANG_DIFF;
	if ([ext isEqualToString:@"cmake"])
		return LANG_CMAKE;
	if ([ext isEqualToString:@"ps1"] || [ext isEqualToString:@"psm1"] ||
	    [ext isEqualToString:@"psd1"])
		return LANG_POWERSHELL;
	if ([ext isEqualToString:@"r"] || [ext isEqualToString:@"rmd"])
		return LANG_R;
	if ([ext isEqualToString:@"kt"] || [ext isEqualToString:@"kts"])
		return LANG_KOTLIN;
	if ([ext isEqualToString:@"scala"] || [ext isEqualToString:@"sc"])
		return LANG_SCALA;
	if ([ext isEqualToString:@"tex"] || [ext isEqualToString:@"latex"] ||
	    [ext isEqualToString:@"sty"] || [ext isEqualToString:@"cls"])
		return LANG_LATEX;

	return LANG_NORMAL_TEXT;
}

bool isCStyleLanguage(int languageIndex)
{
	switch (languageIndex)
	{
		case LANG_C:
		case LANG_CPP:
		case LANG_JAVA:
		case LANG_JAVASCRIPT:
		case LANG_CSS:
		case LANG_RUST:
		case LANG_GO:
		case LANG_OBJC:
		case LANG_SWIFT:
		case LANG_TYPESCRIPT:
		case LANG_PHP:
		case LANG_KOTLIN:
		case LANG_SCALA:
			return true;
		default:
			return false;
	}
}
