namespace Literals

module Issue110 =
    let hexA = +0xA1B2C3D4
    let hexB = -0xCC100000

    // regression checks
    let hexC = 0xCC100000
    let binA = +0b0000_1010
    let binB = -0b1010_0000
    let binC = 0b1010_0000
    let octA = +0o1237777700
    let octB = -0o1237777700
    let octC = 0o1237777700
    let i8a = +0001y
    let i8b = -0001y
    let u8 = 0001uy
    let f32a = +0.001e-003
    let f32b = -0.001E+003
    let f32c = 0.001e-003
    let f128a = +0.001m
    let f128b = -0.001m
    let f128c = 0.001m

    // invalid literals
    let hexD = 0xa0bcde0o
    let hexE = +0xa0bcd0o
    let hexF = -0xa0bcd0o
    let binD = 0b1010_1110xf000
    let binE = +0b1010_1110xf000
    let binF = -0b1010_1110xf000
    let binG = 0b1010_1110o
    let binH = +0b1010_1110o
    let binI = -0b1010_1110o
    let octD = 0o3330xaBcDeF
    let octE = +0o3330xaBcDe
    let octF = -0o3330xaBcDe
    let octG = 0o3330b
    let octH = 0o3330b
    let octI = 0o3330b

module Issue111 =
    // invalid literals
    let a = 0000_123abc
    let b = +000_123abc
    let c = -0001_23abc
    let d = 00123_000b
    let e = +0123_000o
    let f = -0123_000xcd

module Issue112 =
    let i64 = 0001L
    let u64 = 001UL
    let f32a = 001.F
    let f32b = +01.0F
    let f32c = -01.00000F
    let f32d = 0b0000_0010lf
    let f32e = 0o000_010lf
    let f32f = 0x0000000000000010lf
    let f64a = 0b0000_0010LF
    let f64b = 0o000_010LF
    let f64c = 0x0000000000000010LF
    let f128a = 001.M
    let f128b = +01.0M
    let f128c = -01.00000M

    // regression checks
    let i32 = -0001l
    let u32 = +001ul
    let i128 = 9999999999999999999999999999I
    let f32g = 001.f
    let f32h = +01.0f
    let f32i = -01.00000f
    let f64d = 010000e+009
    let f64e = +001.0e-009
    let f64f = -001.e+009
    let f128d = 001.m
    let f128e = +01.0m
    let f128f = -01.00000m

    // arithmetic expressions
    let a = -001.f+01.0F
    let b = +0b0111_111UL-0x100UL
    let c = -01.0F + +001.f
    let d = -0x100UL - +0b0111_111UL
