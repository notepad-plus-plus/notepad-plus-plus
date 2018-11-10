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

#include "nscore.h"

#include "nsUniversalDetector.h"

#include "nsMBCSGroupProber.h"
#include "nsSBCSGroupProber.h"
#include "nsEscCharsetProber.h"
#include "nsLatin1Prober.h"

nsUniversalDetector::nsUniversalDetector(PRUint32 aLanguageFilter)
{
  mNbspFound = PR_FALSE;
  mDone = PR_FALSE;
  mBestGuess = -1;   //illegal value as signal
  mInTag = PR_FALSE;
  mEscCharSetProber = nsnull;

  mStart = PR_TRUE;
  mDetectedCharset = nsnull;
  mGotData = PR_FALSE;
  mInputState = ePureAscii;
  mLastChar = '\0';
  mLanguageFilter = aLanguageFilter;

  PRUint32 i;
  for (i = 0; i < NUM_OF_CHARSET_PROBERS; i++)
    mCharSetProbers[i] = nsnull;
}

nsUniversalDetector::~nsUniversalDetector()
{
  for (PRInt32 i = 0; i < NUM_OF_CHARSET_PROBERS; i++)
    delete mCharSetProbers[i];

  delete mEscCharSetProber;
}

void
nsUniversalDetector::Reset()
{
  mNbspFound = PR_FALSE;
  mDone = PR_FALSE;
  mBestGuess = -1;   //illegal value as signal
  mInTag = PR_FALSE;

  mStart = PR_TRUE;
  mDetectedCharset = nsnull;
  mGotData = PR_FALSE;
  mInputState = ePureAscii;
  mLastChar = '\0';

  if (mEscCharSetProber)
    mEscCharSetProber->Reset();

  PRUint32 i;
  for (i = 0; i < NUM_OF_CHARSET_PROBERS; i++)
    if (mCharSetProbers[i])
      mCharSetProbers[i]->Reset();
}

//---------------------------------------------------------------------
#define SHORTCUT_THRESHOLD      (float)0.95
#define MINIMUM_THRESHOLD      (float)0.20

nsresult nsUniversalDetector::HandleData(const char* aBuf, PRUint32 aLen)
{
  if(mDone)
    return NS_OK;

  if (aLen > 0)
    mGotData = PR_TRUE;

  /* If the data starts with BOM, we know it is UTF. */
  if (mStart)
  {
    mStart = PR_FALSE;
    if (aLen > 2)
      switch (aBuf[0])
        {
        case '\xEF':
          if (('\xBB' == aBuf[1]) && ('\xBF' == aBuf[2]))
            /* EF BB BF: UTF-8 encoded BOM. */
            mDetectedCharset = "UTF-8";
        break;
        case '\xFE':
          if ('\xFF' == aBuf[1])
            /* FE FF: UTF-16, big endian BOM. */
            mDetectedCharset = "UTF-16";
        break;
        case '\xFF':
          if ('\xFE' == aBuf[1])
          {
            if (aLen > 3          &&
                aBuf[2] == '\x00' &&
                aBuf[3] == '\x00')
            {
                /* FF FE 00 00: UTF-32 (LE). */
                mDetectedCharset = "UTF-32";
            }
            else
            {
                /* FF FE: UTF-16, little endian BOM. */
                mDetectedCharset = "UTF-16";
            }
          }
          break;
        case '\x00':
          if (aLen > 3           &&
              aBuf[1] == '\x00' &&
              aBuf[2] == '\xFE' &&
              aBuf[3] == '\xFF')
          {
              /* 00 00 FE FF: UTF-32 (BE). */
              mDetectedCharset = "UTF-32";
          }
          break;
        }

      if (mDetectedCharset)
      {
        mDone = PR_TRUE;
        return NS_OK;
      }
  }

  PRUint32 i;
  for (i = 0; i < aLen; i++)
  {
    /* If every other character is ASCII or 0xA0, we don't run charset
     * probers.
     * 0xA0 (NBSP in a few charset) is apparently a rare exception
     * of non-ASCII character often contained in nearly-ASCII text. */
    if (aBuf[i] & '\x80' && aBuf[i] != '\xA0')
    {
      /* We got a non-ASCII byte (high-byte) */
      if (mInputState != eHighbyte)
      {
        //adjust state
        mInputState = eHighbyte;

        //kill mEscCharSetProber if it is active
        if (mEscCharSetProber) {
          delete mEscCharSetProber;
          mEscCharSetProber = nsnull;
        }

        //start multibyte and singlebyte charset prober
        if (nsnull == mCharSetProbers[0])
        {
          mCharSetProbers[0] = new nsMBCSGroupProber(mLanguageFilter);
          if (nsnull == mCharSetProbers[0])
            return NS_ERROR_OUT_OF_MEMORY;
        }
        if (nsnull == mCharSetProbers[1] &&
            (mLanguageFilter & NS_FILTER_NON_CJK))
        {
          mCharSetProbers[1] = new nsSBCSGroupProber;
          if (nsnull == mCharSetProbers[1])
            return NS_ERROR_OUT_OF_MEMORY;
        }
        if (nsnull == mCharSetProbers[2])
        {
          mCharSetProbers[2] = new nsLatin1Prober;
          if (nsnull == mCharSetProbers[2])
            return NS_ERROR_OUT_OF_MEMORY;
        }
      }
    }
    else
    {
      /* Just pure ASCII or NBSP so far. */
      if (aBuf[i] == '\xA0')
      {
        /* ASCII with the only exception of NBSP seems quite common.
         * I doubt it is really necessary to train a model here, so let's
         * just make an exception.
         */
          mNbspFound = PR_TRUE;
      }
      else if (mInputState == ePureAscii &&
               (aBuf[i] == '\033' || (aBuf[i] == '{' && mLastChar == '~')))
      {
        /* We found an escape character or HZ "~{". */
        mInputState = eEscAscii;
      }
      mLastChar = aBuf[i];
    }
  }

  nsProbingState st;
  switch (mInputState)
  {
  case eEscAscii:
    if (nsnull == mEscCharSetProber) {
      mEscCharSetProber = new nsEscCharSetProber(mLanguageFilter);
      if (nsnull == mEscCharSetProber)
        return NS_ERROR_OUT_OF_MEMORY;
    }
    st = mEscCharSetProber->HandleData(aBuf, aLen);
    if (st == eFoundIt)
    {
      mDone = PR_TRUE;
      mDetectedCharset = mEscCharSetProber->GetCharSetName();
    }
    else if (mNbspFound)
    {
      mDetectedCharset = "ISO-8859-1";
    }
    else
    {
      /* ASCII with the ESC character (or the sequence "~{") is still
       * ASCII until proven otherwise. */
      mDetectedCharset = "ASCII";
    }
    break;
  case eHighbyte:
    for (i = 0; i < NUM_OF_CHARSET_PROBERS; i++)
    {
      if (mCharSetProbers[i])
      {
        st = mCharSetProbers[i]->HandleData(aBuf, aLen);
        if (st == eFoundIt)
        {
          mDone = PR_TRUE;
          mDetectedCharset = mCharSetProbers[i]->GetCharSetName();
          return NS_OK;
        }
      }
    }
    break;

  default:
    if (mNbspFound)
    {
      /* ISO-8859-1 is a good result candidate for ASCII + NBSP.
       * (though it could have been any ISO-8859 encoding). */
      mDetectedCharset = "ISO-8859-1";
    }
    else
    {
      /* Pure ASCII */
      mDetectedCharset = "ASCII";
    }
    break;
  }
  return NS_OK;
}


//---------------------------------------------------------------------
void nsUniversalDetector::DataEnd()
{
  if (!mGotData)
  {
    // we haven't got any data yet, return immediately
    // caller program sometimes call DataEnd before anything has been sent to detector
    return;
  }

  if (mDetectedCharset)
  {
    mDone = PR_TRUE;
    Report(mDetectedCharset);
    return;
  }

  switch (mInputState)
  {
  case eHighbyte:
    {
      float proberConfidence;
      float maxProberConfidence = (float)0.0;
      PRInt32 maxProber = 0;

      for (PRInt32 i = 0; i < NUM_OF_CHARSET_PROBERS; i++)
      {
        if (mCharSetProbers[i])
        {
          proberConfidence = mCharSetProbers[i]->GetConfidence();
          if (proberConfidence > maxProberConfidence)
          {
            maxProberConfidence = proberConfidence;
            maxProber = i;
          }
        }
      }
      //do not report anything because we are not confident of it, that's in fact a negative answer
      if (maxProberConfidence > MINIMUM_THRESHOLD)
        Report(mCharSetProbers[maxProber]->GetCharSetName());
    }
    break;
  case eEscAscii:
    break;
  default:
    ;
  }
  return;
}
