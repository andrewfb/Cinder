/*
 Copyright (c) 2012, The Cinder Project, All rights reserved.

 This code is intended for use with the Cinder C++ library: http://libcinder.org

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#include "cinder/app/cocoa/CinderViewMac.h"
#include "cinder/app/Renderer.h"
#include "cinder/app/TouchEvent.h"
#include "cinder/app/cocoa/PlatformCocoa.h"
#include "cinder/cocoa/CinderCocoa.h"

#import <Cocoa/Cocoa.h>

using namespace cinder;
using namespace cinder::app;

@interface CinderViewMac ()
- (void)windowDidEnterFullScreen:(NSNotification *)notification;
- (void)windowDidExitFullScreen:(NSNotification *)notification;
@end

@implementation CinderViewMac

@synthesize readyToDraw = mReadyToDraw;
@synthesize receivesEvents = mReceivesEvents;

- (id)initWithFrame:(NSRect)frame
{
	self = [super initWithFrame:frame];
	mReadyToDraw = NO;
	mReceivesEvents = YES;
	mFullScreen = NO;
	mHighDensityDisplayEnabled = NO;
	mMultiTouchEnabled = NO;
	mMarkedText = nil;
	mMarkedRange = NSMakeRange(NSNotFound, 0);

	mTouchIdMap = nil;
	mDelegate = nil;

	// Initialize modifier key state tracking
	memset(mModifierKeyPressed, 0, sizeof(mModifierKeyPressed));

	return self;
}

- (CinderViewMac *)initWithFrame:(NSRect)frame renderer:(RendererRef)renderer sharedRenderer:(RendererRef)sharedRenderer
			appReceivesEvents:(BOOL)appReceivesEvents highDensityDisplay:(BOOL)highDensityDisplay enableMultiTouch:(BOOL)enableMultiTouch
{
	self = [super initWithFrame:frame];
	mRenderer = renderer;
	mReadyToDraw = NO;
	mFullScreen = NO;
	mReceivesEvents = appReceivesEvents;
	mHighDensityDisplayEnabled = highDensityDisplay;
	mMultiTouchEnabled = enableMultiTouch;
	mMarkedText = nil;
	mMarkedRange = NSMakeRange(NSNotFound, 0);

	mTouchIdMap = nil;
	mDelegate = nil;

	// Initialize modifier key state tracking
	memset(mModifierKeyPressed, 0, sizeof(mModifierKeyPressed));

	[self setupRendererWithFrame:frame renderer:renderer sharedRenderer:sharedRenderer];

	return self;
}

- (void)setupRendererWithFrame:(NSRect)frame renderer:(RendererRef)renderer sharedRenderer:(RendererRef)sharedRenderer
{
	mRenderer = renderer;
	mRenderer->setup( NSRectToCGRect( frame ), self, sharedRenderer, mHighDensityDisplayEnabled );

	mContentScaleFactor = ( mHighDensityDisplayEnabled ? self.window.backingScaleFactor : 1 );

	// register for drop events
	if( mReceivesEvents )
		[self registerForDraggedTypes:[NSArray arrayWithObject:NSFilenamesPboardType]];

	// register for touch events
	if( mDelegate && mMultiTouchEnabled ) {
		[self setAcceptsTouchEvents:YES];
		[self setWantsRestingTouches:YES];
		if( ! mTouchIdMap )
			mTouchIdMap = [[NSMutableDictionary alloc] initWithCapacity:10];
	}

	// listen to window fullscreen notifications so we can keep state consistent when user is manipulating the window mode via system controls
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowDidEnterFullScreen:) name:NSWindowDidEnterFullScreenNotification object:[self window]];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowDidExitFullScreen:) name:NSWindowDidExitFullScreenNotification object:[self window]];
}

- (float)contentScaleFactor
{
	return mContentScaleFactor;
}

- (void)viewDidChangeBackingProperties
{
	if( mHighDensityDisplayEnabled ) {
		mContentScaleFactor = self.window.backingScaleFactor;
		mRenderer->defaultResize();
	}
	else {
		mContentScaleFactor = 1.0f;
	}
}

- (BOOL)isFullScreen
{
	return mFullScreen;
}

- (void)setFullScreen:(BOOL)fullScreen options:(const cinder::app::FullScreenOptions *)options
{
	if( fullScreen == mFullScreen )
		return;

    // ignore kiosk mode flag if exiting fullscreen and use stored setting
    mFullScreenModeKiosk = ( fullScreen ? options->isKioskModeEnabled() : mFullScreenModeKiosk );
	if( ! mFullScreenModeKiosk ) {
		bool hasFullScreenButton = [[self window] collectionBehavior] & NSWindowCollectionBehaviorFullScreenPrimary;
		if( ! hasFullScreenButton ) {
			[[self window] setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary | NSWindowCollectionBehaviorIgnoresCycle];
		}
		if( fullScreen ) { // we need to force the window to be resizable if entering fullscreen
			[[self window] setStyleMask:([[self window] styleMask] | NSResizableWindowMask)];
		}
		[[self window] toggleFullScreen:nil];
	}
	else if( fullScreen ) {
        // Kiosk Mode
        NSMutableDictionary *dict = [NSMutableDictionary dictionary];
        if( ! options->isSecondaryDisplayBlankingEnabled() )
            [dict setObject:[NSNumber numberWithBool:NO] forKey:NSFullScreenModeAllScreens];
		if( ! options->isExclusive() )
			[dict setObject:[NSNumber numberWithUnsignedInteger:( NSApplicationPresentationHideMenuBar | NSApplicationPresentationHideDock )] forKey:NSFullScreenModeApplicationPresentationOptions];

		cinder::DisplayMac *displayMac = dynamic_cast<cinder::DisplayMac*>( options->getDisplay().get() );
        NSScreen *screen = ( displayMac ? displayMac->getNsScreen() : [[self window] screen] );
		[[self window] setCollectionBehavior:NSWindowCollectionBehaviorIgnoresCycle | NSWindowCollectionBehaviorTransient];		
        [self enterFullScreenMode:screen withOptions:dict];
    }
    else {
		// Exit kiosk
        [self exitFullScreenModeWithOptions:nil];
        [[self window] becomeKeyWindow];
        [[self window] makeFirstResponder:self];
    }
    
	mFullScreen = fullScreen;
}

- (void)draw
{
	if( ! mReadyToDraw )
		return;

	[self drawRect:[self bounds]];
}

- (void)makeCurrentContext
{
	mRenderer->makeCurrentContext();
}

- (void)drawRect:(NSRect)rect
{
	[super drawRect:rect];

	if( ! mReadyToDraw )
		return;

	mRenderer->startDraw();
	[mDelegate draw];
	mRenderer->finishDraw();
}

- (void)setFrameSize:(NSSize)newSize
{
	[super setFrameSize:newSize];

	if( mRenderer )
		mRenderer->setFrameSize( newSize.width, newSize.height );

	if( mReadyToDraw ) {
		mRenderer->makeCurrentContext();
		mRenderer->defaultResize();
		[mDelegate resize];
		[self draw];
	}
}

- (BOOL)isFlipped
{
	return YES;
}

- (BOOL)isOpaque
{
	return YES;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[mMarkedText release];
	[super dealloc];
}

/////////////////////////////////////////////////////////////////////////////////
// Event Handling
- (BOOL)acceptsFirstResponder
{
	return mReceivesEvents;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
	return mReceivesEvents;
}

- (int)prepMouseEventModifiers:(NSEvent *)evt
{
	unsigned int result = 0;
	if( [evt modifierFlags] & NSControlKeyMask ) result |= cinder::app::MouseEvent::CTRL_DOWN;
	if( [evt modifierFlags] & NSShiftKeyMask ) result |= cinder::app::MouseEvent::SHIFT_DOWN;
	if( [evt modifierFlags] & NSAlternateKeyMask ) result |= cinder::app::MouseEvent::ALT_DOWN;
	if( [evt modifierFlags] & NSCommandKeyMask ) result |= cinder::app::MouseEvent::META_DOWN;

	// Check which mouse buttons are currently pressed
	NSUInteger pressedButtons = [NSEvent pressedMouseButtons];
	if( pressedButtons & (1 << 0) ) result |= cinder::app::MouseEvent::LEFT_DOWN;   // Left button
	if( pressedButtons & (1 << 1) ) result |= cinder::app::MouseEvent::RIGHT_DOWN;  // Right button
	if( pressedButtons & (1 << 2) ) result |= cinder::app::MouseEvent::MIDDLE_DOWN; // Middle button

	return result;
}

- (int)prepKeyEventModifiers:(NSEvent *)evt
{
	unsigned int result = 0;
	
	if( [evt modifierFlags] & NSShiftKeyMask ) result |= cinder::app::KeyEvent::SHIFT_DOWN;
	if( [evt modifierFlags] & NSAlternateKeyMask ) result |= cinder::app::KeyEvent::ALT_DOWN;
	if( [evt modifierFlags] & NSCommandKeyMask ) result |= cinder::app::KeyEvent::META_DOWN;
	if( [evt modifierFlags] & NSControlKeyMask ) result |= cinder::app::KeyEvent::CTRL_DOWN;
	
	return result;
}

- (void)keyDown:(NSEvent*)theEvent
{
	NSString *chars = [theEvent characters];
	uint32_t c32	= ([chars length] > 0 ) ? [chars characterAtIndex:0] : 0;
	NSString *charsNoMods	= [theEvent charactersIgnoringModifiers];
	char c					= ([charsNoMods length] > 0 ) ? [charsNoMods characterAtIndex:0] : 0;

	int code	= [theEvent keyCode];
	int mods	= [self prepKeyEventModifiers:theEvent];

	cinder::app::KeyEvent keyEvent( [mDelegate getWindowRef], cinder::app::KeyEvent::translateNativeKeyCode( code ), c32,
									c, mods, code);
	[mDelegate keyDown:&keyEvent];

	// Always call interpretKeyEvents for IME and dead key support
	// The text input system will call insertText: with composed characters
	// Applications can ignore keyChar events if they don't want text input
	[self interpretKeyEvents:[NSArray arrayWithObject:theEvent]];
}

- (void)keyUp:(NSEvent*)theEvent
{
	NSString *chars = [theEvent characters];
	uint32_t c32	= ([chars length] > 0 ) ? [chars characterAtIndex:0] : 0;
	NSString *charsNoMods	= [theEvent charactersIgnoringModifiers];
	char c					= ([charsNoMods length] > 0 ) ? [charsNoMods characterAtIndex:0] : 0;

	int code	= [theEvent keyCode];
	int mods	= [self prepKeyEventModifiers:theEvent];
	
	cinder::app::KeyEvent keyEvent( [mDelegate getWindowRef], cinder::app::KeyEvent::translateNativeKeyCode( code ),
									c32, c, mods, code );
	[mDelegate keyUp:&keyEvent];
}

- (void)flagsChanged:(NSEvent*)theEvent
{
	int code = [theEvent keyCode];
	int mods = [self prepKeyEventModifiers:theEvent];
	NSEventModifierFlags flags = [theEvent modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;

	// Determine which modifier key changed by checking its specific bit
	// Map key codes to their corresponding modifier flags
	// Left/Right Command: 55, 54
	// Left/Right Option: 58, 61
	// Left/Right Shift: 56, 60
	// Left/Right Control: 59, 62
	NSEventModifierFlags keyFlag = 0;
	if (code == 55 || code == 54) {
		keyFlag = NSEventModifierFlagCommand;
	}
	else if (code == 58 || code == 61) {
		keyFlag = NSEventModifierFlagOption;
	}
	else if (code == 56 || code == 60) {
		keyFlag = NSEventModifierFlagShift;
	}
	else if (code == 59 || code == 62) {
		keyFlag = NSEventModifierFlagControl;
	}

	// Guard against invalid key codes
	if (code >= 256 || keyFlag == 0) {
		return;
	}

	// Determine action using toggle detection
	// When a modifier flag is set but we already recorded it as pressed,
	// treat this as a toggle to release. This handles spurious duplicate
	// flagsChanged events that macOS sometimes sends.
	// State is cleared on focus loss (applicationWillResignActive) to prevent stuck keys.
	BOOL action; // YES = press, NO = release

	if (keyFlag & flags) {
		// Flag is currently set
		if (mModifierKeyPressed[code]) {
			// Already pressed - this is a duplicate event (window switching)
			// Treat as release to clean up state
			action = NO;
		}
		else {
			// First time seeing it pressed
			action = YES;
		}
	}
	else {
		// Flag is cleared - this is a release
		action = NO;
	}

	// Update state tracking
	mModifierKeyPressed[code] = action;

	// Emit the event
	cinder::app::KeyEvent keyEvent( [mDelegate getWindowRef], cinder::app::KeyEvent::translateNativeKeyCode( code ), 0, 0, mods, code);
	if (action) {
		[mDelegate keyDown:&keyEvent];
	}
	else {
		[mDelegate keyUp:&keyEvent];
	}
}

// Text input support for dead keys and IME
- (void)insertText:(id)string
{
	NSString *characters;
	if ([string isKindOfClass:[NSAttributedString class]]) {
		characters = [string string];
	}
	else {
		characters = (NSString*)string;
	}

	// Emit keyChar for each composed character
	for (NSUInteger i = 0; i < [characters length]; i++) {
		uint32_t c32 = [characters characterAtIndex:i];

		// Filter out function key sentinels (0xF700-0xF7FF)
		// These are private-use Unicode codepoints that macOS uses for non-printable keys
		// We already delivered these via keyDown, so don't emit them as characters
		if (c32 >= 0xF700 && c32 <= 0xF7FF) {
			continue;
		}

		char c = (c32 < 256) ? (char)c32 : 0;

		// Get current modifier state (no key code for composed characters)
		NSEvent *currentEvent = [NSApp currentEvent];
		int mods = currentEvent ? [self prepKeyEventModifiers:currentEvent] : 0;

		cinder::app::KeyEvent charEvent([mDelegate getWindowRef], 0, c32, c, mods, 0);
		[mDelegate keyChar:&charEvent];
	}
}

- (void)doCommandBySelector:(SEL)selector
{
	// Make this a no-op to prevent AppKit from executing navigation commands
	// that would swallow key-up events and cause stuck keys.
	// Applications handle their own keyboard shortcuts via the raw key events.
}

#pragma mark - NSTextInputClient Protocol

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange
{
	[self insertText:string];
}

- (void)setMarkedText:(id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange
{
	if (mMarkedText != string) {
		[mMarkedText release];
		if ([string isKindOfClass:[NSAttributedString class]]) {
			mMarkedText = [[NSMutableAttributedString alloc] initWithAttributedString:string];
		}
		else {
			mMarkedText = [[NSMutableAttributedString alloc] initWithString:string];
		}
	}
	mMarkedRange = NSMakeRange(0, [mMarkedText length]);
}

- (void)unmarkText
{
	[mMarkedText release];
	mMarkedText = nil;
	mMarkedRange = NSMakeRange(NSNotFound, 0);
}

- (NSRange)selectedRange
{
	return NSMakeRange(NSNotFound, 0);
}

- (NSRange)markedRange
{
	return mMarkedRange;
}

- (BOOL)hasMarkedText
{
	return mMarkedText != nil && mMarkedRange.location != NSNotFound;
}

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)range actualRange:(NSRangePointer)actualRange
{
	return nil;
}

- (NSArray<NSAttributedStringKey> *)validAttributesForMarkedText
{
	return @[];
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange
{
	return NSZeroRect;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point
{
	return NSNotFound;
}

- (void)mouseDown:(NSEvent*)theEvent
{
	NSPoint curPoint		= [theEvent locationInWindow];
	int x					= (curPoint.x - [self frame].origin.x);
	int y					= ([self frame].size.height - ( curPoint.y - [self frame].origin.y ));
	int mods				= [self prepMouseEventModifiers:theEvent];

	cinder::app::MouseEvent mouseEvent( [mDelegate getWindowRef], cinder::app::MouseEvent::LEFT_DOWN, x, y, mods, 0.0f, (uint32_t)[theEvent modifierFlags] );
	[mDelegate mouseDown:&mouseEvent];
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
	NSPoint curPoint		= [theEvent locationInWindow];
	int x					= (curPoint.x - [self frame].origin.x);
	int y					= ([self frame].size.height - ( curPoint.y - [self frame].origin.y ));
	int mods				= [self prepMouseEventModifiers:theEvent];

	cinder::app::MouseEvent mouseEvent( [mDelegate getWindowRef], cinder::app::MouseEvent::RIGHT_DOWN, x, y, mods, 0.0f, (uint32_t)[theEvent modifierFlags] );
	[mDelegate mouseDown:&mouseEvent];
}

- (void)otherMouseDown:(NSEvent *)theEvent
{
	NSPoint curPoint		= [theEvent locationInWindow];
	int x					= (curPoint.x - [self frame].origin.x);
	int y					= ([self frame].size.height - ( curPoint.y - [self frame].origin.y ));
	int mods				= [self prepMouseEventModifiers:theEvent];

	cinder::app::MouseEvent mouseEvent( [mDelegate getWindowRef], cinder::app::MouseEvent::MIDDLE_DOWN, x, y, mods, 0.0f, (uint32_t)[theEvent modifierFlags] );
	[mDelegate mouseDown:&mouseEvent];
}

- (void)mouseUp:(NSEvent*)theEvent
{
	NSPoint curPoint		= [theEvent locationInWindow];
	int x					= (curPoint.x - [self frame].origin.x);
	int y					= ([self frame].size.height - ( curPoint.y - [self frame].origin.y ));
	int mods				= [self prepMouseEventModifiers:theEvent];

	cinder::app::MouseEvent mouseEvent( [mDelegate getWindowRef], cinder::app::MouseEvent::LEFT_DOWN, x, y, mods, 0.0f, (uint32_t)[theEvent modifierFlags] );
	[mDelegate mouseUp:&mouseEvent];
}

- (void)rightMouseUp:(NSEvent*)theEvent
{
	NSPoint curPoint		= [theEvent locationInWindow];
	int x					= (curPoint.x - [self frame].origin.x);
	int y					= ([self frame].size.height - ( curPoint.y - [self frame].origin.y ));
	int mods				= [self prepMouseEventModifiers:theEvent];

	cinder::app::MouseEvent mouseEvent( [mDelegate getWindowRef], cinder::app::MouseEvent::RIGHT_DOWN, x, y, mods, 0.0f, (uint32_t)[theEvent modifierFlags] );
	[mDelegate mouseUp:&mouseEvent];
}

- (void)otherMouseUp:(NSEvent*)theEvent
{
	NSPoint curPoint		= [theEvent locationInWindow];
	int x					= (curPoint.x - [self frame].origin.x);
	int y					= ([self frame].size.height - ( curPoint.y - [self frame].origin.y ));
	int mods				= [self prepMouseEventModifiers:theEvent];

	cinder::app::MouseEvent mouseEvent( [mDelegate getWindowRef], cinder::app::MouseEvent::MIDDLE_DOWN, x, y, mods, 0.0f, (uint32_t)[theEvent modifierFlags] );
	[mDelegate mouseUp:&mouseEvent];
}

- (void)mouseMoved:(NSEvent*)theEvent
{
	NSPoint curPoint		= [theEvent locationInWindow];
	int x					= (curPoint.x - [self frame].origin.x);
	int y					= ([self frame].size.height - ( curPoint.y - [self frame].origin.y ));
	int mods				= [self prepMouseEventModifiers:theEvent];
	
	cinder::app::MouseEvent mouseEvent( [mDelegate getWindowRef], 0, x, y, mods, 0.0f, (uint32_t)[theEvent modifierFlags] );
	[mDelegate mouseMove:&mouseEvent];
}

- (void)rightMouseDragged:(NSEvent*)theEvent
{
	NSPoint curPoint		= [theEvent locationInWindow];
	int x					= (curPoint.x - [self frame].origin.x);
	int y					= ([self frame].size.height - ( curPoint.y - [self frame].origin.y ));
	int mods				= [self prepMouseEventModifiers:theEvent];

	cinder::app::MouseEvent mouseEvent( [mDelegate getWindowRef], cinder::app::MouseEvent::RIGHT_DOWN, x, y, mods, 0.0f, (uint32_t)[theEvent modifierFlags] );
	[mDelegate mouseDrag:&mouseEvent];
}

- (void)otherMouseDragged:(NSEvent*)theEvent
{
	NSPoint curPoint		= [theEvent locationInWindow];
	int x					= (curPoint.x - [self frame].origin.x);
	int y					= ([self frame].size.height - ( curPoint.y - [self frame].origin.y ));
	int mods				= [self prepMouseEventModifiers:theEvent];

	cinder::app::MouseEvent mouseEvent( [mDelegate getWindowRef], cinder::app::MouseEvent::MIDDLE_DOWN, x, y, mods, 0.0f, (uint32_t)[theEvent modifierFlags] );
	[mDelegate mouseDrag:&mouseEvent];
}

- (void)mouseDragged:(NSEvent*)theEvent
{
	NSPoint curPoint		= [theEvent locationInWindow];
	int x					= (curPoint.x - [self frame].origin.x);
	int y					= ([self frame].size.height - ( curPoint.y - [self frame].origin.y ));
	int mods				= [self prepMouseEventModifiers:theEvent];

	cinder::app::MouseEvent mouseEvent( [mDelegate getWindowRef], cinder::app::MouseEvent::LEFT_DOWN, x, y, mods, 0.0f, (uint32_t)[theEvent modifierFlags] );
	[mDelegate mouseDrag:&mouseEvent];
}

- (void)scrollWheel:(NSEvent*)theEvent
{
	float wheelDelta		= [theEvent deltaX] + [theEvent deltaY];
	NSPoint curPoint		= [theEvent locationInWindow];
	int x					= (curPoint.x - [self frame].origin.x);
	int y					= ([self frame].size.height - ( curPoint.y - [self frame].origin.y ));
	int mods				= [self prepMouseEventModifiers:theEvent];
	
	cinder::app::MouseEvent mouseEvent( [mDelegate getWindowRef], 0, x, y, mods, wheelDelta / 4.0f, (uint32_t)[theEvent modifierFlags] );
	[mDelegate mouseWheel:&mouseEvent];
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender
{
	return NSDragOperationCopy;
}

- (BOOL)prepareForDragOperation:(id <NSDraggingInfo>)sender
{
	return YES;
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender
{
    NSPasteboard *pboard = [sender draggingPasteboard];
	
    if( [[pboard types] containsObject:NSFilenamesPboardType] ) {
        NSArray *files = [pboard propertyListForType:NSFilenamesPboardType];
        size_t numberOfFiles = [files count];
		std::vector<cinder::fs::path> paths;
		for( size_t i = 0; i < numberOfFiles; ++i )
			paths.push_back( cinder::fs::path( [[files objectAtIndex:i] UTF8String] ) );
		NSPoint curPoint = [sender draggingLocation];
		int x = curPoint.x - [self frame].origin.x;
		int y = [self frame].size.height - ( curPoint.y - [self frame].origin.y );

		// Get current modifier key state
		unsigned int modifiers = 0;
		NSUInteger nsModifiers = [NSEvent modifierFlags];
		if( nsModifiers & NSControlKeyMask ) modifiers |= cinder::app::FileDropEvent::CTRL_DOWN;
		if( nsModifiers & NSShiftKeyMask ) modifiers |= cinder::app::FileDropEvent::SHIFT_DOWN;
		if( nsModifiers & NSAlternateKeyMask ) modifiers |= cinder::app::FileDropEvent::ALT_DOWN;
		if( nsModifiers & NSCommandKeyMask ) modifiers |= cinder::app::FileDropEvent::META_DOWN;

		cinder::app::FileDropEvent fileDropEvent( [mDelegate getWindowRef], x, y, paths, modifiers );
		[mDelegate fileDrop:&fileDropEvent];
	}

    return YES;
}

- (void)applicationWillResignActive:(NSNotification *)aNotification
{
	// Clear modifier key state to prevent stuck keys when focus is lost
	// This prevents issues when user switches windows while holding modifier keys
	memset(mModifierKeyPressed, 0, sizeof(mModifierKeyPressed));

   	std::vector<cinder::app::TouchEvent::Touch> touchList;
	double eventTime = cinder::app::getElapsedSeconds();
	for( const auto &prevPt : mTouchPrevPointMap ) {
		touchList.push_back( cinder::app::TouchEvent::Touch( prevPt.second, prevPt.second, prevPt.first, eventTime, nil ) );
	}

	if( ! touchList.empty() ) {
		cinder::app::TouchEvent touchEvent( [mDelegate getWindowRef], touchList );
		[mDelegate touchesEnded:&touchEvent];
	}

	mTouchPrevPointMap.clear();
	[mTouchIdMap removeAllObjects];
}

- (cinder::app::RendererRef)getRenderer
{
	return mRenderer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// MultiTouch
- (void)setDelegate:(id<CinderViewDelegate>)delegate
{
	mDelegate = delegate;
	if( delegate && mMultiTouchEnabled ) {
		[self setAcceptsTouchEvents:YES];
		if( ! mTouchIdMap )
			mTouchIdMap = [[NSMutableDictionary alloc] initWithCapacity:10];
	}
	else {
		[self setAcceptsTouchEvents:NO];
	}
}

- (uint32_t)addTouchToMap:(NSTouch *)touch withPoint:(cinder::vec2)point
{
	uint32_t candidateId = 0;
	NSArray *currentValues = [mTouchIdMap allValues];
	bool found = true;
	while( found ) {
		candidateId++;
		if( [currentValues indexOfObjectIdenticalTo:[NSNumber numberWithInt:candidateId]] == NSNotFound )
			found = false;
	}
	
	[mTouchIdMap setObject:[NSNumber numberWithInt:candidateId] forKey:[touch identity]];
	mTouchPrevPointMap[candidateId] = point;
	return candidateId;
}

- (void)removeTouchFromMap:(NSTouch *)touch
{
	NSNumber *num = [mTouchIdMap objectForKey:[touch identity]];
	uint32_t curId = [num unsignedIntValue];
	[mTouchIdMap removeObjectForKey:[touch identity]];
	mTouchPrevPointMap.erase( curId );
}

- (std::pair<uint32_t,cinder::vec2>)updateTouch:(NSTouch *)touch withPoint:(cinder::vec2)point
{
	uint32_t curId = 0;
	NSNumber *num = [mTouchIdMap objectForKey:[touch identity]];
	if( num ) {
		curId = [num unsignedIntValue];
		cinder::vec2 prevPt = mTouchPrevPointMap[curId];
		mTouchPrevPointMap[curId] = point;
		return std::make_pair( curId, prevPt );		
	}
	else {
		// sometimes we will get a move event for a touch we have no record of
		// this can happen when the app resigns and the user never ends the touch
		return std::make_pair( [self addTouchToMap:touch withPoint:point], point );
	}	
}

- (void)updateActiveTouches:(NSEvent *)event
{
	mActiveTouches.clear();
	NSSet *touches = [event touchesMatchingPhase:NSTouchPhaseTouching inView:self];
	float width = [self frame].size.width;
	float height = [self frame].size.height;
	double eventTime = [event timestamp];
	for( NSTouch *touch in touches ) {
		NSPoint rawPt = [touch normalizedPosition];
		cinder::vec2 pt( rawPt.x * width, height - rawPt.y * height );
		std::pair<uint32_t,cinder::vec2> prev = [self updateTouch:touch withPoint:pt];
		mActiveTouches.push_back( cinder::app::TouchEvent::Touch( pt, prev.second, prev.first, eventTime, touch ) );
	}
}

- (const std::vector<cinder::app::TouchEvent::Touch>&)getActiveTouches
{
	return mActiveTouches;
}

- (void)touchesBeganWithEvent:(NSEvent *)event
{
	std::vector<cinder::app::TouchEvent::Touch> touchList;
	NSSet *touches = [event touchesMatchingPhase:NSTouchPhaseBegan inView:self];
	float width = [self frame].size.width;
	float height = [self frame].size.height;
	double eventTime = [event timestamp];
	for( NSTouch *touch in touches ) {
		NSPoint rawPt = [touch normalizedPosition];
		cinder::vec2 pt( rawPt.x * width, height - rawPt.y * height );
		touchList.push_back( cinder::app::TouchEvent::Touch( pt, pt, [self addTouchToMap:touch withPoint:pt], eventTime, touch ) );
	}
	[self updateActiveTouches:event];
	if( mDelegate && ( ! touchList.empty() ) ) {
		cinder::app::TouchEvent touchEvent( [mDelegate getWindowRef], touchList, event );
		[mDelegate touchesBegan:&touchEvent];
	}
}

- (void)touchesMovedWithEvent:(NSEvent *)event
{
   	std::vector<cinder::app::TouchEvent::Touch> touchList;
	NSSet *touches = [event touchesMatchingPhase:NSTouchPhaseMoved inView:self];
	float width = [self frame].size.width;
	float height = [self frame].size.height;
	double eventTime = [event timestamp];
	for( NSTouch *touch in touches ) {
		NSPoint rawPt = [touch normalizedPosition];
		cinder::vec2 pt( rawPt.x * width, height - rawPt.y * height );
		std::pair<uint32_t,cinder::vec2> prev = [self updateTouch:touch withPoint:pt];
		touchList.push_back( cinder::app::TouchEvent::Touch( pt, prev.second, prev.first, eventTime, touch ) );
	}
	[self updateActiveTouches:event];
	if( mDelegate && ( ! touchList.empty() ) ) {
		cinder::app::TouchEvent touchEvent( [mDelegate getWindowRef], touchList, event );
		[mDelegate touchesMoved:&touchEvent];
	}
}

- (void)touchesEndedWithEvent:(NSEvent *)event
{
   	std::vector<cinder::app::TouchEvent::Touch> touchList;
	NSSet *touches = [event touchesMatchingPhase:NSTouchPhaseEnded inView:self];
	float width = [self frame].size.width;
	float height = [self frame].size.height;
	double eventTime = [event timestamp];
	for( NSTouch *touch in touches ) {
		NSPoint rawPt = [touch normalizedPosition];
		cinder::vec2 pt( rawPt.x * width, height - rawPt.y * height );
		std::pair<uint32_t,cinder::vec2> prev = [self updateTouch:touch withPoint:pt];
		touchList.push_back( cinder::app::TouchEvent::Touch( pt, prev.second, prev.first, eventTime, touch ) );
		[self removeTouchFromMap:touch];
	}

	[self updateActiveTouches:event];
	if( mDelegate && ( ! touchList.empty() ) ) {
		cinder::app::TouchEvent touchEvent( [mDelegate getWindowRef], touchList, event );
		[mDelegate touchesEnded:&touchEvent];
	}
}
 
- (void)touchesCancelledWithEvent:(NSEvent *)event
{
   	std::vector<cinder::app::TouchEvent::Touch> touchList;
	NSSet *touches = [event touchesMatchingPhase:NSTouchPhaseCancelled inView:self];
	float width = [self frame].size.width;
	float height = [self frame].size.height;
	double eventTime = [event timestamp];
	for( NSTouch *touch in touches ) {
		NSPoint rawPt = [touch normalizedPosition];
		cinder::vec2 pt( rawPt.x * width, height - rawPt.y * height );
		std::pair<uint32_t,cinder::vec2> prev = [self updateTouch:touch withPoint:pt];
		touchList.push_back( cinder::app::TouchEvent::Touch( pt, prev.second, prev.first, eventTime, touch ) );
		[self removeTouchFromMap:touch];
	}

	[self updateActiveTouches:event];
	if( mDelegate && ( ! touchList.empty() ) ) {
		cinder::app::TouchEvent touchEvent( [mDelegate getWindowRef], touchList, event );
		[mDelegate touchesEnded:&touchEvent];
	}
}

- (void)windowDidEnterFullScreen:(NSNotification *)notification
{
	mFullScreenModeKiosk = NO; // this notification only comes via, non kiosk, 10.7+ mode
	mFullScreen = YES;
}

- (void)windowDidExitFullScreen:(NSNotification *)notification
{
	mFullScreen = NO;
}

@end
