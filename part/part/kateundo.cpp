/* This file is part of the KDE libraries
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

// $Id$

#include "kateundo.h"

#include "katedocument.h"
#include "kateview.h"
#include "katecursor.h"

/**
 Private class, only for KateUndoGroup, no need to use it elsewhere
 */
 class KateUndo
{
  public:
    KateUndo (uint type, uint line, uint col, uint len, const QString &text);
    ~KateUndo ();

  public:
    // Invalid examples: insert / remove 0 length text
    // I could probably fix this in KateDocument, but it's more work there
    // (and probably better here too)
    bool isValid();

    // Saves a bit of memory and potentially many calls when undo/redoing.
    bool merge(KateUndo* u);

    void undo (KateDocument *doc);
    void redo (KateDocument *doc);

    // The cursor before the action took place
    KateTextCursor cursorBefore() const;
    KateTextCursor cursorAfter() const;

    inline uint type() const { return m_type; }

    inline uint line () const { return m_line; }
    inline uint col () const { return m_col; }
    inline uint len() const { return m_len; }

    inline const QString& text() const { return m_text; };

  private:
    uint m_type;
    uint m_line;
    uint m_col;
    uint m_len;
    QString m_text;
};

KateUndo::KateUndo (uint type, uint line, uint col, uint len, const QString &text)
: m_type (type),
  m_line (line),
  m_col (col),
  m_len (len),
  m_text (text)
{
}

KateUndo::~KateUndo ()
{
}

bool KateUndo::isValid()
{
  if (m_type == KateUndoGroup::editInsertText || m_type == KateUndoGroup::editRemoveText)
    if (len() == 0)
      return false;

  return true;
}

bool KateUndo::merge(KateUndo* u)
{
  if (m_type != u->type())
    return false;

  if (m_type == KateUndoGroup::editInsertText
      && m_line == u->line()
      && (m_col + m_len) == u->col())
  {
    m_text += u->text();
    m_len += u->len();
    return true;
  }
  else if (m_type == KateUndoGroup::editRemoveText
      && m_line == u->line()
      && m_col == (u->col() + u->len()))
  {
    m_text.prepend(u->text());
    m_col = u->col();
    m_len += u->len();
    return true;
  }

  return false;
}

void KateUndo::undo (KateDocument *doc)
{
  if (m_type == KateUndoGroup::editInsertText)
  {
    doc->editRemoveText (m_line, m_col, m_len);
  }
  else if (m_type == KateUndoGroup::editRemoveText)
  {
    doc->editInsertText (m_line, m_col, m_text);
  }
  else if (m_type == KateUndoGroup::editWrapLine)
  {
    doc->editUnWrapLine (m_line, m_col);
  }
  else if (m_type == KateUndoGroup::editUnWrapLine)
  {
    doc->editWrapLine (m_line, m_col);
  }
  else if (m_type == KateUndoGroup::editInsertLine)
  {
    doc->editRemoveLine (m_line);
  }
  else if (m_type == KateUndoGroup::editRemoveLine)
  {
    doc->editInsertLine (m_line, m_text);
  }
}

void KateUndo::redo (KateDocument *doc)
{
  if (m_type == KateUndoGroup::editRemoveText)
  {
    doc->editRemoveText (m_line, m_col, m_len);
  }
  else if (m_type == KateUndoGroup::editInsertText)
  {
    doc->editInsertText (m_line, m_col, m_text);
  }
  else if (m_type == KateUndoGroup::editUnWrapLine)
  {
    doc->editUnWrapLine (m_line, m_col);
  }
  else if (m_type == KateUndoGroup::editWrapLine)
  {
    doc->editWrapLine (m_line, m_col);
  }
  else if (m_type == KateUndoGroup::editRemoveLine)
  {
    doc->editRemoveLine (m_line);
  }
  else if (m_type == KateUndoGroup::editInsertLine)
  {
    doc->editInsertLine (m_line, m_text);
  }
}

KateTextCursor KateUndo::cursorBefore() const
{
  if (m_type == KateUndoGroup::editInsertLine || m_type == KateUndoGroup::editUnWrapLine)
    return KateTextCursor(m_line+1, m_col);
  else if (m_type == KateUndoGroup::editRemoveText)
    return KateTextCursor(m_line, m_col+m_len);

  return KateTextCursor(m_line, m_col);
}

KateTextCursor KateUndo::cursorAfter() const
{
  if (m_type == KateUndoGroup::editRemoveLine || m_type == KateUndoGroup::editWrapLine)
    return KateTextCursor(m_line+1, m_col);
  else if (m_type == KateUndoGroup::editInsertText)
    return KateTextCursor(m_line, m_col+m_len);

  return KateTextCursor(m_line, m_col);
}

KateUndoGroup::KateUndoGroup (KateDocument *doc)
: m_doc (doc)
{
  m_items.setAutoDelete (true);
}

KateUndoGroup::~KateUndoGroup ()
{
}

void KateUndoGroup::undo ()
{
  if (m_items.count() == 0)
    return;

  m_doc->editStart (false);

  for (KateUndo* u = m_items.last(); u; u = m_items.prev())
    u->undo(m_doc);

  if (m_doc->activeView() != 0L && m_items.first())
  {
    m_doc->activeView()->editSetCursor (m_items.first()->cursorBefore());
  }

  m_doc->editEnd ();
}

void KateUndoGroup::redo ()
{
  if (m_items.count() == 0)
    return;

  m_doc->editStart (false);

  for (KateUndo* u = m_items.first(); u; u = m_items.next())
    u->redo(m_doc);

  if (m_doc->activeView() != 0L && m_items.last())
  {
    m_doc->activeView()->editSetCursor (m_items.first()->cursorAfter());
  }

  m_doc->editEnd ();
}

void KateUndoGroup::addItem (uint type, uint line, uint col, uint len, const QString &text)
{
  addItem(new KateUndo(type, line, col, len, text));
}

void KateUndoGroup::addItem(KateUndo* u)
{
  if (!u->isValid())
    delete u;
  else if (m_items.last() && m_items.last()->merge(u))
    delete u;
  else
    m_items.append(u);
}

bool KateUndoGroup::merge(KateUndoGroup* newGroup)
{
  if (newGroup->isOnlyType(singleType())) {
    // Take all of its items first -> last
    KateUndo* u = newGroup->m_items.take(0);
    while (u) {
      addItem(u);
      u = newGroup->m_items.take(0);
    }
    return true;
  }
  return false;
}

uint KateUndoGroup::singleType()
{
  uint ret = editInvalid;

  for (KateUndo* u = m_items.first(); u; u = m_items.next()) {
    if (ret == editInvalid)
      ret = u->type();
    else if (ret != u->type())
      return editInvalid;
  }

  return ret;
}

bool KateUndoGroup::isOnlyType(uint type)
{
  if (type == editInvalid) return false;

  for (KateUndo* u = m_items.first(); u; u = m_items.next())
    if (u->type() != type)
      return false;

  return true;
}
