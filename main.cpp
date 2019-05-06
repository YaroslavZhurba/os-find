#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string>
#include <iostream>
#include <vector>

using namespace std;

void print_info() {
    cout << "Usage:" << endl;
    cout << "  ./Task2 <absolute_path> <-inum num> <-name name> <-size [-=+]size> <-nlinks num> <-exec absolute_path>"
         << endl;
    cout.flush();
}

void wrong_input(string message) {
    cout << message << endl;
    print_info();
    exit(EXIT_FAILURE);
}

void print_error(string message) {
    cout << message << endl << "Error: " << strerror(errno) << endl;
}

struct My_argc {
    void init(int argc, char *argv[]) {
        if ((argc % 2) != 0) {
            wrong_input("Wrong number of arguments");
        }
        for (int i = 2; i < argc; i += 2) {
            if (static_cast<string>(argv[i]) == "-inum") {
                set_inode(static_cast<string>(argv[i + 1]));
            } else if (static_cast<string>(argv[i]) == "-name") {
                set_name(static_cast<string>(argv[i + 1]));
            } else if (static_cast<string>(argv[i]) == "-size") {
                set_size(static_cast<string>(argv[i + 1]));
            } else if (static_cast<string>(argv[i]) == "-nlinks") {
                set_links(static_cast<string>(argv[i + 1]));
            } else if (static_cast<string>(argv[i]) == "-exec") {
                set_path(static_cast<string>(argv[i + 1]));
            } else {
                wrong_input("No such mod arg: " + static_cast<string>(argv[i]));
            }
        }
    }

    void set_inode(string val) {
        unsigned long long val_inode = 0;
        try {
            val_inode = stoull(val);
        } catch (invalid_argument &) {
            wrong_input("Invalid argument inode" + val);
        }
        inode = val_inode;
        check[0] = 1;
    }

    void set_name(string val) {
        name = val;
        check[1] = 1;
    }

    void set_size(string val) {
        bool sign = false;
        check[2] = 1;
        if (val[0] == '-') {
            sign = true;
            check[2] = -1;
        } else if (val[0] == '+') {
            sign = true;
            check[2] = 2;
        } else if (val[0] == '=') {
            sign = true;
        }
        long long val_size = 0;
        try {
            if (sign) {
                val_size = stoll(val.substr(1));
            } else {
                val_size = stoll(val);
            }
        } catch (exception &) {
            wrong_input("Invalid argument size:" + val);
        }
        size = val_size;
    }

    void set_links(string val) {
        unsigned long long val_links = 0;
        try {
            val_links = stoull(val);
        } catch (exception &) {
            wrong_input("Invalid argument links:" + val);
        }
        links = static_cast<nlink_t>(val_links);
        check[3] = 1;
    }

    void set_path(string val) {
        path = val;
        check[4] = 1;
    }

    int get_check(int x) {
        return check[x];
    }

    ino_t get_inode() {
        return inode;
    }

    string get_name() {
        return name;
    }

    off_t get_size() {
        return size;
    }

    nlink_t get_links() {
        return links;
    }

    string get_path() {
        return path;
    }

private:
    ino_t inode;
    string name;
    off_t size;
    nlink_t links;
    string path;


    //inode = 0, name = 1, size (-1 == <, 1 == =, 2 == >) = 2, links = 3, path = 4;
    int check[5] = {0, 0, 0, 0, 0};
};

void recursive_walker(string dir_str, My_argc &my_argc, vector<string> &files) {
    const char *my_path = dir_str.c_str();
    DIR *dir = opendir(my_path);
    if (dir == nullptr) {
        print_error("Can't open directory " + dir_str);
        return;
    }
    dirent *dir_entry;
    errno = 0;
    while (dir_entry = readdir(dir)) {
        string name = dir_entry->d_name;
        string path = dir_str + "/" + name;
        if (dir_entry->d_type == DT_DIR) {
            if (name == "." || name == "..") {
                continue;
            }
            recursive_walker(path, my_argc, files);
        } else if (dir_entry->d_type == DT_REG) {
            struct stat my_buf;
            if (stat(path.c_str(), &my_buf) < 0) {
                print_error("Error with file: " + path);
            } else if ((!my_argc.get_check(0) || my_buf.st_ino == my_argc.get_inode()) &&
                       (!my_argc.get_check(1) || name == my_argc.get_name()) &&
                       (my_argc.get_check(2) == 0 ||
                        (my_argc.get_check(2) == -1 && my_buf.st_size < my_argc.get_size()) ||
                        (my_argc.get_check(2) == 2 && my_buf.st_size > my_argc.get_size()) ||
                        (my_argc.get_check(2) != 0 && my_buf.st_size == my_argc.get_size())) &&
                       (!my_argc.get_check(3) || my_buf.st_nlink == my_argc.get_links())) {
                files.push_back(path);
            }
        }
        errno = 0;
    }

    if (errno != 0) {
        print_error("Error reading directory " + dir_str);
    }
    if (closedir(dir) < 0) {
        print_error("Error closing directory " + dir_str);
    }

    if (my_argc.get_check(4)) {
        vector<char *> vec(3);
        vec[0] = const_cast<char *>(my_argc.get_path().c_str());
        vec[2] = nullptr;
        for (auto &file: files) {
            const char *c_string = file.c_str();
            vec[1] = const_cast<char *>(c_string);
            int status;
            switch (auto pid = fork()) {
                case -1:
                    print_error("Can't fork");
                case 0:
                    if (execvp(vec[0], vec.data()) < 0) {
                        print_error("Can't execute: " + string(vec[0]));
                        exit(EXIT_FAILURE);
                    }
                default:
                    if (waitpid(pid, &status, WUNTRACED) < 0) {
                        print_error("Error while executing: " + string(vec[0]));
                    }
            }
        }
    }

}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        wrong_input("Number of arguments can't be less than two!");
    }
    string dir = argv[1];
    My_argc my_argc;
    my_argc.init(argc, argv);

    vector<string> files;
    recursive_walker(dir, my_argc, files);
    cout << "Found " << files.size() << " files" << endl;
    for (auto &file: files) {
        cout << file << endl;
    }

    return 0;
}
