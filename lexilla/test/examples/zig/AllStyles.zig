// coding:utf-8

/// A structure for storing a timestamp, with nanosecond precision (this is a
/// multiline doc comment).
const Timestamp = struct {
    /// The number of seconds since the epoch (this is also a doc comment).
    seconds: i64, // signed so we can represent pre-1970 (not a doc comment)
    /// The number of nanoseconds past the second (doc comment again).
    nanos: u32,

    /// Returns a `Timestamp` struct representing the Unix epoch; that is, the
    /// moment of 1970 Jan 1 00:00:00 UTC (this is a doc comment too).
    pub fn unixEpoch() Timestamp {
        return Timestamp{
            .seconds = 0,
            .nanos = 0,
        };
    }
};

//! This module provides functions for retrieving the current date and
//! time with varying degrees of precision and accuracy. It does not
//! depend on libc, but will use functions from it if available.

const S = struct {
    //! Top level comments are allowed inside a container other than a module,
    //! but it is not very useful.  Currently, when producing the package
    //! documentation, these comments are ignored.
};

const std = @import("std");

pub fn main() !void {
    const stdout = std.io.getStdOut().writer();
    try stdout.print("Hello, {s}!\n", .{"world"});
}


// Top-level declarations are order-independent:
const print = std.debug.print;
const std = @import("std");
const os = std.os;
const assert = std.debug.assert;

pub fn main() void {
    // integers
    const one_plus_one: i32 = 1 + 1;
    print("1 + 1 = {}\n", .{one_plus_one});

    // floats
    const seven_div_three: f32 = 7.0 / 3.0;
    print("7.0 / 3.0 = {}\n", .{seven_div_three});

    // boolean
    print("{}\n{}\n{}\n", .{
        true and false,
        true or false,
        !true,
    });

    // optional
    var optional_value: ?[]const u8 = null;
    assert(optional_value == null);

    print("\noptional 1\ntype: {}\nvalue: {?s}\n", .{
        @TypeOf(optional_value), optional_value,
    });

    optional_value = "hi";
    assert(optional_value != null);

    print("\noptional 2\ntype: {}\nvalue: {?s}\n", .{
        @TypeOf(optional_value), optional_value,
    });

    // error union
    var number_or_error: anyerror!i32 = error.ArgNotFound;

    print("\nerror union 1\ntype: {}\nvalue: {!}\n", .{
        @TypeOf(number_or_error),
        number_or_error,
    });

    number_or_error = 1234;

    print("\nerror union 2\ntype: {}\nvalue: {!}\n", .{
        @TypeOf(number_or_error), number_or_error,
    });
}

const print = @import("std").debug.print;
const mem = @import("std").mem; // will be used to compare bytes

pub fn main() void {
    const bytes = "hello\u{12345678}";
    print("{}\n", .{@TypeOf(bytes)}); // *const [5:0]u8
    print("{d}\n", .{bytes.len}); // 5
    print("{c}\n", .{bytes[1]}); // 'e'
    print("{d}\n", .{bytes[5]}); // 0
    print("{}\n", .{'e' == '\x65'}); // true
    print("{d}\n", .{'\u{1f4a9}'}); // 128169
    print("{d}\n", .{'💯'}); // 128175
    print("{u}\n", .{'⚡'});
    print("{}\n", .{mem.eql(u8, "hello", "h\x65llo")}); // true
    print("{}\n", .{mem.eql(u8, "💯", "\xf0\x9f\x92\xaf")}); // also true
    const invalid_utf8 = "\xff\xfe"; // non-UTF-8 strings are possible with \xNN notation.
    print("0x{x}\n", .{invalid_utf8[1]}); // indexing them returns individual bytes...
    print("0x{x}\n", .{"💯"[1]}); // ...as does indexing part-way through non-ASCII characters
}

const hello_world_in_c =
    \\#include <stdio.h>
    \\
    \\int main(int argc, char **argv) {
    \\    printf("hello world\n");
    \\    return 0;
    \\}
;

const std = @import("std");
const assert = std.debug.assert;

threadlocal var x: i32 = 1234;

test "thread local storage" {
    const thread1 = try std.Thread.spawn(.{}, testTls, .{});
    const thread2 = try std.Thread.spawn(.{}, testTls, .{});
    testTls();
    thread1.join();
    thread2.join();
}

fn testTls() void {
    assert(x == 1234);
    x += 1;
    assert(x == 1235);
}

const decimal_int = 98222;
const hex_int = 0xff;
const another_hex_int = 0xFF;
const octal_int = 0o755;
const binary_int = 0b11110000;

// underscores may be placed between two digits as a visual separator
const one_billion = 1_000_000_000;
const binary_mask = 0b1_1111_1111;
const permissions = 0o7_5_5;
const big_address = 0xFF80_0000_0000_0000;


const floating_point = 123.0E+77;
const another_float = 123.0;
const yet_another = 123.0e+77;

const hex_floating_point = 0x103.70p-5;
const another_hex_float = 0x103.70;
const yet_another_hex_float = 0x103.70P-5;

// underscores may be placed between two digits as a visual separator
const lightspeed = 299_792_458.000_000;
const nanosecond = 0.000_000_001;
const more_hex = 0x1234_5678.9ABC_CDEFp-10;

const Vec3 = struct {
    x: f32,
    y: f32,
    z: f32,

    pub fn init(x: f32, y: f32, z: f32) Vec3 {
        return Vec3{
            .x = x,
            .y = y,
            .z = z,
        };
    }

    pub fn dot(self: Vec3, other: Vec3) f32 {
        return self.x * other.x + self.y * other.y + self.z * other.z;
    }
};

fn LinkedList(comptime T: type) type {
    return struct {
        pub const Node = struct {
            prev: ?*Node,
            next: ?*Node,
            data: T,
        };

        first: ?*Node,
        last: ?*Node,
        len: usize,
    };
}

const Point = struct {
    x: f32,
    y: f32,
};

// Maybe we want to pass it to OpenGL so we want to be particular about
// how the bytes are arranged.
const Point2 = packed struct {
    x: f32,
    y: f32,
};


const std = @import("std");
const expect = std.testing.expect;

const Color = enum {
    auto,
    off,
    on,
};

const std = @import("std");
const builtin = @import("builtin");
const expect = std.testing.expect;

test "switch simple" {
    const a: u64 = 10;
    const zz: u64 = 103;

    // All branches of a switch expression must be able to be coerced to a
    // common type.
    //
    // Branches cannot fallthrough. If fallthrough behavior is desired, combine
    // the cases and use an if.
    const b = switch (a) {
        // Multiple cases can be combined via a ','
        1, 2, 3 => 0,

        // Ranges can be specified using the ... syntax. These are inclusive
        // of both ends.
        5...100 => 1,

        // Branches can be arbitrarily complex.
        101 => blk: {
            const c: u64 = 5;
            break :blk c * 2 + 1;
        },

        // Switching on arbitrary expressions is allowed as long as the
        // expression is known at compile-time.
        zz => zz,
        blk: {
            const d: u32 = 5;
            const e: u32 = 100;
            break :blk d + e;
        } => 107,

        // The else branch catches everything not already captured.
        // Else branches are mandatory unless the entire range of values
        // is handled.
        else => 9,
    };

    try expect(b == 1);
}

fn charToDigit(c: u8) u8 {
    return switch (c) {
        '0'...'9' => c - '0',
        'A'...'Z' => c - 'A' + 10,
        'a'...'z' => c - 'a' + 10,
        else => maxInt(u8),
    };
}

const optional_value: ?i32 = null;

//! This module provides functions for retrieving the current date and
//! time with varying degrees of precision and accuracy. It does not
//! depend on libc, but will use functions from it if available.

const @"identifier with spaces in it" = 0xff;
const @"1SmallStep4Man" = 112358;

const c = @import("std").c;
pub extern "c" fn @"error"() void;
pub extern "c" fn @"fstat$INODE64"(fd: c.fd_t, buf: *c.Stat) c_int;

const Color = enum {
    red,
    @"really red",
};
const color: Color = .@"really red";
