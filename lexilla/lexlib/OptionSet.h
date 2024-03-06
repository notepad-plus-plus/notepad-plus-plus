// Scintilla source code edit control
/** @file OptionSet.h
 ** Manage descriptive information about an options struct for a lexer.
 ** Hold the names, positions, and descriptions of boolean, integer and string options and
 ** allow setting options and retrieving metadata about the options.
 **/
// Copyright 2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef OPTIONSET_H
#define OPTIONSET_H

namespace Lexilla {

template <typename T>
class OptionSet {
	typedef T Target;
	typedef bool T::*plcob;
	typedef int T::*plcoi;
	typedef std::string T::*plcos;
	struct Option {
		int opType;
		union {
			plcob pb;
			plcoi pi;
			plcos ps;
		};
		std::string value;
		std::string description;
		Option() :
			opType(SC_TYPE_BOOLEAN), pb(nullptr) {
		}
		Option(plcob pb_, std::string_view description_="") :
			opType(SC_TYPE_BOOLEAN), pb(pb_), description(description_) {
		}
		Option(plcoi pi_, std::string_view description_) :
			opType(SC_TYPE_INTEGER), pi(pi_), description(description_) {
		}
		Option(plcos ps_, std::string_view description_) :
			opType(SC_TYPE_STRING), ps(ps_), description(description_) {
		}
		bool Set(T *base, const char *val) {
			value = val;
			switch (opType) {
			case SC_TYPE_BOOLEAN: {
					const bool option = atoi(val) != 0;
					if ((*base).*pb != option) {
						(*base).*pb = option;
						return true;
					}
					break;
				}
			case SC_TYPE_INTEGER: {
					const int option = atoi(val);
					if ((*base).*pi != option) {
						(*base).*pi = option;
						return true;
					}
					break;
				}
			case SC_TYPE_STRING: {
					if ((*base).*ps != val) {
						(*base).*ps = val;
						return true;
					}
					break;
				}
			default:
				break;
			}
			return false;
		}
		const char *Get() const noexcept {
			return value.c_str();
		}
	};
	typedef std::map<std::string, Option, std::less<>> OptionMap;
	OptionMap nameToDef;
	std::string names;
	std::string wordLists;

	void AppendName(const char *name) {
		if (!names.empty())
			names += "\n";
		names += name;
	}
public:
	void DefineProperty(const char *name, plcob pb, std::string_view description="") {
		nameToDef[name] = Option(pb, description);
		AppendName(name);
	}
	void DefineProperty(const char *name, plcoi pi, std::string_view description="") {
		nameToDef[name] = Option(pi, description);
		AppendName(name);
	}
	void DefineProperty(const char *name, plcos ps, std::string_view description="") {
		nameToDef[name] = Option(ps, description);
		AppendName(name);
	}
	template <typename E>
	void DefineProperty(const char *name, E T::*pe, std::string_view description="") {
		static_assert(std::is_enum<E>::value);
		plcoi pi {};
		static_assert(sizeof(pe) == sizeof(pi));
		memcpy(&pi, &pe, sizeof(pe));
		nameToDef[name] = Option(pi, description);
		AppendName(name);
	}
	const char *PropertyNames() const noexcept {
		return names.c_str();
	}
	int PropertyType(const char *name) const {
		typename OptionMap::const_iterator const it = nameToDef.find(name);
		if (it != nameToDef.end()) {
			return it->second.opType;
		}
		return SC_TYPE_BOOLEAN;
	}
	const char *DescribeProperty(const char *name) const {
		typename OptionMap::const_iterator const it = nameToDef.find(name);
		if (it != nameToDef.end()) {
			return it->second.description.c_str();
		}
		return "";
	}

	bool PropertySet(T *base, const char *name, const char *val) {
		typename OptionMap::iterator const it = nameToDef.find(name);
		if (it != nameToDef.end()) {
			return it->second.Set(base, val);
		}
		return false;
	}

	const char *PropertyGet(const char *name) const {
		typename OptionMap::const_iterator const it = nameToDef.find(name);
		if (it != nameToDef.end()) {
			return it->second.Get();
		}
		return nullptr;
	}

	void DefineWordListSets(const char * const wordListDescriptions[]) {
		if (wordListDescriptions) {
			for (size_t wl = 0; wordListDescriptions[wl]; wl++) {
				if (wl > 0)
					wordLists += "\n";
				wordLists += wordListDescriptions[wl];
			}
		}
	}

	const char *DescribeWordListSets() const noexcept {
		return wordLists.c_str();
	}
};

}

#endif
