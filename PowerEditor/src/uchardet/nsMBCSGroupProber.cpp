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
 *			Proofpoint, Inc.
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

#include <stdio.h>

#include "nsMBCSGroupProber.h"
#include "nsUniversalDetector.h"

#if defined(DEBUG_chardet) || defined(DEBUG_jgmyers)
const char *ProberName[] = 
{
  "UTF8",
  "SJIS",
  "EUCJP",
  "GB18030",
  "EUCKR",
  "Big5",
  "EUCTW",
};

#endif

nsMBCSGroupProber::nsMBCSGroupProber(PRUint32 aLanguageFilter)
{
  for (PRUint32 i = 0; i < NUM_OF_PROBERS; i++)
    mProbers[i] = nsnull;

  mProbers[0] = new nsUTF8Prober();
  if (aLanguageFilter & NS_FILTER_JAPANESE) 
  {
    mProbers[1] = new nsSJISProber(aLanguageFilter == NS_FILTER_JAPANESE);
    mProbers[2] = new nsEUCJPProber(aLanguageFilter == NS_FILTER_JAPANESE);
  }
  if (aLanguageFilter & NS_FILTER_CHINESE_SIMPLIFIED)
    mProbers[3] = new nsGB18030Prober(aLanguageFilter == NS_FILTER_CHINESE_SIMPLIFIED);
  if (aLanguageFilter & NS_FILTER_KOREAN)
    mProbers[4] = new nsEUCKRProber(aLanguageFilter == NS_FILTER_KOREAN);
  if (aLanguageFilter & NS_FILTER_CHINESE_TRADITIONAL) 
  {
    mProbers[5] = new nsBig5Prober(aLanguageFilter == NS_FILTER_CHINESE_TRADITIONAL);
    mProbers[6] = new nsEUCTWProber(aLanguageFilter == NS_FILTER_CHINESE_TRADITIONAL);
  }
  Reset();
}

nsMBCSGroupProber::~nsMBCSGroupProber()
{
  for (PRUint32 i = 0; i < NUM_OF_PROBERS; i++)
  {
    delete mProbers[i];
  }
}

const char* nsMBCSGroupProber::GetCharSetName()
{
  if (mBestGuess == -1)
  {
    GetConfidence();
    if (mBestGuess == -1)
      mBestGuess = 0;
  }
  return mProbers[mBestGuess]->GetCharSetName();
}

void  nsMBCSGroupProber::Reset(void)
{
  mActiveNum = 0;
  for (PRUint32 i = 0; i < NUM_OF_PROBERS; i++)
  {
    if (mProbers[i])
    {
      mProbers[i]->Reset();
      mIsActive[i] = PR_TRUE;
      ++mActiveNum;
    }
    else
      mIsActive[i] = PR_FALSE;
  }
  mBestGuess = -1;
  mState = eDetecting;
  mKeepNext = 0;
}

nsProbingState nsMBCSGroupProber::HandleData(const char* aBuf, PRUint32 aLen)
{
  nsProbingState st;
  PRUint32 start = 0;
  PRUint32 keepNext = mKeepNext;

  //do filtering to reduce load to probers
  for (PRUint32 pos = 0; pos < aLen; ++pos)
  {
    if (aBuf[pos] & 0x80)
    {
      if (!keepNext)
        start = pos;
      keepNext = 2;
    }
    else if (keepNext)
    {
      if (--keepNext == 0)
      {
        for (PRUint32 i = 0; i < NUM_OF_PROBERS; i++)
        {
          if (!mIsActive[i])
            continue;
          st = mProbers[i]->HandleData(aBuf + start, pos + 1 - start);
          if (st == eFoundIt)
          {
            mBestGuess = i;
            mState = eFoundIt;
            return mState;
          }
        }
      }
    }
  }

  if (keepNext) {
    for (PRUint32 i = 0; i < NUM_OF_PROBERS; i++)
    {
      if (!mIsActive[i])
        continue;
      st = mProbers[i]->HandleData(aBuf + start, aLen - start);
      if (st == eFoundIt)
      {
        mBestGuess = i;
        mState = eFoundIt;
        return mState;
      }
    }
  }
  mKeepNext = keepNext;

  return mState;
}

float nsMBCSGroupProber::GetConfidence(void)
{
  PRUint32 i;
  float bestConf = 0.0, cf;

  switch (mState)
  {
  case eFoundIt:
    return (float)0.99;
  case eNotMe:
    return (float)0.01;
  default:
    for (i = 0; i < NUM_OF_PROBERS; i++)
    {
      if (!mIsActive[i])
        continue;
      cf = mProbers[i]->GetConfidence();
      if (bestConf < cf)
      {
        bestConf = cf;
        mBestGuess = i;
      }
    }
  }
  return bestConf;
}

#ifdef DEBUG_chardet
void nsMBCSGroupProber::DumpStatus()
{
  PRUint32 i;
  float cf;
  
  GetConfidence();
  for (i = 0; i < NUM_OF_PROBERS; i++)
  {
    if (!mIsActive[i])
      printf("  MBCS inactive: [%s] (confidence is too low).\r\n", ProberName[i]);
    else
    {
      cf = mProbers[i]->GetConfidence();
      printf("  MBCS %1.3f: [%s]\r\n", cf, ProberName[i]);
    }
  }
}
#endif

#ifdef DEBUG_jgmyers
void nsMBCSGroupProber::GetDetectorState(nsUniversalDetector::DetectorState (&states)[nsUniversalDetector::NumDetectors], PRUint32 &offset)
{
  for (PRUint32 i = 0; i < NUM_OF_PROBERS; ++i) {
    states[offset].name = ProberName[i];
    states[offset].isActive = mIsActive[i];
    states[offset].confidence = mIsActive[i] ? mProbers[i]->GetConfidence() : 0.0;
    ++offset;
  }
}
#endif /* DEBUG_jgmyers */
