#include <iostream> 
#include <unistd.h>
#include <wait.h>

using namespace std;

static void StartProgram(const char* pPath, char* argv[]) {
	auto pid = fork();
	if (pid < 0) {
		cerr << "Failure of calling fork()" << endl;
		return;
	} else if (pid == 0) {
		//in child process
		pid = vfork();
		if (pid < 0) {
			cerr << "Failure of calling vfork()" << endl;
			_exit(1);
		} else if (pid == 0) {
			//The execv never returns in none error
			//process context has been replaced with pPath after calling execv

			if (execv(pPath, argv) < 0 ) {
				cerr << "Fail to calling execv()" << endl;
				_exit(1);
			}
		} else {
			_exit(0);
		}
	} else {
		//in current process(parent process)
		int state;
		//if doesn't call wait system call to wait child process state, 
		//will generate defunct(zoombie) process after child process executing is completed
		// In line 16: _exit(1) or line 24: _exit(0);
		// except the current process(parent process) has completed execution before the child process terminate, in this case, the child process's parent process will become the root process(pid == 1), and the root process will wait child process's state, so no defunct process will be generated.
		waitpid(pid, &state, 0);
	}


}

int main(int argc, char* argv[]) {
	StartProgram(argv[1], argv + 2);
	return 0;
}
