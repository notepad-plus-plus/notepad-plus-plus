PlatGTK.o: PlatGTK.cxx \
 ../include/Scintilla.h ../include/ScintillaWidget.h \
 ../src/UniConversion.h ../src/XPM.h Converter.h
ScintillaGTK.o: ScintillaGTK.cxx \
 ../include/Scintilla.h ../include/ScintillaWidget.h \
 ../include/SciLexer.h ../include/PropSet.h ../src/PropSetSimple.h \
 ../include/Accessor.h ../include/KeyWords.h ../src/SVector.h \
 ../src/SplitVector.h ../src/Partitioning.h ../src/RunStyles.h \
 ../src/ContractionState.h ../src/CellBuffer.h ../src/CallTip.h \
 ../src/KeyMap.h ../src/Indicator.h ../src/XPM.h ../src/LineMarker.h \
 ../src/Style.h ../src/AutoComplete.h ../src/ViewStyle.h \
 ../src/Decoration.h ../src/CharClassify.h ../src/Document.h \
 ../src/Selection.h ../src/PositionCache.h ../src/Editor.h \
 ../src/ScintillaBase.h ../src/UniConversion.h scintilla-marshal.h \
 ../src/ExternalLexer.h Converter.h
AutoComplete.o: ../src/AutoComplete.cxx ../include/Platform.h \
 ../src/CharClassify.h ../src/AutoComplete.h
CallTip.o: ../src/CallTip.cxx ../include/Platform.h \
 ../include/Scintilla.h ../src/CallTip.h
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
DocumentAccessor.o: ../src/DocumentAccessor.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/DocumentAccessor.h \
 ../src/SplitVector.h ../src/Partitioning.h ../src/RunStyles.h \
 ../src/CellBuffer.h ../include/Scintilla.h ../src/CharClassify.h \
 ../src/Decoration.h ../src/Document.h
Document.o: ../src/Document.cxx ../include/Platform.h \
 ../include/Scintilla.h ../src/SplitVector.h ../src/Partitioning.h \
 ../src/RunStyles.h ../src/CellBuffer.h ../src/PerLine.h \
 ../src/CharClassify.h ../src/Decoration.h ../src/Document.h \
 ../src/RESearch.h
Editor.o: ../src/Editor.cxx ../include/Platform.h ../include/Scintilla.h \
 ../src/SplitVector.h ../src/Partitioning.h ../src/RunStyles.h \
 ../src/ContractionState.h ../src/CellBuffer.h ../src/KeyMap.h \
 ../src/Indicator.h ../src/XPM.h ../src/LineMarker.h ../src/Style.h \
 ../src/ViewStyle.h ../src/CharClassify.h ../src/Decoration.h \
 ../src/Document.h ../src/Selection.h ../src/PositionCache.h \
 ../src/Editor.h
ExternalLexer.o: ../src/ExternalLexer.cxx ../include/Platform.h \
 ../include/Scintilla.h ../include/SciLexer.h ../include/PropSet.h \
 ../include/Accessor.h ../src/DocumentAccessor.h ../include/KeyWords.h \
 ../src/ExternalLexer.h
Indicator.o: ../src/Indicator.cxx ../include/Platform.h \
 ../include/Scintilla.h ../src/Indicator.h
KeyMap.o: ../src/KeyMap.cxx ../include/Platform.h ../include/Scintilla.h \
 ../src/KeyMap.h
KeyWords.o: ../src/KeyWords.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexAbaqus.o: ../src/LexAbaqus.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexAda.o: ../src/LexAda.cxx ../include/Platform.h ../include/Accessor.h \
 ../src/StyleContext.h ../include/PropSet.h ../include/KeyWords.h \
 ../include/SciLexer.h
LexAPDL.o: ../src/LexAPDL.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexAsm.o: ../src/LexAsm.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexAsn1.o: ../src/LexAsn1.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexASY.o: ../src/LexASY.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h ../src/CharacterSet.h
LexAU3.o: ../src/LexAU3.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexAVE.o: ../src/LexAVE.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexBaan.o: ../src/LexBaan.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexBash.o: ../src/LexBash.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h ../src/CharacterSet.h
LexBasic.o: ../src/LexBasic.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexBullant.o: ../src/LexBullant.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexCaml.o: ../src/LexCaml.cxx ../include/Platform.h ../include/PropSet.h \
 ../src/PropSetSimple.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexCLW.o: ../src/LexCLW.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexCmake.o: ../src/LexCmake.cxx ../include/Platform.h \
 ../src/CharClassify.h ../include/PropSet.h ../include/Accessor.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexCOBOL.o: ../src/LexCOBOL.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h ../src/StyleContext.h
LexConf.o: ../src/LexConf.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../include/KeyWords.h ../include/Scintilla.h \
 ../include/SciLexer.h
LexCPP.o: ../src/LexCPP.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h ../src/CharacterSet.h
LexCrontab.o: ../src/LexCrontab.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexCsound.o: ../src/LexCsound.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexCSS.o: ../src/LexCSS.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexD.o: ../src/LexD.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexEiffel.o: ../src/LexEiffel.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexErlang.o: ../src/LexErlang.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexEScript.o: ../src/LexEScript.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexFlagship.o: ../src/LexFlagship.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexForth.o: ../src/LexForth.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexFortran.o: ../src/LexFortran.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexGAP.o: ../src/LexGAP.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexGui4Cli.o: ../src/LexGui4Cli.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexHaskell.o: ../src/LexHaskell.cxx ../include/Platform.h \
 ../include/PropSet.h ../src/PropSetSimple.h ../include/Accessor.h \
 ../src/StyleContext.h ../include/KeyWords.h ../include/Scintilla.h \
 ../include/SciLexer.h
LexHTML.o: ../src/LexHTML.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h ../src/CharacterSet.h
LexInno.o: ../src/LexInno.cxx ../include/Platform.h ../src/CharClassify.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexKix.o: ../src/LexKix.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexLisp.o: ../src/LexLisp.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../include/KeyWords.h ../include/Scintilla.h \
 ../include/SciLexer.h ../src/StyleContext.h
LexLout.o: ../src/LexLout.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexLua.o: ../src/LexLua.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h ../src/CharacterSet.h
LexMagik.o: ../src/LexMagik.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexMarkdown.o: ../src/LexMarkdown.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexMatlab.o: ../src/LexMatlab.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexMetapost.o: ../src/LexMetapost.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h ../src/StyleContext.h
LexMMIXAL.o: ../src/LexMMIXAL.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexMPT.o: ../src/LexMPT.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../include/KeyWords.h ../include/Scintilla.h \
 ../include/SciLexer.h
LexMSSQL.o: ../src/LexMSSQL.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexMySQL.o: ../src/LexMySQL.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexNimrod.o: ../src/LexNimrod.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexNsis.o: ../src/LexNsis.cxx ../include/Platform.h ../src/CharClassify.h \
 ../include/PropSet.h ../include/Accessor.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexOpal.o: ../src/LexOpal.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../include/KeyWords.h ../include/Scintilla.h \
 ../include/SciLexer.h ../src/StyleContext.h
LexOthers.o: ../src/LexOthers.cxx ../include/Platform.h \
 ../src/CharClassify.h ../include/PropSet.h ../include/Accessor.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexPascal.o: ../src/LexPascal.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h ../src/StyleContext.h \
 ../src/CharacterSet.h
LexPB.o: ../src/LexPB.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexPerl.o: ../src/LexPerl.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h ../src/CharacterSet.h
LexPLM.o: ../src/LexPLM.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../include/KeyWords.h ../include/Scintilla.h \
 ../include/SciLexer.h ../src/StyleContext.h
LexPOV.o: ../src/LexPOV.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexPowerPro.o: ../src/LexPowerPro.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h \
 ../src/CharacterSet.h
LexPowerShell.o: ../src/LexPowerShell.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexProgress.o: ../src/LexProgress.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexPS.o: ../src/LexPS.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexPython.o: ../src/LexPython.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexR.o: ../src/LexR.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexRebol.o: ../src/LexRebol.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h ../src/StyleContext.h
LexRuby.o: ../src/LexRuby.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../include/KeyWords.h ../include/Scintilla.h \
 ../include/SciLexer.h
LexScriptol.o: ../src/LexScriptol.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexSmalltalk.o: ../src/LexSmalltalk.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexSML.o: ../src/LexSML.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexSorcus.o: ../src/LexSorcus.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexSpecman.o: ../src/LexSpecman.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexSpice.o: ../src/LexSpice.cxx ../include/Platform.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/PropSet.h \
 ../include/KeyWords.h ../include/SciLexer.h
LexSQL.o: ../src/LexSQL.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexTACL.o: ../src/LexTACL.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../include/KeyWords.h ../include/Scintilla.h \
 ../include/SciLexer.h ../src/StyleContext.h
LexTADS3.o: ../src/LexTADS3.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexTAL.o: ../src/LexTAL.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../include/KeyWords.h ../include/Scintilla.h \
 ../include/SciLexer.h ../src/StyleContext.h
LexTCL.o: ../src/LexTCL.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexTeX.o: ../src/LexTeX.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../include/KeyWords.h ../include/Scintilla.h \
 ../include/SciLexer.h ../src/StyleContext.h
LexVB.o: ../src/LexVB.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexVerilog.o: ../src/LexVerilog.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h \
 ../include/KeyWords.h ../include/Scintilla.h ../include/SciLexer.h
LexVHDL.o: ../src/LexVHDL.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
LexYAML.o: ../src/LexYAML.cxx ../include/Platform.h ../include/PropSet.h \
 ../include/Accessor.h ../src/StyleContext.h ../include/KeyWords.h \
 ../include/Scintilla.h ../include/SciLexer.h
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
 ../src/Decoration.h ../src/Document.h ../src/Selection.h \
 ../src/PositionCache.h
PropSet.o: ../src/PropSet.cxx ../include/Platform.h ../include/PropSet.h \
 ../src/PropSetSimple.h
RESearch.o: ../src/RESearch.cxx ../src/CharClassify.h ../src/RESearch.h
RunStyles.o: ../src/RunStyles.cxx ../include/Platform.h \
 ../include/Scintilla.h ../src/SplitVector.h ../src/Partitioning.h \
 ../src/RunStyles.h
ScintillaBase.o: ../src/ScintillaBase.cxx ../include/Platform.h \
 ../include/Scintilla.h ../include/PropSet.h ../src/PropSetSimple.h \
 ../include/SciLexer.h ../include/Accessor.h ../src/DocumentAccessor.h \
 ../include/KeyWords.h ../src/SplitVector.h ../src/Partitioning.h \
 ../src/RunStyles.h ../src/ContractionState.h ../src/CellBuffer.h \
 ../src/CallTip.h ../src/KeyMap.h ../src/Indicator.h ../src/XPM.h \
 ../src/LineMarker.h ../src/Style.h ../src/ViewStyle.h \
 ../src/AutoComplete.h ../src/CharClassify.h ../src/Decoration.h \
 ../src/Document.h ../src/Selection.h ../src/PositionCache.h \
 ../src/Editor.h ../src/ScintillaBase.h
Selection.o: ../src/Selection.cxx ../include/Platform.h \
 ../include/Scintilla.h ../src/Selection.h
StyleContext.o: ../src/StyleContext.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../src/StyleContext.h
Style.o: ../src/Style.cxx ../include/Platform.h ../include/Scintilla.h \
 ../src/Style.h
UniConversion.o: ../src/UniConversion.cxx ../src/UniConversion.h
ViewStyle.o: ../src/ViewStyle.cxx ../include/Platform.h \
 ../include/Scintilla.h ../src/SplitVector.h ../src/Partitioning.h \
 ../src/RunStyles.h ../src/Indicator.h ../src/XPM.h ../src/LineMarker.h \
 ../src/Style.h ../src/ViewStyle.h
WindowAccessor.o: ../src/WindowAccessor.cxx ../include/Platform.h \
 ../include/PropSet.h ../include/Accessor.h ../include/WindowAccessor.h \
 ../include/Scintilla.h
XPM.o: ../src/XPM.cxx ../include/Platform.h ../src/XPM.h
