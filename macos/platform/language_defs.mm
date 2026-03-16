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
	 IDM_LANG_BASE + 16},
};
const int g_numLanguages = sizeof(g_languages) / sizeof(g_languages[0]);

int guessLanguage(const std::wstring& filePath)
{
	NSString* path = WideToNSString(filePath.c_str());
	NSString* ext = [path.pathExtension lowercaseString];

	if ([ext isEqualToString:@"c"] || [ext isEqualToString:@"h"])
		return 1;
	if ([ext isEqualToString:@"cpp"] || [ext isEqualToString:@"cc"] ||
	    [ext isEqualToString:@"cxx"] || [ext isEqualToString:@"hpp"] ||
	    [ext isEqualToString:@"hh"] || [ext isEqualToString:@"hxx"] ||
	    [ext isEqualToString:@"mm"])
		return 2;
	if ([ext isEqualToString:@"java"])
		return 3;
	if ([ext isEqualToString:@"py"] || [ext isEqualToString:@"pyw"])
		return 4;
	if ([ext isEqualToString:@"js"] || [ext isEqualToString:@"jsx"] ||
	    [ext isEqualToString:@"ts"] || [ext isEqualToString:@"tsx"])
		return 5;
	if ([ext isEqualToString:@"html"] || [ext isEqualToString:@"htm"])
		return 6;
	if ([ext isEqualToString:@"css"] || [ext isEqualToString:@"scss"] ||
	    [ext isEqualToString:@"less"])
		return 7;
	if ([ext isEqualToString:@"xml"] || [ext isEqualToString:@"xsl"] ||
	    [ext isEqualToString:@"xslt"] || [ext isEqualToString:@"plist"])
		return 8;
	if ([ext isEqualToString:@"json"])
		return 9;
	if ([ext isEqualToString:@"md"] || [ext isEqualToString:@"markdown"])
		return 10;
	if ([ext isEqualToString:@"sql"])
		return 11;
	if ([ext isEqualToString:@"sh"] || [ext isEqualToString:@"bash"] ||
	    [ext isEqualToString:@"zsh"])
		return 12;
	if ([ext isEqualToString:@"rs"])
		return 13;
	if ([ext isEqualToString:@"go"])
		return 14;
	if ([ext isEqualToString:@"m"])
		return 15;
	if ([ext isEqualToString:@"swift"])
		return 16;

	return 0;
}
