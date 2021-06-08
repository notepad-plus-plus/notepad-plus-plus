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

#ifndef __JPCNTX_H__
#define __JPCNTX_H__

#define NUM_OF_CATEGORY 6

#include "nscore.h" 

#define ENOUGH_REL_THRESHOLD  100
#define MAX_REL_THRESHOLD     1000

//hiragana frequency category table
extern const PRUint8 jp2CharContext[83][83];

class JapaneseContextAnalysis
{
public:
  JapaneseContextAnalysis() {Reset(PR_FALSE);}

  void HandleData(const char* aBuf, PRUint32 aLen);

  void HandleOneChar(const char* aStr, PRUint32 aCharLen)
  {
    PRInt32 order;

    //if we received enough data, stop here   
    if (mTotalRel > MAX_REL_THRESHOLD)   mDone = PR_TRUE;
    if (mDone)       return;
     
    //Only 2-bytes characters are of our interest
    order = (aCharLen == 2) ? GetOrder(aStr) : -1;
    if (order != -1 && mLastCharOrder != -1)
    {
      mTotalRel++;
      //count this sequence to its category counter
      mRelSample[jp2CharContext[mLastCharOrder][order]]++;
    }
    mLastCharOrder = order;
  }

  float GetConfidence(void);
  void      Reset(PRBool aIsPreferredLanguage);
  void      SetOpion(){}
  PRBool GotEnoughData() {return mTotalRel > ENOUGH_REL_THRESHOLD;}

protected:
  virtual PRInt32 GetOrder(const char* str, PRUint32 *charLen) = 0;
  virtual PRInt32 GetOrder(const char* str) = 0;

  //category counters, each integer counts sequences in its category
  PRUint32 mRelSample[NUM_OF_CATEGORY];

  //total sequence received
  PRUint32 mTotalRel;

  //Number of sequences needed to trigger detection
  PRUint32 mDataThreshold;
  
  //The order of previous char
  PRInt32  mLastCharOrder;

  //if last byte in current buffer is not the last byte of a character, we
  //need to know how many byte to skip in next buffer.
  PRUint32 mNeedToSkipCharNum;

  //If this flag is set to PR_TRUE, detection is done and conclusion has been made
  PRBool   mDone;
};


class SJISContextAnalysis : public JapaneseContextAnalysis
{
  //SJISContextAnalysis(){};
protected:
  PRInt32 GetOrder(const char* str, PRUint32 *charLen);

  PRInt32 GetOrder(const char* str)
  {
    //We only interested in Hiragana, so first byte is '\202'
    if (*str == '\202' && 
          (unsigned char)*(str+1) >= (unsigned char)0x9f && 
          (unsigned char)*(str+1) <= (unsigned char)0xf1)
      return (unsigned char)*(str+1) - (unsigned char)0x9f;
    return -1;
  }
};

class EUCJPContextAnalysis : public JapaneseContextAnalysis
{
protected:
  PRInt32 GetOrder(const char* str, PRUint32 *charLen);
  PRInt32 GetOrder(const char* str)
    //We only interested in Hiragana, so first byte is '\244'
  {
    if (*str == '\244' &&
          (unsigned char)*(str+1) >= (unsigned char)0xa1 &&
          (unsigned char)*(str+1) <= (unsigned char)0xf3)
      return (unsigned char)*(str+1) - (unsigned char)0xa1;
    return -1;
  }
};

#endif /* __JPCNTX_H__ */

