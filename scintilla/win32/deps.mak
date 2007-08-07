PlatWin.o: PlatWin.cxx ../include/Platform.h PlatformRes.h \
  ../src/UniConversion.h ../src/XPM.h
ScintillaWin.o: ScintillaWin.cxx ../include/Platform.h \
  ../include/Scintilla.h ../include/SString.h ../src/ContractionState.h \
  ../src/SVector.h ../src/CellBuffer.h ../src/CallTip.h ../src/KeyMap.h \
  ../src/Indicator.h ../src/XPM.h ../src/LineMarker.h ../src/Style.h \
  ../src/AutoComplete.h ../src/ViewStyle.h ../src/CharClassify.h ../src/Document.h \
  ../src/Editor.h ../src/ScintillaBase.h ../src/UniConversion.h
AutoComplete.o: ../src/AutoComplete.cxx ../include/Platform.h \
  ../include/PropSet.h ../include/SString.h ../src/AutoComplete.h
CallTip.o: ../src/CallTip.cxx ../include/Platform.h \
  ../include/Scintilla.h ../src/CallTip.h
CellBuffer.o: ../src/CellBuffer.cxx ../include/Platform.h \
  ../include/Scintilla.h ../src/SVector.h ../src/CellBuffer.h
CharClassify.o: ../src/CharClassify.cxx ../src/CharClassify.h
ContractionState.o: ../src/ContractionState.cxx ../include/Platform.h \
  ../src/ContractionState.h
Document.o: ../src/Document.cxx ../include/Platform.h \
  ../include/Scintilla.h ../src/SVector.h ../src/CellBuffer.h \
 ../src/CharClassify.h  ../src/Document.h ../src/RESearch.h
DocumentAccessor.o: ../src/DocumentAccessor.cxx ../include/Platform.h \
  ../include/PropSet.h ../include/SString.h ../src/SVector.h \
  ../include/Accessor.h ../src/DocumentAccessor.h ../src/CellBuffer.h \
  ../include/Scintilla.h ../src/CharClassify.h ../src/Document.h
Editor.o: ../src/Editor.cxx ../include/Platform.h ../include/Scintilla.h \
  ../src/ContractionState.h ../src/SVector.h ../src/CellBuffer.h \
  ../src/KeyMap.h ../src/Indicator.h ../src/XPM.h ../src/LineMarker.h \
  ../src/Style.h ../src/ViewStyle.h ../src/CharClassify.h ../src/Document.h ../src/Editor.h
ExternalLexer.o: ../src/ExternalLexer.cxx ../include/Platform.h \
  ../include/SciLexer.h ../include/PropSet.h ../include/SString.h \
  ../include/Accessor.h ../src/DocumentAccessor.h ../include/KeyWords.h \
  ../src/ExternalLexer.h
Indicator.o: ../src/Indicator.cxx ../include/Platform.h \
  ../include/Scintilla.h ../src/Indicator.h
KeyMap.o: ../src/KeyMap.cxx ../include/Platform.h ../include/Scintilla.h \
  ../src/KeyMap.h
KeyWords.o: ../src/KeyWords.cxx ../include/Platform.h \
  ../include/PropSet.h ../include/SString.h ../include/Accessor.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexAda.o: ../src/LexAda.cxx ../include/Platform.h ../include/Accessor.h \
  ../src/StyleContext.h ../include/PropSet.h ../include/SString.h \
  ../include/KeyWords.h ../include/SciLexer.h
LexAPDL.o: ../src/LexAPDL.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../src/StyleContext.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexAsm.o: ../src/LexAsm.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../src/StyleContext.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexAU3.o: ../src/LexAU3.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../src/StyleContext.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexAVE.o: ../src/LexAVE.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../src/StyleContext.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexBaan.o: ../src/LexBaan.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../src/StyleContext.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexBash.o: ../src/LexBash.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../include/KeyWords.h \
  ../include/Scintilla.h ../include/SciLexer.h
LexBullant.o: ../src/LexBullant.cxx ../include/Platform.h \
  ../include/PropSet.h ../include/SString.h ../include/Accessor.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexCLW.o: ../src/LexCLW.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../src/StyleContext.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexConf.o: ../src/LexConf.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../include/KeyWords.h \
  ../include/Scintilla.h ../include/SciLexer.h
LexCPP.o: ../src/LexCPP.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../src/StyleContext.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexCrontab.o: ../src/LexCrontab.cxx ../include/Platform.h \
  ../include/PropSet.h ../include/SString.h ../include/Accessor.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexCSS.o: ../src/LexCSS.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../src/StyleContext.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexEiffel.o: ../src/LexEiffel.cxx ../include/Platform.h \
  ../include/PropSet.h ../include/SString.h ../include/Accessor.h \
  ../src/StyleContext.h ../include/KeyWords.h ../include/Scintilla.h \
  ../include/SciLexer.h
LexErlang.o: ../src/LexErlang.cxx ../include/Platform.h \
  ../include/PropSet.h ../include/SString.h ../include/Accessor.h \
  ../src/StyleContext.h ../include/KeyWords.h ../include/Scintilla.h \
  ../include/SciLexer.h
LexEScript.o: ../src/LexEScript.cxx ../include/Platform.h \
  ../include/PropSet.h ../include/SString.h ../include/Accessor.h \
  ../src/StyleContext.h ../include/KeyWords.h ../include/Scintilla.h \
  ../include/SciLexer.h
LexForth.o: ../src/LexForth.cxx ../include/Platform.h \
  ../include/PropSet.h ../include/SString.h ../include/Accessor.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexFortran.o: ../src/LexFortran.cxx ../include/Platform.h \
  ../include/PropSet.h ../include/SString.h ../include/Accessor.h \
  ../src/StyleContext.h ../include/KeyWords.h ../include/Scintilla.h \
  ../include/SciLexer.h
LexGui4Cli.o: ../src/LexGui4Cli.cxx ../include/Platform.h \
  ../include/PropSet.h ../include/SString.h ../include/Accessor.h \
  ../src/StyleContext.h ../include/KeyWords.h ../include/Scintilla.h \
  ../include/SciLexer.h
LexHTML.o: ../src/LexHTML.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../src/StyleContext.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexKix.o: ../src/LexKix.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../src/StyleContext.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexLisp.o: ../src/LexLisp.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../include/KeyWords.h \
  ../include/Scintilla.h ../include/SciLexer.h
LexLout.o: ../src/LexLout.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../src/StyleContext.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexLua.o: ../src/LexLua.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../src/StyleContext.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexMatlab.o: ../src/LexMatlab.cxx ../include/Platform.h \
  ../include/PropSet.h ../include/SString.h ../include/Accessor.h \
  ../src/StyleContext.h ../include/KeyWords.h ../include/Scintilla.h \
  ../include/SciLexer.h
LexMetapost.o: ../src/LexMetapost.cxx ../include/Platform.h \
  ../include/PropSet.h ../include/SString.h ../include/Accessor.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h \
  ../src/StyleContext.h
LexMMIXAL.o: ../src/LexMMIXAL.cxx ../include/Platform.h \
  ../include/PropSet.h ../include/SString.h ../include/Accessor.h \
  ../src/StyleContext.h ../include/KeyWords.h ../include/Scintilla.h \
  ../include/SciLexer.h
LexMPT.o: ../src/LexMPT.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../include/KeyWords.h \
  ../include/Scintilla.h ../include/SciLexer.h
LexMSSQL.o: ../src/LexMSSQL.cxx ../include/Platform.h \
  ../include/PropSet.h ../include/SString.h ../include/Accessor.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexNsis.o: ../src/LexNsis.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../include/KeyWords.h \
  ../include/Scintilla.h ../include/SciLexer.h
LexOthers.o: ../src/LexOthers.cxx ../include/Platform.h \
  ../include/PropSet.h ../include/SString.h ../include/Accessor.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexPascal.o: ../src/LexPascal.cxx ../include/Platform.h \
  ../include/PropSet.h ../include/SString.h ../include/Accessor.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h \
  ../src/StyleContext.h
LexPB.o: ../src/LexPB.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../src/StyleContext.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexPerl.o: ../src/LexPerl.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../include/KeyWords.h \
  ../include/Scintilla.h ../include/SciLexer.h
LexPOV.o: ../src/LexPOV.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../src/StyleContext.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexPS.o: ../src/LexPS.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../src/StyleContext.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexPython.o: ../src/LexPython.cxx ../include/Platform.h \
  ../include/PropSet.h ../include/SString.h ../include/Accessor.h \
  ../src/StyleContext.h ../include/KeyWords.h ../include/Scintilla.h \
  ../include/SciLexer.h
LexRuby.o: ../src/LexRuby.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../include/KeyWords.h \
  ../include/Scintilla.h ../include/SciLexer.h
LexScriptol.o: ../src/LexScriptol.cxx ../include/Platform.h \
  ../include/PropSet.h ../include/SString.h ../include/Accessor.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexSpecman.o: ../src/LexSpecman.cxx ../include/Platform.h \
  ../include/PropSet.h ../include/SString.h ../include/Accessor.h \
  ../src/StyleContext.h ../include/KeyWords.h ../include/Scintilla.h \
  ../include/SciLexer.h
LexSQL.o: ../src/LexSQL.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../include/KeyWords.h \
  ../include/Scintilla.h ../include/SciLexer.h
LexTeX.o: ../src/LexTeX.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../include/KeyWords.h \
  ../include/Scintilla.h ../include/SciLexer.h ../src/StyleContext.h
LexVB.o: ../src/LexVB.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../src/StyleContext.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexVerilog.o: ../src/LexVerilog.cxx ../include/Platform.h \
  ../include/PropSet.h ../include/SString.h ../include/Accessor.h \
  ../src/StyleContext.h ../include/KeyWords.h ../include/Scintilla.h \
  ../include/SciLexer.h
LexYAML.o: ../src/LexYAML.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h ../include/Accessor.h ../src/StyleContext.h \
  ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LineMarker.o: ../src/LineMarker.cxx ../include/Platform.h \
  ../include/Scintilla.h ../src/XPM.h ../src/LineMarker.h
PropSet.o: ../src/PropSet.cxx ../include/Platform.h ../include/PropSet.h \
  ../include/SString.h
RESearch.o: ../src/RESearch.cxx ../src/RESearch.h
ScintillaBase.o: ../src/ScintillaBase.cxx ../include/Platform.h \
  ../include/Scintilla.h ../include/PropSet.h ../include/SString.h \
  ../src/ContractionState.h ../src/SVector.h ../src/CellBuffer.h \
  ../src/CallTip.h ../src/KeyMap.h ../src/Indicator.h ../src/XPM.h \
  ../src/LineMarker.h ../src/Style.h ../src/ViewStyle.h \
  ../src/AutoComplete.h ../src/CharClassify.h ../src/Document.h ../src/Editor.h \
  ../src/ScintillaBase.h
Style.o: ../src/Style.cxx ../include/Platform.h ../include/Scintilla.h \
  ../src/Style.h
StyleContext.o: ../src/StyleContext.cxx ../include/Platform.h \
  ../include/PropSet.h ../include/SString.h ../include/Accessor.h \
  ../src/StyleContext.h
UniConversion.o: ../src/UniConversion.cxx ../src/UniConversion.h
ViewStyle.o: ../src/ViewStyle.cxx ../include/Platform.h \
  ../include/Scintilla.h ../src/Indicator.h ../src/XPM.h \
  ../src/LineMarker.h ../src/Style.h ../src/ViewStyle.h
WindowAccessor.o: ../src/WindowAccessor.cxx ../include/Platform.h \
  ../include/PropSet.h ../include/SString.h ../include/Accessor.h \
  ../include/WindowAccessor.h ../include/Scintilla.h
XPM.o: ../src/XPM.cxx ../include/Platform.h ../src/XPM.h
