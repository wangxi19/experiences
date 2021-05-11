## Linux c Signal

### Regist signal handler. These are two method to handle signal

- One: sa_handler
  Mustn't set `(struct sigaction)::sa_flags` as **SA_SIGINFO**
  Use `(struct sigaction)::sa_handler` storing signal handler function address
  Only one int signal number is passed to the handler function in this method

- Two: sa_sigaction
  Must set `(struct sigaction)::sa_flags` as **SA_SIGINFO**
  Use `(struct sigaction)::sa_sigaction`  hold the signal handler address
  Not only one int signal number but also `struct siginfo*` which contains sender pid and sender status(Only valid in exit status) are passed to the handler function in this method
  
 Both two above method are using `sigaction(signum, (struct sigaction*), null)` registing the signum to hand it
 
 
 ### Some use case
 
 - create a shell
   use **SIGCHLD** handling all changes of childrens' status.
   use `fork()` creating child process, `setpgid(0,0)` to change child process group id to block **signal** sended from **terminal** to **parent process** to the child process
   use `execve` rewriting child process memory mapping to execute child program
   use `waitpid(pid, &oStatus, WNOHANG|WUNTRACED|WCONTINUED);` getting child current status (WNOHANG indicate the function returns immediately, 
   WUNTRACED indicate will get 'SIGSTOP|SIGTSTP' status if process with that pid was stopped, if WUNTRACED isn't specified, the stopped status willn't get.
   WCONTINUED get 'SIGCONT' status, same WUNTRACED. ) to update children processes' status in real time
   
 
 ### Some trick
 
 If child has more than one status changes in a vary short time. The parent **SIGCHLD** handler may be called only once, waitpid will get the child current status, 
 I guess kernel only remain the last one status for optimizing performance. 
