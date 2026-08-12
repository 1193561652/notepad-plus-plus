#pragma once

#include <QString>

namespace NppQtCompat {

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
constexpr auto SkipEmptyParts = Qt::SkipEmptyParts;
#else
constexpr auto SkipEmptyParts = QString::SkipEmptyParts;
#endif

}
