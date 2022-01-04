# tcl tests

#simple example

proc Echo_Server {port} {
    set s [socket -server EchoAccept $port]
    vwait forever;
}

# Bug #1947

$s($i,"n")
set n $showArray($i,"neighbor")
