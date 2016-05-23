
// This is a generated file. Do not edit!

#ifndef GLBINDING_COMPILER_DETECTION_H
#define GLBINDING_COMPILER_DETECTION_H

#ifdef __cplusplus
# define GLBINDING_COMPILER_IS_Comeau 0
# define GLBINDING_COMPILER_IS_Intel 0
# define GLBINDING_COMPILER_IS_PathScale 0
# define GLBINDING_COMPILER_IS_Embarcadero 0
# define GLBINDING_COMPILER_IS_Borland 0
# define GLBINDING_COMPILER_IS_Watcom 0
# define GLBINDING_COMPILER_IS_OpenWatcom 0
# define GLBINDING_COMPILER_IS_SunPro 0
# define GLBINDING_COMPILER_IS_HP 0
# define GLBINDING_COMPILER_IS_Compaq 0
# define GLBINDING_COMPILER_IS_zOS 0
# define GLBINDING_COMPILER_IS_XL 0
# define GLBINDING_COMPILER_IS_VisualAge 0
# define GLBINDING_COMPILER_IS_PGI 0
# define GLBINDING_COMPILER_IS_Cray 0
# define GLBINDING_COMPILER_IS_TI 0
# define GLBINDING_COMPILER_IS_Fujitsu 0
# define GLBINDING_COMPILER_IS_SCO 0
# define GLBINDING_COMPILER_IS_AppleClang 0
# define GLBINDING_COMPILER_IS_Clang 0
# define GLBINDING_COMPILER_IS_GNU 0
# define GLBINDING_COMPILER_IS_MSVC 0
# define GLBINDING_COMPILER_IS_ADSP 0
# define GLBINDING_COMPILER_IS_IAR 0
# define GLBINDING_COMPILER_IS_ARMCC 0
# define GLBINDING_COMPILER_IS_MIPSpro 0

#if defined(__COMO__)
# undef GLBINDING_COMPILER_IS_Comeau
# define GLBINDING_COMPILER_IS_Comeau 1

#elif defined(__INTEL_COMPILER) || defined(__ICC)
# undef GLBINDING_COMPILER_IS_Intel
# define GLBINDING_COMPILER_IS_Intel 1

#elif defined(__PATHCC__)
# undef GLBINDING_COMPILER_IS_PathScale
# define GLBINDING_COMPILER_IS_PathScale 1

#elif defined(__BORLANDC__) && defined(__CODEGEARC_VERSION__)
# undef GLBINDING_COMPILER_IS_Embarcadero
# define GLBINDING_COMPILER_IS_Embarcadero 1

#elif defined(__BORLANDC__)
# undef GLBINDING_COMPILER_IS_Borland
# define GLBINDING_COMPILER_IS_Borland 1

#elif defined(__WATCOMC__) && __WATCOMC__ < 1200
# undef GLBINDING_COMPILER_IS_Watcom
# define GLBINDING_COMPILER_IS_Watcom 1

#elif defined(__WATCOMC__)
# undef GLBINDING_COMPILER_IS_OpenWatcom
# define GLBINDING_COMPILER_IS_OpenWatcom 1

#elif defined(__SUNPRO_CC)
# undef GLBINDING_COMPILER_IS_SunPro
# define GLBINDING_COMPILER_IS_SunPro 1

#elif defined(__HP_aCC)
# undef GLBINDING_COMPILER_IS_HP
# define GLBINDING_COMPILER_IS_HP 1

#elif defined(__DECCXX)
# undef GLBINDING_COMPILER_IS_Compaq
# define GLBINDING_COMPILER_IS_Compaq 1

#elif defined(__IBMCPP__) && defined(__COMPILER_VER__)
# undef GLBINDING_COMPILER_IS_zOS
# define GLBINDING_COMPILER_IS_zOS 1

#elif defined(__IBMCPP__) && !defined(__COMPILER_VER__) && __IBMCPP__ >= 800
# undef GLBINDING_COMPILER_IS_XL
# define GLBINDING_COMPILER_IS_XL 1

#elif defined(__IBMCPP__) && !defined(__COMPILER_VER__) && __IBMCPP__ < 800
# undef GLBINDING_COMPILER_IS_VisualAge
# define GLBINDING_COMPILER_IS_VisualAge 1

#elif defined(__PGI)
# undef GLBINDING_COMPILER_IS_PGI
# define GLBINDING_COMPILER_IS_PGI 1

#elif defined(_CRAYC)
# undef GLBINDING_COMPILER_IS_Cray
# define GLBINDING_COMPILER_IS_Cray 1

#elif defined(__TI_COMPILER_VERSION__)
# undef GLBINDING_COMPILER_IS_TI
# define GLBINDING_COMPILER_IS_TI 1

#elif defined(__FUJITSU) || defined(__FCC_VERSION) || defined(__fcc_version)
# undef GLBINDING_COMPILER_IS_Fujitsu
# define GLBINDING_COMPILER_IS_Fujitsu 1

#elif defined(__SCO_VERSION__)
# undef GLBINDING_COMPILER_IS_SCO
# define GLBINDING_COMPILER_IS_SCO 1

#elif defined(__clang__) && defined(__apple_build_version__)
# undef GLBINDING_COMPILER_IS_AppleClang
# define GLBINDING_COMPILER_IS_AppleClang 1

#elif defined(__clang__)
# undef GLBINDING_COMPILER_IS_Clang
# define GLBINDING_COMPILER_IS_Clang 1

#elif defined(__GNUC__)
# undef GLBINDING_COMPILER_IS_GNU
# define GLBINDING_COMPILER_IS_GNU 1

#elif defined(_MSC_VER)
# undef GLBINDING_COMPILER_IS_MSVC
# define GLBINDING_COMPILER_IS_MSVC 1

#elif defined(__VISUALDSPVERSION__) || defined(__ADSPBLACKFIN__) || defined(__ADSPTS__) || defined(__ADSP21000__)
# undef GLBINDING_COMPILER_IS_ADSP
# define GLBINDING_COMPILER_IS_ADSP 1

#elif defined(__IAR_SYSTEMS_ICC__ ) || defined(__IAR_SYSTEMS_ICC)
# undef GLBINDING_COMPILER_IS_IAR
# define GLBINDING_COMPILER_IS_IAR 1

#elif defined(__ARMCC_VERSION)
# undef GLBINDING_COMPILER_IS_ARMCC
# define GLBINDING_COMPILER_IS_ARMCC 1

#elif defined(_SGI_COMPILER_VERSION) || defined(_COMPILER_VERSION)
# undef GLBINDING_COMPILER_IS_MIPSpro
# define GLBINDING_COMPILER_IS_MIPSpro 1


#endif

#  if GLBINDING_COMPILER_IS_AppleClang

#    if !(((__clang_major__ * 100) + __clang_minor__) >= 400)
#      error Unsupported compiler version
#    endif

# define GLBINDING_COMPILER_VERSION_MAJOR (__clang_major__)
# define GLBINDING_COMPILER_VERSION_MINOR (__clang_minor__)
# define GLBINDING_COMPILER_VERSION_PATCH (__clang_patchlevel__)
# if defined(_MSC_VER)
   /* _MSC_VER = VVRR */
#  define GLBINDING_SIMULATE_VERSION_MAJOR (_MSC_VER / 100)
#  define GLBINDING_SIMULATE_VERSION_MINOR (_MSC_VER % 100)
# endif
# define GLBINDING_COMPILER_VERSION_TWEAK (__apple_build_version__)

#    if ((__clang_major__ * 100) + __clang_minor__) >= 400 && __has_feature(cxx_thread_local)
#      define GLBINDING_COMPILER_CXX_THREAD_LOCAL 1
#    else
#      define GLBINDING_COMPILER_CXX_THREAD_LOCAL 0
#    endif

#  elif GLBINDING_COMPILER_IS_Clang

#    if !(((__clang_major__ * 100) + __clang_minor__) >= 304)
#      error Unsupported compiler version
#    endif

# define GLBINDING_COMPILER_VERSION_MAJOR (__clang_major__)
# define GLBINDING_COMPILER_VERSION_MINOR (__clang_minor__)
# define GLBINDING_COMPILER_VERSION_PATCH (__clang_patchlevel__)
# if defined(_MSC_VER)
   /* _MSC_VER = VVRR */
#  define GLBINDING_SIMULATE_VERSION_MAJOR (_MSC_VER / 100)
#  define GLBINDING_SIMULATE_VERSION_MINOR (_MSC_VER % 100)
# endif

#    if ((__clang_major__ * 100) + __clang_minor__) >= 304 && __has_feature(cxx_thread_local)
#      define GLBINDING_COMPILER_CXX_THREAD_LOCAL 1
#    else
#      define GLBINDING_COMPILER_CXX_THREAD_LOCAL 0
#    endif

#  elif GLBINDING_COMPILER_IS_GNU

#    if !((__GNUC__ * 100 + __GNUC_MINOR__) >= 404)
#      error Unsupported compiler version
#    endif

# define GLBINDING_COMPILER_VERSION_MAJOR (__GNUC__)
# if defined(__GNUC_MINOR__)
#  define GLBINDING_COMPILER_VERSION_MINOR (__GNUC_MINOR__)
# endif
# if defined(__GNUC_PATCHLEVEL__)
#  define GLBINDING_COMPILER_VERSION_PATCH (__GNUC_PATCHLEVEL__)
# endif

#    if (__GNUC__ * 100 + __GNUC_MINOR__) >= 408 && __cplusplus >= 201103L
#      define GLBINDING_COMPILER_CXX_THREAD_LOCAL 1
#    else
#      define GLBINDING_COMPILER_CXX_THREAD_LOCAL 0
#    endif

#  elif GLBINDING_COMPILER_IS_MSVC

#    if !(_MSC_VER >= 1600)
#      error Unsupported compiler version
#    endif

  /* _MSC_VER = VVRR */
# define GLBINDING_COMPILER_VERSION_MAJOR (_MSC_VER / 100)
# define GLBINDING_COMPILER_VERSION_MINOR (_MSC_VER % 100)
# if defined(_MSC_FULL_VER)
#  if _MSC_VER >= 1400
    /* _MSC_FULL_VER = VVRRPPPPP */
#   define GLBINDING_COMPILER_VERSION_PATCH (_MSC_FULL_VER % 100000)
#  else
    /* _MSC_FULL_VER = VVRRPPPP */
#   define GLBINDING_COMPILER_VERSION_PATCH (_MSC_FULL_VER % 10000)
#  endif
# endif
# if defined(_MSC_BUILD)
#  define GLBINDING_COMPILER_VERSION_TWEAK (_MSC_BUILD)
# endif

#    if _MSC_VER >= 1900
#      define GLBINDING_COMPILER_CXX_THREAD_LOCAL 1
#    else
#      define GLBINDING_COMPILER_CXX_THREAD_LOCAL 0
#    endif

#  else
#    error Unsupported compiler
#  endif

#  if GLBINDING_COMPILER_CXX_THREAD_LOCAL
#    define GLBINDING_THREAD_LOCAL thread_local
#  elif GLBINDING_COMPILER_IS_GNU || GLBINDING_COMPILER_IS_Clang || GLBINDING_COMPILER_IS_AppleClang
#    define GLBINDING_THREAD_LOCAL __thread
#  elif GLBINDING_COMPILER_IS_MSVC
#    define GLBINDING_THREAD_LOCAL __declspec(thread)
#  else
// GLBINDING_THREAD_LOCAL not defined for this configuration.
#  endif

#endif

#endif
