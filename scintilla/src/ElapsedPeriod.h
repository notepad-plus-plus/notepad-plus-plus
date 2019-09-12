// Scintilla source code edit control
/** @file ElapsedPeriod.h
 ** Encapsulate C++ <chrono> to simplify use.
 **/
// Copyright 2018 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef ELAPSEDPERIOD_H
#define ELAPSEDPERIOD_H

namespace Scintilla {

// Simplified access to high precision timing.
class ElapsedPeriod {
	std::chrono::high_resolution_clock::time_point tp;
public:
	/// Capture the moment
	ElapsedPeriod() noexcept : tp(std::chrono::high_resolution_clock::now()) {
	}
	/// Return duration as floating point seconds
	double Duration(bool reset=false) {
		const std::chrono::high_resolution_clock::time_point tpNow =
			std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> stylingDuration =
			std::chrono::duration_cast<std::chrono::duration<double>>(tpNow - tp);
		if (reset) {
			tp = tpNow;
		}
		return stylingDuration.count();
	}
};

}

#endif
