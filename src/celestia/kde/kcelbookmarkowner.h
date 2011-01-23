/***************************************************************************
                          kcelbookmarkowner.h  -  description
                             -------------------
    begin                : Fri Aug 30 2002
    copyright            : (C) 2002 by chris
    email                : chris@tux.teyssier.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __kcelbookmarkowner_h__
#define __kcelbookmarkowner_h__

#include <kbookmarkmanager.h>
#include "celestia/url.h"

class KCelBookmarkOwner : virtual public KBookmarkOwner {

public:
    virtual ~KCelBookmarkOwner() {};

    virtual QString currentIcon() const { return QString::null; };
    virtual Url currentUrl(Url::UrlType /*type*/=Url::Absolute) const { return Url(); };
};

#endif


