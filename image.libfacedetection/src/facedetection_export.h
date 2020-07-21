
#ifndef FACEDETECTION_EXPORT_H
#define FACEDETECTION_EXPORT_H

#ifdef FACEDETECTION_STATIC_DEFINE
#  define FACEDETECTION_EXPORT
#  define FACEDETECTION_NO_EXPORT
#else
#  ifndef FACEDETECTION_EXPORT
#    ifdef facedetection_EXPORTS
        /* We are building this library */
#      define FACEDETECTION_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define FACEDETECTION_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef FACEDETECTION_NO_EXPORT
#    define FACEDETECTION_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef FACEDETECTION_DEPRECATED
#  define FACEDETECTION_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef FACEDETECTION_DEPRECATED_EXPORT
#  define FACEDETECTION_DEPRECATED_EXPORT FACEDETECTION_EXPORT FACEDETECTION_DEPRECATED
#endif

#ifndef FACEDETECTION_DEPRECATED_NO_EXPORT
#  define FACEDETECTION_DEPRECATED_NO_EXPORT FACEDETECTION_NO_EXPORT FACEDETECTION_DEPRECATED
#endif

#define DEFINE_NO_DEPRECATED 0
#if DEFINE_NO_DEPRECATED
# define FACEDETECTION_NO_DEPRECATED
#endif

#endif
