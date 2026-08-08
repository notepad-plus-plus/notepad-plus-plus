// Scintilla source code edit control
/** @file RunThreads.h
 ** Run a function over multiple threads, optimizing the single thread case.
 **/
// Copyright 2026 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef RUNTHREADS_H
#define RUNTHREADS_H

namespace Scintilla::Internal {

template<typename Func>
void RunThreads(size_t threads, Func fun) {
	if (threads <= 1) {
		// Run on 1 thread and avoid std::async overhead
		fun();
	} else {
		// Run on multiple threads
		std::vector<std::future<void>> futures(threads);
		for (size_t th = 0; th < threads; th++) {
			futures[th] = std::async(std::launch::async, fun);
		}
		for (std::future<void> &f : futures) {
			f.get();
		}
	}
}

}

#endif
