# p99 middlemand

Tool to filter server select list to just project1999 servers.

Fixes a common UDP networking issue with GNU/Linux wifi when playing
Everquest over wine (server list fails to populate).

Original thread:

https://www.project1999.com/forums/showthread.php?t=218666

## Usage

Compile the tool:

```sh
make
```

Run the tool:

```sh
./bin/p99-login-middlemand
```

Update your eqhost.txt file to point to:

```config
[LoginServer]
Host=localhost:5998
```

Start up Everquest and enjoy the experience!
