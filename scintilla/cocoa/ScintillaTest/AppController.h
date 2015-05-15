/**
 * AppController.h
 * SciTest
 *
 * Created by Mike Lischke on 01.04.09.
 * Copyright 2009 Sun Microsystems, Inc. All rights reserved.
 * This file is dual licensed under LGPL v2.1 and the Scintilla license (http://www.scintilla.org/License.txt).
 */

#import <Cocoa/Cocoa.h>

#import "ScintillaView.h"
#import "InfoBar.h"

@interface AppController : NSObject {
  IBOutlet NSBox *mEditHost;
  ScintillaView* mEditor;
}

- (void) awakeFromNib;
- (void) setupEditor;
- (IBAction) searchText: (id) sender;

@end
