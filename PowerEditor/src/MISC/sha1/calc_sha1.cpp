// This file is part of Notepad++ project
// Copyright (C)2023 Don HO <don.h@free.fr>

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

#include <cstdint>
#include "sha1.h"
#include "calc_sha1.h"

void calc_sha1(unsigned char hash[20], const void *input, size_t len)
{
    CSHA1 sha1;
    sha1.Update((const uint8_t*)input, static_cast<unsigned int>(len));
    sha1.Final();
    sha1.GetHash(hash);
}
