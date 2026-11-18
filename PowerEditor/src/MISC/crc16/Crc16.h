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


#ifndef _CRC16_H_
#define _CRC16_H_
#include <assert.h>

class CRC16_ISO_3309
{
public :
    CRC16_ISO_3309(unsigned short polynom = 0x1021, unsigned short initVal = 0xFFFF)
        :_polynom(polynom), _initVal(initVal) {};
    ~CRC16_ISO_3309(){};

    void set(unsigned short polynom, unsigned short initVal) {
        _polynom = polynom;
        _initVal = initVal;
    };

    unsigned short calculate(unsigned char *data, unsigned short count)
    {
        unsigned short fcs = _initVal;
        unsigned short d, i, k;
        for (i=0; i<count; i++)
        {
            d = *data++ << 8;
            for (k=0; k<8; k++)
            {
                if ((fcs ^ d) & 0x8000)
                    fcs = (fcs << 1) ^ _polynom;
                else
                    fcs = (fcs << 1);
                d <<= 1;
            }
        }
        return(fcs);
    }

private :
    unsigned short _polynom;
    unsigned short _initVal;

};

const bool bits8 = true;
const bool bits16 = false;

class CRC16 : public CRC16_ISO_3309
{
public:
    CRC16(){};
    ~CRC16(){};
    unsigned short calculate(const unsigned char *data, unsigned short count)
    {
        assert(data != NULL);
        assert(count != 0);

        //unsigned short wordResult;
        unsigned char *pBuffer = new unsigned char[count];

        // Reverse all bits of the byte then copy the result byte by byte in the array
        for (int i = 0 ; i < count ; i++)
            pBuffer[i] = reverseByte<unsigned char>(data[i]);

        // calculate CRC : by default polynom = 0x1021, init val = 0xFFFF)
        unsigned short wordResult = CRC16_ISO_3309::calculate(pBuffer, count);

        // Reverse the WORD bits
        wordResult = reverseByte<unsigned short>(wordResult);

        // XOR FFFF
        wordResult ^= 0xFFFF;

        // Invert MSB/LSB
        wordResult = wordResult << 8 | wordResult >> 8 ;

        delete [] pBuffer;

        return wordResult;
    };

private:
    template <class IntType>
    IntType reverseByte(IntType val2Reverses)
    {
        IntType reversedValue = 0;
        long mask = 1;
        int nBits = sizeof(val2Reverses) * 8;
        for (int i = 0 ; i < nBits ; i++)
            if ((mask << i) & val2Reverses)
                reversedValue += (mask << (nBits - 1 - i));

        return reversedValue;
    };
};

#endif