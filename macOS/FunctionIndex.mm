// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Created 2026-11-01. Uses the upstream Notepad++ Function List XML schema:
// commentExpr, classRange, open/closeSymbole, and chained name expressions.
// Original XML rule notices are preserved in the bundled functionList directory.
#import "FunctionIndex.h"
#import "Storage.h"
#define BOOST_REGEX_STANDALONE
#define BOOST_REGEX_MAX_STATE_COUNT 1000000
#include <boost/regex.hpp>
#include <algorithm>
#include <string>
#include <vector>

namespace {
// XML attribute normalization would turn the line breaks in (?x) expressions
// into spaces, causing a # comment to swallow the rest of the expression.
// Preserve literal attribute whitespace as character references before parsing.
static NSData *PreserveAttributeWhitespace(NSData *xml) {
    std::string input(static_cast<const char *>(xml.bytes), xml.length), output;
    output.reserve(input.size()); char quote = 0; bool tag = false;
    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        if (!quote && input.compare(i, 4, "<!--") == 0) {
            size_t end = input.find("-->", i + 4);
            if (end == std::string::npos) return xml;
            output.append(input, i, end + 3 - i); i = end + 2; continue;
        }
        if (quote) {
            if (c == quote) quote = 0;
            else if (c == '\n' || c == '\r' || c == '\t') {
                output += c == '\n' ? "&#10;" : c == '\r' ? "&#13;" : "&#9;"; continue;
            }
        } else if (c == '<') tag = true;
        else if (c == '>') tag = false;
        else if (tag && (c == '\'' || c == '"')) quote = c;
        output += c;
    }
    return [NSData dataWithBytes:output.data() length:output.size()];
}
struct Span { size_t start, end; };
struct Symbol { std::string name, scope; size_t position; bool isClass; };
static std::string Attr(NSXMLElement *node, NSString *key) {
    return [node attributeForName:key].stringValue.UTF8String ?: "";
}
static NSXMLElement *Child(NSXMLElement *node, NSString *name) {
    return [node elementsForName:name].firstObject;
}
class Indexer {
    const std::string text;
    std::vector<Span> comments;
    std::vector<Symbol> symbols;
    size_t matches = 0;
    bool inside(size_t position, const std::vector<Span> &zones) const {
        return std::any_of(zones.begin(), zones.end(), [=](Span s) { return position >= s.start && position < s.end; });
    }
    template<class Callback> void each(const std::string &expr, Span zone, Callback callback) {
        if (expr.empty() || zone.start >= zone.end) return;
        boost::regex regex(expr, boost::regex::perl | boost::regex::icase);
        boost::match_results<std::string::const_iterator> match;
        size_t cursor = zone.start;
        while (cursor < zone.end) {
            auto flags = boost::match_default;
            if (cursor > 0) flags |= boost::match_prev_avail;
            if (!boost::regex_search(text.begin() + cursor, text.begin() + zone.end, match, regex, flags)) break;
            Span result{cursor + static_cast<size_t>(match.position()), cursor + static_cast<size_t>(match.position() + match.length())};
            if (++matches > 100000) throw std::runtime_error("Function List match limit exceeded.");
            if (!callback(result)) break;
            cursor = std::max(cursor + 1, result.end);
        }
    }
    Span name(Span zone, NSXMLElement *container) {
        for (NSXMLNode *child in container.children) {
            if (child.kind != NSXMLElementKind) continue;
            std::string expression = Attr((NSXMLElement *)child, @"expr");
            if (expression.empty()) continue;
            bool found = false;
            each(expression, zone, [&](Span result) {
                if (result.start == result.end) return true;
                zone = result; found = true; return false;
            });
            if (!found) return {0, 0};
        }
        return zone;
    }
    size_t closing(size_t start, size_t end, const std::string &open, const std::string &close) {
        if (open.empty() || close.empty()) return end;
        boost::regex opening(open, boost::regex::perl | boost::regex::icase);
        size_t depth = 1, result = end;
        each("(?:" + open + ")|(?:" + close + ")", {start, end}, [&](Span s) {
            if (inside(s.start, comments)) return true;
            if (boost::regex_match(text.begin() + s.start, text.begin() + s.end, opening)) ++depth;
            else if (--depth == 0) { result = s.end; return false; }
            return true;
        });
        return result;
    }
    void functions(NSXMLElement *rule, Span zone, const std::string &scope, const std::vector<Span> &excluded) {
        if (!rule) return;
        each(Attr(rule, @"mainExpr"), zone, [&](Span match) {
            if (inside(match.start, comments)) return true;
            Span label = name(match, Child(rule, @"functionName"));
            if (label.start == label.end || inside(label.start, comments) || inside(label.start, excluded)) return true;
            std::string group = scope;
            NSXMLElement *className = Child(rule, @"className");
            if (group.empty() && className) {
                Span classLabel = name(match, className);
                if (classLabel.end > classLabel.start) group = text.substr(classLabel.start, classLabel.end - classLabel.start);
            }
            symbols.push_back({text.substr(label.start, label.end - label.start), group, label.start, false});
            return true;
        });
    }
    std::vector<Span> classes(NSXMLElement *rule, Span zone, const std::string &parent, unsigned depth) {
        std::vector<Span> found;
        if (!rule || depth > 32) return found;
        each(Attr(rule, @"mainExpr"), zone, [&](Span match) {
            if (inside(match.start, comments) || inside(match.start, found)) return true;
            Span label = name(match, Child(rule, @"className"));
            if (label.end <= label.start || inside(label.start, comments)) return true;
            std::string className = text.substr(label.start, label.end - label.start);
            std::string full = parent.empty() ? className : parent + "::" + className;
            const std::string open = Attr(rule, @"openSymbole"), close = Attr(rule, @"closeSymbole");
            size_t end = open.empty() ? match.end : closing(match.end, zone.end, open, close);
            found.push_back({match.start, end});
            symbols.push_back({className, parent, label.start, true});
            Span body{open.empty() ? label.end : match.end, end};
            auto nested = classes(rule, body, full, depth + 1);
            functions(Child(rule, @"function"), {match.start, end}, full, nested);
            return true;
        });
        return found;
    }
public:
    explicit Indexer(NSData *data) : text(static_cast<const char *>(data.bytes), data.length) {}
    NSArray *parse(NSXMLElement *parser) {
        each(Attr(parser, @"commentExpr"), {0, text.size()}, [&](Span s) { comments.push_back(s); return true; });
        auto classZones = classes(Child(parser, @"classRange"), {0, text.size()}, "", 0);
        functions(Child(parser, @"function"), {0, text.size()}, "", classZones);
        std::stable_sort(symbols.begin(), symbols.end(), [](const Symbol &a, const Symbol &b) { return a.position < b.position; });
        NSMutableArray *result = [NSMutableArray new];
        for (const auto &s : symbols) {
            NSString *label = [[NSString alloc] initWithBytes:s.name.data() length:s.name.size() encoding:NSUTF8StringEncoding];
            NSString *scope = [[NSString alloc] initWithBytes:s.scope.data() length:s.scope.size() encoding:NSUTF8StringEncoding];
            if (!label || !scope) continue;
            [result addObject:@{@"name":label, @"scope":scope, @"position":@(s.position), @"kind":s.isClass ? @"class" : @"function"}];
        }
        return result;
    }
};
}

NSArray<NSDictionary *> *NXFunctionIndex(NSData *text, NSString *ruleFile, NSError **error) {
    if (text.length > 16 * 1024 * 1024) { if (error) *error = NXError(@"Function List supports documents up to 16 MB."); return nil; }
    NSData *xml = [NSData dataWithContentsOfFile:ruleFile options:0 error:error];
    if (!xml) return nil;
    NSXMLDocument *document = [[NSXMLDocument alloc] initWithData:PreserveAttributeWhitespace(xml) options:NSXMLNodeLoadExternalEntitiesNever error:error];
    if (!document || document.DTD) { if (error && !*error) *error = NXError(@"Function List rules must not contain a DTD."); return nil; }
    NSXMLElement *parser = Child(Child(document.rootElement, @"functionList"), @"parser");
    if (!parser) { if (error) *error = NXError(@"This file does not contain a Function List parser."); return nil; }
    try { return Indexer(text).parse(parser); }
    catch (const std::exception &exception) {
        if (error) *error = NXError([NSString stringWithFormat:@"Function List could not parse this document: %s", exception.what()]);
        return nil;
    }
}
NSString *NXFunctionRule(NSString *path, NSString *lexer) {
    NSDictionary *extensions = @{@"h":@"cpp", @"hpp":@"cpp", @"cc":@"cpp", @"cxx":@"cpp", @"m":@"cpp", @"mm":@"cpp",
        @"py":@"python", @"js":@"javascript.js", @"jsx":@"javascript.js", @"ts":@"typescript", @"tsx":@"typescript",
        @"sh":@"bash", @"ps1":@"powershell", @"pl":@"perl", @"rb":@"ruby", @"rs":@"rust", @"f":@"fortran",
        @"f90":@"fortran", @"bas":@"vb", @"vbs":@"vb", @"bat":@"batch", @"cmd":@"batch", @"asm":@"asm",
        @"pas":@"pascal", @"html":@"xml", @"xhtml":@"xml", @"svg":@"xml", @"au3":@"autoit", @"nsi":@"nsis"};
    NSString *ext = path.pathExtension.lowercaseString;
    NSString *rule = extensions[ext] ?: (ext.length ? ext : lexer);
    if ([path.lastPathComponent.lowercaseString isEqual:@"makefile"]) rule = @"makefile";
    NSString *directory = [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"functionList"];
    NSString *file = [directory stringByAppendingPathComponent:[rule stringByAppendingPathExtension:@"xml"]];
    if ([NSFileManager.defaultManager fileExistsAtPath:file] && ![rule isEqual:@"overrideMap"]) return file;
    file = [directory stringByAppendingPathComponent:[lexer stringByAppendingPathExtension:@"xml"]];
    return [NSFileManager.defaultManager fileExistsAtPath:file] ? file : nil;
}
