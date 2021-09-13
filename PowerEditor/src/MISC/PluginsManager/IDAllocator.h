// IDAllocator.h code is copyrighted (C) 2010 by Dave Brotherstone

// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


#pragma once

class IDAllocator
{
public:
	IDAllocator(int start, int maximumID);

	/// Returns -1 if not enough available
	int allocate(int quantity);

	bool isInRange(int id) { return (id >= _start && id < _nextID); }

private:
	int _start = 0;
	int _nextID = 0;
	int _maximumID = 0;
};

