" File contains examples of all SCE_ST_* lexical states 0-16 "
" Smalltalk code from the lexer that generates the character classification table."
| lexTable classificationBlock charClasses |
charClasses := #(#DecDigit #Letter #Special #Upper #BinSel).
lexTable := ByteArray new: 128.
classificationBlock := [ :charClass :chars |
    | flag |
    flag := 1 bitShift: (charClasses indexOf: charClass) - 1.
    chars do: [ :char | lexTable at: char codePoint + 1 put: ((lexTable at: char codePoint + 1) bitOr: flag)]].

classificationBlock
    value: #DecDigit value: '0123456789';
    value: #Letter value: '_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';
    value: #Special value: '()[]{};.^:';
    value: #BinSel value: '~@%&*-+=|\/,<>?!';
    value: #Upper value: 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'.

((String new: 500) streamContents: [ :stream |
    stream crLf; nextPutAll: 'static int ClassificationTable[256] = {'.
    lexTable keysAndValuesDo: [ :index :value |
        ((index - 1) rem: 16) == 0 ifTrue: [
            stream crLf; tab]
        ifFalse: [
            stream space].
        stream print: value.
        index ~= 256 ifTrue: [
            stream nextPut: $,]].
    stream crLf; nextPutAll: '};'; crLf.

    charClasses keysAndValuesDo: [ :index :name |
        stream
            crLf;
            nextPutAll: (
                ('static inline bool is<1s>(unsigned char ch) {return (ch %< 0x80) && ((ClassificationTable[ch] & <2p>) != 0);}')
                    expandMacrosWith: name with: (1 bitShift: (index - 1)))
    ]]) edit

" Some more syntax examples:
  ^ is return (SCE_ST_RETURN)
  true or false is bool (SCE_ST_BOOL)
  self (SCE_ST_SELF)
  super (SCE_ST_SUPER)
  nil (SCE_ST_NIL)
"
foo
  ^ Array with: 1 with: 2 with: false with: self with: super with: nil.

" Issue 274: A decimal separator is not required for scaled decimal numbers"
32.0s2
4.0e3
32s2
4e3
