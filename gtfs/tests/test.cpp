#include "../src/gtfs.hpp"
#include <string>
#include <sys/_types/_pid_t.h>
#include <unistd.h>

// Assumes files are located within the current directory
string directory;
int verbose;

// **Test 1**: Testing that data written by one process is then successfully read by another process.
void writer() {
    gtfs_t *gtfs = gtfs_init(directory, verbose);
    string filename = "test1.txt";
    file_t *fl = gtfs_open_file(gtfs, filename, 100);

    string str = "Hi, I'm the writer.\n";
    write_t *wrt = gtfs_write_file(gtfs, fl, 10, str.length(), str.c_str());
    gtfs_sync_write_file(wrt);

    gtfs_close_file(gtfs, fl);
}

void reader() {
    gtfs_t *gtfs = gtfs_init(directory, verbose);
    string filename = "test1.txt";
    file_t *fl = gtfs_open_file(gtfs, filename, 100);

    string str = "Hi, I'm the writer.\n";
    char *data = gtfs_read_file(gtfs, fl, 10, str.length());
    if (data != NULL) {
        str.compare(string(data)) == 0 ? cout << PASS : cout << FAIL;
    } else {
        cout << FAIL;
    }
    gtfs_close_file(gtfs, fl);
}

void test_write_read() {
    int pid;
    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(-1);
    }
    if (pid == 0) {
        writer();
        exit(0);
    }
    waitpid(pid, NULL, 0);
    reader();
}

// **Test 2**: Testing that aborting a write returns the file to its original contents.

void test_abort_write() {

    gtfs_t *gtfs = gtfs_init(directory, verbose);
    string filename = "test2.txt";
    file_t *fl = gtfs_open_file(gtfs, filename, 100);

    string str = "Testing string.\n";
    write_t *wrt1 = gtfs_write_file(gtfs, fl, 0, str.length(), str.c_str());
    gtfs_sync_write_file(wrt1);

    write_t *wrt2 = gtfs_write_file(gtfs, fl, 20, str.length(), str.c_str());
    gtfs_abort_write_file(wrt2);

    char *data1 = gtfs_read_file(gtfs, fl, 0, str.length());
    if (data1 != NULL) {
        // First write was synced so reading should be successfull
        if (str.compare(string(data1)) != 0) {
            cout << FAIL;
        }
        // Second write was aborted and there was no string written in that offset
        char *data2 = gtfs_read_file(gtfs, fl, 20, str.length());
        if (data2 == NULL) {
            cout << FAIL;
        } else if (string(data2).compare("") == 0) {
            cout << PASS;
        }
    } else {
        cout << FAIL;
    }
    gtfs_close_file(gtfs, fl);
}

// **Test 3**: Testing that the logs are truncated.

void test_truncate_log() {

    gtfs_t *gtfs = gtfs_init(directory, verbose);
    string filename = "test3.txt";
    file_t *fl = gtfs_open_file(gtfs, filename, 100);

    string str = "Testing string.\n";
    write_t *wrt1 = gtfs_write_file(gtfs, fl, 0, str.length(), str.c_str());
    gtfs_sync_write_file(wrt1);

    write_t *wrt2 = gtfs_write_file(gtfs, fl, 20, str.length(), str.c_str());
    gtfs_sync_write_file(wrt2);

    cout << "Before GTFS cleanup\n";
    system("ls -l .");

    gtfs_clean(gtfs);

    cout << "After GTFS cleanup\n";
    system("ls -l .");

    cout << "If log is truncated: " << PASS << "If exactly same output:" << FAIL;

    gtfs_close_file(gtfs, fl);

}

// TODO: Implement any additional tests

// **Test 4**: Testing that crash behavior works as expected.

void crash_writer() {
    gtfs_t *gtfs = gtfs_init(directory, verbose);
    string filename = "test4.txt";
    file_t *fl = gtfs_open_file(gtfs, filename, 100);

    string str = "Hi, I'm the writer.\n";
    write_t *wrt = gtfs_write_file(gtfs, fl, 10, str.length(), str.c_str());
    write_t *wrt2 = gtfs_write_file(gtfs, fl, 20, str.length(), str.c_str());
    gtfs_sync_write_file(wrt2);
    abort();
    gtfs_sync_write_file(wrt);
    gtfs_close_file(gtfs, fl);
}

void crash_reader() {
    gtfs_t *gtfs = gtfs_init(directory, verbose);
    string filename = "test4.txt";
    file_t *fl = gtfs_open_file(gtfs, filename, 100);

    string str = "Hi, I'm the writer.\n";
    char *data = gtfs_read_file(gtfs, fl, 10, str.length());
    if (data != NULL) {
        string(data).compare("") == 0 ? cout << PASS : cout << FAIL;
    } else {
        cout << data << FAIL;
    }
    gtfs_close_file(gtfs, fl);
}

void test_crash_recovery() {
    int pid;
    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(-1);
    }
    if (pid == 0) {
        crash_writer();
        exit(0);
    }
    waitpid(pid, NULL, 0);
    crash_reader();
}

// **Test 5**: Testing that the file cannot be opened if size too small.

void test_open_size_too_small() {

    gtfs_t *gtfs = gtfs_init(directory, verbose);
    string filename = "test5.txt";
    file_t *fl = gtfs_open_file(gtfs, filename, 100);

    string str = "Testing string.\n";
    write_t *wrt1 = gtfs_write_file(gtfs, fl, 0, str.length(), str.c_str());
    gtfs_sync_write_file(wrt1);
    gtfs_close_file(gtfs, fl);

    fl = gtfs_open_file(gtfs, filename, 50);
    fl == NULL ? cout << PASS : cout << FAIL;
    gtfs_close_file(gtfs, fl);
}

// **Test 6**: Testing that size expands when opened with a larger file.

void test_expand_file() {

    gtfs_t *gtfs = gtfs_init(directory, verbose);
    string filename = "test6.txt";
    file_t *fl = gtfs_open_file(gtfs, filename, 100);
    gtfs_close_file(gtfs, fl);

    fl = gtfs_open_file(gtfs, filename,200);
    int len = gtfs_get_file_length(fl);
    if (len == 200) {
        cout << PASS;
    } else {
        cout << FAIL;
    }
    gtfs_close_file(gtfs, fl);
}

// **Test 7**: Testing that only one process can open a file at a time.

void opener(pid_t parent) {
    gtfs_t *gtfs = gtfs_init(directory, verbose);
    string filename = "test7.txt";
    file_t *fl = gtfs_open_file(gtfs, filename, 100);
    if (parent != getpid()) {
        if (fl == NULL) {
            cout << PASS;
        } else {
            cout << FAIL;
        }
    } 
}

void test_multi_open() {
    pid_t parent = getpid();
    int pid;
    opener(parent);
    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(-1);
    }

    if (pid == 0) {
        opener(parent);
        exit(0);
    }
    opener(parent);
}

// **Test 8**: Testing that the most recent version is read.

void test_write_write_read() {

    gtfs_t *gtfs = gtfs_init(directory, verbose);
    string filename = "test8.txt";
    file_t *fl = gtfs_open_file(gtfs, filename, 100);

    string hello = "Hello\n";
    string world = "World\n";
    write_t *wrt1 = gtfs_write_file(gtfs, fl, 0, hello.length(), hello.c_str());
    write_t *wrt2 = gtfs_write_file(gtfs, fl, 0, world.length(), world.c_str());

    char *data1 = gtfs_read_file(gtfs, fl, 0, world.length());
    if (data1 != NULL) {
        // First write was synced so reading should be successfull
        if (world.compare(string(data1)) != 0) {
            cout << FAIL;
        } else {
            cout << PASS;
        }
    } else {
        cout << FAIL;
    }
    gtfs_close_file(gtfs, fl);
}

// **Test 9**: Testing that empty string returned when reading past end of file.

void test_read_beyond_file() {

    gtfs_t *gtfs = gtfs_init(directory, verbose);
    string filename = "test1.txt";
    file_t *fl = gtfs_open_file(gtfs, filename, 100);
    char *data1 = gtfs_read_file(gtfs, fl, 200, 10);
    if (data1 != NULL) {
        string(data1).compare("") == 0 ? cout << PASS : cout << FAIL;
    } else {
        cout << FAIL;
    }
    gtfs_close_file(gtfs, fl);
}

int main(int argc, char **argv) {
    if (argc < 2)
        printf("Usage: ./test verbose_flag\n");
    else
        verbose = strtol(argv[1], NULL, 10);

    // Get current directory path
    char cwd[256];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        directory = string(cwd);
    } else {
        cout << "[cwd] Something went wrong.\n";
    }

    // Call existing tests
    cout << "================== Test 1 ==================\n";
    cout << "Testing that data written by one process is then successfully read by another process.\n";
    test_write_read();

    cout << "================== Test 2 ==================\n";
    cout << "Testing that aborting a write returns the file to its original contents.\n";
    test_abort_write();

    cout << "================== Test 3 ==================\n";
    cout << "Testing that the logs are truncated.\n";
    test_truncate_log();

    // TODO: Call any additional tests

    cout << "================== Test 4  ==================\n";
    cout << "Testing that crash behavior works as expected.\n";
    test_crash_recovery();

    cout << "================== Test 5 ==================\n";
    cout << "Testing that the file cannot be opened if size too small.\n";
    test_open_size_too_small();

    cout << "================== Test 6 ==================\n";
    cout << "Testing that size expands when opened with a larger file.\n";
    test_expand_file();

    cout << "================== Test 7 ==================\n";
    cout << "Testing that only one process can open a file at a time.\n";
    test_multi_open();

    cout << "================== Test 8 ==================\n";
    cout << "Testing that the most recent version is read.\n";
    test_write_write_read();

    cout << "================== Test 9 ==================\n";
    cout << "Testing that empty string returned when reading past end of file.\n";
    test_write_write_read();
}
