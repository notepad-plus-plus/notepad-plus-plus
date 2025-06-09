// Scintilla source code edit control
/** @file SparseState.h
 ** Hold lexer state that may change rarely.
 ** This is often per-line state such as whether a particular type of section has been entered.
 ** A state continues until it is changed.
 **/
// Copyright 2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef SPARSESTATE_H
#define SPARSESTATE_H

namespace Lexilla {

template <typename T>
class SparseState {
	struct State {
		Sci_Position position;
		T value;
		constexpr State(Sci_Position position_, T value_) noexcept :
			position(position_), value(std::move(value_)) {
		}
		inline bool operator<(const State &other) const noexcept {
			return position < other.position;
		}
		inline bool operator==(const State &other) const noexcept {
			return (position == other.position) && (value == other.value);
		}
	};
	Sci_Position positionFirst;
	typedef std::vector<State> stateVector;
	stateVector states;

	typename stateVector::iterator Find(Sci_Position position) {
		const State searchValue(position, T());
		return std::lower_bound(states.begin(), states.end(), searchValue);
	}

public:
	explicit SparseState(Sci_Position positionFirst_=-1) {
		positionFirst = positionFirst_;
	}
	void Set(Sci_Position position, T value) {
		Delete(position);
		if (states.empty() || (value != states[states.size()-1].value)) {
			states.emplace_back(position, value);
		}
	}
	T ValueAt(Sci_Position position) {
		if (states.empty())
			return T();
		if (position < states[0].position)
			return T();
		typename stateVector::iterator low = Find(position);
		if (low == states.end()) {
			return states[states.size()-1].value;
		} else {
			if (low->position > position) {
				--low;
			}
			return low->value;
		}
	}
	bool Delete(Sci_Position position) {
		typename stateVector::iterator low = Find(position);
		if (low != states.end()) {
			states.erase(low, states.end());
			return true;
		}
		return false;
	}
	size_t size() const {
		return states.size();
	}

	// Returns true if Merge caused a significant change
	bool Merge(const SparseState<T> &other, Sci_Position ignoreAfter) {
		// Changes caused beyond ignoreAfter are not significant
		Delete(ignoreAfter+1);

		bool different = true;
		bool changed = false;
		typename stateVector::iterator low = Find(other.positionFirst);
		if (static_cast<size_t>(states.end() - low) == other.states.size()) {
			// Same number in other as after positionFirst in this
			different = !std::equal(low, states.end(), other.states.begin());
		}
		if (different) {
			if (low != states.end()) {
				states.erase(low, states.end());
				changed = true;
			}
			typename stateVector::const_iterator startOther = other.states.begin();
			if (!states.empty() && !other.states.empty() && states.back().value == startOther->value)
				++startOther;
			if (startOther != other.states.end()) {
				states.insert(states.end(), startOther, other.states.end());
				changed = true;
			}
		}
		return changed;
	}
};

}

#endif
