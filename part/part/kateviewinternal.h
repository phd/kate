/* This file is part of the KDE libraries
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2002 Christoph Cullmann <cullmann@kde.org>

   Based on:
     KWriteView : Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#ifndef _KATE_VIEW_INTERNAL_
#define _KATE_VIEW_INTERNAL_

#include "katecursor.h"
#include "katelinerange.h"

#include <qscrollbar.h>
#include <qpoint.h>
#include <qpixmap.h>
#include <qtimer.h>

class KateView;
class KateDocument;
class KateIconBorder;

class QHBoxLayout;
class QVBoxLayout;

enum Bias
{
    left  = -1,
    none  =  0,
    right =  1
};

/**
 * This class is required because QScrollBar's sliderMoved() signal is
 * really supposed to be a sliderDragged() signal... so this way we can capture
 * MMB slider moves as well
 */
class KateScrollBar : public QScrollBar
{
  Q_OBJECT

  public:
    KateScrollBar(Orientation orientation, QWidget* parent, const char* name = 0L);

  signals:
    void sliderMMBMoved(int value);

  protected:
    virtual void mousePressEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);

  protected slots:
    void sliderMaybeMoved(int value);

  private:
  bool m_middleMouseDown;
};

class KateViewInternal : public QWidget
{
    Q_OBJECT

    friend class KateView;
    friend class KateIconBorder;

  public:
    KateViewInternal ( KateView *view, KateDocument *doc );
    ~KateViewInternal ();

  // BEGIN EDIT STUFF
  public:
    void editStart ();
    void editEnd (int editTagLineStart, int editTagLineEnd);

    void editInsertText (int line, int col, int len);
    void editRemoveText (int line, int col, int len);

    void editWrapLine (int line, int col, int len);
    void editUnWrapLine (int line, int col);

    void editInsertLine (int line);
    void editRemoveLine (int line);

    void editSetCursor (const KateTextCursor &cursor);

    void setViewTagLinesFrom(int line);

  private:
    void setTagLinesFrom(int line);

    uint editSessionNumber;
    bool editIsRunning;
    bool editCursorChanged;
    int tagLinesFrom;
  // END

  // BEGIN TAG & CLEAR & UPDATE STUFF
  public:
    bool tagLine (const KateTextCursor& virtualCursor);

    bool tagLines (int start, int end, bool realLines = false);
    bool tagLines (KateTextCursor start, KateTextCursor end, bool realCursors = false);

    void tagAll ();

    void clear ();
  // END

  private:
    void updateView (bool changed = false, int viewLinesScrolled = 0);
    void makeVisible (const KateTextCursor& c, uint endCol, bool force = false);

  public:
    inline const KateTextCursor& startPos() const { return m_startPos; }
    inline uint startLine () const { return m_startPos.line(); }
    inline uint startX () const { return m_startX; }

    KateTextCursor endPos () const;
    uint endLine () const;

    LineRange yToLineRange(uint y) const;

    void prepareForDynWrapChange();
    void dynWrapChanged();

  public slots:
    void slotIncFontSizes();
    void slotDecFontSizes();

  private slots:
    void scrollLines(int line); // connected to the sliderMoved of the m_lineScroll
    void scrollViewLines(int offset);
    void scrollNextPage();
    void scrollPrevPage();
    void scrollPrevLine();
    void scrollNextLine();
    void scrollColumns (int x); // connected to the valueChanged of the m_columnScroll

  public:
    void doReturn();
    void doDelete();
    void doBackspace();
    void doPaste();
    void doTranspose();
    void doDeleteWordLeft();
    void doDeleteWordRight();

    void cursorLeft(bool sel=false);
    void cursorRight(bool sel=false);
    void wordLeft(bool sel=false);
    void wordRight(bool sel=false);
    void home(bool sel=false);
    void end(bool sel=false);
    void cursorUp(bool sel=false);
    void cursorDown(bool sel=false);
    void cursorToMatchingBracket(bool sel=false);
    void scrollUp();
    void scrollDown();
    void topOfView(bool sel=false);
    void bottomOfView(bool sel=false);
    void pageUp(bool sel=false);
    void pageDown(bool sel=false);
    void top(bool sel=false);
    void bottom(bool sel=false);
    void top_home(bool sel=false);
    void bottom_end(bool sel=false);

    inline const KateTextCursor& getCursor() { return cursor; }
    QPoint cursorCoordinates();

    void paintText (int x, int y, int width, int height, bool paintOnlyDirty = false);

  // EVENT HANDLING STUFF - IMPORTANT
  protected:
    void paintEvent(QPaintEvent *e);
    bool eventFilter( QObject *obj, QEvent *e );
    void keyPressEvent( QKeyEvent* );
    void keyReleaseEvent( QKeyEvent* );
    void resizeEvent( QResizeEvent* );
    void mousePressEvent(       QMouseEvent* );
    void mouseDoubleClickEvent( QMouseEvent* );
    void mouseReleaseEvent(     QMouseEvent* );
    void mouseMoveEvent(        QMouseEvent* );
    void timerEvent( QTimerEvent* );
    void dragEnterEvent( QDragEnterEvent* );
    void dragMoveEvent( QDragMoveEvent* );
    void dropEvent( QDropEvent* );
    void showEvent ( QShowEvent *);
    void wheelEvent(QWheelEvent* e);
    void focusInEvent (QFocusEvent *);
    void focusOutEvent (QFocusEvent *);

  private slots:
    void tripleClickTimeout();

  signals:
    // emitted when KateViewInternal is not handling its own URI drops
    void dropEventPass(QDropEvent*);

  private slots:
    void slotRegionVisibilityChangedAt(unsigned int);
    void slotRegionBeginEndAddedRemoved(unsigned int);
    void slotCodeFoldingChanged();

  private:
    void moveChar( Bias bias, bool sel );
    void moveWord( Bias bias, bool sel );
    void moveEdge( Bias bias, bool sel );
    KateTextCursor maxStartPos(bool changed = false);
    void scrollPos(KateTextCursor& c, bool force = false);
    void scrollLines( int lines, bool sel );

    uint linesDisplayed() const;

    int lineToY(uint viewLine) const;

    void updateSelection( const KateTextCursor&, bool keepSel );
    void updateCursor( const KateTextCursor& newCursor, bool force = false );
    void updateBracketMarks();

    void paintCursor();

    void placeCursor( const QPoint& p, bool keepSelection = false, bool updateSelection = true );
    bool isTargetSelected( const QPoint& p );

    void doDrag();

    KateView *m_view;
    KateDocument* m_doc;
    class KateIconBorder *leftBorder;

    int mouseX;
    int mouseY;
    int scrollX;
    int scrollY;
    int scrollTimer;

    KateTextCursor cursor;
    KateTextCursor displayCursor;
    int cursorTimer;
    int cXPos;

    bool possibleTripleClick;

    // Bracket mark
    KateTextRange bm;

    enum DragState { diNone, diPending, diDragging };

    struct _dragInfo {
      DragState    state;
      QPoint       start;
      QTextDrag*   dragObject;
    } dragInfo;

    uint iconBorderHeight;

    //
    // line scrollbar + first visible (virtual) line in the current view
    //
    QScrollBar *m_lineScroll;
    QWidget* m_dummy;
    QVBoxLayout* m_lineLayout;
    QHBoxLayout* m_colLayout;

    QPixmap drawBuffer;

    // These are now cursors to account for word-wrap.
    KateTextCursor m_startPos;
    KateTextCursor m_oldStartPos;

    // This is set to false on resize or scroll (other than that called by makeVisible),
    // so that makeVisible is again called when a key is pressed and the cursor is in the same spot
    bool m_madeVisible;
    bool m_shiftKeyPressed;

    // How many lines to should be kept visible above/below the cursor when possible
    void setAutoCenterLines(int viewLines, bool updateView = true);
    int m_autoCenterLines;
    int m_minLinesVisible;

    //
    // column scrollbar + x position
    //
    QScrollBar *m_columnScroll;
    bool m_columnScrollDisplayed;
    int m_startX;
    int m_oldStartX;

    // has selection changed while your mouse or shift key is pressed
    bool m_selChangedByUser;

    //
    // lines Ranges, mostly useful to speedup + dyn. word wrap
    //
    QMemArray<LineRange> lineRanges;

    // Used to determine if the scrollbar will appear/disappear in non-wrapped mode
    bool scrollbarVisible(uint startLine);
    int maxLen(uint startLine);

    // returns the maximum X value / col value a cursor can take for a specific line range
    int lineMaxCursorX(const LineRange& range);
    int lineMaxCol(const LineRange& range);

    // get the values for a specific range.
    // specify lastLine to get the next line of a range.
    LineRange range(int realLine, const LineRange* previous = 0L);

    LineRange currentRange();
    LineRange previousRange();
    LineRange nextRange();

    // Finds the lineRange currently occupied by the cursor.
    LineRange range(const KateTextCursor& realCursor);

    // Returns the lineRange of the specified realLine + viewLine.
    LineRange range(uint realLine, int viewLine);

    // find the view line of cursor c (0 = same line, 1 = down one, etc.)
    uint viewLine(const KateTextCursor& realCursor);

    // find the view line of the cursor, relative to the display (0 = top line of view, 1 = second line, etc.)
    // if limitToVisible is true, only lines which are currently visible will be searched for, and -1 returned if the line is not visible.
    int displayViewLine(const KateTextCursor& virtualCursor, bool limitToVisible = false);

    // find the index of the last view line for a specific line
    uint lastViewLine(uint realLine);

    // count the number of view lines for a real line
    uint viewLineCount(uint realLine);

    // find the cursor offset by (offset) view lines from a cursor.
    // when keepX is true, the column position will be calculated based on the x
    // position of the specified cursor.
    KateTextCursor viewLineOffset(const KateTextCursor& virtualCursor, int offset, bool keepX = false);

    // These variable holds the most recent maximum real & visible column number
    bool m_preserveMaxX;
    int m_currentMaxX;


    bool m_updatingView;
    int m_wrapChangeViewLine;
    KateTextCursor m_cachedMaxStartPos;

  private slots:
    void doDragScroll();
    void startDragScroll();
    void stopDragScroll();

  private:
    // Timer for drag & scroll
    QTimer m_dragScrollTimer;

    static const int scrollTime = 30;
    static const int scrollMargin = 16;

    // needed to stop the column scroll bar from hiding / unhiding during a dragScroll.
    bool m_suppressColumnScrollBar;

  //TextHint
 public:
   void enableTextHints(int timeout);
   void disableTextHints();

 private:
   int m_textHintTimer;
   bool m_textHintEnabled;
   int m_textHintTimeout;
   int m_textHintMouseX;
   int m_textHintMouseY;
};

#endif
