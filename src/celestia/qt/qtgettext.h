/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include <QTranslator>
#include "libintl.h"

class CelestiaQTranslator : public QTranslator
{
    inline QString translate(const char* /* context */,
                             const char* msgid,
                             const char* disambiguation = nullptr,
                             int n = -1) const override;
};

QString
CelestiaQTranslator::translate(const char*,
                               const char *msgid,
                               const char*,
                               int) const
{
    return QString(gettext(msgid));
}
