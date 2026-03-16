// app_state.mm — Global AppContext storage
// Part of the Notepad++ macOS port modular refactor.

#include "app_state.h"

static AppContext s_ctx;

AppContext& ctx()
{
	return s_ctx;
}
