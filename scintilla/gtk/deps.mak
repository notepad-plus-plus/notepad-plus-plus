PlatGTK.o: PlatGTK.cxx \
 ../include/Scintilla.h ../include/ScintillaWidget.h \
 ../lexlib/StringCopy.h ../src/XPM.h ../src/UniConversion.h Converter.h
ScintillaGTK.o: ScintillaGTK.cxx \
 ../include/ILexer.h ../include/Scintilla.h ../include/ScintillaWidget.h \
 ../include/SciLexer.h ../lexlib/StringCopy.h ../lexlib/LexerModule.h \
 ../src/SplitVector.h ../src/Partitioning.h ../src/RunStyles.h \
 ../src/ContractionState.h ../src/CellBuffer.h ../src/CallTip.h \
 ../src/KeyMap.h ../src/Indicator.h ../src/XPM.h ../src/LineMarker.h \
 ../src/Style.h ../src/ViewStyle.h ../src/CharClassify.h \
 ../src/Decoration.h ../src/CaseFolder.h ../src/Document.h \
 ../src/CaseConvert.h ../src/UniConversion.h ../src/Selection.h \
 ../src/PositionCache.h ../src/EditModel.h ../src/MarginView.h \
 ../src/EditView.h ../src/Editor.h ../src/AutoComplete.h \
 ../src/ScintillaBase.h ../src/ExternalLexer.h scintilla-marshal.h \
 Converter.h
AutoComplete.o: ../src/AutoComplete.cxx ../include/Platform.h \
 ../include/Scintilla.h ../lexlib/CharacterSet.h ../src/AutoComplete.h
CallTip.o: ../src/CallTip.cxx ../include/Platform.h \
 ../include/Scintilla.h ../lexlib/StringCopy.h ../src/CallTip.h
CaseConvert.o: ../src/CaseConvert.cxx ../lexlib/StringCopy.h \
 ../src/CaseConvert.h ../src/UniConversion.h ../src/UnicodeFromUTF8.h
CaseFolder.o: ../src/CaseFolder.cxx ../src/CaseFolder.h \
 ../src/CaseConvert.h ../src/UniConversion.h
Catalogue.o: ../src/Catalogue.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/LexerModule.h \
 ../src/Catalogue.h
CellBuffer.o: ../src/CellBuffer.cxx ../include/Platform.h \
 ../include/Scintilla.h ../src/SplitVector.h ../src/Partitioning.h \
 ../src/CellBuffer.h ../src/UniConversion.h
CharClassify.o: ../src/CharClassify.cxx ../src/CharClassify.h
ContractionState.o: ../src/ContractionState.cxx ../include/Platform.h \
 ../src/SplitVector.h ../src/Partitioning.h ../src/RunStyles.h \
 ../src/ContractionState.h
Decoration.o: ../src/Decoration.cxx ../include/Platform.h \
 ../include/Scintilla.h ../src/SplitVector.h ../src/Partitioning.h \
 ../src/RunStyles.h ../src/Decoration.h
Document.o: ../src/Document.cxx ../include/Platform.h ../include/ILexer.h \
 ../include/Scintilla.h ../lexlib/CharacterSet.h ../src/SplitVector.h \
 ../src/Partitioning.h ../src/RunStyles.h ../src/CellBuffer.h \
 ../src/PerLine.h ../src/CharClassify.h ../src/Decoration.h \
 ../src/CaseFolder.h ../src/Document.h ../src/RESearch.h \
 ../src/UniConversion.h
EditModel.o: ../src/EditModel.cxx ../include/Platform.h \
 ../include/ILexer.h ../include/Scintilla.h ../lexlib/StringCopy.h \
 ../src/SplitVector.h ../src/Partitioning.h ../src/RunStyles.h \
 ../src/ContractionState.h ../src/CellBuffer.h ../src/KeyMap.h \
 ../src/Indicator.h ../src/XPM.h ../src/LineMarker.h ../src/Style.h \
 ../src/ViewStyle.h ../src/CharClassify.h ../src/Decoration.h \
 ../src/CaseFolder.h ../src/Document.h ../src/UniConversion.h \
 ../src/Selection.h ../src/PositionCache.h ../src/EditModel.h
EditView.o: ../src/EditView.cxx ../include/Platform.h ../include/ILexer.h \
 ../include/Scintilla.h ../lexlib/StringCopy.h ../src/SplitVector.h \
 ../src/Partitioning.h ../src/RunStyles.h ../src/ContractionState.h \
 ../src/CellBuffer.h ../src/KeyMap.h ../src/Indicator.h ../src/XPM.h \
 ../src/LineMarker.h ../src/Style.h ../src/ViewStyle.h \
 ../src/CharClassify.h ../src/Decoration.h ../src/CaseFolder.h \
 ../src/Document.h ../src/UniConversion.h ../src/Selection.h \
 ../src/PositionCache.h ../src/EditModel.h ../src/MarginView.h \
 ../src/EditView.h
Editor.o: ../src/Editor.cxx ../include/Platform.h ../include/ILexer.h \
 ../include/Scintilla.h ../lexlib/StringCopy.h ../src/SplitVector.h \
 ../src/Partitioning.h ../src/RunStyles.h ../src/ContractionState.h \
 ../src/CellBuffer.h ../src/KeyMap.h ../src/Indicator.h ../src/XPM.h \
 ../src/LineMarker.h ../src/Style.h ../src/ViewStyle.h \
 ../src/CharClassify.h ../src/Decoration.h ../src/CaseFolder.h \
 ../src/Document.h ../src/UniConversion.h ../src/Selection.h \
 ../src/PositionCache.h ../src/EditModel.h ../src/MarginView.h \
 ../src/EditView.h ../src/Editor.h
ExternalLexer.o: ../src/ExternalLexer.cxx ../include/Platform.h \
 ../include/ILexer.h ../include/Scintilla.h ../include/SciLexer.h \
 ../lexlib/LexerModule.h ../src/Catalogue.h ../src/ExternalLexer.h
Indicator.o: ../src/Indicator.cxx ../include/Platform.h \
 ../include/Scintilla.h ../src/Indicator.h ../src/XPM.h
KeyMap.o: ../src/KeyMap.cxx ../include/Platform.h ../include/Scintilla.h \
 ../src/KeyMap.h
LineMarker.o: ../src/LineMarker.cxx ../include/Platform.h \
 ../include/Scintilla.h ../lexlib/StringCopy.h ../src/XPM.h \
 ../src/LineMarker.h
MarginView.o: ../src/MarginView.cxx ../include/Platform.h \
 ../include/ILexer.h ../include/Scintilla.h ../lexlib/StringCopy.h \
 ../src/SplitVector.h ../src/Partitioning.h ../src/RunStyles.h \
 ../src/ContractionState.h ../src/CellBuffer.h ../src/KeyMap.h \
 ../src/Indicator.h ../src/XPM.h ../src/LineMarker.h ../src/Style.h \
 ../src/ViewStyle.h ../src/CharClassify.h ../src/Decoration.h \
 ../src/CaseFolder.h ../src/Document.h ../src/UniConversion.h \
 ../src/Selection.h ../src/PositionCache.h ../src/EditModel.h \
 ../src/MarginView.h ../src/EditView.h
PerLine.o: ../src/PerLine.cxx ../include/Platform.h \
 ../include/Scintilla.h ../src/SplitVector.h ../src/Partitioning.h \
 ../src/CellBuffer.h ../src/PerLine.h
PositionCache.o: ../src/PositionCache.cxx ../include/Platform.h \
 ../include/ILexer.h ../include/Scintilla.h ../src/SplitVector.h \
 ../src/Partitioning.h ../src/RunStyles.h ../src/ContractionState.h \
 ../src/CellBuffer.h ../src/KeyMap.h ../src/Indicator.h ../src/XPM.h \
 ../src/LineMarker.h ../src/Style.h ../src/ViewStyle.h \
 ../src/CharClassify.h ../src/Decoration.h ../src/CaseFolder.h \
 ../src/Document.h ../src/UniConversion.h ../src/Selection.h \
 ../src/PositionCache.h
RESearch.o: ../src/RESearch.cxx ../src/CharClassify.h ../src/RESearch.h
RunStyles.o: ../src/RunStyles.cxx ../include/Platform.h \
 ../include/Scintilla.h ../src/SplitVector.h ../src/Partitioning.h \
 ../src/RunStyles.h
ScintillaBase.o: ../src/ScintillaBase.cxx ../include/Platform.h \
 ../include/ILexer.h ../include/Scintilla.h ../include/SciLexer.h \
 ../lexlib/PropSetSimple.h ../lexlib/LexerModule.h ../src/Catalogue.h \
 ../src/SplitVector.h ../src/Partitioning.h ../src/RunStyles.h \
 ../src/ContractionState.h ../src/CellBuffer.h ../src/CallTip.h \
 ../src/KeyMap.h ../src/Indicator.h ../src/XPM.h ../src/LineMarker.h \
 ../src/Style.h ../src/ViewStyle.h ../src/CharClassify.h \
 ../src/Decoration.h ../src/CaseFolder.h ../src/Document.h \
 ../src/Selection.h ../src/PositionCache.h ../src/EditModel.h \
 ../src/MarginView.h ../src/EditView.h ../src/Editor.h \
 ../src/AutoComplete.h ../src/ScintillaBase.h
Selection.o: ../src/Selection.cxx ../include/Platform.h \
 ../include/Scintilla.h ../src/Selection.h
Style.o: ../src/Style.cxx ../include/Platform.h ../include/Scintilla.h \
 ../src/Style.h
UniConversion.o: ../src/UniConversion.cxx ../src/UniConversion.h
ViewStyle.o: ../src/ViewStyle.cxx ../include/Platform.h \
 ../include/Scintilla.h ../src/SplitVector.h ../src/Partitioning.h \
 ../src/RunStyles.h ../src/Indicator.h ../src/XPM.h ../src/LineMarker.h \
 ../src/Style.h ../src/ViewStyle.h
XPM.o: ../src/XPM.cxx ../include/Platform.h ../src/XPM.h
Accessor.o: ../lexlib/Accessor.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/PropSetSimple.h \
 ../lexlib/WordList.h ../lexlib/LexAccessor.h ../lexlib/Accessor.h
CharacterCategory.o: ../lexlib/CharacterCategory.cxx \
 ../lexlib/StringCopy.h ../lexlib/CharacterCategory.h
CharacterSet.o: ../lexlib/CharacterSet.cxx ../lexlib/CharacterSet.h
LexerBase.o: ../lexlib/LexerBase.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/PropSetSimple.h \
 ../lexlib/WordList.h ../lexlib/LexAccessor.h ../lexlib/Accessor.h \
 ../lexlib/LexerModule.h ../lexlib/LexerBase.h
LexerModule.o: ../lexlib/LexerModule.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/PropSetSimple.h \
 ../lexlib/WordList.h ../lexlib/LexAccessor.h ../lexlib/Accessor.h \
 ../lexlib/LexerModule.h ../lexlib/LexerBase.h ../lexlib/LexerSimple.h
LexerNoExceptions.o: ../lexlib/LexerNoExceptions.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/PropSetSimple.h \
 ../lexlib/WordList.h ../lexlib/LexAccessor.h ../lexlib/Accessor.h \
 ../lexlib/LexerModule.h ../lexlib/LexerBase.h \
 ../lexlib/LexerNoExceptions.h
LexerSimple.o: ../lexlib/LexerSimple.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/PropSetSimple.h \
 ../lexlib/WordList.h ../lexlib/LexAccessor.h ../lexlib/Accessor.h \
 ../lexlib/LexerModule.h ../lexlib/LexerBase.h ../lexlib/LexerSimple.h
PropSetSimple.o: ../lexlib/PropSetSimple.cxx ../lexlib/PropSetSimple.h
StyleContext.o: ../lexlib/StyleContext.cxx ../include/ILexer.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h
WordList.o: ../lexlib/WordList.cxx ../lexlib/StringCopy.h \
 ../lexlib/WordList.h
LexA68k.o: ../lexers/LexA68k.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexAPDL.o: ../lexers/LexAPDL.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexASY.o: ../lexers/LexASY.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/Accessor.h ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexAU3.o: ../lexers/LexAU3.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/Accessor.h ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexAVE.o: ../lexers/LexAVE.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/Accessor.h ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexAVS.o: ../lexers/LexAVS.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/Accessor.h ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexAbaqus.o: ../lexers/LexAbaqus.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexAda.o: ../lexers/LexAda.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/Accessor.h ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexAsm.o: ../lexers/LexAsm.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h ../lexlib/OptionSet.h
LexAsn1.o: ../lexers/LexAsn1.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexBaan.o: ../lexers/LexBaan.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexBash.o: ../lexers/LexBash.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexBasic.o: ../lexers/LexBasic.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h ../lexlib/OptionSet.h
LexBullant.o: ../lexers/LexBullant.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexCLW.o: ../lexers/LexCLW.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/Accessor.h ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexCOBOL.o: ../lexers/LexCOBOL.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexCPP.o: ../lexers/LexCPP.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/Accessor.h ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h ../lexlib/OptionSet.h ../lexlib/SparseState.h \
 ../lexlib/SubStyles.h
LexCSS.o: ../lexers/LexCSS.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/Accessor.h ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexCaml.o: ../lexers/LexCaml.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/PropSetSimple.h \
 ../lexlib/WordList.h ../lexlib/LexAccessor.h ../lexlib/Accessor.h \
 ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexCmake.o: ../lexers/LexCmake.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexCoffeeScript.o: ../lexers/LexCoffeeScript.cxx ../include/Platform.h \
 ../include/ILexer.h ../include/Scintilla.h ../include/SciLexer.h \
 ../lexlib/WordList.h ../lexlib/LexAccessor.h ../lexlib/Accessor.h \
 ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexConf.o: ../lexers/LexConf.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexCrontab.o: ../lexers/LexCrontab.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexCsound.o: ../lexers/LexCsound.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexD.o: ../lexers/LexD.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h ../lexlib/OptionSet.h
LexDMAP.o: ../lexers/LexDMAP.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexDMIS.o: ../lexers/LexDMIS.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexECL.o: ../lexers/LexECL.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/PropSetSimple.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h ../lexlib/OptionSet.h
LexEScript.o: ../lexers/LexEScript.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexEiffel.o: ../lexers/LexEiffel.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexErlang.o: ../lexers/LexErlang.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexFlagship.o: ../lexers/LexFlagship.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexForth.o: ../lexers/LexForth.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexFortran.o: ../lexers/LexFortran.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexGAP.o: ../lexers/LexGAP.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/Accessor.h ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexGui4Cli.o: ../lexers/LexGui4Cli.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexHTML.o: ../lexers/LexHTML.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/StringCopy.h \
 ../lexlib/WordList.h ../lexlib/LexAccessor.h ../lexlib/Accessor.h \
 ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexHaskell.o: ../lexers/LexHaskell.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/PropSetSimple.h \
 ../lexlib/WordList.h ../lexlib/LexAccessor.h ../lexlib/Accessor.h \
 ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/CharacterCategory.h ../lexlib/LexerModule.h \
 ../lexlib/OptionSet.h
LexInno.o: ../lexers/LexInno.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexKVIrc.o: ../lexers/LexKVIrc.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexKix.o: ../lexers/LexKix.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/Accessor.h ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexLaTeX.o: ../lexers/LexLaTeX.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/PropSetSimple.h \
 ../lexlib/WordList.h ../lexlib/LexAccessor.h ../lexlib/Accessor.h \
 ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h ../lexlib/LexerBase.h
LexLisp.o: ../lexers/LexLisp.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexLout.o: ../lexers/LexLout.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexLua.o: ../lexers/LexLua.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/Accessor.h ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexMMIXAL.o: ../lexers/LexMMIXAL.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexMPT.o: ../lexers/LexMPT.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/Accessor.h ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexMSSQL.o: ../lexers/LexMSSQL.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexMagik.o: ../lexers/LexMagik.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexMarkdown.o: ../lexers/LexMarkdown.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexMatlab.o: ../lexers/LexMatlab.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexMetapost.o: ../lexers/LexMetapost.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexModula.o: ../lexers/LexModula.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/PropSetSimple.h \
 ../lexlib/WordList.h ../lexlib/LexAccessor.h ../lexlib/Accessor.h \
 ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexMySQL.o: ../lexers/LexMySQL.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexNimrod.o: ../lexers/LexNimrod.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexNsis.o: ../lexers/LexNsis.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexOScript.o: ../lexers/LexOScript.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexOpal.o: ../lexers/LexOpal.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexOthers.o: ../lexers/LexOthers.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexPB.o: ../lexers/LexPB.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/Accessor.h ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexPLM.o: ../lexers/LexPLM.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/Accessor.h ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexPO.o: ../lexers/LexPO.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/Accessor.h ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexPOV.o: ../lexers/LexPOV.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/Accessor.h ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexPS.o: ../lexers/LexPS.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/Accessor.h ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexPascal.o: ../lexers/LexPascal.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexPerl.o: ../lexers/LexPerl.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h ../lexlib/OptionSet.h
LexPowerPro.o: ../lexers/LexPowerPro.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexPowerShell.o: ../lexers/LexPowerShell.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexProgress.o: ../lexers/LexProgress.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexPython.o: ../lexers/LexPython.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexR.o: ../lexers/LexR.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/Accessor.h ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexRebol.o: ../lexers/LexRebol.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexRuby.o: ../lexers/LexRuby.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexRust.o: ../lexers/LexRust.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/PropSetSimple.h \
 ../lexlib/WordList.h ../lexlib/LexAccessor.h ../lexlib/Accessor.h \
 ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h ../lexlib/OptionSet.h
LexSML.o: ../lexers/LexSML.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/Accessor.h ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexSQL.o: ../lexers/LexSQL.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/Accessor.h ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h ../lexlib/OptionSet.h ../lexlib/SparseState.h
LexSTTXT.o: ../lexers/LexSTTXT.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexScriptol.o: ../lexers/LexScriptol.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexSmalltalk.o: ../lexers/LexSmalltalk.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexSorcus.o: ../lexers/LexSorcus.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexSpecman.o: ../lexers/LexSpecman.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexSpice.o: ../lexers/LexSpice.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexTACL.o: ../lexers/LexTACL.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexTADS3.o: ../lexers/LexTADS3.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexTAL.o: ../lexers/LexTAL.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/Accessor.h ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexTCL.o: ../lexers/LexTCL.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/Accessor.h ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexTCMD.o: ../lexers/LexTCMD.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexTeX.o: ../lexers/LexTeX.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/Accessor.h ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexTxt2tags.o: ../lexers/LexTxt2tags.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexVB.o: ../lexers/LexVB.cxx ../include/ILexer.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/WordList.h ../lexlib/LexAccessor.h \
 ../lexlib/Accessor.h ../lexlib/StyleContext.h ../lexlib/CharacterSet.h \
 ../lexlib/LexerModule.h
LexVHDL.o: ../lexers/LexVHDL.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexVerilog.o: ../lexers/LexVerilog.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
LexVisualProlog.o: ../lexers/LexVisualProlog.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/CharacterCategory.h \
 ../lexlib/LexerModule.h ../lexlib/OptionSet.h
LexYAML.o: ../lexers/LexYAML.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/WordList.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/StyleContext.h \
 ../lexlib/CharacterSet.h ../lexlib/LexerModule.h
