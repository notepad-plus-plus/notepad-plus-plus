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
 * The Original Code is mozilla.org code.
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

// for japanese encoding, obeserve characteristic:
// 1, kana character (or hankaku?) often have hight frequency of appereance
// 2, kana character often exist in group
// 3, certain combination of kana is never used in japanese language

#include "nsEUCJPProber.h"

void  nsEUCJPProber::Reset(void)
{
  mCodingSM->Reset(); 
  mState = eDetecting;
  mContextAnalyser.Reset(mIsPreferredLanguage);
  mDistributionAnalyser.Reset(mIsPreferredLanguage);
}

nsProbingState nsEUCJPProber::HandleData(const char* aBuf, PRUint32 aLen)
{
  nsSMState codingState;

  for (PRUint32 i = 0; i < aLen; i++)
  {
    codingState = mCodingSM->NextState(aBuf[i]);
    if (codingState == eItsMe)
    {
      mState = eFoundIt;
      break;
    }
    if (codingState == eStart)
    {
      PRUint32 charLen = mCodingSM->GetCurrentCharLen();

      if (i == 0)
      {
        mLastChar[1] = aBuf[0];
        mContextAnalyser.HandleOneChar(mLastChar, charLen);
        mDistributionAnalyser.HandleOneChar(mLastChar, charLen);
      }
      else
      {
        mContextAnalyser.HandleOneChar(aBuf+i-1, charLen);
        mDistributionAnalyser.HandleOneChar(aBuf+i-1, charLen);
      }
    }
  }

  mLastChar[0] = aBuf[aLen-1];

  if (mState == eDetecting)
    if (mContextAnalyser.GotEnoughData() && GetConfidence() > SHORTCUT_THRESHOLD)
      mState = eFoundIt;

  return mState;
}

float nsEUCJPProber::GetConfidence(void)
{
  float contxtCf = mContextAnalyser.GetConfidence();
  float distribCf = mDistributionAnalyser.GetConfidence();

  return (contxtCf > distribCf ? contxtCf : distribCf);
}

