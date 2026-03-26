: \DUP ( n - n n ) DUP ;
: DUP\ ( n - ) \DUP 2DROP ;
: (DROP ( n - ) DUP\ 2 DROP ;
: DROP( ( n - ) \DUP DUP\ (DROP ;
: !FOO ;
: 'BAR ;
: ,BAZ ;
: -QUX ;
: @QUUX ;
: _CORGE ;

: OP_PREFIXES ( -)
    !
    2!
    !FOO
    #
    #S
    '
    'BAR
    *
    */MOD
    +!
    ,
    ,BAZ
    -
    -QUX
    .
    .S
    /
    /MOD
    <
    0<
    =
    0=
    >
    >NUMBER
    ?
    ?DUP
    @
    2@
    @QUUX
    [
    ]
    _CORGE
;

\ redefine '\' as a newline
: \ CR ;
