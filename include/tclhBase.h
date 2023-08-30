#ifndef TCLHBASE_H
#define TCLHBASE_H

/*
 * Copyright (c) 2022-2023, Ashok P. Nadkarni
 * All rights reserved.
 *
 * See the file LICENSE for license
 */

#define TCLH_VERSION_MAJOR 1
#define TCLH_VERSION_MINOR 0

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "tcl.h"

#ifdef _WIN32
#include <windows.h>
#endif

/* TBD - constrain length of all arguments in error messages */

#if (TCL_MAJOR_VERSION > 8) || (TCL_MAJOR_VERSION == 8 && TCL_MINOR_VERSION >= 7)
#define TCLH_TCLAPI_VERSION 0x0807
#else
#define TCLH_TCLAPI_VERSION 0x0806
#endif

#if TCL_MAJOR_VERSION > 8
# if TCL_UTF_MAX != 4
#   error TCL_UTF_MAX must be 4 if Tcl major version is 9 or above.
# endif
#else
# if TCL_UTF_MAX != 3
#   error TCL_UTF_MAX must be 3 if Tcl major version is less than 9.
# endif
#endif

#if TCLH_TCLAPI_VERSION >= 0x0807
#define TCLH_TCL87API
#include "tommath.h"
#else
#define TCLH_USE_TCL_TOMMATH
#include "tclTomMath.h"
#endif

/* Common definitions included by all Tcl helper *implementations* */

/* See https://gcc.gnu.org/wiki/Visibility for these definitions */
#if defined _WIN32 || defined __CYGWIN__
  #define TCLH_HELPER_DLL_IMPORT __declspec(dllimport)
  #define TCLH_HELPER_DLL_EXPORT __declspec(dllexport)
  #define TCLH_HELPER_DLL_LOCAL
#else
  #if __GNUC__ >= 4
    #define TCLH_HELPER_DLL_IMPORT __attribute__ ((visibility ("default")))
    #define TCLH_HELPER_DLL_EXPORT __attribute__ ((visibility ("default")))
    #define TCLH_HELPER_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define TCLH_HELPER_DLL_IMPORT
    #define TCLH_HELPER_DLL_EXPORT
    #define TCLH_HELPER_DLL_LOCAL
  #endif
#endif

/*
 * Now we use the generic helper definitions above to define TCLH_API and
 * TCLH_LOCAL. TCLH_API is used for the exported API symbols from the shared
 * library. It either DLL
 * imports or DLL exports (or does nothing for static build) TCLH_LOCAL is
 * used for non-api symbols.
 */
#ifdef STATIC_BUILD /* defined in Tcl configure for static build */
# define TCLH_API
# define TCLH_LOCAL
#else
  #ifdef TCLH_DLL_EXPORTS // defined if we are building the TCLH DLL (instead of using it)
    #define TCLH_API TCLH_HELPER_DLL_EXPORT
  #else
    #define TCLH_API TCLH_HELPER_DLL_IMPORT
  #endif // TCLH_DLL_EXPORTS
  #define TCLH_LOCAL TCLH_HELPER_DLL_LOCAL
#endif // FOX_DLL

#ifndef TCLH_INLINE
#ifdef _MSC_VER
# define TCLH_INLINE __inline
#else
# define TCLH_INLINE static inline
#endif
#endif

#ifndef TCLH_PANIC
#define TCLH_PANIC Tcl_Panic
#endif

#ifndef TCLH_ASSERT_LEVEL
# ifdef NDEBUG
#  define TCLH_ASSERT_LEVEL 0
# else
#  define TCLH_ASSERT_LEVEL 2
# endif
#endif

#define TCLH_MAKESTRINGLITERAL(s_) # s_
 /*
  * Stringifying special CPP symbols (__LINE__) needs another level of macro
  */
#define TCLH_MAKESTRINGLITERAL2(s_) TCLH_MAKESTRINGLITERAL(s_)

#if TCLH_ASSERT_LEVEL == 0
# define TCLH_ASSERT(bool_) (void) 0
#elif TCLH_ASSERT_LEVEL == 1
# define TCLH_ASSERT(bool_) (void)( (bool_) || (__debugbreak(), 0))
#elif TCLSH_ENABLE_ASSERT == 2 && defined(_WIN32)
#define TCLSH_ASSERT(bool_)                                             \
    (void)((bool_)                                                      \
           || (DebugOutput("Assertion (" #bool_                         \
                           ") failed at line " TCLH_MAKESTRINGLITERAL2( \
                               __LINE__) " in file " __FILE__ "\n"),    \
               0))
#else
# define TCLH_ASSERT(bool_) (void)( (bool_) || (TCLH_PANIC("Assertion (" #bool_ ") failed at line %d in file %s.", __LINE__, __FILE__), 0) )
#endif

#if defined(static_assert)
# define TCLH_STATIC_ASSERT(e_) static_assert(e_)
#elif defined(C_ASSERT)
# define TCLH_STATIC_ASSERT(e_) C_ASSERT(e_)
#else
# define TCLH_STATIC_ASSERT(e_) assert(e_)
#endif

#ifdef TCLH_IMPL
#ifndef TCLH_EMBEDDER
#error TCLH_EMBEDDER not defined. Please #define this to name of your package.
#endif
#endif

/* Typedef: Tclh_LibContext
 *
 * This is a handle to a context internally maintained by the Tclh library
 * and shared between modules.
 */
typedef struct Tclh_LibContext Tclh_LibContext;

/* 
 * Typedef: Tclh_ReturnCode
 * Holds a Tcl return code defined in Tcl (e.g. TCL_OK, TCL_ERROR etc.)
 */
typedef int Tclh_ReturnCode;

/*
 * Typedef: Tcl_Size
 * This typedef is defined for 8.6 for compatibility with 8.7 and up.
 */
#if TCLH_TCLAPI_VERSION < 0x0807
typedef int Tcl_Size;
# define Tcl_GetSizeIntFromObj Tcl_GetIntFromObj
# define Tcl_NewSizeIntObj Tcl_NewIntObj
# define TCL_SIZE_MAX      INT_MAX
# define TCL_SIZE_MODIFIER ""
#endif

/*
 * Typedef: Tclh_Bool
 * Holds 0 or non-0 indicating false and true.
 */
typedef int Tclh_Bool;

TCLH_INLINE char *
Tclh_memdup(void *from, int len)
{
    void *to = Tcl_Alloc(len);
    memcpy(to, from, len);
    return to;
}

TCLH_INLINE Tcl_Size Tclh_strlen(const char *s) {
    size_t len = strlen(s);
    TCLH_ASSERT(len <= TCL_SIZE_MAX);
    return (Tcl_Size)len;
}

TCLH_INLINE char *Tclh_strdup(const char *from) {
    Tcl_Size len = Tclh_strlen(from) + 1;
    char *to = Tcl_Alloc(len);
    memcpy(to, from, len);
    return to;
}

TCLH_INLINE char *Tclh_strdupn(const char *from, Tcl_Size len) {
    char *to = Tcl_Alloc(len+1);
    memcpy(to, from, len);
    to[len] = '\0';
    return to;
}

/* Section: Library initialization
 *
 * Each module in the library has a initialization function that needs to
 * be called before invoking any of that module's functions. The Tclh
 * context for the interpreter is automatically created at that time
 * through <Tclh_LibInit> if it does not exist.
 * 
 * However, for efficiency reasons, it is recommended that <Tclh_LibInit> be
 * explicitly called before any other module initialization and the
 * context returned be passed to other module API's. This avoids a separate
 * lookup to retrieve the context from the interpreter.
 */

/* Function: Tclh_LibInit 
 *
 * Parameters:
 * interp - Tcl interpreter to initialize
 * tclhCtxPP - location to store pointer to the Tclh context. May be NULL.
 *
 * Initializes and returns a Tclh context for the current interpreter. This
 * can then be optionally passed to various functions to bypass an additional
 * lookup to retrieve the context.
 * 
 * The context will be automatically freed when the interpreter is deleted.
 *
 * Returns:
 * TCL_OK    - Library was successfully initialized.
 * TCL_ERROR - Initialization failed. Library functions must not be called.
 *             An error message is left in the interpreter result.
*/
TCLH_LOCAL Tclh_ReturnCode Tclh_LibInit(Tcl_Interp *interp, Tclh_LibContext **tclhCtxPP);

/*
 * Error handling functions are also here because they are used by all modules.
 */

/* Section: Error reporting utilities
 *
 * Provides convenient error reporting functions for common error types.
 * The <Tclh_ErrorLibInit> function must be called before any other functions in
 * this module. In addition to storing an appropriate error message in the
 * interpreter, all functions record a Tcl error code in a consistent format.
 */

/* Macro: TCLH_CHECK_RESULT
 * Checks if a value and returns from the calling function if it
 * is anything other than TCL_OK.
 *
 * Parameters:
 * value_or_fncall - Value to check. Commonly this may be a function call which
 *                   returns one of the standard Tcl result values.
 */
#define TCLH_CHECK_RESULT(value_or_fncall)      \
    do {                                        \
        int result = value_or_fncall;           \
        if (result != TCL_OK)                   \
            return result;                      \
    } while (0)
#define TCLH_CHECK TCLH_CHECK_RESULT

/* Macro: TCLH_CHECK_NARGS
 * Checks the number of arguments passed as the objv[] array to a function
 * implementing a Tcl command to be within a specified range.
 *
 * Parameters:
 * min_ - minimum number of arguments
 * max_ - maximum number of arguments
 * message_ - the additional message argument passed to Tcl_WrongNumArgs to
 *            generate the error message.
 *
 * If the number arguments does not fall in within the range, stores an
 * error message in the interpreter and returns from the caller with a
 * TCL_ERROR result code. The macros assumes that variable names *objv* and
 * *objc* in the caller's context refer to the argument array and number of
 * arguments respectively. The generated error message uses one argument
 * from the passed objv array. See Tcl_WrongNumArgs for more information
 * about the generated error message.
 */
#define TCLH_CHECK_NARGS(ip_, min_, max_, message_)   \
    do {                                                \
        if (objc < min_ || objc > max_) {               \
            return Tclh_ErrorNumArgs(ip_, 1, objv, message_);    \
        }                                               \
    } while(0)


/* Function: Tclh_ErrorExists
 * Reports an error that an object already exists.
 *
 * Parameters:
 * interp  - Tcl interpreter in which to report the error.
 * type    - String indicating type of object, e.g. *File*. Defaults to *Object*
 *           if passed as NULL.
 * searchObj - The object being searched for, e.g. the file name. This is
 *           included in the error message if not *NULL*.
 * message - Additional text to append to the standard error message. May be NULL.
 *
 *
 * The Tcl *errorCode* variable is set to a list of three elements: the
 * *TCLH_EMBEDDER* macro value set by the extension, the literal string
 * *EXISTS* and the error message.
 *
 * Returns:
 * TCL_ERROR - Always returns this value so caller can just pass on the return
 *             value from this function.
 */
TCLH_LOCAL Tclh_ReturnCode
Tclh_ErrorExists(Tcl_Interp *interp,
                 const char *type,
                 Tcl_Obj *searchObj,
                 const char *message);

/* Function: Tclh_ErrorGeneric
 * Reports a generic error.
 *
 * Parameters:
 * interp - Tcl interpreter in which to report the error.
 * code   - String literal value to use for the error code. Defaults to
 *          *ERROR* if *NULL*.
 * message - Additional text to add to the error message. May be NULL.
 *
 * This is a catchall function to report any errors that do not fall into one
 * of the other categories of errors. If *message* is not *NULL*, it is stored
 * as the Tcl result. Otherwise, a generic *Unknown error* message is stored.
 *
 * The Tcl *errorCode* variable is set to a list of three elements: the
 * *TCLH_EMBEDDER* macro value set by the extension, the literal string *ERROR*
 * and the error message.
 *
 * Returns:
 * TCL_ERROR - Always returns this value so caller can just pass on the return
 *             value from this function.
 */
TCLH_LOCAL Tclh_ReturnCode
Tclh_ErrorGeneric(Tcl_Interp *interp, const char *code, const char *message);

/* Function: Tclh_ErrorNotFoundStr
 * Reports an error where an string is not found or is not accessible.
 *
 * Parameters:
 * interp  - Tcl interpreter in which to report the error.
 * type    - String indicating type of object, e.g. *File*. Defaults to *Object*
 *           if passed as NULL.
 * string - The object being searched for, e.g. the file name. This is
 *           included in the error message if not *NULL*.
 * message - Additional text to append to the standard error message. May be NULL.
 *
 *
 * The Tcl *errorCode* variable is set to a list of three elements: the
 * *TCLH_EMBEDDER* macro value set by the extension, the literal string
 * *NOT_FOUND* and the error message.
 *
 * Returns:
 * TCL_ERROR - Always returns this value so caller can just pass on the return
 *             value from this function.
 */
TCLH_LOCAL Tclh_ReturnCode Tclh_ErrorNotFoundStr(Tcl_Interp *interp,
                                      const char *type,
                                      const char *search,
                                      const char *message);

/* Function: Tclh_ErrorNotFound
 * Reports an error where an object is not found or is not accessible.
 *
 * Parameters:
 * interp  - Tcl interpreter in which to report the error.
 * type    - String indicating type of object, e.g. *File*. Defaults to *Object*
 *           if passed as NULL.
 * searchObj - The object being searched for, e.g. the file name. This is
 *           included in the error message if not *NULL*.
 * message - Additional text to append to the standard error message. May be NULL.
 *
 *
 * The Tcl *errorCode* variable is set to a list of three elements: the
 * *TCLH_EMBEDDER* macro value set by the extension, the literal string
 * *NOT_FOUND* and the error message.
 *
 * Returns:
 * TCL_ERROR - Always returns this value so caller can just pass on the return
 *             value from this function.
 */
TCLH_LOCAL Tclh_ReturnCode Tclh_ErrorNotFound(Tcl_Interp *interp,
                                   const char *type,
                                   Tcl_Obj *searchObj,
                                   const char *message);

/* Function: Tclh_ErrorOperFailed
 * Reports the failure of an operation.
 *
 * Parameters:
 * interp  - Tcl interpreter in which to report the error.
 * oper    - String describing the operation, e.g. *delete*. May be passed
 *           as *NULL*.
 * operandObj - The object on which the operand was attempted, e.g. the file
 *           name. This is included in the error message if not *NULL*.
 * message - Additional text to append to the standard error message. May be NULL.
 *
 *
 * The Tcl *errorCode* variable is set to a list of three elements: the
 * *TCLH_EMBEDDER* macro value set by the extension, the literal string
 * *OPER_FAILED* and the error message.
 *
 * Returns:
 * TCL_ERROR - Always returns this value so caller can just pass on the return
 *             value from this function.
 */
TCLH_LOCAL Tclh_ReturnCode Tclh_ErrorOperFailed(Tcl_Interp *interp,
                                     const char *type,
                                     Tcl_Obj *searchObj,
                                     const char *message);

/* Function: Tclh_ErrorInvalidValueStr
 * Reports an invalid argument passed in to a command function.
 *
 * Parameters:
 * interp    - Tcl interpreter in which to report the error.
 * badValue - The argument that was found to be invalid. This is
 *           included in the error message if not *NULL*.
 * message - Additional text to append to the standard error message. May be NULL.
 *
 * The Tcl *errorCode* variable is set to a list of three elements: the
 * *TCLH_EMBEDDER* macro value set by the extension, the literal string
 * *INVALID_VALUE* and the error message.
 *
 * Returns:
 * TCL_ERROR - Always returns this value so caller can just pass on the return
 *             value from this function.
 */
TCLH_LOCAL Tclh_ReturnCode Tclh_ErrorInvalidValueStr(Tcl_Interp *interp,
                                          const char *badValue,
                                          const char *message);

/* Function: Tclh_ErrorInvalidValue
 * Reports an invalid argument passed in to a command function.
 *
 * Parameters:
 * interp    - Tcl interpreter in which to report the error.
 * badArgObj - The object argument that was found to be invalid. This is
 *           included in the error message if not *NULL*.
 * message - Additional text to append to the standard error message. May be NULL.
 *
 * The Tcl *errorCode* variable is set to a list of three elements: the
 * *TCLH_EMBEDDER* macro value set by the extension, the literal string
 * *INVALID_VALUE* and the error message.
 *
 * Returns:
 * TCL_ERROR - Always returns this value so caller can just pass on the return
 *             value from this function.
 */
TCLH_LOCAL Tclh_ReturnCode Tclh_ErrorInvalidValue(Tcl_Interp *interp,
                                       Tcl_Obj *badArgObj,
                                       const char *message);

/* Function: Tclh_ErrorOptionMissing
 * Reports a required option is not specified.
 *
 * Parameters:
 * interp  - Tcl interpreter in which to report the error.
 * optNameObj - The option name. May be NULL.
 * message - Additional text to append to the standard error message. May be
 * NULL.
 *
 * The Tcl *errorCode* variable is set to a list of three elements: the
 * *TCLH_EMBEDDER* macro value set by the extension, the literal string
 * *OPTION_MISSING* and the error message.
 *
 * Returns:
 * TCL_ERROR - Always returns this value so caller can just pass on the return
 *             value from this function.
 */
TCLH_LOCAL Tclh_ReturnCode Tclh_ErrorOptionMissingStr(Tcl_Interp *interp,
                                                   const char *optName,
                                                   const char *message);

/* Function: Tclh_ErrorOptionValueMissing
 * Reports no option value has been specified for an option to a command
 *
 * Parameters:
 * interp  - Tcl interpreter in which to report the error.
 * optNameObj - The option name for which the value is missing. May be NULL.
 * message - Additional text to append to the standard error message. May be
 * NULL.
 *
 * The Tcl *errorCode* variable is set to a list of three elements: the
 * *TCLH_EMBEDDER* macro value set by the extension, the literal string
 * *OPTION_VALUE_MISSING* and the error message.
 *
 * Returns:
 * TCL_ERROR - Always returns this value so caller can just pass on the return
 *             value from this function.
 */
TCLH_LOCAL Tclh_ReturnCode Tclh_ErrorOptionValueMissing(Tcl_Interp *interp,
                                                        Tcl_Obj *optionNameObj,
                                                        const char *message);

/* Function: Tclh_ErrorNumArgs
 * Reports an invalid number of arguments passed into a command function.
 *
 * Parameters:
 * interp - Tcl interpreter in which to report the error.
 * objc   - Number of elements in the objv[] array to include in the error message.
 * objv   - Array containing command arguments.
 * message - Additional text to add to the error message. May be NULL.
 *
 * The Tcl *errorCode* variable is set to a list of three elements: the
 * *TCLH_EMBEDDER* macro value set by the extension, the literal string *ERROR*
 * and the error message.
 *
 * This is a simple wrapper around the standard *Tcl_WrongNumArgs* function
 * except it returns the *TCL_ERROR* as a result. See that function description
 * for details.
 *
 * Returns:
 * TCL_ERROR - Always returns this value so caller can just pass on the return
 *             value from this function.
 */
TCLH_LOCAL Tclh_ReturnCode Tclh_ErrorNumArgs(Tcl_Interp *interp,
                                  int objc,
                                  Tcl_Obj *const objv[],
                                  const char *message);


/* Function: Tclh_ErrorWrongType
 * Reports an error where a value is of the wrong type.
 *
 * Parameters:
 * interp  - Tcl interpreter in which to report the error.
 * argObj  - The value in question. This is
 *           included in the error message if not *NULL*.
 * message - Additional text to append to the standard error message. May be *NULL*.
 *
 *
 * The Tcl *errorCode* variable is set to a list of three elements: the
 * *TCLH_EMBEDDER* macro value set by the extension, the literal string
 * *WRONG_TYPE* and the error message.
 *
 * Returns:
 * TCL_ERROR - Always returns this value so caller can just pass on the return
 *             value from this function.
 */
TCLH_LOCAL Tclh_ReturnCode
Tclh_ErrorWrongType(Tcl_Interp *interp, Tcl_Obj *argObj, const char *message);

/* Function: Tclh_ErrorAllocation
 * Reports an error where an allocation failed.
 *
 * Parameters:
 * interp  - Tcl interpreter in which to report the error.
 * type    - String indicating type of object, e.g. *Memory*. Defaults to *Object*
 *           if passed as NULL.
 * message - Additional text to append to the standard error message. May be NULL.
 *
 * The Tcl *errorCode* variable is set to a list of three elements: the
 * *TCLH_EMBEDDER* macro value set by the extension, the literal string
 * *ALLOCATION* and the error message.
 *
 * Returns:
 * TCL_ERROR - Always returns this value so caller can just pass on the return
 *             value from this function.
 */
TCLH_LOCAL Tclh_ReturnCode
Tclh_ErrorAllocation(Tcl_Interp *interp, const char *type, const char *message);

/* Function: Tclh_ErrorRange
 * Reports an out-of-range error for integers.
 *
 * Parameters:
 * interp  - Tcl interpreter in which to report the error.
 * objP    - Value that is out of range. May be NULL.
 * low     - low end of permitted range (inclusive)
 * high    - high end of permitted range (inclusive)
 *
 * Returns:
 * TCL_ERROR - Always returns this value so caller can just pass on the return
 *             value from this function.
 */
TCLH_LOCAL Tclh_ReturnCode Tclh_ErrorRange(Tcl_Interp *interp,
                                Tcl_Obj *objP,
                                Tcl_WideInt low,
                                Tcl_WideInt high);

/* Function: Tclh_ErrorEncodingFailed
 * Reports an encoding failure converting from utf8
 *
 * Parameters:
 * interp  - Tcl interpreter in which to report the error.
 * encoding_status - an encoding status value as returned by Tcl_ExternalToUtf
 * utf8 - Tcl internal string that failed to be transformed
 * utf8Len - length of the string. If < 0, treated as nul terminated.
 *
 * Returns:
 * TCL_ERROR - Always returns this value so caller can just pass on the return
 *             value from this function.
 */
TCLH_LOCAL Tclh_ReturnCode Tclh_ErrorEncodingFromUtf8(Tcl_Interp *ip,
                                           int encoding_status,
                                           const char *utf8,
                                           Tcl_Size utf8Len);

#ifdef _WIN32
/* Function: Tclh_ErrorWindowsError
 * Reports a Windows error code message.
 *
 * Parameters:
 * interp  - Tcl interpreter in which to report the error.
 * winerror - Windows error codnfe
 * message - Additional text to append to the standard error message. May be NULL.
 *
 * Returns:
 * TCL_ERROR - Always returns this value so caller can just pass on the return
 *             value from this function.
 */
TCLH_LOCAL Tclh_ReturnCode Tclh_ErrorWindowsError(Tcl_Interp *interp,
                                       unsigned int winerror,
                                       const char *message);
#endif


#ifdef TCLH_SHORTNAMES
#define ErrorGeneric    Tclh_ErrorGeneric
#define ErrorInvalidValue Tclh_ErrorInvalidValue
#define ErrorInvalidValueStr Tclh_ErrorInvalidValueStr
#define ErrorNotFound   Tclh_ErrorNotFound
#define ErrorNotFoundStr   Tclh_ErrorNotFoundStr
#define ErrorExists     Tclh_ErrorExists
#define ErrorWrongType  Tclh_ErrorWrongType
#define ErrorNumArgs    Tclh_ErrorNumArgs
#define ErrorOperFailed Tclh_ErrorOperFailed
#define ErrorAllocation Tclh_ErrorAllocation
#define ErrorRange      Tclh_ErrorRange
#define ErrorEncodingFromUtf8 Tclh_ErrorEncodingFromUtf8
#ifdef _WIN32
#define ErrorWindowsError Tclh_ErrorWindowsError
#endif
#endif

#ifdef TCLH_IMPL
#include "tclhBaseImpl.c"
#endif

#endif /* TCLHBASE_H */
