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

#include "nsLatin1Prober.h"
#include "prmem.h"
#include <stdio.h>

#define UDF    0        // undefined
#define OTH    1        //other
#define ASC    2        // ascii capital letter
#define ASS    3        // ascii small letter
#define ACV    4        // accent capital vowel
#define ACO    5        // accent capital other
#define ASV    6        // accent small vowel
#define ASO    7        // accent small other
#define CLASS_NUM   8    // total classes

static const unsigned char Latin1_CharToClass[] = 
{
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,   // 00 - 07
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,   // 08 - 0F
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,   // 10 - 17
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,   // 18 - 1F
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,   // 20 - 27
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,   // 28 - 2F
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,   // 30 - 37
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,   // 38 - 3F
  OTH, ASC, ASC, ASC, ASC, ASC, ASC, ASC,   // 40 - 47
  ASC, ASC, ASC, ASC, ASC, ASC, ASC, ASC,   // 48 - 4F
  ASC, ASC, ASC, ASC, ASC, ASC, ASC, ASC,   // 50 - 57
  ASC, ASC, ASC, OTH, OTH, OTH, OTH, OTH,   // 58 - 5F
  OTH, ASS, ASS, ASS, ASS, ASS, ASS, ASS,   // 60 - 67
  ASS, ASS, ASS, ASS, ASS, ASS, ASS, ASS,   // 68 - 6F
  ASS, ASS, ASS, ASS, ASS, ASS, ASS, ASS,   // 70 - 77
  ASS, ASS, ASS, OTH, OTH, OTH, OTH, OTH,   // 78 - 7F
  OTH, UDF, OTH, ASO, OTH, OTH, OTH, OTH,   // 80 - 87
  OTH, OTH, ACO, OTH, ACO, UDF, ACO, UDF,   // 88 - 8F
  UDF, OTH, OTH, OTH, OTH, OTH, OTH, OTH,   // 90 - 97
  OTH, OTH, ASO, OTH, ASO, UDF, ASO, ACO,   // 98 - 9F
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,   // A0 - A7
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,   // A8 - AF
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,   // B0 - B7
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,   // B8 - BF
  ACV, ACV, ACV, ACV, ACV, ACV, ACO, ACO,   // C0 - C7
  ACV, ACV, ACV, ACV, ACV, ACV, ACV, ACV,   // C8 - CF
  ACO, ACO, ACV, ACV, ACV, ACV, ACV, OTH,   // D0 - D7
  ACV, ACV, ACV, ACV, ACV, ACO, ACO, ACO,   // D8 - DF
  ASV, ASV, ASV, ASV, ASV, ASV, ASO, ASO,   // E0 - E7
  ASV, ASV, ASV, ASV, ASV, ASV, ASV, ASV,   // E8 - EF
  ASO, ASO, ASV, ASV, ASV, ASV, ASV, OTH,   // F0 - F7
  ASV, ASV, ASV, ASV, ASV, ASO, ASO, ASO,   // F8 - FF
};


/* 0 : illegal 
   1 : very unlikely 
   2 : normal 
   3 : very likely
*/
static const unsigned char Latin1ClassModel[] = 
{
/*      UDF OTH ASC ASS ACV ACO ASV ASO  */
/*UDF*/  0,  0,  0,  0,  0,  0,  0,  0,
/*OTH*/  0,  3,  3,  3,  3,  3,  3,  3,
/*ASC*/  0,  3,  3,  3,  3,  3,  3,  3, 
/*ASS*/  0,  3,  3,  3,  1,  1,  3,  3,
/*ACV*/  0,  3,  3,  3,  1,  2,  1,  2,
/*ACO*/  0,  3,  3,  3,  3,  3,  3,  3, 
/*ASV*/  0,  3,  1,  3,  1,  1,  1,  3, 
/*ASO*/  0,  3,  1,  3,  1,  1,  3,  3,
};

void  nsLatin1Prober::Reset(void)
{
  mState = eDetecting;
  mLastCharClass = OTH;
  for (int i = 0; i < FREQ_CAT_NUM; i++)
    mFreqCounter[i] = 0;
}


nsProbingState nsLatin1Prober::HandleData(const char* aBuf, PRUint32 aLen)
{
  char *newBuf1 = 0;
  PRUint32 newLen1 = 0;

  if (!FilterWithEnglishLetters(aBuf, aLen, &newBuf1, newLen1)) {
    newBuf1 = (char*)aBuf;
    newLen1 = aLen;
  }
  
  unsigned char charClass;
  unsigned char freq;
  for (PRUint32 i = 0; i < newLen1; i++)
  {
    charClass = Latin1_CharToClass[(unsigned char)newBuf1[i]];
    freq = Latin1ClassModel[mLastCharClass*CLASS_NUM + charClass];
    if (freq == 0) {
      mState = eNotMe;
      break;
    }
    mFreqCounter[freq]++;
    mLastCharClass = charClass;
  }

  if (newBuf1 != aBuf)
    PR_FREEIF(newBuf1);

  return mState;
}

float nsLatin1Prober::GetConfidence(void)
{
  if (mState == eNotMe)
    return 0.01f;
  
  float confidence;
  PRUint32 total = 0;
  for (PRInt32 i = 0; i < FREQ_CAT_NUM; i++)
    total += mFreqCounter[i];

  if (!total)
    confidence = 0.0f;
  else
  {
    const float floatTotal = static_cast<float>(total);
    confidence = static_cast<float>(mFreqCounter[3]) / floatTotal;
    confidence -= static_cast<float>(mFreqCounter[1]) * 20.0f / floatTotal;
  }

  if (confidence < 0.0f)
    confidence = 0.0f;
  
  // lower the confidence of latin1 so that other more accurate detector 
  // can take priority.
  confidence *= 0.50f;

  return confidence;
}

#ifdef DEBUG_chardet
void  nsLatin1Prober::DumpStatus()
{
  printf(" Latin1Prober: %1.3f [%s]\r\n", GetConfidence(), GetCharSetName());
}
#endif


