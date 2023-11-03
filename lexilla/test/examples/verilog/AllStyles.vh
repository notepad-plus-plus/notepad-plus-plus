// Examples drawn from https://verilogams.com/refman/basics/index.html

// SCE_V_DEFAULT {0}

/*
 * SCE_V_COMMENT {1}
 */

// SCE_V_COMMENTLINE {2}
// multiple
// comment lines
// are folded

//{ explicit folds
//  are folded,
//} too

//! SCE_V_COMMENTLINEBANG {3}
//! multiple
//! bang comments
//! are folded

// SCE_V_NUMBER {4}
1'b0
8'hx
8'hfffx
12'hfx
64'o0
0x7f
0o23
0b1011
42_839
0.1
1.3u
5.46K
1.2E12
1.30e-2
236.123_763e-12

// SCE_V_WORD {5}
always

// SCE_V_STRING {6}
"\tsome\ttext\r\n"

// SCE_V_WORD2 {7}
special

// SCE_V_WORD3 {8}
$async$and$array

// SCE_V_PREPROCESSOR {9}
`define __VAMS_ENABLE__
`ifdef __VAMS_ENABLE__
    parameter integer del = 1 from [1:100];
`else
    parameter del = 1;
`endif

// SCE_V_OPERATOR {10}
+-/=!@#%^&*()[]{}<|>~

// SCE_V_IDENTIFIER {11}
q
x$z
\my_var
\/x1/n1
\\x1\n1
\{a,b}
\V(p,n)

// SCE_V_STRINGEOL {12}
"\n

// SCE_V_USER {19}
my_var

// SCE_V_COMMENT_WORD {20}
// TODO write a comment

module mod(clk, q, reset) // folded when fold.verilog.flags=1
// SCE_V_INPUT {21}
  input clk;

// SCE_V_OUTPUT {22}
  output q;

// SCE_V_INOUT {23}
  inout reset;
endmodule

// SCE_V_PORT_CONNECT {24}
mod m1(
  .clk(clk),
  .q(q),
  .reset(reset)
);
