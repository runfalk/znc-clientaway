Clientaway
==========
This is a simple plugin to the IRC bouncer [ZNC](https://wiki.znc.in/ZNC).
When connecting to ZNC through multiple clients it is not possible to use
auto away functionality on one of them. This is because ZNC will pass through
the `/away [<reason>]` command to the server. That means a client with auto
away functionality that's idle will constantly send away commands even when
other clients are in active use.

This plugin makes `/away [<reason>]` work on a per client basis. This means
all connected clients need to be marked as away before the away command is
sent to the server.


Compiling
---------
Use `znc-buildmod` on `clientaway.cpp`, follow the instructions on the
[ZNC Wiki](https://wiki.znc.in/Compiling_modules). If you don't have the
`znc-buildmod` command you probably need to install the `znc-dev`/`znc-devel`
package from your distribution. You need at least ZNC version 1.7.0 to run
this plugin.
