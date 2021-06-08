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
 *          Shy Shalom <shooshX@gmail.com>
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer: All Rights Reserved.
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

#ifndef nsHebrewProber_h__
#define nsHebrewProber_h__

#include "nsSBCharSetProber.h"

// This prober doesn't actually recognize a language or a charset.
// It is a helper prober for the use of the Hebrew model probers
class nsHebrewProber: public nsCharSetProber
{
public:
  nsHebrewProber(void) :mLogicalProb(0), mVisualProb(0) { Reset(); }

  virtual ~nsHebrewProber(void) {}
  virtual nsProbingState HandleData(const char* aBuf, PRUint32 aLen);
  virtual const char* GetCharSetName();
  virtual void Reset(void);

  virtual nsProbingState GetState(void);

  virtual float     GetConfidence(void) { return (float)0.0; }
  virtual void      SetOpion() {}

  void SetModelProbers(nsCharSetProber *logicalPrb, nsCharSetProber *visualPrb) 
  { mLogicalProb = logicalPrb; mVisualProb = visualPrb; }

#ifdef DEBUG_chardet
  virtual void  DumpStatus();
#endif

protected:
  static PRBool isFinal(char c);
  static PRBool isNonFinal(char c);

  PRInt32 mFinalCharLogicalScore, mFinalCharVisualScore;

  // The two last characters seen in the previous buffer.
  char mPrev, mBeforePrev;

  // These probers are owned by the group prober.
  nsCharSetProber *mLogicalProb, *mVisualProb;
};

/**
 * ** General ideas of the Hebrew charset recognition **
 *
 * Four main charsets exist in Hebrew:
 * "ISO-8859-8" - Visual Hebrew
 * "windows-1255" - Logical Hebrew 
 * "ISO-8859-8-I" - Logical Hebrew
 * "x-mac-hebrew" - ?? Logical Hebrew ??
 *
 * Both "ISO" charsets use a completely identical set of code points, whereas
 * "windows-1255" and "x-mac-hebrew" are two different proper supersets of 
 * these code points. windows-1255 defines additional characters in the range
 * 0x80-0x9F as some misc punctuation marks as well as some Hebrew-specific 
 * diacritics and additional 'Yiddish' ligature letters in the range 0xc0-0xd6.
 * x-mac-hebrew defines similar additional code points but with a different 
 * mapping.
 *
 * As far as an average Hebrew text with no diacritics is concerned, all four 
 * charsets are identical with respect to code points. Meaning that for the 
 * main Hebrew alphabet, all four map the same values to all 27 Hebrew letters 
 * (including final letters).
 *
 * The dominant difference between these charsets is their directionality.
 * "Visual" directionality means that the text is ordered as if the renderer is
 * not aware of a BIDI rendering algorithm. The renderer sees the text and 
 * draws it from left to right. The text itself when ordered naturally is read 
 * backwards. A buffer of Visual Hebrew generally looks like so:
 * "[last word of first line spelled backwards] [whole line ordered backwards
 * and spelled backwards] [first word of first line spelled backwards] 
 * [end of line] [last word of second line] ... etc' "
 * adding punctuation marks, numbers and English text to visual text is
 * naturally also "visual" and from left to right.
 * 
 * "Logical" directionality means the text is ordered "naturally" according to
 * the order it is read. It is the responsibility of the renderer to display 
 * the text from right to left. A BIDI algorithm is used to place general 
 * punctuation marks, numbers and English text in the text.
 *
 * Texts in x-mac-hebrew are almost impossible to find on the Internet. From 
 * what little evidence I could find, it seems that its general directionality
 * is Logical.
 *
 * To sum up all of the above, the Hebrew probing mechanism knows about two
 * charsets:
 * Visual Hebrew - "ISO-8859-8" - backwards text - Words and sentences are
 *    backwards while line order is natural. For charset recognition purposes
 *    the line order is unimportant (In fact, for this implementation, even 
 *    word order is unimportant).
 * Logical Hebrew - "windows-1255" - normal, naturally ordered text.
 *
 * "ISO-8859-8-I" is a subset of windows-1255 and doesn't need to be 
 *    specifically identified.
 * "x-mac-hebrew" is also identified as windows-1255. A text in x-mac-hebrew
 *    that contain special punctuation marks or diacritics is displayed with
 *    some unconverted characters showing as question marks. This problem might
 *    be corrected using another model prober for x-mac-hebrew. Due to the fact
 *    that x-mac-hebrew texts are so rare, writing another model prober isn't 
 *    worth the effort and performance hit.
 *
 * *** The Prober ***
 *
 * The prober is divided between two nsSBCharSetProbers and an nsHebrewProber,
 * all of which are managed, created, fed data, inquired and deleted by the
 * nsSBCSGroupProber. The two nsSBCharSetProbers identify that the text is in
 * fact some kind of Hebrew, Logical or Visual. The final decision about which
 * one is it is made by the nsHebrewProber by combining final-letter scores
 * with the scores of the two nsSBCharSetProbers to produce a final answer.
 *
 * The nsSBCSGroupProber is responsible for stripping the original text of HTML
 * tags, English characters, numbers, low-ASCII punctuation characters, spaces
 * and new lines. It reduces any sequence of such characters to a single space.
 * The buffer fed to each prober in the SBCS group prober is pure text in
 * high-ASCII.
 * The two nsSBCharSetProbers (model probers) share the same language model:
 * Win1255Model.
 * The first nsSBCharSetProber uses the model normally as any other
 * nsSBCharSetProber does, to recognize windows-1255, upon which this model was
 * built. The second nsSBCharSetProber is told to make the pair-of-letter
 * lookup in the language model backwards. This in practice exactly simulates
 * a visual Hebrew model using the windows-1255 logical Hebrew model.
 *
 * The nsHebrewProber is not using any language model. All it does is look for
 * final-letter evidence suggesting the text is either logical Hebrew or visual
 * Hebrew. Disjointed from the model probers, the results of the nsHebrewProber
 * alone are meaningless. nsHebrewProber always returns 0.00 as confidence
 * since it never identifies a charset by itself. Instead, the pointer to the
 * nsHebrewProber is passed to the model probers as a helper "Name Prober".
 * When the Group prober receives a positive identification from any prober,
 * it asks for the name of the charset identified. If the prober queried is a
 * Hebrew model prober, the model prober forwards the call to the
 * nsHebrewProber to make the final decision. In the nsHebrewProber, the
 * decision is made according to the final-letters scores maintained and Both
 * model probers scores. The answer is returned in the form of the name of the
 * charset identified, either "windows-1255" or "ISO-8859-8".
 *
 */
#endif /* nsHebrewProber_h__ */
