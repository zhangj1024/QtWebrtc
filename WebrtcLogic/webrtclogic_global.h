#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(WEBRTCLOGIC_LIB)
#  define WEBRTCLOGIC_EXPORT Q_DECL_EXPORT
# else
#  define WEBRTCLOGIC_EXPORT Q_DECL_IMPORT
# endif
#else
# define WEBRTCLOGIC_EXPORT
#endif
