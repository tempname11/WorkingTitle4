// https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950
// code not scrutinized or modified, aside from preprocessor stuff

#include <string.h>
#include <limits.h>     /* PATH_MAX */
#include <sys/stat.h>   /* mkdir(2) */
#include <errno.h>

#ifdef WINDOWS
  #include <direct.h>
  #define mkdir(path, _) _mkdir(path)
#endif
#ifndef PATH_MAX
  #define PATH_MAX 1024
#endif

int mkdir_p(const char* path)
{
  /* Adapted from http://stackoverflow.com/a/2336245/119527 */
  const size_t len = strlen(path);
  char _path[PATH_MAX];
  char* p;

  errno = 0;

  /* Copy string so its mutable */
  if (len > sizeof(_path) - 1) {
    errno = ENAMETOOLONG;
    return -1;
  }
  strcpy(_path, path);

  /* Iterate the string */
  for (p = _path + 1; *p; p++) {
    if (*p == '/') {
      /* Temporarily truncate */
      *p = '\0';

      if (mkdir(_path, S_IRWXU) != 0) {
        if (errno != EEXIST)
          return -1;
      }

      *p = '/';
    }
  }

  if (mkdir(_path, S_IRWXU) != 0) {
    if (errno != EEXIST)
      return -1;
  }

  return 0;
}