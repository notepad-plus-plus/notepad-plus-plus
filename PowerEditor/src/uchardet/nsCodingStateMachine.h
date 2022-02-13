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
#ifndef nsCodingStateMachine_h__
#define nsCodingStateMachine_h__

#include "nsPkgInt.h"

typedef enum {
   eStart = 0,
   eError = 1,
   eItsMe = 2 
} nsSMState;

#define GETCLASS(c) GETFROMPCK(((unsigned char)(c)), mModel->classTable)

//state machine model
struct SMModel
{
  nsPkgInt classTable;
  PRUint32 classFactor = 0;
  nsPkgInt stateTable;
  const PRUint32* charLenTable = nullptr;
  const char* name = nullptr;
  SMModel(){};
  SMModel(nsPkgInt a,PRUint32 b,nsPkgInt c,const PRUint32* d, const char* e):
	classTable(a), classFactor(b), stateTable(c), charLenTable(d), name(e){};
} ;

class nsCodingStateMachine {
public:
  nsCodingStateMachine(const SMModel* sm) : mModel(sm) { mCurrentState = eStart; }
  nsSMState NextState(char c) {
    //for each byte we get its class , if it is first byte, we also get byte length
    PRUint32 byteCls = GETCLASS(c);
    if (mCurrentState == eStart)
    { 
      mCurrentBytePos = 0; 
      mCurrentCharLen = mModel->charLenTable[byteCls];
    }
    //from byte's class and stateTable, we get its next state
    mCurrentState=(nsSMState)GETFROMPCK(mCurrentState*(mModel->classFactor)+byteCls,
                                       mModel->stateTable);
    mCurrentBytePos++;
    return mCurrentState;
  }
  PRUint32  GetCurrentCharLen(void) {return mCurrentCharLen;}
  void      Reset(void) {mCurrentState = eStart;}
  const char * GetCodingStateMachine() {return mModel->name;}

protected:
  nsSMState mCurrentState = eStart;
  PRUint32 mCurrentCharLen = 0;
  PRUint32 mCurrentBytePos = 0;

  const SMModel* mModel = nullptr;
};

extern const SMModel UTF8SMModel;
extern const SMModel Big5SMModel;
extern const SMModel EUCJPSMModel;
extern const SMModel EUCKRSMModel;
extern const SMModel EUCTWSMModel;
extern const SMModel GB18030SMModel;
extern const SMModel SJISSMModel;


extern const SMModel HZSMModel;
extern const SMModel ISO2022CNSMModel;
extern const SMModel ISO2022JPSMModel;
extern const SMModel ISO2022KRSMModel;

#endif /* nsCodingStateMachine_h__ */

