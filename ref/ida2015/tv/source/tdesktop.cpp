/*------------------------------------------------------------*/
/* filename -       tdesktop.cpp                              */
/*                                                            */
/* function(s)                                                */
/*                  TDeskTop member functions                 */
/*------------------------------------------------------------*/

/*------------------------------------------------------------*/
/*                                                            */
/*    Turbo Vision -  Version 1.0                             */
/*                                                            */
/*                                                            */
/*    Copyright (c) 1991 by Borland International             */
/*    All Rights Reserved.                                    */
/*                                                            */
/*------------------------------------------------------------*/

#define Uses_TDeskTop
#define Uses_TRect
#define Uses_TPoint
#define Uses_TEvent
#define Uses_TBackground
#define Uses_opstream
#define Uses_ipstream
#include <tv.h>

TDeskInit::TDeskInit( TBackground *(*cBackground)( const TRect & ) ) :
    createBackground( cBackground )
{
}

TDeskTop::TDeskTop( const TRect& bounds ) :
    TDeskInit( TDeskTop::initBackground ),
    TGroup(bounds),
    background(NULL)
{
    growMode = gfGrowHiX | gfGrowHiY;

    if( createBackground != 0 &&
        (background = createBackground( getExtent() )) != 0 )
        insert( background );
}

void TDeskTop::shutDown()
{
    background = 0;
    TGroup::shutDown();
}

inline Boolean Tileable( const TView *p )
{
    return Boolean( (p->options & ofTileable) != 0 && (p->state & sfVisible) != 0 );
}

static short cascadeNum;
static TView *lastView;

static void doCount( TView* p, void * )
{
    if( Tileable( p ) )
        {
        cascadeNum++;
        lastView = p;
        }
}

//lint -e{818} Pointer parameter could be declared as pointing to const
static void doCascade( TView* p, void *r )
{
    if( Tileable( p ) && cascadeNum >= 0 )
        {
        TRect NR = *(const TRect *)r;
        NR.a.x += cascadeNum;
        NR.a.y += cascadeNum;
        p->locate( NR );
        cascadeNum--;
        }
}

void TDeskTop::cascade( const TRect &r )
{
    TPoint min, max;
    cascadeNum = 0;
    forEach( doCount, 0 );
    if( cascadeNum > 0 )
        {
        lastView->sizeLimits( min, max );
        if( (min.x > r.b.x - r.a.x - cascadeNum) ||
            (min.y > r.b.y - r.a.y - cascadeNum) )
            tileError();
        else
            {
            cascadeNum--;
            lock();
            forEach( doCascade, (void *)&r );
            unlock();
            }
        }
}

void TDeskTop::handleEvent(TEvent& event)
{
    TGroup::handleEvent( event );
    if( event.what == evCommand )
        {
        switch( event.message.command )
            {
            case cmNext:
                if (valid(cmReleasedFocus))
                    selectNext( False );
                break;
            case cmPrev:
                if (valid(cmReleasedFocus))
                    current->putInFrontOf( background );
                break;
            default:
                return;
            }
        clearEvent( event );
        }
}

TBackground *TDeskTop::initBackground( const TRect &r )
{
    return new TBackground( r, defaultBkgrnd );
}

static short iSqr( short i )
{
    short res1 = 2;
    short res2 = i/res1;
    while( abs( res1 - res2 ) > 1 )
        {
        res1 = (res1 + res2)/2;
        res2 = i/res1;
        }
    return res1 < res2 ? res1 : res2;
}

static void mostEqualDivisors(short n, short& x, short& y)
{
    short  i;

    i = iSqr( n );
    if( n % i != 0 )
        if( n % (i+1) == 0 )
            i++;
    if( i < (n/i) )
        i = n/i;

    x = n/i;
    y = i;
}

static short numCols, numRows, numTileable, leftOver, tileNum;

//lint -e{818} Pointer parameter could be declared as pointing to const
static void doCountTileable( TView* p, void * )
{
    if( Tileable( p ) )
        numTileable++;
}

static int dividerLoc( int lo, int hi, int num, int pos)
{
    return int(int32(hi-lo)*pos/int32(num)+lo);
}

static TRect calcTileRect( short pos, const TRect &r )
{
    short x, y;
    TRect nRect;

    short d = (numCols - leftOver) * numRows;
    if( pos <  d )
        {
        x = pos / numRows;
        y = pos % numRows;
        }
    else
        {
        x = (pos-d)/(numRows+1) + (numCols-leftOver);
        y = (pos-d)%(numRows+1);
        }
    nRect.a.x = dividerLoc( r.a.x, r.b.x, numCols, x );
    nRect.b.x = dividerLoc( r.a.x, r.b.x, numCols, x+1 );
    if( pos >= d )
        {
        nRect.a.y = dividerLoc(r.a.y, r.b.y, numRows+1, y);
        nRect.b.y = dividerLoc(r.a.y, r.b.y, numRows+1, y+1);
        }
    else
        {
        nRect.a.y = dividerLoc(r.a.y, r.b.y, numRows, y);
        nRect.b.y = dividerLoc(r.a.y, r.b.y, numRows, y+1);
        }
    return nRect;
}

//lint -e{818} Pointer parameter could be declared as pointing to const
static void doTile( TView* p, void *lR )
{
    if( Tileable( p ) )
        {
        TRect r = calcTileRect( tileNum, *(const TRect *)lR );
        p->locate(r);
        tileNum--;
        }
}

void TDeskTop::tile( const TRect& r )
{
    numTileable =  0;
    forEach( doCountTileable, 0 );
    if( numTileable > 0 )
        {
        mostEqualDivisors( numTileable, numCols, numRows );
        if( ( (r.b.x - r.a.x)/numCols ==  0 ) ||
            ( (r.b.y - r.a.y)/numRows ==  0) )
            tileError();
        else
            {
            leftOver = numTileable % numCols;
            tileNum = numTileable - 1;
            lock();
            forEach( doTile, (void *)&r );
            unlock();
            }
        }
}

void  TDeskTop::tileError()
{
}

#ifndef NO_TV_STREAMS
TStreamable *TDeskTop::build()
{
    return new TDeskTop( streamableInit );
}

TDeskTop::TDeskTop( StreamableInit ) :
    TGroup( streamableInit ),
    TDeskInit( 0 )
{
}
#endif  // ifndef NO_TV_STREAMS


