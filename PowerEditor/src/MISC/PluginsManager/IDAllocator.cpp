// IDAllocator.h code is copyrighted (C) 2010 by Dave Brotherstone
// Part of Notepad++ - Copyright Don Ho
#include "precompiledHeaders.h"

#include "IDAllocator.h"

IDAllocator::IDAllocator(int start, int maximumID)
	: _start(start),
	  _nextID(start),
	  _maximumID(maximumID)
{
}

int IDAllocator::allocate(int quantity)
{
	int retVal = -1;

	if (_nextID + quantity <= _maximumID && quantity > 0)
	{
		retVal = _nextID;
		_nextID += quantity;
	}

	return retVal;
}

	
