# gdb frequently used method

```shell
#execute a batch commands and quit
#emulate to execute gdb directly: begin execute gdb, the program isn't running, so execute 'cont' firstly, 
# and when program executing stopped, then print the program's informations, such as thread information, stack information ...
gdb -batch -ex "cont" -ex "info threads" -ex "thread" -ex "thread apply all bt" /bin/ls
```
