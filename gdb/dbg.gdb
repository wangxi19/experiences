#how to run it
# $gdb
# (gdb) file /path/to/exec
# (gdb) source /path/to/thisfile

#script description:
#turn off gdb output pagination, because it will block executing
set pagination off
#logging out put to file
set logging on /tmp/dbg.output
#set a gdb variable to track watchpoint number
set variable $idx = 3
#set breakpoint, if the breakpoint is hit, then set a watchpoint to watch the function local variable
break main.c:26 if idx > 120 && idx < 150
  commands 1
  watch r->pool
  c
end

break main.c:28 if idx > 120 && idx < 150
  commands 2
#delete watchpoint setted at break main.c:26, to avoid blocking executing when free(r)
  eval "del %d", $idx
#increase idx to track watchpoint number
  set variable $idx = $idx + 1 
  c
end

run
#when executing is blocked(when a watchpoint is hit), the following commands will be executed
bt full
set logging off
quit
