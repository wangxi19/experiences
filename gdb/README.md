# gdb script

a simplest example, intend to descript gdb auto-script usage

- background

the main.c will modify `r->pool` under certain conditions in funcion `func1`, although **watch** can help us to watch a variable changing, but need us set breakpoint and watch `r->pool` manually (because r is a local variable in `func1`, we can only watch it when breaking into `func1`), since the function `func1` will be executed more than 100 times, setting watch point manually is unrealistic

- solution

so gdb auto-script is raised

### How to test

- method 1

```shell
  gcc -g3 -O0 -o dbg ./main.c
  gdb ./dbg -x ./dbg.gdb
  cat /tmp/dbg.gdb
```

- method 2

```
  gcc -g3 -O0 -o dbg ./main.c
  gdb -batch -ex "file ./dbg" -ex "source ./dbg.gdb"
  cat /tmp/dbg.gdb
```

### Explain

see ./dbg.gdb
