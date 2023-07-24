#### Test modules

##### Compiling

`make all` should build all the test modules. (server.ko, client.ko, as well as pairs of variants.)

##### Running

First make sure Homa module is installed, or run `sudo insmod homa.ko` in the homa dir.

Then run `sudo insmod server.ko` (or variant modules). This will block, and not return until it receives from the client.

Then run `sudo insmod client.ko` (or variant modules). This should return right away, and so should the server.

Check `dmesg` for anything suspicious. Or better, run `dmesg` with `--follow`.