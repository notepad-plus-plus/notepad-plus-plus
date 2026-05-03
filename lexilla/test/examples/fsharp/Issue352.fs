module Issue352

let asyncFn() = async {
    let expr = backgroundTask { return 4L }
    let (|Even|Odd|) n = if n % 2L |> (=) 0L then Even n else Odd
    match! task { return! expr } |> Async.AwaitTask with
    | Even n ->
        return [ yield! seq { for i in 0L..2L..n do yield i } ]
    | Odd ->
        let! val' = expr |> Async.AwaitTask
        printfn "%s(%s): %d is odd."
        <| System.IO.Path.Combine(__SOURCE_DIRECTORY__, __SOURCE_FILE__)
        <| __LINE__
        <| val'
        return []
}
