#include "include/klapi.h"
#include "include/common/klconfig.h"
#include "include/misc/klutils.h"
#include "include/value/klcfunc.h"
#include "include/value/klvalue.h"
#include "include/vm/klexception.h"

#include <endian.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


static KlException os_clock(KlState* state);
static KlException os_getenv(KlState* state);
static KlException os_execve(KlState* state);
static KlException os_fork(KlState* state);
static KlException os_createclass(KlState* state);


KlException KLCONFIG_LIBRARY_OS_ENTRYFUNCNAME(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 1));
  KLAPI_PROTECT(klapi_pushstring(state, "os"));
  KLAPI_PROTECT(os_createclass(state));
  KLAPI_PROTECT(klapi_storeglobal(state, klapi_getstring(state, -2), -1));
  return klapi_return(state, 0);
}

static KlException os_clock(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 1));
  klapi_pushfloat(state, klcast(KlFloat, clock()) / klcast(KlFloat, CLOCKS_PER_SEC));
  return klapi_return(state, 1);
}

static KlException os_getenv(KlState* state) {
  if (kl_unlikely(klapi_narg(state) != 1))
    return klapi_throw_internal(state, KL_E_ARGNO, "expected exactly one argument");
  if (kl_unlikely(!klapi_checkstring(state, -1)))
    return klapi_throw_internal(state, KL_E_TYPE, "expected string, got %s",
                                klstring_content(klapi_typename(state, klapi_access(state, -1))));

  KlString* envkey = klapi_getstring(state, -1);
  const char* envval = getenv(klstring_content(envkey));
  KLAPI_PROTECT(klapi_checkstack(state, 1));
  if (envval) {
    KLAPI_PROTECT(klapi_pushstring(state, envval));
  } else {
    klapi_pushnil(state, 1);
  }
  return klapi_return(state, 1);
}

static KlException construct_string_array(KlState* state, KlArray *array, char ***string_array, size_t *len) {
  for (size_t i = 0; i < klarray_size(array); ++i) {
    if (kl_unlikely(!klvalue_isstring(klarray_access(array, i))))
      return klapi_throw_internal(state, KL_E_TYPE, "expected a string array");
  }
  char **carray =
      malloc(sizeof(const char *) * (klarray_size(array) + 1));
  if (kl_unlikely(!carray)) {
    return klapi_throw_internal(
        state, KL_E_OOM, "out of memory when constructing array for execve()");
  }

  for (unsigned i = 0; i < klarray_size(array); ++i) {
    carray[i] = strdup(klstring_content( klcast(KlString *, klvalue_getgcobj(klarray_access(array, i)))));
    if (kl_unlikely(!carray[i])) {
      for (unsigned j = 0; j < i; ++i)
        free(carray[j]);
      free(carray);
      return klapi_throw_internal(
          state, KL_E_OOM,
          "out of memory when constructing array for execve()");
    }
  }

  carray[klarray_size(array)] = NULL;
  *string_array = carray;
  *len = klarray_size(array);
  return KL_E_NONE;
}

static void destroy_string_array(char **array, size_t len) {
  for (size_t i = 0; i < len; ++i)
    free(array[i]);
  free(array);
}

static KlException os_execve(KlState* state) {
  KLAPI_PROTECT(klapi_checkargs(state, 2, 3, "string", "array", "array"));

  const char *executable = klstring_content(klapi_getstringb(state, 0));
  KlArray *arguments = klapi_getarrayb(state, 1);
  char **argv;
  size_t argc;
  KLAPI_PROTECT(construct_string_array(state, arguments, &argv, &argc));
  if (klapi_narg(state) == 2) {
    execv(executable, argv);
  } else {
    char **envp;
    size_t envc;
    KlArray *environments = klapi_getarrayb(state, 2);
    KlException exception = construct_string_array(state, environments, &envp, &envc);
    if (kl_unlikely(exception)) {
      destroy_string_array(argv, argc);
      return exception;
    }
    execve(executable, argv, envp);
    destroy_string_array(envp, envc);
  }

  destroy_string_array(argv, argc);
  return klapi_throw_internal(state, KL_E_INVLD, "%s", strerror(errno));
  return klapi_return(state, 0);
}

static KlException os_fork(KlState* state) {
  klapi_checkstack(state, 1);
  pid_t pid = fork();
  if (kl_unlikely(pid == -1))
    return klapi_throw_internal(state, KL_E_INVLD, "%s", strerror(errno));
  klapi_pushint(state, pid);
  return klapi_return(state, 1);
}
static KlException os_createclass(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 3));
  KlClass* osclass = klclass_create(klstate_getmm(state), 1, KLOBJECT_DEFAULT_ATTROFF, NULL, NULL);
  if (kl_unlikely(!osclass))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating os");
  klapi_pushobj(state, osclass, KL_CLASS);

  KLAPI_PROTECT(klapi_pushstring(state, "clock"));
  klapi_pushcfunc(state, os_clock);
  KLAPI_PROTECT(klapi_class_newshared_normal(state, osclass, klapi_getstring(state, -2)));

  KLAPI_PROTECT(klapi_setstring(state, -2, "getenv"));
  klapi_setcfunc(state, -1, os_getenv);
  KLAPI_PROTECT(klapi_class_newshared_normal(state, osclass, klapi_getstring(state, -2)));

  KLAPI_PROTECT(klapi_setstring(state, -2, "execve"));
  klapi_setcfunc(state, -1, os_execve);
  KLAPI_PROTECT(klapi_class_newshared_normal(state, osclass, klapi_getstring(state, -2)));

  KLAPI_PROTECT(klapi_setstring(state, -2, "fork"));
  klapi_setcfunc(state, -1, os_fork);
  KLAPI_PROTECT(klapi_class_newshared_normal(state, osclass, klapi_getstring(state, -2)));

  klapi_pop(state, 2);
  return KL_E_NONE;
}
