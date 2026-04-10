/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C) 2017-2025 Pavel Nedev (pg.nedev@gmail.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

// WARNING: This file contains Notepad++ internal defines that are not part of its plugins API and are not intended
// for use outside Notepad++ itself. This means that those might change in future Notepad++ versions without notice!

#pragma once

#include <windows.h>

#define WM_TABSETSTYLE	(WM_APP + 0x024)

constexpr int MARK_HIDELINESBEGIN	= 19;
constexpr int MARK_HIDELINESEND		= 18;
