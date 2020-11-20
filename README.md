# README for Erich and Boxian's Yalnix OS

### Testing

In the test/ directory, we have our test file `myinit.c`, which tests different syscalls depending on the arguments given to ./yalnix:

1. GetPid
2. Delay
3. UserBrk
4. Fork/Exec/Wait
5. Terminal Syscalls
6. Pipe
7. Lock/Cvar

the `nth` case, use command-line args from outside the test/ directory:

```bash
./yalnix test/myinit n
```

Note, cases 5,6,7 require the flag `-x` for terminal use

### Running with a Console and shells

```bash
./yalnix -x test/init
```