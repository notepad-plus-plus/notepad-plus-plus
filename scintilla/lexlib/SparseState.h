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

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

template <typename T>
class SparseState {
	struct State {
		int position;
		T value;
		State(int position_, T value_) : position(position_), value(value_) {
		}
		inline bool operator<(const State &other) const {
			return position < other.position;
		}
		inline bool operator==(const State &other) const {
			return (position == other.position) && (value == other.value);
		}
	};
	int positionFirst;
	typedef std::vector<State> stateVector;
	stateVector states;

	typename stateVector::iterator Find(int position) {
		State searchValue(position, T());
		return std::lower_bound(states.begin(), states.end(), searchValue);
	}

public:
	SparseState(int positionFirst_=-1) {
		positionFirst = positionFirst_;
	}
	void Set(int position, T value) {
		Delete(position);
		if (states.empty() || (value != states[states.size()-1].value)) {
			states.push_back(State(position, value));
		}
	}
	T ValueAt(int position) {
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
	bool Delete(int position) {
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
	bool Merge(const SparseState<T> &other, int ignoreAfter) {
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

#ifdef SCI_NAMESPACE
}
#endif

#endif
