module FormatSpecifiersTest

let x = List.fold (*) 24.5 [ 1.; 2.; 3. ]

// expect "147.00"
printfn "Speed: %.2f m/s" x
printfn $"Speed: %.2f{x} m/s"
printfn $"Speed: {x:f2} m/s"
printfn $@"Speed: %.2f{x} m/s"
printfn @$"Speed: {x:f2} m/s"

// expect " 147%"
printfn """%% increase:% .0F%% over last year""" x
printfn $"""%% increase:% .0F{x}%% over last year"""
printfn $"""%% increase:{x / 100.,5:P0} over last year"""
printfn $@"""%% increase:% .0F{x}%% over last year"""
printfn @$"""%% increase:{x / 100.,5:P0} over last year"""

// expect "1.5E+002"
// NB: units should look like text even without a space
printfn @"Time: %-0.1Esecs" x
printfn $"Time: %-0.1E{x}secs"
printfn $"Time: {x:E1}secs"
printfn $@"Time: %-0.1E{x}secs"
printfn @$"Time: {x:E1}secs"

// expect "\"         +147\""
printfn @"""Temp: %+12.3g K""" x
printfn $"""{'"'}Temp: %+12.3g{x} K{'"'}"""
printfn $"""{'"'}Temp: {'+',9}{x:g3} K{'"'}"""
printfn $@"""Temp: %+12.3g{x} K"""
printfn @$"""Temp: {'+',9}{x:g3} K"""

// Since F# 6.0
printfn @"%B" 0b1_000_000
printfn "%B" "\x40"B.[0]
printfn $"""%B{'\064'B}"""
printfn $@"""%B{0b1_000_000}"""
printfn @$"""%B{'\064'B}"""

// These don't work
printfn ``%.2f`` x
printfn $"%.2f" x
printfn $@"%.2f" x
printfn @$"%.2f" x
printfn $"%.2f {x}"
printfn $@"%.2f {x}"
printfn @$"%.2f {x}"
printfn $"""%.2f {x}"""
printfn $@"""%.2f {x}"""
printfn @$"""%.2f {x}"""
