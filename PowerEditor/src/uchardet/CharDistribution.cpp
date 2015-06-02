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

#include "CharDistribution.h"

#include "JISFreq.tab"
#include "Big5Freq.tab"
#include "EUCKRFreq.tab"
#include "EUCTWFreq.tab"
#include "GB2312Freq.tab"

#define SURE_YES 0.99f
#define SURE_NO  0.01f

//return confidence base on received data
float CharDistributionAnalysis::GetConfidence(void)
{ 
  //if we didn't receive any character in our consideration range, or the
  // number of frequent characters is below the minimum threshold, return
  // negative answer
  if (mTotalChars <= 0 || mFreqChars <= mDataThreshold)
    return SURE_NO;

  if (mTotalChars != mFreqChars) {
    float r = mFreqChars / ((mTotalChars - mFreqChars) * mTypicalDistributionRatio);

    if (r < SURE_YES)
      return r;
  }
  //normalize confidence, (we don't want to be 100% sure)
  return SURE_YES;
}

EUCTWDistributionAnalysis::EUCTWDistributionAnalysis()
{
  mCharToFreqOrder = EUCTWCharToFreqOrder;
  mTableSize = EUCTW_TABLE_SIZE;
  mTypicalDistributionRatio = EUCTW_TYPICAL_DISTRIBUTION_RATIO;
}

EUCKRDistributionAnalysis::EUCKRDistributionAnalysis()
{
  mCharToFreqOrder = EUCKRCharToFreqOrder;
  mTableSize = EUCKR_TABLE_SIZE;
  mTypicalDistributionRatio = EUCKR_TYPICAL_DISTRIBUTION_RATIO;
}

GB2312DistributionAnalysis::GB2312DistributionAnalysis()
{
  mCharToFreqOrder = GB2312CharToFreqOrder;
  mTableSize = GB2312_TABLE_SIZE;
  mTypicalDistributionRatio = GB2312_TYPICAL_DISTRIBUTION_RATIO;
}

Big5DistributionAnalysis::Big5DistributionAnalysis()
{
  mCharToFreqOrder = Big5CharToFreqOrder;
  mTableSize = BIG5_TABLE_SIZE;
  mTypicalDistributionRatio = BIG5_TYPICAL_DISTRIBUTION_RATIO;
}

SJISDistributionAnalysis::SJISDistributionAnalysis()
{
  mCharToFreqOrder = JISCharToFreqOrder;
  mTableSize = JIS_TABLE_SIZE;
  mTypicalDistributionRatio = JIS_TYPICAL_DISTRIBUTION_RATIO;
}

EUCJPDistributionAnalysis::EUCJPDistributionAnalysis()
{
  mCharToFreqOrder = JISCharToFreqOrder;
  mTableSize = JIS_TABLE_SIZE;
  mTypicalDistributionRatio = JIS_TYPICAL_DISTRIBUTION_RATIO;
}

