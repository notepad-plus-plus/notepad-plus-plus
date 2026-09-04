def dbg_args(a, b=1, c:, d: 6, &block) = puts("Args passed: #{[a, b, c, d, block.call]}")
dbg_args(0, c: 5) { 7 }

class A
	def attr = @attr
	def attr=(value)
		@attr = value
	end
	def attr? = !!@attr
	def attr! = @attr = true
	# unary operator
	def -@ = 1
	def +@ = 1
	def ! = 1
	def !@ = 1
	# binary operator
	def +(value) = 1 + value
	def -(value) = 1 - value
	def *(value) = 1 * value
	def **(value) = 1 ** value
	def /(value) = 1 / value
	def %(value) = 1 % value
	def &(value) = 1 & value
	def ^(value) = 1 ^ value
	def >>(value) = 1 >> value
	def <<(value) = 1 << value
	def ==(other) = true
	def !=(other) = true
	def ===(other) = true
	def =~(other) = true
	def <=>(other) = true
	def <(other) = true
	def <=(other) = true
	def >(other) = true
	def >=(other) = true
	# element reference and assignment
	def [](a, b) = puts(a + b)
	def []=(a, b, c)
		puts a + b + c
	end
	# array decomposition
	def dec(((a, b), c)) = puts(a + b + c)
	# class method
	def self::say(*s) = puts(s)
	def self.say(*s) = puts(s)
	# test short method name
	def a = 1
	def ab = 1
end

# class method
def String.hello
  "Hello, world!"
end
# singleton method
greeting = "Hello"
def greeting.broaden
  self + ", world!"
end
# one line definition
def a(b, c) b; c end
# parentheses omitted
def ab c
	puts c
end

# Test folding of multi-line SCE_RB_STRING_QW
puts %W(
a
b
c
)
