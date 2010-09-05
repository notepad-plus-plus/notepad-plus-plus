PlatGTK.o: PlatGTK.cxx \
 ../include/Scintilla.h ../include/ScintillaWidget.h \
 ../src/UniConversion.h ../src/XPM.h Converter.h
ScintillaGTK.o: ScintillaGTK.cxx \
 ../include/ILexer.h ../include/Scintilla.h ../include/ScintillaWidget.h \
 ../include/SciLexer.h ../src/SVector.h ../src/SplitVector.h \
 ../src/Partitioning.h ../src/RunStyles.h ../src/ContractionState.h \
 ../src/CellBuffer.h ../src/CallTip.h ../src/KeyMap.h ../src/Indicator.h \
 ../src/XPM.h ../src/LineMarker.h ../src/Style.h ../src/AutoComplete.h \
 ../src/ViewStyle.h ../src/Decoration.h ../src/CharClassify.h \
 ../src/Document.h ../src/Selection.h ../src/PositionCache.h \
 ../src/Editor.h ../src/ScintillaBase.h ../src/UniConversion.h \
 scintilla-marshal.h ../lexlib/LexerModule.h ../src/ExternalLexer.h \
 Converter.h
AutoComplete.o: ../src/AutoComplete.cxx ../include/Platform.h \
 ../lexlib/CharacterSet.h ../src/AutoComplete.h
CallTip.o: ../src/CallTip.cxx ../include/Platform.h \
 ../include/Scintilla.h ../src/CallTip.h
Catalogue.o: ../src/Catalogue.cxx ../include/ILexer.h \
 ../include/Scintilla.h ../include/SciLexer.h ../lexlib/LexerModule.h \
 ../src/Catalogue.h
CellBuffer.o: ../src/CellBuffer.cxx ../include/Platform.h \
 ../include/Scintilla.h ../src/SplitVector.h ../src/Partitioning.h \
 ../src/CellBuffer.h
CharClassify.o: ../src/CharClassify.cxx ../src/CharClassify.h
ContractionState.o: ../src/ContractionState.cxx ../include/Platform.h \
 ../src/SplitVector.h ../src/Partitioning.h ../src/RunStyles.h \
 ../src/ContractionState.h
Decoration.o: ../src/Decoration.cxx ../include/Platform.h \
 ../include/Scintilla.h ../src/SplitVector.h ../src/Partitioning.h \
 ../src/RunStyles.h ../src/Decoration.h
Document.o: ../src/Document.cxx ../include/Platform.h ../include/ILexer.h \
 ../include/Scintilla.h ../src/SplitVector.h ../src/Partitioning.h \
 ../src/RunStyles.h ../src/CellBuffer.h ../src/PerLine.h \
 ../src/CharClassify.h ../lexlib/CharacterSet.h ../src/Decoration.h \
 ../src/Document.h ../src/RESearch.h ../src/UniConversion.h
Editor.o: ../src/Editor.cxx ../include/Platform.h ../include/ILexer.h \
 ../include/Scintilla.h ../src/SplitVector.h ../src/Partitioning.h \
 ../src/RunStyles.h ../src/ContractionState.h ../src/CellBuffer.h \
 ../src/KeyMap.h ../src/Indicator.h ../src/XPM.h ../src/LineMarker.h \
 ../src/Style.h ../src/ViewStyle.h ../src/CharClassify.h \
 ../src/Decoration.h ../src/Document.h ../src/Selection.h \
 ../src/PositionCache.h ../src/Editor.h
ExternalLexer.o: ../src/ExternalLexer.cxx ../include/Platform.h \
 ../include/ILexer.h ../include/Scintilla.h ../include/SciLexer.h \
 ../lexlib/LexerModule.h ../src/Catalogue.h ../src/ExternalLexer.h
Indicator.o: ../src/Indicator.cxx ../include/Platform.h \
 ../include/Scintilla.h ../src/Indicator.h
KeyMap.o: ../src/KeyMap.cxx ../include/Platform.h ../include/Scintilla.h \
 ../src/KeyMap.h
LineMarker.o: ../src/LineMarker.cxx ../include/Platform.h \
 ../include/Scintilla.h ../src/XPM.h ../src/LineMarker.h
PerLine.o: ../src/PerLine.cxx ../include/Platform.h \
 ../include/Scintilla.h ../src/SplitVector.h ../src/Partitioning.h \
 ../src/CellBuffer.h ../src/PerLine.h
PositionCache.o: ../src/PositionCache.cxx ../include/Platform.h \
 ../include/Scintilla.h ../src/SplitVector.h ../src/Partitioning.h \
 ../src/RunStyles.h ../src/ContractionState.h ../src/CellBuffer.h \
 ../src/KeyMap.h ../src/Indicator.h ../src/XPM.h ../src/LineMarker.h \
 ../src/Style.h ../src/ViewStyle.h ../src/CharClassify.h \
 ../src/Decoration.h ../include/ILexer.h ../src/Document.h \
 ../src/Selection.h ../src/PositionCache.h
RESearch.o: ../src/RESearch.cxx ../src/CharClassify.h ../src/RESearch.h
RunStyles.o: ../src/RunStyles.cxx ../include/Platform.h \
 ../include/Scintilla.h ../src/SplitVector.h ../src/Partitioning.h \
 ../src/RunStyles.h
ScintillaBase.o: ../src/ScintillaBase.cxx ../include/Platform.h \
 ../include/ILexer.h ../include/Scintilla.h ../lexlib/PropSetSimple.h \
 ../include/SciLexer.h ../lexlib/LexerModule.h ../src/Catalogue.h \
 ../src/SplitVector.h ../src/Partitioning.h ../src/RunStyles.h \
 ../src/ContractionState.h ../src/CellBuffer.h ../src/CallTip.h \
 ../src/KeyMap.h ../src/Indicator.h ../src/XPM.h ../src/LineMarker.h \
 ../src/Style.h ../src/ViewStyle.h ../src/AutoComplete.h \
 ../src/CharClassify.h ../src/Decoration.h ../src/Document.h \
 ../src/Selection.h ../src/PositionCache.h ../src/Editor.h \
 ../src/ScintillaBase.h
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
