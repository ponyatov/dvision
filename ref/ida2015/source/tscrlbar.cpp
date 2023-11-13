/*------------------------------------------------------------*/
/* filename -       tscrlbar.cpp                              */
/*                                                            */
/* function(s)                                                */
/*                  TScrollBar member functions               */
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

#define Uses_TKeys
#define Uses_TScrollBar
#define Uses_TRect
#define Uses_TDrawBuffer
#define Uses_TEvent
#define Uses_TGroup
#define Uses_opstream
#define Uses_ipstream
#include <tv.h>

#define cpScrollBar  "\x04\x05\x05"

TScrollBar::TScrollBar( const TRect& bounds ) :
    TView( bounds ),
    value( 0 ),
    minVal( 0 ),
    maxVal( 0 ),
    pgStep( 1 ),
    arStep( 1 )
{
    if( size.x == 1 )
        {
        growMode = gfGrowLoX | gfGrowHiX | gfGrowHiY;
        memcpy( chars, vChars, sizeof(vChars) );
        }
    else
        {
        growMode = gfGrowLoY | gfGrowHiX | gfGrowHiY;
        memcpy( chars, hChars, sizeof(hChars) );
        }
}

void TScrollBar::draw()
{
    drawPos(getPos());
}

void TScrollBar::drawPos( int pos )
{
    TDrawBuffer b;

    int s = getSize() - 1;
    b.moveChar( 0, chars[0], getColor(2), 1 );
    if( maxVal == minVal )
        b.moveChar( 1, chars[4], getColor(1), ushort(s-1) );
    else
        {
        b.moveChar( 1, chars[2], getColor(1), ushort(s-1) );
        b.moveChar( ushort(pos), chars[3], getColor(3), 1 );
        }

    b.moveChar( ushort(s), chars[1], getColor(2), 1 );
    writeBuf( 0, 0, ushort(size.x), ushort(size.y), b );
}

TPalette& TScrollBar::getPalette() const
{
    static TPalette palette( cpScrollBar, sizeof( cpScrollBar )-1 );
    return palette;
}

int TScrollBar::getPos() const
{
    int r = maxVal - minVal;
    if( r == 0 )
        return 1;
    else
        return (((int32(value - minVal) * (getSize() - 3)) + (r >> 1)) / r) + 1;
}

int TScrollBar::getSize() const
{
    int s;

    if( size.x == 1 )
        s = size.y;
    else
        s = size.x;

    return qmax( 3, s );
}

static TPoint mouse;
static int p, g_size;
static TRect extent;

int TScrollBar::getPartCode() const
{
    int part= - 1;
    if( ::extent.contains(mouse) )
        {
        int mark = (size.x == 1) ? mouse.y : mouse.x;

        if (mark == p)
            part= sbIndicator;
        else
            {
            if( mark < 1 )
                part = sbLeftArrow;
            else if( mark < p )
                part= sbPageLeft;
            else if( mark < g_size )
                part= sbPageRight;
            else
                part= sbRightArrow;

            if( size.x == 1 )
                part += 4;
            }
        }
    return part;
}

void TScrollBar::handleEvent( TEvent& event )
{
    Boolean Tracking;
    int clickPart, i = 0; //i is initialized to remove diagnostic ONLY

    TView::handleEvent(event);
    switch( event.what )
        {
        case evMouseDown:
            message(owner, evBroadcast, cmScrollBarClicked,this); // Clicked()
            mouse = makeLocal( event.mouse.where );
            ::extent = getExtent();
            ::extent.grow(1, 1);
            p = getPos();
            g_size = getSize() - 1;
            clickPart= getPartCode();
            if( clickPart != sbIndicator )
                {
                do  {
                    mouse = makeLocal( event.mouse.where );
                    if( getPartCode() == clickPart )
                        setValue(value + scrollStep(clickPart) );
                    } while( mouseEvent(event, evMouseAuto) );
                }
            else
                {
                do  {
                    mouse = makeLocal( event.mouse.where );
                    Tracking = ::extent.contains(mouse);
                    if( Tracking )
                        {
                        if( size.x == 1 )
                            i = mouse.y;
                        else
                            i = mouse.x;
                        i = qmax( i, 1 );
                        i = qmin( i, g_size-1 );
                        }
                    else
                        i = getPos();
                    if(i != p )
                        {
                        drawPos(i);
                        p = i;
                        }
                    } while( mouseEvent(event,evMouseMove) );
                if( Tracking && g_size > 2 )
                    {
                    g_size -= 2;
                    setValue(((int32(p - 1) * (maxVal - minVal) + (g_size >> 1)) / g_size) + minVal);
                    }
                }
            clearEvent(event);
            break;
        case  evKeyDown:
            if( (state & sfVisible) != 0 )
                {
                clickPart = sbIndicator;
                if( size.y == 1 )
                    switch( ctrlToArrow(event.keyDown.keyCode) )
                        {
                        case kbLeft:
                            clickPart = sbLeftArrow;
                            break;
                        case kbRight:
                            clickPart = sbRightArrow;
                            break;
                        case kbCtrlLeft:
                            clickPart = sbPageLeft;
                            break;
                        case kbCtrlRight:
                            clickPart = sbPageRight;
                            break;
                        case kbHome:
                            i = minVal;
                            break;
                        case kbEnd:
                            i = maxVal;
                            break;
                        default:
                            return;
                        }
                else
                    switch( ctrlToArrow(event.keyDown.keyCode) )
                        {
                        case kbUp:
                            clickPart = sbUpArrow;
                            break;
                        case kbDown:
                            clickPart = sbDownArrow;
                            break;
                        case kbPgUp:
                            clickPart = sbPageUp;
                            break;
                        case kbPgDn:
                            clickPart = sbPageDown;
                            break;
                        case kbCtrlPgUp:
                            i = minVal;
                            break;
                        case kbCtrlPgDn:
                            i = maxVal;
                            break;
                        default:
                            return;
                        }
                message(owner,evBroadcast,cmScrollBarClicked,this); // Clicked
                if( clickPart != sbIndicator )
                    i = value + scrollStep(clickPart);
                setValue(i);
                clearEvent(event);
                }
        }
}

void TScrollBar::scrollDraw()
{
    message(owner, evBroadcast, cmScrollBarChanged,this);
}

int TScrollBar::scrollStep( int part )
{
    int  step;

    if( !(part & 2) )
        step = arStep;
    else
        step = pgStep;
    if( !(part & 1) )
        return -step;
    else
        return step;
}

void TScrollBar::setParams( int aValue,
                            int aMin,
                            int aMax,
                            int aPgStep,
                            int aArStep
                          )
{
 int  sValue;

    aMax = qmax( aMax, aMin );
    aValue = qmax( aMin, aValue );
    aValue = qmin( aMax, aValue );
    sValue = value;
    if( sValue != aValue || minVal != aMin || maxVal != aMax )
        {
        value = aValue;
        minVal = aMin;
        maxVal = aMax;
        drawView();
        if( sValue != aValue )
            scrollDraw();
        }
    pgStep = aPgStep;
    arStep = aArStep;
}

void TScrollBar::setRange( int aMin, int aMax )
{
    setParams( value, aMin, aMax, pgStep, arStep );
}

void TScrollBar::setStep( int aPgStep, int aArStep )
{
    setParams( value, minVal, maxVal, aPgStep, aArStep );
}

void TScrollBar::setValue( int aValue )
{
    setParams( aValue, minVal, maxVal, pgStep, arStep );
}

#ifndef NO_TV_STREAMS
void TScrollBar::write( opstream& os )
{
    TView::write( os );
    os << value << minVal << maxVal << pgStep << arStep;
    os.writeBytes(chars, sizeof(chars));
}

void *TScrollBar::read( ipstream& is )
{
    TView::read( is );
    is >> value >> minVal >> maxVal >> pgStep >> arStep;
    is.readBytes(chars, sizeof(TScrollChars));
    return this;
}

TStreamable *TScrollBar::build()
{
    return new TScrollBar( streamableInit );
}

TScrollBar::TScrollBar( StreamableInit ) : TView( streamableInit )
{
}
#endif  // ifndef NO_TV_STREAMS
