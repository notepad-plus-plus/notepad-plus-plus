// Scintilla source code edit control
/** @file LexHex.cxx
 ** Lexers for Motorola S-Record, Intel HEX and Tektronix extended HEX.
 **
 ** Written by Markus Heidelberg
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

/*
 *  Motorola S-Record
 * ===============================
 *
 * Each record (line) is built as follows:
 *
 *    field       digits          states
 *
 *  +----------+
 *  | start    |  1 ('S')         SCE_HEX_RECSTART
 *  +----------+
 *  | type     |  1               SCE_HEX_RECTYPE, (SCE_HEX_RECTYPE_UNKNOWN)
 *  +----------+
 *  | count    |  2               SCE_HEX_BYTECOUNT, SCE_HEX_BYTECOUNT_WRONG
 *  +----------+
 *  | address  |  4/6/8           SCE_HEX_NOADDRESS, SCE_HEX_DATAADDRESS, SCE_HEX_RECCOUNT, SCE_HEX_STARTADDRESS, (SCE_HEX_ADDRESSFIELD_UNKNOWN)
 *  +----------+
 *  | data     |  0..504/502/500  SCE_HEX_DATA_ODD, SCE_HEX_DATA_EVEN, SCE_HEX_DATA_EMPTY, (SCE_HEX_DATA_UNKNOWN)
 *  +----------+
 *  | checksum |  2               SCE_HEX_CHECKSUM, SCE_HEX_CHECKSUM_WRONG
 *  +----------+
 *
 *
 *  Intel HEX
 * ===============================
 *
 * Each record (line) is built as follows:
 *
 *    field       digits          states
 *
 *  +----------+
 *  | start    |  1 (':')         SCE_HEX_RECSTART
 *  +----------+
 *  | count    |  2               SCE_HEX_BYTECOUNT, SCE_HEX_BYTECOUNT_WRONG
 *  +----------+
 *  | address  |  4               SCE_HEX_NOADDRESS, SCE_HEX_DATAADDRESS, (SCE_HEX_ADDRESSFIELD_UNKNOWN)
 *  +----------+
 *  | type     |  2               SCE_HEX_RECTYPE, (SCE_HEX_RECTYPE_UNKNOWN)
 *  +----------+
 *  | data     |  0..510          SCE_HEX_DATA_ODD, SCE_HEX_DATA_EVEN, SCE_HEX_DATA_EMPTY, SCE_HEX_EXTENDEDADDRESS, SCE_HEX_STARTADDRESS, (SCE_HEX_DATA_UNKNOWN)
 *  +----------+
 *  | checksum |  2               SCE_HEX_CHECKSUM, SCE_HEX_CHECKSUM_WRONG
 *  +----------+
 *
 *
 * Folding:
 *
 *   Data records (type 0x00), which follow an extended address record (type
 *   0x02 or 0x04), can be folded. The extended address record is the fold
 *   point at fold level 0, the corresponding data records are set to level 1.
 *
 *   Any record, which is not a data record, sets the fold level back to 0.
 *   Any line, which is not a record (blank lines and lines starting with a
 *   character other than ':'), leaves the fold level unchanged.
 *
 *
 *  Tektronix extended HEX
 * ===============================
 *
 * Each record (line) is built as follows:
 *
 *    field       digits          states
 *
 *  +----------+
 *  | start    |  1 ('%')         SCE_HEX_RECSTART
 *  +----------+
 *  | length   |  2               SCE_HEX_BYTECOUNT, SCE_HEX_BYTECOUNT_WRONG
 *  +----------+
 *  | type     |  1               SCE_HEX_RECTYPE, (SCE_HEX_RECTYPE_UNKNOWN)
 *  +----------+
 *  | checksum |  2               SCE_HEX_CHECKSUM, SCE_HEX_CHECKSUM_WRONG
 *  +----------+
 *  | address  |  9               SCE_HEX_DATAADDRESS, SCE_HEX_STARTADDRESS, (SCE_HEX_ADDRESSFIELD_UNKNOWN)
 *  +----------+
 *  | data     |  0..241          SCE_HEX_DATA_ODD, SCE_HEX_DATA_EVEN
 *  +----------+
 *
 *
 *  General notes for all lexers
 * ===============================
 *
 * - Depending on where the helper functions are invoked, some of them have to
 *   read beyond the current position. In case of malformed data (record too
 *   short), it has to be ensured that this either does not have bad influence
 *   or will be captured deliberately.
 *
 * - States in parentheses in the upper format descriptions indicate that they
 *   should not appear in a valid hex file.
 *
 * - State SCE_HEX_GARBAGE means garbage data after the intended end of the
 *   record, the line is too long then. This state is used in all lexers.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

// prototypes for general helper functions
static inline bool IsNewline(const int ch);
static int GetHexaNibble(char hd);
static int GetHexaChar(char hd1, char hd2);
static int GetHexaChar(Sci_PositionU pos, Accessor &styler);
static bool ForwardWithinLine(StyleContext &sc, Sci_Position nb = 1);
static bool PosInSameRecord(Sci_PositionU pos1, Sci_PositionU pos2, Accessor &styler);
static Sci_Position CountByteCount(Sci_PositionU startPos, Sci_Position uncountedDigits, Accessor &styler);
static int CalcChecksum(Sci_PositionU startPos, Sci_Position cnt, bool twosCompl, Accessor &styler);

// prototypes for file format specific helper functions
static Sci_PositionU GetSrecRecStartPosition(Sci_PositionU pos, Accessor &styler);
static int GetSrecByteCount(Sci_PositionU recStartPos, Accessor &styler);
static Sci_Position CountSrecByteCount(Sci_PositionU recStartPos, Accessor &styler);
static int GetSrecAddressFieldSize(Sci_PositionU recStartPos, Accessor &styler);
static int GetSrecAddressFieldType(Sci_PositionU recStartPos, Accessor &styler);
static int GetSrecDataFieldType(Sci_PositionU recStartPos, Accessor &styler);
static Sci_Position GetSrecRequiredDataFieldSize(Sci_PositionU recStartPos, Accessor &styler);
static int GetSrecChecksum(Sci_PositionU recStartPos, Accessor &styler);
static int CalcSrecChecksum(Sci_PositionU recStartPos, Accessor &styler);

static Sci_PositionU GetIHexRecStartPosition(Sci_PositionU pos, Accessor &styler);
static int GetIHexByteCount(Sci_PositionU recStartPos, Accessor &styler);
static Sci_Position CountIHexByteCount(Sci_PositionU recStartPos, Accessor &styler);
static int GetIHexAddressFieldType(Sci_PositionU recStartPos, Accessor &styler);
static int GetIHexDataFieldType(Sci_PositionU recStartPos, Accessor &styler);
static int GetIHexRequiredDataFieldSize(Sci_PositionU recStartPos, Accessor &styler);
static int GetIHexChecksum(Sci_PositionU recStartPos, Accessor &styler);
static int CalcIHexChecksum(Sci_PositionU recStartPos, Accessor &styler);

static int GetTEHexDigitCount(Sci_PositionU recStartPos, Accessor &styler);
static Sci_Position CountTEHexDigitCount(Sci_PositionU recStartPos, Accessor &styler);
static int GetTEHexAddressFieldType(Sci_PositionU recStartPos, Accessor &styler);
static int GetTEHexChecksum(Sci_PositionU recStartPos, Accessor &styler);
static int CalcTEHexChecksum(Sci_PositionU recStartPos, Accessor &styler);

static inline bool IsNewline(const int ch)
{
    return (ch == '\n' || ch == '\r');
}

static int GetHexaNibble(char hd)
{
	int hexValue = 0;

	if (hd >= '0' && hd <= '9') {
		hexValue += hd - '0';
	} else if (hd >= 'A' && hd <= 'F') {
		hexValue += hd - 'A' + 10;
	} else if (hd >= 'a' && hd <= 'f') {
		hexValue += hd - 'a' + 10;
	} else {
		return -1;
	}

	return hexValue;
}

static int GetHexaChar(char hd1, char hd2)
{
	int hexValue = 0;

	if (hd1 >= '0' && hd1 <= '9') {
		hexValue += 16 * (hd1 - '0');
	} else if (hd1 >= 'A' && hd1 <= 'F') {
		hexValue += 16 * (hd1 - 'A' + 10);
	} else if (hd1 >= 'a' && hd1 <= 'f') {
		hexValue += 16 * (hd1 - 'a' + 10);
	} else {
		return -1;
	}

	if (hd2 >= '0' && hd2 <= '9') {
		hexValue += hd2 - '0';
	} else if (hd2 >= 'A' && hd2 <= 'F') {
		hexValue += hd2 - 'A' + 10;
	} else if (hd2 >= 'a' && hd2 <= 'f') {
		hexValue += hd2 - 'a' + 10;
	} else {
		return -1;
	}

	return hexValue;
}

static int GetHexaChar(Sci_PositionU pos, Accessor &styler)
{
	char highNibble, lowNibble;

	highNibble = styler.SafeGetCharAt(pos);
	lowNibble = styler.SafeGetCharAt(pos + 1);

	return GetHexaChar(highNibble, lowNibble);
}

// Forward <nb> characters, but abort (and return false) if hitting the line
// end. Return true if forwarding within the line was possible.
// Avoids influence on highlighting of the subsequent line if the current line
// is malformed (too short).
static bool ForwardWithinLine(StyleContext &sc, Sci_Position nb)
{
	for (Sci_Position i = 0; i < nb; i++) {
		if (sc.atLineEnd) {
			// line is too short
			sc.SetState(SCE_HEX_DEFAULT);
			sc.Forward();
			return false;
		} else {
			sc.Forward();
		}
	}

	return true;
}

// Checks whether the given positions are in the same record.
static bool PosInSameRecord(Sci_PositionU pos1, Sci_PositionU pos2, Accessor &styler)
{
	return styler.GetLine(pos1) == styler.GetLine(pos2);
}

// Count the number of digit pairs from <startPos> till end of record, ignoring
// <uncountedDigits> digits.
// If the record is too short, a negative count may be returned.
static Sci_Position CountByteCount(Sci_PositionU startPos, Sci_Position uncountedDigits, Accessor &styler)
{
	Sci_Position cnt;
	Sci_PositionU pos;

	pos = startPos;

	while (!IsNewline(styler.SafeGetCharAt(pos, '\n'))) {
		pos++;
	}

	// number of digits in this line minus number of digits of uncounted fields
	cnt = static_cast<Sci_Position>(pos - startPos) - uncountedDigits;

	// Prepare round up if odd (digit pair incomplete), this way the byte
	// count is considered to be valid if the checksum is incomplete.
	if (cnt >= 0) {
		cnt++;
	}

	// digit pairs
	cnt /= 2;

	return cnt;
}

// Calculate the checksum of the record.
// <startPos> is the position of the first character of the starting digit
// pair, <cnt> is the number of digit pairs.
static int CalcChecksum(Sci_PositionU startPos, Sci_Position cnt, bool twosCompl, Accessor &styler)
{
	int cs = 0;

	for (Sci_PositionU pos = startPos; pos < startPos + cnt; pos += 2) {
		int val = GetHexaChar(pos, styler);

		if (val < 0) {
			return val;
		}

		// overflow does not matter
		cs += val;
	}

	if (twosCompl) {
		// low byte of two's complement
		return -cs & 0xFF;
	} else {
		// low byte of one's complement
		return ~cs & 0xFF;
	}
}

// Get the position of the record "start" field (first character in line) in
// the record around position <pos>.
static Sci_PositionU GetSrecRecStartPosition(Sci_PositionU pos, Accessor &styler)
{
	while (styler.SafeGetCharAt(pos) != 'S') {
		pos--;
	}

	return pos;
}

// Get the value of the "byte count" field, it counts the number of bytes in
// the subsequent fields ("address", "data" and "checksum" fields).
static int GetSrecByteCount(Sci_PositionU recStartPos, Accessor &styler)
{
	int val;

	val = GetHexaChar(recStartPos + 2, styler);
	if (val < 0) {
	       val = 0;
	}

	return val;
}

// Count the number of digit pairs for the "address", "data" and "checksum"
// fields in this record. Has to be equal to the "byte count" field value.
// If the record is too short, a negative count may be returned.
static Sci_Position CountSrecByteCount(Sci_PositionU recStartPos, Accessor &styler)
{
	return CountByteCount(recStartPos, 4, styler);
}

// Get the size of the "address" field.
static int GetSrecAddressFieldSize(Sci_PositionU recStartPos, Accessor &styler)
{
	switch (styler.SafeGetCharAt(recStartPos + 1)) {
		case '0':
		case '1':
		case '5':
		case '9':
			return 2; // 16 bit

		case '2':
		case '6':
		case '8':
			return 3; // 24 bit

		case '3':
		case '7':
			return 4; // 32 bit

		default:
			return 0;
	}
}

// Get the type of the "address" field content.
static int GetSrecAddressFieldType(Sci_PositionU recStartPos, Accessor &styler)
{
	switch (styler.SafeGetCharAt(recStartPos + 1)) {
		case '0':
			return SCE_HEX_NOADDRESS;

		case '1':
		case '2':
		case '3':
			return SCE_HEX_DATAADDRESS;

		case '5':
		case '6':
			return SCE_HEX_RECCOUNT;

		case '7':
		case '8':
		case '9':
			return SCE_HEX_STARTADDRESS;

		default: // handle possible format extension in the future
			return SCE_HEX_ADDRESSFIELD_UNKNOWN;
	}
}

// Get the type of the "data" field content.
static int GetSrecDataFieldType(Sci_PositionU recStartPos, Accessor &styler)
{
	switch (styler.SafeGetCharAt(recStartPos + 1)) {
		case '0':
		case '1':
		case '2':
		case '3':
			return SCE_HEX_DATA_ODD;

		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			return SCE_HEX_DATA_EMPTY;

		default: // handle possible format extension in the future
			return SCE_HEX_DATA_UNKNOWN;
	}
}

// Get the required size of the "data" field. Useless for block header and
// ordinary data records (type S0, S1, S2, S3), return the value calculated
// from the "byte count" and "address field" size in this case.
static Sci_Position GetSrecRequiredDataFieldSize(Sci_PositionU recStartPos, Accessor &styler)
{
	switch (styler.SafeGetCharAt(recStartPos + 1)) {
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			return 0;

		default:
			return GetSrecByteCount(recStartPos, styler)
				- GetSrecAddressFieldSize(recStartPos, styler)
				- 1; // -1 for checksum field
	}
}

// Get the value of the "checksum" field.
static int GetSrecChecksum(Sci_PositionU recStartPos, Accessor &styler)
{
	int byteCount;

	byteCount = GetSrecByteCount(recStartPos, styler);

	return GetHexaChar(recStartPos + 2 + byteCount * 2, styler);
}

// Calculate the checksum of the record.
static int CalcSrecChecksum(Sci_PositionU recStartPos, Accessor &styler)
{
	Sci_Position byteCount;

	byteCount = GetSrecByteCount(recStartPos, styler);

	// sum over "byte count", "address" and "data" fields (6..510 digits)
	return CalcChecksum(recStartPos + 2, byteCount * 2, false, styler);
}

// Get the position of the record "start" field (first character in line) in
// the record around position <pos>.
static Sci_PositionU GetIHexRecStartPosition(Sci_PositionU pos, Accessor &styler)
{
	while (styler.SafeGetCharAt(pos) != ':') {
		pos--;
	}

	return pos;
}

// Get the value of the "byte count" field, it counts the number of bytes in
// the "data" field.
static int GetIHexByteCount(Sci_PositionU recStartPos, Accessor &styler)
{
	int val;

	val = GetHexaChar(recStartPos + 1, styler);
	if (val < 0) {
	       val = 0;
	}

	return val;
}

// Count the number of digit pairs for the "data" field in this record. Has to
// be equal to the "byte count" field value.
// If the record is too short, a negative count may be returned.
static Sci_Position CountIHexByteCount(Sci_PositionU recStartPos, Accessor &styler)
{
	return CountByteCount(recStartPos, 11, styler);
}

// Get the type of the "address" field content.
static int GetIHexAddressFieldType(Sci_PositionU recStartPos, Accessor &styler)
{
	if (!PosInSameRecord(recStartPos, recStartPos + 7, styler)) {
		// malformed (record too short)
		// type cannot be determined
		return SCE_HEX_ADDRESSFIELD_UNKNOWN;
	}

	switch (GetHexaChar(recStartPos + 7, styler)) {
		case 0x00:
			return SCE_HEX_DATAADDRESS;

		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
			return SCE_HEX_NOADDRESS;

		default: // handle possible format extension in the future
			return SCE_HEX_ADDRESSFIELD_UNKNOWN;
	}
}

// Get the type of the "data" field content.
static int GetIHexDataFieldType(Sci_PositionU recStartPos, Accessor &styler)
{
	switch (GetHexaChar(recStartPos + 7, styler)) {
		case 0x00:
			return SCE_HEX_DATA_ODD;

		case 0x01:
			return SCE_HEX_DATA_EMPTY;

		case 0x02:
		case 0x04:
			return SCE_HEX_EXTENDEDADDRESS;

		case 0x03:
		case 0x05:
			return SCE_HEX_STARTADDRESS;

		default: // handle possible format extension in the future
			return SCE_HEX_DATA_UNKNOWN;
	}
}

// Get the required size of the "data" field. Useless for an ordinary data
// record (type 00), return the "byte count" in this case.
static int GetIHexRequiredDataFieldSize(Sci_PositionU recStartPos, Accessor &styler)
{
	switch (GetHexaChar(recStartPos + 7, styler)) {
		case 0x01:
			return 0;

		case 0x02:
		case 0x04:
			return 2;

		case 0x03:
		case 0x05:
			return 4;

		default:
			return GetIHexByteCount(recStartPos, styler);
	}
}

// Get the value of the "checksum" field.
static int GetIHexChecksum(Sci_PositionU recStartPos, Accessor &styler)
{
	int byteCount;

	byteCount = GetIHexByteCount(recStartPos, styler);

	return GetHexaChar(recStartPos + 9 + byteCount * 2, styler);
}

// Calculate the checksum of the record.
static int CalcIHexChecksum(Sci_PositionU recStartPos, Accessor &styler)
{
	int byteCount;

	byteCount = GetIHexByteCount(recStartPos, styler);

	// sum over "byte count", "address", "type" and "data" fields (8..518 digits)
	return CalcChecksum(recStartPos + 1, 8 + byteCount * 2, true, styler);
}


// Get the value of the "record length" field, it counts the number of digits in
// the record excluding the percent.
static int GetTEHexDigitCount(Sci_PositionU recStartPos, Accessor &styler)
{
	int val = GetHexaChar(recStartPos + 1, styler);
	if (val < 0)
	       val = 0;

	return val;
}

// Count the number of digits in this record. Has to
// be equal to the "record length" field value.
static Sci_Position CountTEHexDigitCount(Sci_PositionU recStartPos, Accessor &styler)
{
	Sci_PositionU pos;

	pos = recStartPos+1;

	while (!IsNewline(styler.SafeGetCharAt(pos, '\n'))) {
		pos++;
	}

	return static_cast<Sci_Position>(pos - (recStartPos+1));
}

// Get the type of the "address" field content.
static int GetTEHexAddressFieldType(Sci_PositionU recStartPos, Accessor &styler)
{
	switch (styler.SafeGetCharAt(recStartPos + 3)) {
		case '6':
			return SCE_HEX_DATAADDRESS;

		case '8':
			return SCE_HEX_STARTADDRESS;

		default: // handle possible format extension in the future
			return SCE_HEX_ADDRESSFIELD_UNKNOWN;
	}
}

// Get the value of the "checksum" field.
static int GetTEHexChecksum(Sci_PositionU recStartPos, Accessor &styler)
{
	return GetHexaChar(recStartPos+4, styler);
}

// Calculate the checksum of the record (excluding the checksum field).
static int CalcTEHexChecksum(Sci_PositionU recStartPos, Accessor &styler)
{
	Sci_PositionU pos = recStartPos +1;
	Sci_PositionU length = GetTEHexDigitCount(recStartPos, styler);

	int cs = GetHexaNibble(styler.SafeGetCharAt(pos++));//length
	cs += GetHexaNibble(styler.SafeGetCharAt(pos++));//length

	cs += GetHexaNibble(styler.SafeGetCharAt(pos++));//type

	pos += 2;// jump over CS field

	for (; pos <= recStartPos + length; ++pos) {
		int val = GetHexaNibble(styler.SafeGetCharAt(pos));

		if (val < 0) {
			return val;
		}

		// overflow does not matter
		cs += val;
	}

	// low byte
	return cs & 0xFF;

}

static void ColouriseSrecDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *[], Accessor &styler)
{
	StyleContext sc(startPos, length, initStyle, styler);

	while (sc.More()) {
		Sci_PositionU recStartPos;
		int byteCount, reqByteCount, addrFieldSize, addrFieldType, dataFieldSize, dataFieldType;
		int cs1, cs2;

		switch (sc.state) {
			case SCE_HEX_DEFAULT:
				if (sc.atLineStart && sc.Match('S')) {
					sc.SetState(SCE_HEX_RECSTART);
				}
				ForwardWithinLine(sc);
				break;

			case SCE_HEX_RECSTART:
				recStartPos = sc.currentPos - 1;
				addrFieldType = GetSrecAddressFieldType(recStartPos, styler);

				if (addrFieldType == SCE_HEX_ADDRESSFIELD_UNKNOWN) {
					sc.SetState(SCE_HEX_RECTYPE_UNKNOWN);
				} else {
					sc.SetState(SCE_HEX_RECTYPE);
				}

				ForwardWithinLine(sc);
				break;

			case SCE_HEX_RECTYPE:
			case SCE_HEX_RECTYPE_UNKNOWN:
				recStartPos = sc.currentPos - 2;
				byteCount = GetSrecByteCount(recStartPos, styler);
				reqByteCount = GetSrecAddressFieldSize(recStartPos, styler)
						+ GetSrecRequiredDataFieldSize(recStartPos, styler)
						+ 1; // +1 for checksum field

				if (byteCount == CountSrecByteCount(recStartPos, styler)
						&& byteCount == reqByteCount) {
					sc.SetState(SCE_HEX_BYTECOUNT);
				} else {
					sc.SetState(SCE_HEX_BYTECOUNT_WRONG);
				}

				ForwardWithinLine(sc, 2);
				break;

			case SCE_HEX_BYTECOUNT:
			case SCE_HEX_BYTECOUNT_WRONG:
				recStartPos = sc.currentPos - 4;
				addrFieldSize = GetSrecAddressFieldSize(recStartPos, styler);
				addrFieldType = GetSrecAddressFieldType(recStartPos, styler);

				sc.SetState(addrFieldType);
				ForwardWithinLine(sc, addrFieldSize * 2);
				break;

			case SCE_HEX_NOADDRESS:
			case SCE_HEX_DATAADDRESS:
			case SCE_HEX_RECCOUNT:
			case SCE_HEX_STARTADDRESS:
			case SCE_HEX_ADDRESSFIELD_UNKNOWN:
				recStartPos = GetSrecRecStartPosition(sc.currentPos, styler);
				dataFieldType = GetSrecDataFieldType(recStartPos, styler);

				// Using the required size here if possible has the effect that the
				// checksum is highlighted at a fixed position after this field for
				// specific record types, independent on the "byte count" value.
				dataFieldSize = GetSrecRequiredDataFieldSize(recStartPos, styler);

				sc.SetState(dataFieldType);

				if (dataFieldType == SCE_HEX_DATA_ODD) {
					for (int i = 0; i < dataFieldSize * 2; i++) {
						if ((i & 0x3) == 0) {
							sc.SetState(SCE_HEX_DATA_ODD);
						} else if ((i & 0x3) == 2) {
							sc.SetState(SCE_HEX_DATA_EVEN);
						}

						if (!ForwardWithinLine(sc)) {
							break;
						}
					}
				} else {
					ForwardWithinLine(sc, dataFieldSize * 2);
				}
				break;

			case SCE_HEX_DATA_ODD:
			case SCE_HEX_DATA_EVEN:
			case SCE_HEX_DATA_EMPTY:
			case SCE_HEX_DATA_UNKNOWN:
				recStartPos = GetSrecRecStartPosition(sc.currentPos, styler);
				cs1 = CalcSrecChecksum(recStartPos, styler);
				cs2 = GetSrecChecksum(recStartPos, styler);

				if (cs1 != cs2 || cs1 < 0 || cs2 < 0) {
					sc.SetState(SCE_HEX_CHECKSUM_WRONG);
				} else {
					sc.SetState(SCE_HEX_CHECKSUM);
				}

				ForwardWithinLine(sc, 2);
				break;

			case SCE_HEX_CHECKSUM:
			case SCE_HEX_CHECKSUM_WRONG:
			case SCE_HEX_GARBAGE:
				// record finished or line too long
				sc.SetState(SCE_HEX_GARBAGE);
				ForwardWithinLine(sc);
				break;

			default:
				// prevent endless loop in faulty state
				sc.SetState(SCE_HEX_DEFAULT);
				break;
		}
	}
	sc.Complete();
}

static void ColouriseIHexDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *[], Accessor &styler)
{
	StyleContext sc(startPos, length, initStyle, styler);

	while (sc.More()) {
		Sci_PositionU recStartPos;
		int byteCount, addrFieldType, dataFieldSize, dataFieldType;
		int cs1, cs2;

		switch (sc.state) {
			case SCE_HEX_DEFAULT:
				if (sc.atLineStart && sc.Match(':')) {
					sc.SetState(SCE_HEX_RECSTART);
				}
				ForwardWithinLine(sc);
				break;

			case SCE_HEX_RECSTART:
				recStartPos = sc.currentPos - 1;
				byteCount = GetIHexByteCount(recStartPos, styler);
				dataFieldSize = GetIHexRequiredDataFieldSize(recStartPos, styler);

				if (byteCount == CountIHexByteCount(recStartPos, styler)
						&& byteCount == dataFieldSize) {
					sc.SetState(SCE_HEX_BYTECOUNT);
				} else {
					sc.SetState(SCE_HEX_BYTECOUNT_WRONG);
				}

				ForwardWithinLine(sc, 2);
				break;

			case SCE_HEX_BYTECOUNT:
			case SCE_HEX_BYTECOUNT_WRONG:
				recStartPos = sc.currentPos - 3;
				addrFieldType = GetIHexAddressFieldType(recStartPos, styler);

				sc.SetState(addrFieldType);
				ForwardWithinLine(sc, 4);
				break;

			case SCE_HEX_NOADDRESS:
			case SCE_HEX_DATAADDRESS:
			case SCE_HEX_ADDRESSFIELD_UNKNOWN:
				recStartPos = sc.currentPos - 7;
				addrFieldType = GetIHexAddressFieldType(recStartPos, styler);

				if (addrFieldType == SCE_HEX_ADDRESSFIELD_UNKNOWN) {
					sc.SetState(SCE_HEX_RECTYPE_UNKNOWN);
				} else {
					sc.SetState(SCE_HEX_RECTYPE);
				}

				ForwardWithinLine(sc, 2);
				break;

			case SCE_HEX_RECTYPE:
			case SCE_HEX_RECTYPE_UNKNOWN:
				recStartPos = sc.currentPos - 9;
				dataFieldType = GetIHexDataFieldType(recStartPos, styler);

				// Using the required size here if possible has the effect that the
				// checksum is highlighted at a fixed position after this field for
				// specific record types, independent on the "byte count" value.
				dataFieldSize = GetIHexRequiredDataFieldSize(recStartPos, styler);

				sc.SetState(dataFieldType);

				if (dataFieldType == SCE_HEX_DATA_ODD) {
					for (int i = 0; i < dataFieldSize * 2; i++) {
						if ((i & 0x3) == 0) {
							sc.SetState(SCE_HEX_DATA_ODD);
						} else if ((i & 0x3) == 2) {
							sc.SetState(SCE_HEX_DATA_EVEN);
						}

						if (!ForwardWithinLine(sc)) {
							break;
						}
					}
				} else {
					ForwardWithinLine(sc, dataFieldSize * 2);
				}
				break;

			case SCE_HEX_DATA_ODD:
			case SCE_HEX_DATA_EVEN:
			case SCE_HEX_DATA_EMPTY:
			case SCE_HEX_EXTENDEDADDRESS:
			case SCE_HEX_STARTADDRESS:
			case SCE_HEX_DATA_UNKNOWN:
				recStartPos = GetIHexRecStartPosition(sc.currentPos, styler);
				cs1 = CalcIHexChecksum(recStartPos, styler);
				cs2 = GetIHexChecksum(recStartPos, styler);

				if (cs1 != cs2 || cs1 < 0 || cs2 < 0) {
					sc.SetState(SCE_HEX_CHECKSUM_WRONG);
				} else {
					sc.SetState(SCE_HEX_CHECKSUM);
				}

				ForwardWithinLine(sc, 2);
				break;

			case SCE_HEX_CHECKSUM:
			case SCE_HEX_CHECKSUM_WRONG:
			case SCE_HEX_GARBAGE:
				// record finished or line too long
				sc.SetState(SCE_HEX_GARBAGE);
				ForwardWithinLine(sc);
				break;

			default:
				// prevent endless loop in faulty state
				sc.SetState(SCE_HEX_DEFAULT);
				break;
		}
	}
	sc.Complete();
}

static void FoldIHexDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *[], Accessor &styler)
{
	Sci_PositionU endPos = startPos + length;

	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelCurrent = SC_FOLDLEVELBASE;
	if (lineCurrent > 0)
		levelCurrent = styler.LevelAt(lineCurrent - 1);

	Sci_PositionU lineStartNext = styler.LineStart(lineCurrent + 1);
	int levelNext = SC_FOLDLEVELBASE; // default if no specific line found

	for (Sci_PositionU i = startPos; i < endPos; i++) {
		bool atEOL = i == (lineStartNext - 1);
		int style = styler.StyleAt(i);

		// search for specific lines
		if (style == SCE_HEX_EXTENDEDADDRESS) {
			// extended addres record
			levelNext = SC_FOLDLEVELBASE | SC_FOLDLEVELHEADERFLAG;
		} else if (style == SCE_HEX_DATAADDRESS
			|| (style == SCE_HEX_DEFAULT
				&& i == (Sci_PositionU)styler.LineStart(lineCurrent))) {
			// data record or no record start code at all
			if (levelCurrent & SC_FOLDLEVELHEADERFLAG) {
				levelNext = SC_FOLDLEVELBASE + 1;
			} else {
				// continue level 0 or 1, no fold point
				levelNext = levelCurrent;
			}
		}

		if (atEOL || (i == endPos - 1)) {
			styler.SetLevel(lineCurrent, levelNext);

			lineCurrent++;
			lineStartNext = styler.LineStart(lineCurrent + 1);
			levelCurrent = levelNext;
			levelNext = SC_FOLDLEVELBASE;
		}
	}
}

static void ColouriseTEHexDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *[], Accessor &styler)
{
	StyleContext sc(startPos, length, initStyle, styler);

	while (sc.More()) {
		Sci_PositionU recStartPos;
		int digitCount, addrFieldType;
		int cs1, cs2;

		switch (sc.state) {
			case SCE_HEX_DEFAULT:
				if (sc.atLineStart && sc.Match('%')) {
					sc.SetState(SCE_HEX_RECSTART);
				}
				ForwardWithinLine(sc);
				break;

			case SCE_HEX_RECSTART:

				recStartPos = sc.currentPos - 1;

				if (GetTEHexDigitCount(recStartPos, styler) == CountTEHexDigitCount(recStartPos, styler)) {
					sc.SetState(SCE_HEX_BYTECOUNT);
				} else {
					sc.SetState(SCE_HEX_BYTECOUNT_WRONG);
				}

				ForwardWithinLine(sc, 2);
				break;

			case SCE_HEX_BYTECOUNT:
			case SCE_HEX_BYTECOUNT_WRONG:
				recStartPos = sc.currentPos - 3;
				addrFieldType = GetTEHexAddressFieldType(recStartPos, styler);

				if (addrFieldType == SCE_HEX_ADDRESSFIELD_UNKNOWN) {
					sc.SetState(SCE_HEX_RECTYPE_UNKNOWN);
				} else {
					sc.SetState(SCE_HEX_RECTYPE);
				}

				ForwardWithinLine(sc);
				break;

			case SCE_HEX_RECTYPE:
			case SCE_HEX_RECTYPE_UNKNOWN:
				recStartPos = sc.currentPos - 4;
				cs1 = CalcTEHexChecksum(recStartPos, styler);
				cs2 = GetTEHexChecksum(recStartPos, styler);

				if (cs1 != cs2 || cs1 < 0 || cs2 < 0) {
					sc.SetState(SCE_HEX_CHECKSUM_WRONG);
				} else {
					sc.SetState(SCE_HEX_CHECKSUM);
				}

				ForwardWithinLine(sc, 2);
				break;


			case SCE_HEX_CHECKSUM:
			case SCE_HEX_CHECKSUM_WRONG:
				recStartPos = sc.currentPos - 6;
				addrFieldType = GetTEHexAddressFieldType(recStartPos, styler);

				sc.SetState(addrFieldType);
				ForwardWithinLine(sc, 9);
				break;

			case SCE_HEX_DATAADDRESS:
			case SCE_HEX_STARTADDRESS:
			case SCE_HEX_ADDRESSFIELD_UNKNOWN:
				recStartPos = sc.currentPos - 15;
				digitCount = GetTEHexDigitCount(recStartPos, styler) - 14;

				sc.SetState(SCE_HEX_DATA_ODD);

				for (int i = 0; i < digitCount; i++) {
					if ((i & 0x3) == 0) {
						sc.SetState(SCE_HEX_DATA_ODD);
					} else if ((i & 0x3) == 2) {
						sc.SetState(SCE_HEX_DATA_EVEN);
					}

					if (!ForwardWithinLine(sc)) {
						break;
					}
				}
				break;

			case SCE_HEX_DATA_ODD:
			case SCE_HEX_DATA_EVEN:
			case SCE_HEX_GARBAGE:
				// record finished or line too long
				sc.SetState(SCE_HEX_GARBAGE);
				ForwardWithinLine(sc);
				break;

			default:
				// prevent endless loop in faulty state
				sc.SetState(SCE_HEX_DEFAULT);
				break;
		}
	}
	sc.Complete();
}

LexerModule lmSrec(SCLEX_SREC, ColouriseSrecDoc, "srec", 0, NULL);
LexerModule lmIHex(SCLEX_IHEX, ColouriseIHexDoc, "ihex", FoldIHexDoc, NULL);
LexerModule lmTEHex(SCLEX_TEHEX, ColouriseTEHexDoc, "tehex", 0, NULL);
