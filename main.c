#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>

unsigned long get_size_dirent(struct dirent* dirent_ptr, char* parent_dir);
unsigned long get_size_path(char*);
void handle_error();

char* combine_paths(char* path1, char* path2) {
  size_t result_len = strlen(path1) + 1 + strlen(path2);
  char* result = calloc(result_len + 1, 1);
  strcat(result, path1);
  strcat(result, "/");
  strcat(result, path2);
  return result;
}

char* dirent_type_str(struct dirent* dirent_ptr) {
  unsigned char dirent_type = dirent_ptr->d_type;
  switch (dirent_type) {
  case DT_DIR:
    return "Dir ";
  case DT_REG:
    return "File";
  }

  if (strcmp(dirent_ptr->d_name, ".") == 0 ||
      strcmp(dirent_ptr->d_name, "..") == 0) {
    // Special case since these are D_UNKNOWN for some reason
    return "Dir ";
  }

  printf("DEBUG: Unknown type %x\n", dirent_type);
  return "Unk ";
}

unsigned long get_size_dir(char* dir_path) {
  DIR* dir_stream = opendir(dir_path);

  unsigned long result = 0;

  struct dirent* dirent_ptr;
  while ((dirent_ptr = readdir(dir_stream))) {
    if (dirent_ptr->d_type == DT_DIR) {
      if (strcmp(dirent_ptr->d_name, ".") != 0 &&
          strcmp(dirent_ptr->d_name, "..") != 0) {
        char* subdir = combine_paths(dir_path, dirent_ptr->d_name);
        result += get_size_dir(subdir);
        free(subdir);
      }
    } else {
      result += get_size_dirent(dirent_ptr, dir_path);
    }
  }

  closedir(dir_stream);

  return result;
}

unsigned long get_size_dirent(struct dirent* dirent_ptr, char* parent_dir) {
  char* path = combine_paths(parent_dir, dirent_ptr->d_name);
  unsigned long result = get_size_path(path);
  free(path);
  return result;
}

unsigned long get_size_path(char* path) {
  struct stat sb;
  if (stat(path, &sb) == -1) {
    handle_error();
    return 0;
  }
  return sb.st_size;
}

void handle_error() {
  printf("ERROR: %s\n", strerror(errno));
  exit(1);
}

int main(int argc, char** argv) {
  char* dir_path = argc > 1 ? argv[1] : ".";
  printf("Stats for directory: %s\n", dir_path);
  printf("Total directory size: %lu\n", get_size_dir(dir_path));
  printf("\n");

  DIR* dir_stream = opendir(dir_path);

  struct dirent* dirent_ptr;
  while ((dirent_ptr = readdir(dir_stream))) {
    printf("%s %s\n", dirent_type_str(dirent_ptr), dirent_ptr->d_name);
    // For some reason, directories have 0 bytes associated with them?
    if (strcmp(dirent_ptr->d_name, "..") == 0) {
      printf("%s\n", "Size: <omitted due to performance concerns>");
    } else if (dirent_ptr->d_type == DT_DIR) {
      char* subdir = combine_paths(dir_path, dirent_ptr->d_name);
      printf("Size: %lu bytes\n", get_size_dir(subdir));
      free(subdir);
    } else {
      printf("Size: %lu bytes\n", get_size_dirent(dirent_ptr, dir_path));
    }
    printf("\n");
  }

  closedir(dir_stream);

  return 0;
}
