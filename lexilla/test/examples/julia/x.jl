
# Comment here
const bar = '\n'

"""
    test_fun(a::Int)
For test only
"""
function test_fun(a::Int, b::T) where T <: Number
    println(a)
    println("foo $(bar)")
end

@enum Unicode α=1 β=2

res = [√i for i in 1:10]

#= Dummy function =#
test_fun²(:sym, true, raw"test", `echo 1`)
