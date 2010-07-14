/*******************************************************************************

Copyright (c) 2007 Adobe Systems Incorporated

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

********************************************************************************/

#include "ScintillaMacOSX.h"
#include "ExtInput.h"

using namespace Scintilla;

// uncomment this for a log to /dev/console
// #define LOG_TSM 1

#if LOG_TSM
FILE* logFile = NULL;
#endif

static EventHandlerUPP tsmHandler;

static EventTypeSpec	tsmSpecs[] = {
	{ kEventClassTextInput, kEventTextInputUpdateActiveInputArea },
//	{ kEventClassTextInput, kEventTextInputUnicodeForKeyEvent },
	{ kEventClassTextInput, kEventTextInputOffsetToPos },
	{ kEventClassTextInput, kEventTextInputPosToOffset },
	{ kEventClassTextInput, kEventTextInputGetSelectedText }
};

#define kScintillaTSM	'ScTs'

// The following structure is attached to the HIViewRef as property kScintillaTSM

struct TSMData
{
	HIViewRef			view;				// this view
	TSMDocumentID		docid;				// the TSM document ID
	EventHandlerRef		handler;			// the event handler
	ScintillaMacOSX*	scintilla;			// the Scintilla pointer
	int					styleMask;			// the document style mask
	int					indicStyle [3];		// indicator styles save
	int					indicColor [3];		// indicator colors save
	int					selStart;			// starting position of selection (Scintilla offset)
	int					selLength;			// UTF-8 number of characters
	int					selCur;				// current position (Scintilla offset)
	int					inhibitRecursion;	// true to stop recursion
	bool				active;				// true if this is active
};

static const int numSpecs = 5;


// Fetch a range of text as UTF-16; delete the buffer after use

static char* getTextPortion (TSMData* data, UInt32 start, UInt32 size)
{
	Scintilla::TextRange range;
	range.chrg.cpMin = start;
	range.chrg.cpMax = start + size;
	range.lpstrText = new char [size + 1];
	range.lpstrText [size] = 0;
	data->scintilla->WndProc (SCI_GETTEXTRANGE, 0, (uptr_t) &range);
	return range.lpstrText;
}

static pascal OSStatus doHandleTSM (EventHandlerCallRef, EventRef inEvent, void* userData);

void ExtInput::attach (HIViewRef viewRef)
{
	if (NULL == tsmHandler)
		tsmHandler = NewEventHandlerUPP (doHandleTSM);
	::UseInputWindow (NULL, FALSE);

#ifdef LOG_TSM
	if (NULL == logFile)
		logFile = fopen ("/dev/console", "a");
#endif

	// create and attach the TSM data
	TSMData* data = new TSMData;

	data->view				= viewRef;
	data->active			= false;
	data->inhibitRecursion	= 0;
	
	::GetControlProperty (viewRef, scintillaMacOSType, 0, sizeof( data->scintilla ), NULL, &data->scintilla);
	
	if (NULL != data->scintilla)
	{
		// create the TSM document ref
		InterfaceTypeList interfaceTypes;
		interfaceTypes[0] = kUnicodeDocumentInterfaceType;
		::NewTSMDocument (1, interfaceTypes, &data->docid, (long) viewRef);
		// install my event handler
		::InstallControlEventHandler (viewRef, tsmHandler, numSpecs, tsmSpecs, data, &data->handler);

		::SetControlProperty (viewRef, kScintillaTSM, 0, sizeof (data), &data);
	}
	else
		delete data;
}

static TSMData* getTSMData (HIViewRef viewRef)
{
	TSMData* data = NULL;
	UInt32 n;
	::GetControlProperty (viewRef, kScintillaTSM, 0, sizeof (data), &n, (UInt32*) &data);
	return data;
}

void ExtInput::detach (HIViewRef viewRef)
{
	TSMData* data = getTSMData (viewRef);
	if (NULL != data)
	{
		::DeleteTSMDocument (data->docid);
		::RemoveEventHandler (data->handler);
		delete data;
	}
}

void ExtInput::activate (HIViewRef viewRef, bool on)
{
	TSMData* data = getTSMData (viewRef);
	if (NULL == data)
		return;

	if (on)
	{
		::ActivateTSMDocument (data->docid);
		HIRect bounds;
		::HIViewGetBounds (viewRef, &bounds);
		::HIViewConvertRect (&bounds, viewRef, NULL);
		RgnHandle hRgn = ::NewRgn();
		::SetRectRgn (hRgn, (short) bounds.origin.x, (short) bounds.origin.y, 
							(short) (bounds.origin.x + bounds.size.width), 
							(short) (bounds.origin.y + bounds.size.height));
#if LOG_TSM
		fprintf (logFile, "TSMSetInlineInputRegion (%08lX, %ld:%ld-%ld:%ld)\n", 
						 (long) data->docid, (long) bounds.origin.x, (long) bounds.origin.y,
						 (long) (bounds.origin.x + bounds.size.width), (long) (bounds.origin.y + bounds.size.height));
		fflush (logFile);
#endif
		::TSMSetInlineInputRegion (data->docid, HIViewGetWindow (data->view), hRgn);
		::DisposeRgn (hRgn);
		::UseInputWindow (NULL, FALSE);
	}
	else
	{
#if LOG_TSM
		fprintf (logFile, "DeactivateTSMDocument (%08lX)\n", (long) data->docid);
		fflush (logFile);
#endif
		::DeactivateTSMDocument (data->docid);
	}
}

static void startInput (TSMData* data, bool delSelection = true)
{
	if (!data->active && 0 == data->inhibitRecursion)
	{
		data->active = true;
		
		// Delete any selection
		if( delSelection )
			data->scintilla->WndProc (SCI_REPLACESEL, 0, reinterpret_cast<sptr_t>(""));
		 
		// need all style bits because we do indicators
		data->styleMask = data->scintilla->WndProc (SCI_GETSTYLEBITS, 0, 0);
		data->scintilla->WndProc (SCI_SETSTYLEBITS, 5, 0);
		
		// Set the target range for successive replacements
		data->selStart	=
		data->selCur	= data->scintilla->WndProc (SCI_GETCURRENTPOS, 0, 0);
		data->selLength	= 0;
		
		// save needed styles
		for (int i = 0; i < 2; i++)
		{
			data->indicStyle [i] = data->scintilla->WndProc (SCI_INDICGETSTYLE, i, 0);
			data->indicColor [i] = data->scintilla->WndProc (SCI_INDICGETFORE, i, 0);
		}
		// set styles and colors
		data->scintilla->WndProc (SCI_INDICSETSTYLE, 0, INDIC_SQUIGGLE);
		data->scintilla->WndProc (SCI_INDICSETFORE,  0, 0x808080);
		data->scintilla->WndProc (SCI_INDICSETSTYLE, 1, INDIC_PLAIN);	// selected converted
		data->scintilla->WndProc (SCI_INDICSETFORE,  1, 0x808080);
		data->scintilla->WndProc (SCI_INDICSETSTYLE, 2, INDIC_PLAIN);	// selected raw
		data->scintilla->WndProc (SCI_INDICSETFORE,  2, 0x0000FF);
		// stop Undo
		data->scintilla->WndProc (SCI_BEGINUNDOACTION, 0, 0);
	}
}

static void stopInput (TSMData* data, int pos)
{
	if (data->active && 0 == data->inhibitRecursion)
	{
		// First fix the doc - this may cause more messages
		// but do not fall into recursion
		data->inhibitRecursion++;
		::FixTSMDocument (data->docid);
		data->inhibitRecursion--;
		data->active = false;
		
		// Remove indicator styles
		data->scintilla->WndProc (SCI_STARTSTYLING, data->selStart, INDICS_MASK);
		data->scintilla->WndProc (SCI_SETSTYLING, pos - data->selStart, 0);
		// Restore old indicator styles and colors
		data->scintilla->WndProc (SCI_SETSTYLEBITS, data->styleMask, 0);
		for (int i = 0; i < 2; i++)
		{
			data->scintilla->WndProc (SCI_INDICSETSTYLE, i, data->indicStyle [i]);
			data->scintilla->WndProc (SCI_INDICSETFORE, i, data->indicColor [i]);
		}

		// remove selection and re-allow selections to display
		data->scintilla->WndProc (SCI_SETSEL, pos, pos);
		data->scintilla->WndProc (SCI_TARGETFROMSELECTION, 0, 0);
		data->scintilla->WndProc (SCI_HIDESELECTION, 0, 0);

		// move the caret behind the current area
		data->scintilla->WndProc (SCI_SETCURRENTPOS, pos, 0);
		// re-enable Undo
		data->scintilla->WndProc (SCI_ENDUNDOACTION, 0, 0);
		// re-colorize
		int32_t startLine = data->scintilla->WndProc (SCI_LINEFROMPOSITION, data->selStart, 0);
		int32_t startPos  = data->scintilla->WndProc (SCI_POSITIONFROMLINE, startLine, 0);
		int32_t endLine   = data->scintilla->WndProc (SCI_LINEFROMPOSITION, pos, 0);
		if (endLine == startLine)
			endLine++;
		int32_t endPos    = data->scintilla->WndProc (SCI_POSITIONFROMLINE, endLine, 0);
		
		data->scintilla->WndProc (SCI_COLOURISE, startPos, endPos);
	}
}

void ExtInput::stop (HIViewRef viewRef)
{
	TSMData* data = getTSMData (viewRef);
	if (NULL != data)
		stopInput (data, data->selStart + data->selLength);
}

static char* UTF16toUTF8 (const UniChar* buf, int len, int& utf8len)
{
	CFStringRef str = CFStringCreateWithCharactersNoCopy (NULL, buf, (UInt32) len, kCFAllocatorNull);
	CFRange range = { 0, len };
	CFIndex bufLen;
	CFStringGetBytes (str, range, kCFStringEncodingUTF8, '?', false, NULL, 0, &bufLen);
	UInt8* utf8buf = new UInt8 [bufLen+1];
	CFStringGetBytes (str, range, kCFStringEncodingUTF8, '?', false, utf8buf, bufLen, NULL);
	utf8buf [bufLen] = 0;
	CFRelease (str);
	utf8len = (int) bufLen;
	return (char*) utf8buf;
}

static int UCS2Length (const char* buf, int len)
{
	int n = 0;
	while (len > 0)
	{
		int bytes = 0;
		char ch = *buf;
		while (ch & 0x80)
			bytes++, ch <<= 1;
		len -= bytes;
		n += bytes;
	}
	return n;
}

static int UTF8Length (const UniChar* buf, int len)
{
	int n = 0;
	while (len > 0)
	{
		UInt32 uch = *buf++;
		len--;
		if (uch >= 0xD800 && uch <= 0xDBFF && len > 0)
		{
			UInt32 uch2 = *buf;
			if (uch2 >= 0xDC00 && uch2 <= 0xDFFF)
			{
				buf++;
				len--;
				uch = ((uch & 0x3FF) << 10) + (uch2 & 0x3FF);
			}
		}
		n++;
		if (uch > 0x7F)
			n++;
		if (uch > 0x7FF)
			n++;
		if (uch > 0xFFFF)
			n++;
		if (uch > 0x1FFFFF)
			n++;
		if (uch > 0x3FFFFFF)
			n++;
	}
	return n;
}

static OSStatus handleTSMUpdateActiveInputArea (TSMData* data, EventRef inEvent)
{
	UInt32				fixLength;
	int					caretPos = -1;
	UInt32				actualSize;
	::TextRangeArray*	hiliteRanges = NULL;
	char*				hiliteBuffer = NULL;
	bool				done;
	
	// extract the text
	UniChar* buffer = NULL;
	UniChar temp [128];
	UniChar* text = temp;
	
	// get the fix length (in bytes)
	OSStatus err = ::GetEventParameter (inEvent, kEventParamTextInputSendFixLen,
								   typeLongInteger, NULL, sizeof (long), NULL, &fixLength);
	// need the size (in bytes)
	if (noErr == err)
		err = ::GetEventParameter (inEvent, kEventParamTextInputSendText,
										typeUnicodeText, NULL, 256, &actualSize, temp);
										
	// then allocate and fetch if necessary
	UInt32 textLength = actualSize / sizeof (UniChar);
	fixLength /= sizeof (UniChar);

	if (noErr == err)
	{
		// this indicates that we are completely done
		done = (fixLength == textLength || fixLength < 0);
		if (textLength >= 128)
		{
			buffer = text = new UniChar [textLength];
			err = ::GetEventParameter (inEvent, kEventParamTextInputSendText,
									   typeUnicodeText, NULL, actualSize, NULL, (void*) text);
		}
		
		// set the text now, but convert it to UTF-8 first
		int utf8len;
		char* utf8 = UTF16toUTF8 (text, textLength, utf8len);
		data->scintilla->WndProc (SCI_SETTARGETSTART, data->selStart, 0);
		data->scintilla->WndProc (SCI_SETTARGETEND, data->selStart + data->selLength, 0);
		data->scintilla->WndProc (SCI_HIDESELECTION, 1, 0);
		data->scintilla->WndProc (SCI_REPLACETARGET, utf8len, (sptr_t) utf8);
		data->selLength = utf8len;
		delete [] utf8;
	}
	
	// attempt to extract the array of hilite ranges
	if (noErr == err)
	{
		::TextRangeArray tempTextRangeArray;
		OSStatus tempErr = ::GetEventParameter (inEvent, kEventParamTextInputSendHiliteRng,
											 typeTextRangeArray, NULL, sizeof (::TextRangeArray), &actualSize, &tempTextRangeArray);
		if (noErr == tempErr)
		{
			// allocate memory and get the stuff!
			hiliteBuffer = new char [actualSize];
			hiliteRanges = (::TextRangeArray*) hiliteBuffer;
			err = ::GetEventParameter (inEvent, kEventParamTextInputSendHiliteRng,
									   typeTextRangeArray, NULL, actualSize, NULL, hiliteRanges);
			if (noErr != err)
			{
				delete [] hiliteBuffer;
				hiliteBuffer = NULL;
				hiliteRanges = NULL;
			}
		}
	}
#if LOG_TSM
	fprintf (logFile, "kEventTextInputUpdateActiveInputArea:\n"
					 "  TextLength = %ld\n"
					 "  FixLength = %ld\n",
					 (long) textLength, (long) fixLength);
	fflush (logFile);
#endif

	if (NULL != hiliteRanges)
	{
		for (int i = 0; i < hiliteRanges->fNumOfRanges; i++)
		{
#if LOG_TSM
			fprintf (logFile, "  Range #%d: %ld-%ld (%d)\n",
								i+1,
								hiliteRanges->fRange[i].fStart,
								hiliteRanges->fRange[i].fEnd,
								hiliteRanges->fRange[i].fHiliteStyle);
			fflush (logFile);
#endif
			// start and end of range, zero based
			long bgn = long (hiliteRanges->fRange[i].fStart) / sizeof (UniChar);
			long end = long (hiliteRanges->fRange[i].fEnd) / sizeof (UniChar);
			if (bgn >= 0 && end >= 0)
			{
				// move the caret if this is requested
				if (hiliteRanges->fRange[i].fHiliteStyle == kTSMHiliteCaretPosition)
					caretPos = bgn;
				else
				{
					// determine which style to use
					int style;
					switch (hiliteRanges->fRange[i].fHiliteStyle)
					{
						case kTSMHiliteRawText:					style = INDIC0_MASK; break;
						case kTSMHiliteSelectedRawText:			style = INDIC0_MASK; break;
						case kTSMHiliteConvertedText:			style = INDIC1_MASK; break;
						case kTSMHiliteSelectedConvertedText:	style = INDIC2_MASK; break;
						default:								style = INDIC0_MASK;
					}
					// bgn and end are Unicode offsets from the starting pos
					// use the text buffer to determine the UTF-8 offsets
					long utf8bgn  = data->selStart + UTF8Length (text, bgn);
					long utf8size = UTF8Length (text + bgn, end - bgn);
					// set indicators
					int oldEnd = data->scintilla->WndProc (SCI_GETENDSTYLED, 0, 0);
					data->scintilla->WndProc (SCI_STARTSTYLING, utf8bgn, INDICS_MASK);
					data->scintilla->WndProc (SCI_SETSTYLING, utf8size, style & ~1);
					data->scintilla->WndProc (SCI_STARTSTYLING, oldEnd, 31);
				}
			}
		}
	}
	if (noErr == err)
	{
		// if the fixed length is == to the new text, we are done
		if (done)
			stopInput (data, data->selStart + UTF8Length (text, textLength));
		else if (caretPos >= 0)
		{
			data->selCur = data->selStart + UTF8Length (text, caretPos);
			data->scintilla->WndProc (SCI_SETCURRENTPOS, data->selCur, 0);
		}
	}
	
	delete [] hiliteBuffer;
	delete [] buffer;
	return err;
}

struct MacPoint {
  short               v;
  short               h;
};

static OSErr handleTSMOffset2Pos (TSMData* data, EventRef inEvent)
{
	long offset;
	
	// get the offfset to convert
	OSStatus err = ::GetEventParameter (inEvent, kEventParamTextInputSendTextOffset,
										typeLongInteger, NULL, sizeof (long), NULL, &offset);
	if (noErr == err)
	{
		// where is the caret now?
		HIPoint where;

		int line = (int) data->scintilla->WndProc (SCI_LINEFROMPOSITION, data->selCur, 0);
		where.x = data->scintilla->WndProc (SCI_POINTXFROMPOSITION, 0, data->selCur);
		where.y = data->scintilla->WndProc (SCI_POINTYFROMPOSITION, 0, data->selCur)
				+ data->scintilla->WndProc (SCI_TEXTHEIGHT, line, 0);
		// convert to window coords
		::HIViewConvertPoint (&where, data->view, NULL);
		// convert to screen coords
		Rect global;
		GetWindowBounds (HIViewGetWindow (data->view), kWindowStructureRgn, &global);
		MacPoint pt;
		pt.h = (short) where.x + global.left;
		pt.v = (short) where.y + global.top;

		// set the result
		err = ::SetEventParameter (inEvent, kEventParamTextInputReplyPoint, typeQDPoint, sizeof (MacPoint), &pt);
#if LOG_TSM
		fprintf (logFile, "kEventTextInputOffsetToPos:\n"
						 "  Offset: %ld\n"
						 "  Pos: %ld:%ld (orig = %ld:%ld)\n", offset, 
						 (long) pt.h, (long) pt.v,
						 (long) where.x, (long) where.y);
		fflush (logFile);
#endif
	}
	return err;
}

static OSErr handleTSMPos2Offset (TSMData* data, EventRef inEvent)
{
	MacPoint		qdPosition;
	long			offset;
	short			regionClass;
	
	// retrieve the global point to convert
	OSStatus err = ::GetEventParameter (inEvent, kEventParamTextInputSendCurrentPoint,
										typeQDPoint, NULL, sizeof (MacPoint), NULL, &qdPosition);
	if (noErr == err)
	{
#if LOG_TSM
		fprintf (logFile, "kEventTextInputPosToOffset:\n"
						 "  Pos: %ld:%ld\n", (long) qdPosition.v, (long) qdPosition.h);
		fflush (logFile);
#endif
		// convert to local coordinates
		HIRect rect;
		rect.origin.x = qdPosition.h;
		rect.origin.y = qdPosition.v;
		rect.size.width =
		rect.size.height = 1;
		::HIViewConvertRect (&rect, NULL, data->view);

		// we always report the position to be within the composition;
		// coords inside the same pane are clipped to the composition,
		// and if the position is outside, then we deactivate this instance
		// this leaves the edit open and active so we can edit multiple panes
		regionClass = kTSMInsideOfActiveInputArea;
		
		// compute the offset (relative value)
		offset = data->scintilla->WndProc (SCI_POSITIONFROMPOINTCLOSE, (uptr_t) rect.origin.x, (sptr_t) rect.origin.y);
		if (offset >= 0)
		{
			// convert to a UTF-16 offset (Brute Force)
			char* buf = getTextPortion (data, 0, offset);
			offset = UCS2Length (buf, offset);
			delete [] buf;
			
#if LOG_TSM
			fprintf (logFile, "  Offset: %ld (class %ld)\n", offset, (long) regionClass);
			fflush (logFile);
#endif
			// store the offset
			err = ::SetEventParameter (inEvent, kEventParamTextInputReplyTextOffset, typeLongInteger, sizeof (long), &offset);
			if (noErr == err)
				err = ::SetEventParameter (inEvent, kEventParamTextInputReplyRegionClass, typeShortInteger, sizeof (short), &regionClass);
		}
		else
		{
			// not this pane!
			err = eventNotHandledErr;
			ExtInput::activate (data->view, false);
		}
			
	}
	return err;	
}

static OSErr handleTSMGetText (TSMData* data, EventRef inEvent)
{
	char* buf = getTextPortion (data, data->selStart, data->selLength);
		
#if LOG_TSM
		fprintf (logFile, "kEventTextInputGetSelectedText:\n"
						  "  Text: \"%s\"\n", buf);
		fflush (logFile);
#endif
	OSStatus status = ::SetEventParameter (inEvent, kEventParamTextInputReplyText, typeUTF8Text, data->selLength, buf);
	delete [] buf;
	return status;
}		

static pascal OSStatus doHandleTSM (EventHandlerCallRef, EventRef inEvent, void* userData)
{
	TSMData* data = (TSMData*) userData;
	
	OSStatus err = eventNotHandledErr;

	switch (::GetEventKind (inEvent))
	{
		case kEventTextInputUpdateActiveInputArea:
			// Make sure that input has been started
			startInput (data);
			err = handleTSMUpdateActiveInputArea (data, inEvent);
			break;
//		case kEventTextInputUnicodeForKeyEvent:
//			err = handleTSMUnicodeInput (inEvent);
//			break;
		case kEventTextInputOffsetToPos:
			err = handleTSMOffset2Pos (data, inEvent);
			break;
		case kEventTextInputPosToOffset:
			err = handleTSMPos2Offset (data, inEvent);
			break;
		case kEventTextInputGetSelectedText:
			// Make sure that input has been started
			startInput (data, false);
			err = handleTSMGetText (data, inEvent);
			break;
	}
	return err;
}

