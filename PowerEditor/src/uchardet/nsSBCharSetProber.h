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
#ifndef nsSingleByteCharSetProber_h__
#define nsSingleByteCharSetProber_h__

#include "nsCharSetProber.h"

/** Codepoints **/

/* Illegal codepoints.*/
#define ILL 255
/* Control character. */
#define CTR 254
/* Symbols and punctuation that does not belong to words. */
#define SYM 253
/* Return/Line feeds. */
#define RET 252
/* Numbers 0-9. */
#define NUM 251

#define SB_ENOUGH_REL_THRESHOLD  1024
#define POSITIVE_SHORTCUT_THRESHOLD  (float)0.95
#define NEGATIVE_SHORTCUT_THRESHOLD  (float)0.05
#define SYMBOL_CAT_ORDER  250

#define NUMBER_OF_SEQ_CAT 4
#define POSITIVE_CAT   (NUMBER_OF_SEQ_CAT-1)
#define PROBABLE_CAT   (NUMBER_OF_SEQ_CAT-2)
#define NEUTRAL_CAT    (NUMBER_OF_SEQ_CAT-3)
#define NEGATIVE_CAT   0

typedef struct
{
  /* [256] table mapping codepoints to chararacter orders. */
  const unsigned char* const charToOrderMap;
  /* freqCharCount x freqCharCount table of 2-char sequence's frequencies. */
  const PRUint8* const precedenceMatrix;
  /* The count of frequent characters. */
  int freqCharCount;
  float  mTypicalPositiveRatio;     // = freqSeqs / totalSeqs
  PRBool keepEnglishLetter;         // says if this script contains English characters (not implemented)
  const char* const charsetName;
} SequenceModel;


class nsSingleByteCharSetProber : public nsCharSetProber{
public:
  nsSingleByteCharSetProber(const SequenceModel *model) 
    :mModel(model), mReversed(PR_FALSE), mNameProber(0) { Reset(); }
  nsSingleByteCharSetProber(const SequenceModel *model, PRBool reversed, nsCharSetProber* nameProber)
    :mModel(model), mReversed(reversed), mNameProber(nameProber) { Reset(); }

  virtual const char* GetCharSetName();
  virtual nsProbingState HandleData(const char* aBuf, PRUint32 aLen);
  virtual nsProbingState GetState(void) {return mState;}
  virtual void      Reset(void);
  virtual float     GetConfidence(void);
  virtual void      SetOpion() {}
  
  // This feature is not implemented yet. any current language model
  // contain this parameter as PR_FALSE. No one is looking at this
  // parameter or calling this method.
  // Moreover, the nsSBCSGroupProber which calls the HandleData of this
  // prober has a hard-coded call to FilterWithoutEnglishLetters which gets rid
  // of the English letters.
  PRBool KeepEnglishLetters() {return mModel->keepEnglishLetter;} // (not implemented)

#ifdef DEBUG_chardet
  virtual void  DumpStatus();
#endif

protected:
  nsProbingState mState;
  const SequenceModel* const mModel;
  const PRBool mReversed; // PR_TRUE if we need to reverse every pair in the model lookup

  //char order of last character
  unsigned char mLastOrder;

  PRUint32 mTotalSeqs;
  PRUint32 mSeqCounters[NUMBER_OF_SEQ_CAT];

  PRUint32 mTotalChar;
  PRUint32 mCtrlChar;
  //characters that fall in our sampling range
  PRUint32 mFreqChar;
  
  // Optional auxiliary prober for name decision. created and destroyed by the GroupProber
  nsCharSetProber* mNameProber; 

};

extern const SequenceModel Windows_1256ArabicModel;
extern const SequenceModel Iso_8859_6ArabicModel;

extern const SequenceModel Koi8rRussianModel;
extern const SequenceModel Win1251RussianModel;
extern const SequenceModel Latin5RussianModel;
extern const SequenceModel MacCyrillicRussianModel;
extern const SequenceModel Ibm866RussianModel;
extern const SequenceModel Ibm855RussianModel;

extern const SequenceModel Iso_8859_7GreekModel;
extern const SequenceModel Windows_1253GreekModel;

extern const SequenceModel Latin5BulgarianModel;
extern const SequenceModel Win1251BulgarianModel;

extern const SequenceModel Iso_8859_2HungarianModel;
extern const SequenceModel Windows_1250HungarianModel;

extern const SequenceModel Win1255Model;

extern const SequenceModel Tis_620ThaiModel;
extern const SequenceModel Iso_8859_11ThaiModel;

extern const SequenceModel Iso_8859_15FrenchModel;
extern const SequenceModel Iso_8859_1FrenchModel;
extern const SequenceModel Windows_1252FrenchModel;

extern const SequenceModel Iso_8859_15SpanishModel;
extern const SequenceModel Iso_8859_1SpanishModel;
extern const SequenceModel Windows_1252SpanishModel;

extern const SequenceModel Iso_8859_1GermanModel;
extern const SequenceModel Windows_1252GermanModel;

extern const SequenceModel Iso_8859_3EsperantoModel;

extern const SequenceModel Iso_8859_3TurkishModel;
extern const SequenceModel Iso_8859_9TurkishModel;

extern const SequenceModel VisciiVietnameseModel;
extern const SequenceModel Windows_1258VietnameseModel;

extern const SequenceModel Iso_8859_15DanishModel;
extern const SequenceModel Iso_8859_1DanishModel;
extern const SequenceModel Windows_1252DanishModel;

#endif /* nsSingleByteCharSetProber_h__ */

