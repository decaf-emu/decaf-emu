
#ifndef GLBINDING_API_H
#define GLBINDING_API_H

#ifdef GLBINDING_STATIC_DEFINE
#  define GLBINDING_API
#  define GLBINDING_NO_EXPORT
#else
#  ifndef GLBINDING_API
#    ifdef glbinding_EXPORTS
        /* We are building this library */
#      define GLBINDING_API __declspec(dllexport)
#    else
        /* We are using this library */
#      define GLBINDING_API __declspec(dllimport)
#    endif
#  endif

#  ifndef GLBINDING_NO_EXPORT
#    define GLBINDING_NO_EXPORT 
#  endif
#endif

#ifndef GLBINDING_DEPRECATED
#  define GLBINDING_DEPRECATED __declspec(deprecated)
#endif

#ifndef GLBINDING_DEPRECATED_EXPORT
#  define GLBINDING_DEPRECATED_EXPORT GLBINDING_API GLBINDING_DEPRECATED
#endif

#ifndef GLBINDING_DEPRECATED_NO_EXPORT
#  define GLBINDING_DEPRECATED_NO_EXPORT GLBINDING_NO_EXPORT GLBINDING_DEPRECATED
#endif

#define DEFINE_NO_DEPRECATED 0
#if DEFINE_NO_DEPRECATED
# define GLBINDING_NO_DEPRECATED
#endif

#endif
