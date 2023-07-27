% SCE_VISUALPROLOG_KEY_MAJOR (1)
goal

% SCE_VISUALPROLOG_KEY_MINOR (2)
procedure

% SCE_VISUALPROLOG_KEY_DIRECTIVE (3)
#include

% SCE_VISUALPROLOG_COMMENT_BLOCK (4)
/**
   SCE_VISUALPROLOG_COMMENT_KEY (6)
   @detail
   SCE_VISUALPROLOG_COMMENT_KEY_ERROR (7)
   @unknown
 /* SCE_VISUALPROLOG_IDENTIFIER (8)
    SCE_VISUALPROLOG_VARIABLE (9)
    SCE_VISUALPROLOG_ANONYMOUS (10)
    SCE_VISUALPROLOG_NUMBER (11)
    SCE_VISUALPROLOG_OPERATOR (12) */ */
lambda = {
  (A) = { (B, _discard) = A*B+1 }
}.

% SCE_VISUALPROLOG_COMMENT_LINE (5)
% comment line

% SCE_VISUALPROLOG_STRING_QUOTE (16)
""

% SCE_VISUALPROLOG_STRING (20)
"string"
'string'
@"verbatim string"
@[<div class="test">]

% SCE_VISUALPROLOG_STRING_ESCAPE (17)
"\n"
'\uAB12'

% SCE_VISUALPROLOG_STRING_ESCAPE_ERROR (18)
"\ "
"open string

% SCE_VISUALPROLOG_STRING_EOL (22)
@#multi-line
  verbatim
  string#

% SCE_VISUALPROLOG_EMBEDDED (23)
[| |]
% SCE_VISUALPROLOG_PLACEHOLDER (24)
{| |}:test
% line state & nesting
[|
    {|
        /*
            % /* 
            */ 
        % */
        [|
            {|
                @!string!
                %
                /*
                */
            |}
        |]
    |}
|]
