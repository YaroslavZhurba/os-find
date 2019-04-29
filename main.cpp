#include <iostream>
#include <sys/wait.h>
#include <iterator>
#include <sstream>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <cstdio>

// JavaDoc lol kek cheburek
void do_shell(std::vector<std::string> args, char **envp) {
    size_t size = args.size();
    std::vector<char *> args_in_c(size);
    args_in_c.push_back(nullptr);
    for (int i = 0; i < size; i++) {
        args_in_c[i] = const_cast<char *>(args[i].c_str());
    }

    switch (auto pid = fork()) {
        case -1:
            std::cerr << "Error ocсured - fork doesn't work: " << strerror(errno) << std::endl;
            return;
        case 0:
            if (execve(args_in_c[0], args_in_c.data(), envp) == -1) {
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        default:
            int status;
            if (waitpid(pid, &status, 0) == -1) {
            } else if (WIFEXITED(status)) {
                std::cerr << "Error occured - process exit code: " << WEXITSTATUS(status) << std::endl;
            } else if (WIFSIGNALED(status)) {
                std::cerr << "Error occured - unhandled signal: " << strsignal(WTERMSIG(status)) << std::endl;
            } else if (WIFSTOPPED(status)) {
                std::cerr << "Error occured - process stopped, cause signal: " << strsignal(WSTOPSIG(status)) << std::endl;
            }
    }
}

int main(int argc, char *argv[], char *envp[]) {
    std::string input;
    while (getline(std::cin, input)) {
        if (input == "exit") {
            break;
        }
        std::istringstream s(input);
        std::vector<std::string> args((std::istream_iterator<std::string>(s)),
                                      std::istream_iterator<std::string>());
        do_shell(args, envp);
    }
    return 0;
}
