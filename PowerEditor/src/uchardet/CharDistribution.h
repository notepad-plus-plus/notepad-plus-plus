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

#ifndef CharDistribution_h__
#define CharDistribution_h__

#include "nscore.h"

#define ENOUGH_DATA_THRESHOLD 1024
 
#define MINIMUM_DATA_THRESHOLD  4

class CharDistributionAnalysis
{
public:
  CharDistributionAnalysis() {Reset(PR_FALSE);}

  //feed a block of data and do distribution analysis
  void HandleData(const char*, PRUint32) {}
  
  //Feed a character with known length
  void HandleOneChar(const char* aStr, PRUint32 aCharLen)
  {
    PRInt32 order;

    //we only care about 2-bytes character in our distribution analysis
    order = (aCharLen == 2) ? GetOrder(aStr) : -1;

    if (order >= 0)
    {
      mTotalChars++;
      //order is valid
      if ((PRUint32)order < mTableSize)
      {
        if (512 > mCharToFreqOrder[order])
          mFreqChars++;
      }
    }
  }

  //return confidence base on existing data
  float GetConfidence(void);

  //Reset analyser, clear any state 
  void      Reset(PRBool aIsPreferredLanguage) 
  {
    mDone = PR_FALSE;
    mTotalChars = 0;
    mFreqChars = 0;
    mDataThreshold = aIsPreferredLanguage ? 0 : MINIMUM_DATA_THRESHOLD;
  }

  //This function is for future extension. Caller can use this function to control
  //analyser's behavior
  void      SetOpion(){}

  //It is not necessary to receive all data to draw conclusion. For charset detection,
  // certain amount of data is enough
  PRBool GotEnoughData() {return mTotalChars > ENOUGH_DATA_THRESHOLD;}

protected:
  //we do not handle character base on its original encoding string, but 
  //convert this encoding string to a number, here called order.
  //This allow multiple encoding of a language to share one frequency table 
  virtual PRInt32 GetOrder(const char* ) {return -1;}
  
  //If this flag is set to PR_TRUE, detection is done and conclusion has been made
  PRBool   mDone;

  //The number of characters whose frequency order is less than 512
  PRUint32 mFreqChars;

  //Total character encounted.
  PRUint32 mTotalChars;

  //Number of hi-byte characters needed to trigger detection
  PRUint32 mDataThreshold;

  //Mapping table to get frequency order from char order (get from GetOrder())
  const PRInt16  *mCharToFreqOrder;

  //Size of above table
  PRUint32 mTableSize;

  //This is a constant value varies from language to language, it is used in 
  //calculating confidence. See my paper for further detail.
  float    mTypicalDistributionRatio;
};


class EUCTWDistributionAnalysis: public CharDistributionAnalysis
{
public:
  EUCTWDistributionAnalysis();
protected:

  //for euc-TW encoding, we are interested 
  //  first  byte range: 0xc4 -- 0xfe
  //  second byte range: 0xa1 -- 0xfe
  //no validation needed here. State machine has done that
  PRInt32 GetOrder(const char* str) {
	  if ((unsigned char)*str >= (unsigned char)0xc4)  
      return 94*((unsigned char)str[0]-(unsigned char)0xc4) + (unsigned char)str[1] - (unsigned char)0xa1;
    else
      return -1;
  }
};


class EUCKRDistributionAnalysis : public CharDistributionAnalysis
{
public:
  EUCKRDistributionAnalysis();
protected:
  //for euc-KR encoding, we are interested 
  //  first  byte range: 0xb0 -- 0xfe
  //  second byte range: 0xa1 -- 0xfe
  //no validation needed here. State machine has done that
  PRInt32 GetOrder(const char* str) 
  { if ((unsigned char)*str >= (unsigned char)0xb0)  
      return 94*((unsigned char)str[0]-(unsigned char)0xb0) + (unsigned char)str[1] - (unsigned char)0xa1;
    else
      return -1;
  }
};

class GB2312DistributionAnalysis : public CharDistributionAnalysis
{
public:
  GB2312DistributionAnalysis();
protected:
  //for GB2312 encoding, we are interested 
  //  first  byte range: 0xb0 -- 0xfe
  //  second byte range: 0xa1 -- 0xfe
  //no validation needed here. State machine has done that
  PRInt32 GetOrder(const char* str) 
  { if ((unsigned char)*str >= (unsigned char)0xb0 && (unsigned char)str[1] >= (unsigned char)0xa1)  
      return 94*((unsigned char)str[0]-(unsigned char)0xb0) + (unsigned char)str[1] - (unsigned char)0xa1;
    else
      return -1;
  }
};


class Big5DistributionAnalysis : public CharDistributionAnalysis
{
public:
  Big5DistributionAnalysis();
protected:
  //for big5 encoding, we are interested 
  //  first  byte range: 0xa4 -- 0xfe
  //  second byte range: 0x40 -- 0x7e , 0xa1 -- 0xfe
  //no validation needed here. State machine has done that
  PRInt32 GetOrder(const char* str) 
  { if ((unsigned char)*str >= (unsigned char)0xa4)  
      if ((unsigned char)str[1] >= (unsigned char)0xa1)
        return 157*((unsigned char)str[0]-(unsigned char)0xa4) + (unsigned char)str[1] - (unsigned char)0xa1 +63;
      else
        return 157*((unsigned char)str[0]-(unsigned char)0xa4) + (unsigned char)str[1] - (unsigned char)0x40;
    else
      return -1;
  }
};

class SJISDistributionAnalysis : public CharDistributionAnalysis
{
public:
  SJISDistributionAnalysis();
protected:
  //for sjis encoding, we are interested 
  //  first  byte range: 0x81 -- 0x9f , 0xe0 -- 0xfe
  //  second byte range: 0x40 -- 0x7e,  0x81 -- oxfe
  //no validation needed here. State machine has done that
  PRInt32 GetOrder(const char* str) 
  { 
    PRInt32 order;
    if ((unsigned char)*str >= (unsigned char)0x81 && (unsigned char)*str <= (unsigned char)0x9f)  
      order = 188 * ((unsigned char)str[0]-(unsigned char)0x81);
    else if ((unsigned char)*str >= (unsigned char)0xe0 && (unsigned char)*str <= (unsigned char)0xef)  
      order = 188 * ((unsigned char)str[0]-(unsigned char)0xe0 + 31);
    else
      return -1;
    order += (unsigned char)*(str+1) - 0x40;
    if ((unsigned char)str[1] > (unsigned char)0x7f)
      order--;
    return order;
  }
};

class EUCJPDistributionAnalysis : public CharDistributionAnalysis
{
public:
  EUCJPDistributionAnalysis();
protected:
  //for euc-JP encoding, we are interested 
  //  first  byte range: 0xa0 -- 0xfe
  //  second byte range: 0xa1 -- 0xfe
  //no validation needed here. State machine has done that
  PRInt32 GetOrder(const char* str) 
  { if ((unsigned char)*str >= (unsigned char)0xa0)  
      return 94*((unsigned char)str[0]-(unsigned char)0xa1) + (unsigned char)str[1] - (unsigned char)0xa1;
    else
      return -1;
  }
};

#endif //CharDistribution_h__

