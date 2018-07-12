/* The Switch doesn't do dynamic loading afaict, so
   this is a stub implementation */

#include "dlfcn.h"

char *__dlerror_string_err = "This is a stub, all calls fail";
char *__dlerror_string_cur = 0;

/* Open the shared object FILE and map it in; return a handle that can be
   passed to `dlsym' to get symbol values from it.  */
void *dlopen (const char *__file, int __mode) {
  return 0; // NULL handle, a failure
}

/* Unmap and close a shared object opened by `dlopen'.
   The handle cannot be used again after calling `dlclose'.  */
int dlclose (void *__handle) {
  return -1; // Non-zero value, a failure
}

/* Find the run-time address in the shared object HANDLE refers to
   of the symbol called NAME.  */
void *dlsym (void *__handle, const char *__name) {
  __dlerror_string_cur = __dlerror_string_err;
  return 0; // NULL pointer
}

char *dlerror() {
  char *ret = __dlerror_string_cur;
  __dlerror_string_cur = 0;
  return ret;
}