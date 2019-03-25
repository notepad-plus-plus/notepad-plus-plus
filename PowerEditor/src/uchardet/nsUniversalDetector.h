/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#ifndef nsUniversalDetector_h__
#define nsUniversalDetector_h__

class nsCharSetProber;

#define NUM_OF_CHARSET_PROBERS  3

typedef enum {
  ePureAscii = 0,
  eEscAscii  = 1,
  eHighbyte  = 2
} nsInputState;

#define NS_FILTER_CHINESE_SIMPLIFIED  0x01
#define NS_FILTER_CHINESE_TRADITIONAL 0x02
#define NS_FILTER_JAPANESE            0x04
#define NS_FILTER_KOREAN              0x08
#define NS_FILTER_NON_CJK             0x10
#define NS_FILTER_ALL                 0x1F
#define NS_FILTER_CHINESE (NS_FILTER_CHINESE_SIMPLIFIED | \
                           NS_FILTER_CHINESE_TRADITIONAL)
#define NS_FILTER_CJK (NS_FILTER_CHINESE_SIMPLIFIED | \
                       NS_FILTER_CHINESE_TRADITIONAL | \
                       NS_FILTER_JAPANESE | \
                       NS_FILTER_KOREAN)

class nsUniversalDetector {
public:
   nsUniversalDetector(PRUint32 aLanguageFilter);
   virtual ~nsUniversalDetector();
   virtual nsresult HandleData(const char* aBuf, PRUint32 aLen);
   virtual void DataEnd(void);

protected:
   virtual void Report(const char* aCharset) = 0;
   virtual void Reset();
   nsInputState  mInputState;
   PRBool  mDone;
   PRBool  mInTag;
   PRBool  mStart;
   PRBool  mGotData;
   char    mLastChar;
   const char *  mDetectedCharset;
   PRInt32 mBestGuess;
   PRUint32 mLanguageFilter;

   nsCharSetProber  *mCharSetProbers[NUM_OF_CHARSET_PROBERS];
   nsCharSetProber  *mEscCharSetProber;
};

#endif

