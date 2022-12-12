## gdb script

### Overview

a simplest example, intend to descript the usage of gdb auto-script

- background

the main.c will modify `r->pool` under certain conditions in funcion `func1`, although **watch** can help us to watch a variable changing, but need us set breakpoint and watch `r->pool` manually (because r is a local variable in `func1`, we can only watch it when breaking into `func1`), since the function `func1` will be executed more than 100 times, setting watch point manually is unrealistic

- solution

so gdb auto-script is raised

### How to test

- method 1

```shell
  gcc -g3 -O0 -o dbg ./main.c
  gdb ./dbg -x ./dbg.gdb
  cat /tmp/dbg.output
```

- method 2

```
  gcc -g3 -O0 -o dbg ./main.c
  gdb -batch -ex "file ./dbg" -ex "source ./dbg.gdb"
  cat /tmp/dbg.output
```

### Explain

see ./dbg.gdb

### More tips about gdb

https://github.com/wangxi19/100-gdb-tips

## gdb convenience variable

### display all convenience variables

```
show convenience
```

- $_siginfo

information about signal, contains signo, sender ...
but if signal is SIGKILL, $_siginfo will be void because SIGKILL cann't be caught, and the subprocess wil exit immediately
now using systemtap to minitor which process sent the SIGKILL
```
man stap
```
