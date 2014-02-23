/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Universal charset detector code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *          Shy Shalom <shooshX@gmail.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
 
#include "nsCharSetProber.h"
#include "prmem.h"

//This filter applies to all scripts which do not use English characters
PRBool nsCharSetProber::FilterWithoutEnglishLetters(const char* aBuf, PRUint32 aLen, char** newBuf, PRUint32& newLen)
{
  char *newptr;
  char *prevPtr, *curPtr;
  
  PRBool meetMSB = PR_FALSE;   
  newptr = *newBuf = (char*)PR_Malloc(aLen);
  if (!newptr)
    return PR_FALSE;

  for (curPtr = prevPtr = (char*)aBuf; curPtr < aBuf+aLen; curPtr++)
  {
    if (*curPtr & 0x80)
    {
      meetMSB = PR_TRUE;
    }
    else if (*curPtr < 'A' || (*curPtr > 'Z' && *curPtr < 'a') || *curPtr > 'z') 
    {
      //current char is a symbol, most likely a punctuation. we treat it as segment delimiter
      if (meetMSB && curPtr > prevPtr) 
      //this segment contains more than single symbol, and it has upper ASCII, we need to keep it
      {
        while (prevPtr < curPtr) *newptr++ = *prevPtr++;  
        prevPtr++;
        *newptr++ = ' ';
        meetMSB = PR_FALSE;
      }
      else //ignore current segment. (either because it is just a symbol or just an English word)
        prevPtr = curPtr+1;
    }
  }
  if (meetMSB && curPtr > prevPtr) 
    while (prevPtr < curPtr) *newptr++ = *prevPtr++;  

  newLen = newptr - *newBuf;

  return PR_TRUE;
}

//This filter applies to all scripts which contain both English characters and upper ASCII characters.
PRBool nsCharSetProber::FilterWithEnglishLetters(const char* aBuf, PRUint32 aLen, char** newBuf, PRUint32& newLen)
{
  //do filtering to reduce load to probers
  char *newptr;
  char *prevPtr, *curPtr;
  PRBool isInTag = PR_FALSE;

  newptr = *newBuf = (char*)PR_Malloc(aLen);
  if (!newptr)
    return PR_FALSE;

  for (curPtr = prevPtr = (char*)aBuf; curPtr < aBuf+aLen; curPtr++)
  {
    if (*curPtr == '>')
      isInTag = PR_FALSE;
    else if (*curPtr == '<')
      isInTag = PR_TRUE;

    if (!(*curPtr & 0x80) &&
        (*curPtr < 'A' || (*curPtr > 'Z' && *curPtr < 'a') || *curPtr > 'z') )
    {
      if (curPtr > prevPtr && !isInTag) // Current segment contains more than just a symbol 
                                        // and it is not inside a tag, keep it.
      {
        while (prevPtr < curPtr) *newptr++ = *prevPtr++;  
        prevPtr++;
        *newptr++ = ' ';
      }
      else
        prevPtr = curPtr+1;
    }
  }

  // If the current segment contains more than just a symbol 
  // and it is not inside a tag then keep it.
  if (!isInTag)
    while (prevPtr < curPtr)
      *newptr++ = *prevPtr++;  

  newLen = newptr - *newBuf;

  return PR_TRUE;
}
