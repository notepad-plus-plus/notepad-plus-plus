(*
    (**  nested comment 1 **)
    (*
        nested comment 2
        (*
            nested comment 3
            (*
                nested comment 4
                (*
                    nested comment 5
                *)
            *)
        *)
    *)
*)
// declare a namespace
// for the module
namespace Issue93

module NestedComments =
    open FSharp.Quotations
    open FSharp.Quotations.Patterns
    // print the arguments
    // of an evaluated expression
    (* Example:
        (*
            printArgs <@ 1 + 2 @> ;;
            // 1
            // 2
        *)
    *)
    let printArgs expr =
        let getVal = function Value (v, _) -> downcast v | _ -> null
        match expr with
        | Call (_, _, args) ->
            List.map getVal args |> List.iter (printfn "%A")
        | _ ->
            printfn "not an evaluated expression"
    (* Example:
        (*
            let constExpr = <@ true @> ;;
            printArgs constExpr ;;
        *)
    *)
    // Prints:
    // "not an evaluated expression"
