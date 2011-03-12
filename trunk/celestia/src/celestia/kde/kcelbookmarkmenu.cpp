// -*- mode: c++; c-basic-offset: 2 -*-
/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/


#include <sys/types.h>
#include <sys/stat.h>

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include "kcelbookmarkmenu.h"
#include <kbookmarkmenu.h>
#include "kbookmarkimporter.h"
#include "kcelbookmarkowner.h"

#include <qfile.h>
#include <qregexp.h>

#include <kaction.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kstdaccel.h>
#include <kstdaction.h>

template class QPtrList<KCelBookmarkMenu>;

/********************************************************************
 *
 * KCelBookmarkMenu
 *
 ********************************************************************/

KCelBookmarkMenu::KCelBookmarkMenu( KBookmarkManager* mgr,
                              KCelBookmarkOwner * _owner, KPopupMenu * _parentMenu,
                              KActionCollection *collec, bool _isRoot, bool _add,
                              const QString & parentAddress )
  : m_bIsRoot(_isRoot), m_bAddBookmark(_add),
    m_pManager(mgr), m_pOwner(_owner),
    m_parentMenu( _parentMenu ),
    m_actionCollection( collec ),
    m_parentAddress( parentAddress )
{
  m_lstSubMenus.setAutoDelete( true );
  m_actions.setAutoDelete( true );

  m_bNSBookmark = m_parentAddress.isNull();
  if ( !m_bNSBookmark ) // not for the netscape bookmark
  {
    //kdDebug(1203) << "KBookmarkMenu::KBookmarkMenu " << this << " address : " << m_parentAddress << endl;

    connect( _parentMenu, SIGNAL( aboutToShow() ),
             SLOT( slotAboutToShow() ) );

    if ( m_bIsRoot )
    {
      connect( m_pManager, SIGNAL( changed(const QString &, const QString &) ),
               SLOT( slotBookmarksChanged(const QString &) ) );
    }
  }

  // add entries that possibly have a shortcut, so they are available _before_ first popup
  if ( m_bIsRoot )
  {
    if ( m_bAddBookmark ) {
          addAddBookmark();
    }

    addEditBookmarks();
  }

  m_bDirty = true;
}

KCelBookmarkMenu::~KCelBookmarkMenu()
{
  //kdDebug(1203) << "KBookmarkMenu::~KBookmarkMenu() " << this << endl;
  QPtrListIterator<KAction> it( m_actions );
  for (; it.current(); ++it )
    it.current()->unplugAll();

  m_lstSubMenus.clear();
  m_actions.clear();
}

void KCelBookmarkMenu::ensureUpToDate()
{
  slotAboutToShow();
}


void KCelBookmarkMenu::slotAboutToShow()
{
  // Did the bookmarks change since the last time we showed them ?
  if ( m_bDirty )
  {
    m_bDirty = false;
    refill();
  }
}

void KCelBookmarkMenu::slotBookmarksChanged( const QString & groupAddress )
{
  if (m_bNSBookmark)
    return;

  if ( groupAddress == m_parentAddress )
  {
    //kdDebug(1203) << "KBookmarkMenu::slotBookmarksChanged -> setting m_bDirty on " << groupAddress << endl;
    m_bDirty = true;
  }
  else
  {
    // Iterate recursively into child menus
    QPtrListIterator<KCelBookmarkMenu> it( m_lstSubMenus );
    for (; it.current(); ++it )
    {
      it.current()->slotBookmarksChanged( groupAddress );
    }
  }
}

void KCelBookmarkMenu::refill()
{
  //kdDebug(1203) << "KBookmarkMenu::refill()" << endl;
  m_lstSubMenus.clear();

  QPtrListIterator<KAction> it( m_actions );
  for (; it.current(); ++it )
    it.current()->unplug( m_parentMenu );

  m_parentMenu->clear();
  m_actions.clear();

  fillBookmarkMenu();
  m_parentMenu->adjustSize();
}

void KCelBookmarkMenu::addAddBookmark()
{
  KAction * paAddBookmarks = new KAction( i18n( "&Add Bookmark" ),
                                          "bookmark_add",
                                          m_bIsRoot ? ALT + Key_B : 0,
                                          this,
                                          SLOT( slotAddBookmark() ),
                                          m_actionCollection, m_bIsRoot ? "add_bookmark" : 0 );

  paAddBookmarks->setStatusText( i18n( "Add a bookmark for the current document" ) );

  paAddBookmarks->plug( m_parentMenu );
  m_actions.append( paAddBookmarks );
  addAddRelativeBookmark();
  addAddSettingsBookmark();
}

void KCelBookmarkMenu::addAddRelativeBookmark()
{
  KAction * paAddBookmarks = new KAction( i18n( "Add &Relative Bookmark" ),
                                          "bookmark_add",
                                          m_bIsRoot ? ALT + Key_R : 0, //m_bIsRoot ? KStdAccel::addBookmark() : KShortcut(),
                                          this,
                                          SLOT( slotAddRelativeBookmark() ),
                                          m_actionCollection, m_bIsRoot ? "add_relative_bookmark" : 0 );

  paAddBookmarks->setStatusText( i18n( "Add a relative bookmark for the current document" ) );

  paAddBookmarks->plug( m_parentMenu );
  m_actions.append( paAddBookmarks );
}

void KCelBookmarkMenu::addAddSettingsBookmark()
{
  KAction * paAddBookmarks = new KAction( i18n( "Add &Settings Bookmark" ),
                                          "bookmark_add",
                                          m_bIsRoot ? CTRL + ALT + Key_S : 0, //m_bIsRoot ? KStdAccel::addBookmark() : KShortcut(),
                                          this,
                                          SLOT( slotAddSettingsBookmark() ),
                                          m_actionCollection, m_bIsRoot ? "add_settings_bookmark" : 0 );

  paAddBookmarks->setStatusText( i18n( "Add a settings bookmark for the current document" ) );

  paAddBookmarks->plug( m_parentMenu );
  m_actions.append( paAddBookmarks );
}

void KCelBookmarkMenu::addEditBookmarks()
{
  KAction * m_paEditBookmarks = KStdAction::editBookmarks( m_pManager, SLOT( slotEditBookmarks() ),
                                                             m_actionCollection, "edit_bookmarks" );
  m_paEditBookmarks->plug( m_parentMenu );
  m_paEditBookmarks->setStatusText( i18n( "Edit your bookmark collection in a separate window" ) );
  m_actions.append( m_paEditBookmarks );
}

void KCelBookmarkMenu::addNewFolder()
{
  KAction * paNewFolder = new KAction( i18n( "&New Folder..." ),
                                       "folder_new", //"folder",
                                       0,
                                       this,
                                       SLOT( slotNewFolder() ),
                                       m_actionCollection );

  paNewFolder->setStatusText( i18n( "Create a new bookmark folder in this menu" ) );

  paNewFolder->plug( m_parentMenu );
  m_actions.append( paNewFolder );
}

void KCelBookmarkMenu::fillBookmarkMenu()
{
  if ( m_bIsRoot )
  {
    if ( m_bAddBookmark )
      addAddBookmark();

    addEditBookmarks();

    if ( m_bAddBookmark )
      addNewFolder();

  }

  KBookmarkGroup parentBookmark = m_pManager->findByAddress( m_parentAddress ).toGroup();
  Q_ASSERT(!parentBookmark.isNull());
  bool separatorInserted = false;
  for ( KBookmark bm = parentBookmark.first(); !bm.isNull();  bm = parentBookmark.next(bm) )
  {
    QString text = bm.text();
    text.replace( QRegExp( "&" ), "&&" );
    if ( !separatorInserted && m_bIsRoot) { // inserted before the first konq bookmark, to avoid the separator if no konq bookmark
      m_parentMenu->insertSeparator();
      separatorInserted = true;
    }
    if ( !bm.isGroup() )
    {
      if ( bm.isSeparator() )
      {
        m_parentMenu->insertSeparator();
      }
      else
      {
        // kdDebug(1203) << "Creating URL bookmark menu item for " << bm.text() << endl;
        // create a normal URL item, with ID as a name
        KAction * action = new KAction( text, bm.icon(), 0,
                                        this, SLOT( slotBookmarkSelected() ),
                                        m_actionCollection, bm.url().url().utf8() );

        action->setStatusText( bm.url().prettyURL() );

        action->plug( m_parentMenu );
        m_actions.append( action );
      }
    }
    else
    {
      // kdDebug(1203) << "Creating bookmark submenu named " << bm.text() << endl;
      KActionMenu * actionMenu = new KActionMenu( text, bm.icon(),
                                                  m_actionCollection, 0L );
      actionMenu->plug( m_parentMenu );
      m_actions.append( actionMenu );
      KCelBookmarkMenu *subMenu = new KCelBookmarkMenu( m_pManager, m_pOwner, actionMenu->popupMenu(),
                                                  m_actionCollection, false,
                                                  m_bAddBookmark,
                                                  bm.address() );
      m_lstSubMenus.append( subMenu );
    }
  }

  if ( !m_bIsRoot && m_bAddBookmark )
  {
    m_parentMenu->insertSeparator();
    addAddBookmark();
    addNewFolder();
  }
}

void KCelBookmarkMenu::slotAddRelativeBookmark()
{
  Url Url = m_pOwner->currentUrl(Url::Relative);
  QString url = QString(Url.getAsString().c_str());;
  if (url.isEmpty())
  {
    KMessageBox::error( 0L, i18n("Can't add bookmark with empty URL"));
    return;
  }
  QString title = QString::fromUtf8(Url::decodeString(Url.getName()).c_str());
  if (title.isEmpty())
    title = url;

  KBookmarkGroup parentBookmark = m_pManager->findByAddress( m_parentAddress ).toGroup();
  Q_ASSERT(!parentBookmark.isNull());
  // If this title is already used, we'll try to find something unused.
  KBookmark ch = parentBookmark.first();
  int count = 1;
  QString uniqueTitle = title;
  do
  {
    while ( !ch.isNull() )
    {
      if ( uniqueTitle == ch.text() )
      {
        // Title already used !
        if ( url != ch.url().url() )
        {
          uniqueTitle = title + QString(" (%1)").arg(++count);
          // New title -> restart search from the beginning
          ch = parentBookmark.first();
          break;
        }
        else
        {
          // this exact URL already exists
          return;
        }
      }
      ch = parentBookmark.next( ch );
    }
  } while ( !ch.isNull() );

  parentBookmark.addBookmark( m_pManager, uniqueTitle, url, m_pOwner->currentIcon() );
  m_pManager->emitChanged( parentBookmark );
}

void KCelBookmarkMenu::slotAddSettingsBookmark()
{
  Url Url = m_pOwner->currentUrl(Url::Settings);
  QString url = QString(Url.getAsString().c_str());;
  if (url.isEmpty())
  {
    KMessageBox::error( 0L, i18n("Can't add bookmark with empty URL"));
    return;
  }
  QString title = QString::fromUtf8(Url::decodeString(Url.getName()).c_str());
  if (title.isEmpty())
    title = url;

  KBookmarkGroup parentBookmark = m_pManager->findByAddress( m_parentAddress ).toGroup();
  Q_ASSERT(!parentBookmark.isNull());
  // If this title is already used, we'll try to find something unused.
  KBookmark ch = parentBookmark.first();
  int count = 1;
  QString uniqueTitle = title;
  do
  {
    while ( !ch.isNull() )
    {
      if ( uniqueTitle == ch.text() )
      {
        // Title already used !
        if ( url != ch.url().url() )
        {
          uniqueTitle = title + QString(" (%1)").arg(++count);
          // New title -> restart search from the beginning
          ch = parentBookmark.first();
          break;
        }
        else
        {
          // this exact URL already exists
          return;
        }
      }
      ch = parentBookmark.next( ch );
    }
  } while ( !ch.isNull() );

  parentBookmark.addBookmark( m_pManager, uniqueTitle, url, m_pOwner->currentIcon() );
  m_pManager->emitChanged( parentBookmark );
}

void KCelBookmarkMenu::slotAddBookmark()
{
  Url Url = m_pOwner->currentUrl(Url::Absolute);
  QString url = QString(Url.getAsString().c_str());;
  if (url.isEmpty())
  {
    KMessageBox::error( 0L, i18n("Can't add bookmark with empty URL"));
    return;
  }
  QString title = QString::fromUtf8(Url::decodeString(Url.getName()).c_str());
  if (title.isEmpty())
    title = url;

  KBookmarkGroup parentBookmark = m_pManager->findByAddress( m_parentAddress ).toGroup();
  Q_ASSERT(!parentBookmark.isNull());
  // If this title is already used, we'll try to find something unused.
  KBookmark ch = parentBookmark.first();
  int count = 1;
  QString uniqueTitle = title;
  do
  {
    while ( !ch.isNull() )
    {
      if ( uniqueTitle == ch.text() )
      {
        // Title already used !
        if ( url != ch.url().url() )
        {
          uniqueTitle = title + QString(" (%1)").arg(++count);
          // New title -> restart search from the beginning
          ch = parentBookmark.first();
          break;
        }
        else
        {
          // this exact URL already exists
          return;
        }
      }
      ch = parentBookmark.next( ch );
    }
  } while ( !ch.isNull() );

  parentBookmark.addBookmark( m_pManager, uniqueTitle, url, m_pOwner->currentIcon() );
  m_pManager->emitChanged( parentBookmark );
}

void KCelBookmarkMenu::slotNewFolder()
{
  if ( !m_pOwner ) return; // this view doesn't handle bookmarks...
  KBookmarkGroup parentBookmark = m_pManager->findByAddress( m_parentAddress ).toGroup();
  Q_ASSERT(!parentBookmark.isNull());
  KBookmarkGroup group = parentBookmark.createNewFolder( m_pManager );
  if ( !group.isNull() )
  {
    KBookmarkGroup parentGroup = group.parentGroup();
    m_pManager->emitChanged( parentGroup );
  }
}

void KCelBookmarkMenu::slotBookmarkSelected()
{
  //kdDebug(1203) << "KBookmarkMenu::slotBookmarkSelected()" << endl;
  if ( !m_pOwner ) return; // this view doesn't handle bookmarks...
  //kdDebug(1203) << sender()->name() << endl;

  // The name of the action is the URL to open
  m_pOwner->openBookmarkURL( QString::fromUtf8(sender()->name()) );
}

// -----------------------------------------------------------------------------


