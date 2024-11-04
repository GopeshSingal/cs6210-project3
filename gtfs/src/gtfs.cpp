#include "gtfs.hpp"
#include "pthread.h"
#include "sys/stat.h"
#include "unistd.h"
#include "errno.h"
#include <__config>
#include <cerrno>
#include <cstddef>
#include <string>
#include <utility>
#include <sys/mman.h>


#define VERBOSE_PRINT(verbose, str...) do { \
    if (verbose) cout << "VERBOSE: "<< __FILE__ << ":" << __LINE__ << " " << __func__ << "(): " << str; \
} while(0)

int do_verbose;
unordered_map<string, gtfs_t*> directories;

gtfs_t* gtfs_init(string directory, int verbose_flag) {
    do_verbose = verbose_flag;
    VERBOSE_PRINT(do_verbose, "Initializing GTFileSystem inside directory " << directory << "\n");
    // TODO: Add locking mechanism to prevent race condition when creating a GTFS
    auto map_fs = directories.find(directory);
    if (map_fs != directories.end()) {
        VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns non NULL.
        return map_fs->second;
    }
    gtfs* gtfs = new gtfs_t;
    gtfs->dirname = directory;

    //! Check if the directory already exists, if not create it
    if (mkdir(directory.c_str(), 0755) == -1) {
        if (errno == EEXIST) {
            std::cout << "Directory already exists!\n";
        } else {
            VERBOSE_PRINT(do_verbose, "Failed\n"); //On success returns non NULL.
            return nullptr;
        }
    }

    directories[directory] = gtfs;
    VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns non NULL.
    return gtfs;
}

int gtfs_clean(gtfs_t *gtfs) {
    int ret = -1;
    if (gtfs) {
        VERBOSE_PRINT(do_verbose, "Cleaning up GTFileSystem inside directory " << gtfs->dirname << "\n");
    } else {
        VERBOSE_PRINT(do_verbose, "GTFileSystem does not exist\n");
        return ret;
    }
    //TODO: Add any additional initializations and checks, and complete the functionality

    VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns 0.
    return ret;
}

file_t* gtfs_open_file(gtfs_t* gtfs, string filename, int file_length) {
    if (gtfs) {
        VERBOSE_PRINT(do_verbose, "Opening file " << filename << " inside directory " << gtfs->dirname << "\n");
    } else {
        VERBOSE_PRINT(do_verbose, "GTFileSystem does not exist\n");
        return NULL;
    }
    //TODO: Add any additional initializations and checks, and complete the functionality
    auto f = gtfs->map.find(filename);
    if (f != gtfs->map.end()) {
        if (f->second->flag != 0) {
            VERBOSE_PRINT(do_verbose, "Another process already opened this file\n");
            return NULL;
        }
        if (f->second->file_length > file_length) {
            VERBOSE_PRINT(do_verbose, "The file length is too short. Data will be lost, aborting\n");
            return NULL;
        }

        int fd = open(filename.c_str(), O_RDWR);

        if (f->second->file_length < file_length) {
            if (ftruncate(fd, file_length) == -1) {
                VERBOSE_PRINT(do_verbose, "File could not be resized\n");
                return NULL;
            }
            munmap(f->second->mapped_file, f->second->file_length);
            f->second->mapped_file = mmap(NULL, file_length, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
            if (f->second->mapped_file == MAP_FAILED) {
                VERBOSE_PRINT(do_verbose, "Memory mapping failed\n");
                return NULL;
            }
            f->second->file_length = file_length;
        }

        f->second->flag = getpid();
        VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns non NULL.
        return f->second;
    }
    string path = gtfs->dirname + "/" + filename;
    int fd = open(path.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        perror("Error opening file");
        return NULL;
    }

    if (ftruncate(fd, file_length) == -1) {
        perror("Error expanding file size");
        return NULL;
    }

    void* mapped_file = mmap(NULL, file_length, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    file_t *fl = new file_t;
    fl->filename = filename;
    fl->file_length = file_length;
    fl->mapped_file = mapped_file;
    fl->file_length = file_length;
    fl->flag = getpid();

    gtfs->map[filename] = fl;

    VERBOSE_PRINT(do_verbose, "Success"); //On success returns non NULL.
    std::cout << " Process ID: " << getpid() << std::endl;
    return fl;
}

int gtfs_close_file(gtfs_t* gtfs, file_t* fl) {
    int ret = -1;
    if (gtfs and fl) {
        VERBOSE_PRINT(do_verbose, "Closing file " << fl->filename << " inside directory " << gtfs->dirname << "\n");
    } else {
        VERBOSE_PRINT(do_verbose, "GTFileSystem or file does not exist\n");
        return ret;
    }
    //TODO: Add any additional initializations and checks, and complete the functionality
    if (fl->flag > 0) {
        fl->flag = 0;
        VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns 0.
        return 0;
    }
    return ret;
}

int gtfs_remove_file(gtfs_t* gtfs, file_t* fl) {
    int ret = -1;
    if (gtfs and fl) {
        VERBOSE_PRINT(do_verbose, "Removing file " << fl->filename << " inside directory " << gtfs->dirname << "\n");
    } else {
        VERBOSE_PRINT(do_verbose, "GTFileSystem or file does not exist\n");
        return ret;
    }
    //TODO: Add any additional initializations and checks, and complete the functionality
    if (fl->flag > 0) {
        return -1;
    }
    string pathname = gtfs->dirname + "/" + fl->filename;
    if (remove(pathname.c_str()) == 0) {
        gtfs->map.erase(fl->filename);
        free(fl);
        VERBOSE_PRINT(do_verbose, "Success\n"); // On success returns 0.
        return 0;
    }
    return ret;
}

char* gtfs_read_file(gtfs_t* gtfs, file_t* fl, int offset, int length) {
    char* ret_data = NULL;
    if (gtfs and fl) {
        VERBOSE_PRINT(do_verbose, "Reading " << length << " bytes starting from offset " << offset << " inside file " << fl->filename << "\n");
    } else {
        VERBOSE_PRINT(do_verbose, "GTFileSystem or file does not exist\n");
        return NULL;
    }
    //TODO: Add any additional initializations and checks, and complete the functionality
    if (fl->flag != getpid()) {
        VERBOSE_PRINT(do_verbose, "This process has not opened this file!\n");
        return nullptr;
    }
    if (offset < 0 or length < 0 or offset + length > fl->file_length) {
        VERBOSE_PRINT(do_verbose, "Invalid offset or length\n");
        return nullptr;
    }
    ret_data = new char[length];  // Allocate sufficient memory for data
    memcpy(ret_data, (char*)fl->mapped_file + offset, length);
    VERBOSE_PRINT(do_verbose, "Success YEET\n"); //On success returns pointer to data read.
    std::cout << "Buffer contents: " << ret_data << std::endl;
    return ret_data;
}

write_t* gtfs_write_file(gtfs_t* gtfs, file_t* fl, int offset, int length, const char* data) {
    write_t *write_id = NULL;
    if (gtfs and fl) {
        VERBOSE_PRINT(do_verbose, "Writing " << length << " bytes starting from offset " << offset << " inside file " << fl->filename << "\n");
    } else {
        VERBOSE_PRINT(do_verbose, "GTFileSystem or file does not exist\n");
        return NULL;
    }

    //TODO: Add any additional initializations and checks, and complete the functionality
    if (fl->flag != getpid()) {
        VERBOSE_PRINT(do_verbose, "This process has not opened this file!\n");
        return nullptr;
    }
    if (offset < 0 or length < 0 or offset > fl->file_length) {
        VERBOSE_PRINT(do_verbose, "Invalid offset or length\n");
        return nullptr;
    }
    //! Create the write_id
    write_id = new write_t;
    write_id->data = new char[length];  // Allocate sufficient memory for data
    memcpy(write_id->data, data, length);
    write_id->length = length;
    write_id->offset = offset;
    write_id->filename = fl->filename;

    //! Copy the data onto the file
    memcpy((char*)fl->mapped_file + offset, data, length);
    if (offset + length > fl->file_length) {
        fl->file_length = offset + length;
    }

    VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns non NULL.
    return write_id;
}

int gtfs_sync_write_file(write_t* write_id) {
    int ret = -1;
    if (write_id) {
        VERBOSE_PRINT(do_verbose, "Persisting write of " << write_id->length << " bytes starting from offset " << write_id->offset << " inside file " << write_id->filename << "\n");
    } else {
        VERBOSE_PRINT(do_verbose, "Write operation does not exist\n");
        return ret;
    }
    //TODO: Add any additional initializations and checks, and complete the functionality

    VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns number of bytes written.
    return ret;
}

int gtfs_abort_write_file(write_t* write_id) {
    int ret = -1;
    if (write_id) {
        VERBOSE_PRINT(do_verbose, "Aborting write of " << write_id->length << " bytes starting from offset " << write_id->offset << " inside file " << write_id->filename << "\n");
    } else {
        VERBOSE_PRINT(do_verbose, "Write operation does not exist\n");
        return ret;
    }
    //TODO: Add any additional initializations and checks, and complete the functionality

    VERBOSE_PRINT(do_verbose, "Success.\n"); //On success returns 0.
    return ret;
}

// BONUS: Implement below API calls to get bonus credits

int gtfs_clean_n_bytes(gtfs_t *gtfs, int bytes){
    int ret = -1;
    if (gtfs) {
        VERBOSE_PRINT(do_verbose, "Cleaning up [ " << bytes << " bytes ] GTFileSystem inside directory " << gtfs->dirname << "\n");
    } else {
        VERBOSE_PRINT(do_verbose, "GTFileSystem does not exist\n");
        return ret;
    }

    VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns 0.
    return ret;
}

int gtfs_sync_write_file_n_bytes(write_t* write_id, int bytes){
    int ret = -1;
    if (write_id) {
        VERBOSE_PRINT(do_verbose, "Persisting [ " << bytes << " bytes ] write of " << write_id->length << " bytes starting from offset " << write_id->offset << " inside file " << write_id->filename << "\n");
    } else {
        VERBOSE_PRINT(do_verbose, "Write operation does not exist\n");
        return ret;
    }

    VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns 0.
    return ret;
}

