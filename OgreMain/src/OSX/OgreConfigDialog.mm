/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#import "OgreConfigDialog.h"

#import "OgreConfigOptionMap.h"
#import "OgreLogManager.h"

using namespace Ogre;

namespace Ogre
{
    ConfigDialog *dlg = NULL;

    ConfigDialog::ConfigDialog() { dlg = this; }

    ConfigDialog::~ConfigDialog() { mWindowDelegate = nil; }

    void ConfigDialog::initialise()
    {
        mWindowDelegate = [[OgreConfigWindowDelegate alloc] init];

        if( !mWindowDelegate )
            OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR, "Could not load config dialog",
                         "ConfigDialog::initialise" );
    }

    bool ConfigDialog::display()
    {
        // Select previously selected rendersystem
        mSelectedRenderSystem = Root::getSingleton().getRenderSystem();
        long retVal = 0;

        @autoreleasepool
        {
            initialise();

            // Run a modal dialog, Abort means cancel, Stop means Ok
            NSModalSession modalSession =
                [NSApp beginModalSessionForWindow:[mWindowDelegate getConfigWindow]];
            for( ;; )
            {
                retVal = [NSApp runModalSession:modalSession];

                // User pressed a button
                if( retVal != NSModalResponseContinue )
                    break;
            }
            [NSApp endModalSession:modalSession];

            // Set the rendersystem
            String selectedRenderSystemName =
                String( mWindowDelegate.getRenderSystemsPopUp.selectedItem.title.UTF8String );
            RenderSystem *rs = Root::getSingleton().getRenderSystemByName( selectedRenderSystemName );
            Root::getSingleton().setRenderSystem( rs );

            // Relinquish control of the table
            mWindowDelegate.getOptionsTable.dataSource = nil;
            mWindowDelegate.getOptionsTable.delegate = nil;
        }

        return ( retVal == NSModalResponseStop ) ? true : false;
    }

}  // namespace Ogre

@implementation OgreConfigWindowDelegate

- (id)init
{
    if( ( self = [super init] ) )
    {
        // This needs to be called in order to use Cocoa from a Carbon app
        NSApplicationLoad();

        // Construct the window manually
        mConfigWindow =
            [[NSWindow alloc] initWithContentRect:NSMakeRect( 0, 0, 512, 512 )
                                        styleMask:( NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                                    NSWindowStyleMaskMiniaturizable )
                                          backing:NSBackingStoreBuffered
                                            defer:NO];

        // Make ourselves the delegate
        [mConfigWindow setDelegate:self];

        // First do the buttons
        mOkButton = [[NSButton alloc] initWithFrame:NSMakeRect( 414, 12, 84, 32 )];
        mOkButton.buttonType = NSButtonTypeMomentaryPushIn;
        mOkButton.bezelStyle = NSBezelStyleRounded;
        mOkButton.title = NSLocalizedString( @"OK", @"okButtonString" );
        mOkButton.action = @selector( okButtonPressed: );
        mOkButton.target = self;
        mOkButton.keyEquivalent = @"\r";
        [mConfigWindow.contentView addSubview:mOkButton];

        mCancelButton = [[NSButton alloc] initWithFrame:NSMakeRect( 330, 12, 84, 32 )];
        mCancelButton.buttonType = NSButtonTypeMomentaryPushIn;
        mCancelButton.bezelStyle = NSBezelStyleRounded;
        mCancelButton.action = @selector( cancelButtonPressed: );
        mCancelButton.target = self;
        mCancelButton.keyEquivalent = @"\e";
        mCancelButton.title = NSLocalizedString( @"Cancel", @"cancelButtonString" );
        [mConfigWindow.contentView addSubview:mCancelButton];

        // Then the Ogre logo out of the framework bundle
        mOgreLogo = [[NSImageView alloc] initWithFrame:NSMakeRect( 0, 295, 512, 220 )];
        NSMutableString *logoPath = [NSBundle bundleForClass:self.class].resourcePath.mutableCopy;
        [logoPath appendString:@"/ogrelogo.png"];

        NSImage *image = [[NSImage alloc] initWithContentsOfFile:logoPath];
        mOgreLogo.image = image;
        mOgreLogo.imageScaling = NSImageScaleAxesIndependently;
        mOgreLogo.editable = NO;
        [mConfigWindow.contentView addSubview:mOgreLogo];

        // Popup menu for rendersystems.  On OS X this is always OpenGL
        mRenderSystemsPopUp = [[NSPopUpButton alloc] initWithFrame:NSMakeRect( 168, 259, 327, 26 )
                                                         pullsDown:NO];
        [mConfigWindow.contentView addSubview:mRenderSystemsPopUp];
        mOptionsPopUp.action = @selector( renderSystemChanged: );
        mOptionsPopUp.target = self;

        NSTextField *renderSystemLabel =
            [[NSTextField alloc] initWithFrame:NSMakeRect( 18, 265, 148, 17 )];
        [renderSystemLabel
            setStringValue:NSLocalizedString( @"Rendering Subsystem", @"renderingSubsystemString" )];
        [renderSystemLabel setEditable:NO];
        [renderSystemLabel setSelectable:NO];
        [renderSystemLabel setDrawsBackground:NO];
        [renderSystemLabel setAlignment:NSTextAlignmentNatural];
        [renderSystemLabel setBezeled:NO];
        [[mConfigWindow contentView] addSubview:renderSystemLabel];

        // The pretty box to contain the table and options
        NSBox *tableBox = [[NSBox alloc] initWithFrame:NSMakeRect( 19, 54, 477, 203 )];
        tableBox.title = NSLocalizedString( @"Rendering System Options", @"optionsBoxString" );
        tableBox.contentViewMargins = NSMakeSize( 0, 0 );
        tableBox.focusRingType = NSFocusRingTypeNone;

        // Set up the tableview
        mOptionsTable = [[NSTableView alloc] init];
        mOptionsTable.delegate = self;
        mOptionsTable.dataSource = self;
        mOptionsTable.headerView = nil;
        mOptionsTable.usesAlternatingRowBackgroundColors = YES;
        [mOptionsTable sizeToFit];

        // Table column to hold option names
        NSTableColumn *column = [[NSTableColumn alloc] initWithIdentifier:@"optionName"];
        column.editable = NO;
        column.minWidth = 237;
        [mOptionsTable addTableColumn:column];
        NSTableColumn *column2 = [[NSTableColumn alloc] initWithIdentifier:@"optionValue"];
        column2.editable = NO;
        column2.minWidth = 200;
        [mOptionsTable addTableColumn:column2];

        // Scroll view to hold the table in case the list grows some day
        NSScrollView *scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect( 22, 42, 439, 135 )];
        scrollView.borderType = NSBezelBorder;
        scrollView.autoresizesSubviews = YES;
        scrollView.autohidesScrollers = YES;
        scrollView.documentView = mOptionsTable;

        [tableBox.contentView addSubview:scrollView];

        mOptionLabel = [[NSTextField alloc] initWithFrame:NSMakeRect( 15, 15, 173, 17 )];
        mOptionLabel.stringValue = NSLocalizedString( @"Select an Option", @"optionLabelString" );
        mOptionLabel.editable = NO;
        mOptionLabel.selectable = NO;
        mOptionLabel.drawsBackground = NO;
        mOptionLabel.alignment = NSTextAlignmentRight;
        mOptionLabel.bezeled = NO;
        [tableBox.contentView addSubview:mOptionLabel];

        mOptionsPopUp = [[NSPopUpButton alloc] initWithFrame:NSMakeRect( 190, 10, 270, 26 )
                                                   pullsDown:NO];
        [tableBox.contentView addSubview:mOptionsPopUp];
        mOptionsPopUp.action = @selector( popUpValueChanged: );
        mOptionsPopUp.target = self;

        [mConfigWindow.contentView addSubview:tableBox];

        // Add renderers to the drop down
        for( const RenderSystem *rs : Root::getSingleton().getAvailableRenderers() )
            [mRenderSystemsPopUp addItemWithTitle:@( rs->getName().c_str() )];

        // Initalise storage for the table data source
        mOptionsKeys = [[NSMutableArray alloc] init];
        mOptionsValues = [[NSMutableArray alloc] init];

        [self refreshConfigOptions];
    }
    return self;
}

- (void)refreshConfigOptions
{
    [mOptionsKeys removeAllObjects];
    [mOptionsValues removeAllObjects];

    // Get detected option values and add them to our config dictionary
    String selectedRenderSystemName = String( mRenderSystemsPopUp.selectedItem.title.UTF8String );
    RenderSystem *rs = Root::getSingleton().getRenderSystemByName( selectedRenderSystemName );
    const ConfigOptionMap &opts = rs->getConfigOptions();
    for( ConfigOptionMap::const_iterator pOpt = opts.begin(); pOpt != opts.end(); ++pOpt )
    {
        [mOptionsKeys addObject:@( pOpt->first.c_str() )];
        [mOptionsValues addObject:@( pOpt->second.currentValue.c_str() )];
    }

    [mOptionsTable reloadData];
}

#pragma mark Window and Control delegate methods

- (void)renderSystemChanged:(id)sender
{
    [self refreshConfigOptions];
}

- (void)popUpValueChanged:(id)sender
{
#pragma unused( sender )
    // Grab a copy of the selected RenderSystem name in String format
    String selectedRenderSystemName = String( mRenderSystemsPopUp.selectedItem.title.UTF8String );

    // Save the current config value
    if( 0 <= mOptionsTable.selectedRow && mOptionsPopUp.selectedItem )
    {
        String value = String( mOptionsPopUp.selectedItem.title.UTF8String );
        String name = String( mOptionsKeys[(NSUInteger)mOptionsTable.selectedRow].UTF8String );

        Root::getSingleton()
            .getRenderSystemByName( selectedRenderSystemName )
            ->setConfigOption( name, value );

        [self refreshConfigOptions];
    }
}

- (BOOL)windowShouldClose:(id)sender
{
#pragma unused( sender )
    // Hide the window
    [mConfigWindow orderOut:nil];

    [NSApp abortModal];

    return true;
}

- (void)cancelButtonPressed:(id)sender
{
#pragma unused( sender )
    // Hide the window
    [mConfigWindow orderOut:nil];

    [NSApp abortModal];
    [NSApp terminate:nil];
}

- (void)okButtonPressed:(id)sender
{
#pragma unused( sender )
    // Hide the window
    [mConfigWindow orderOut:nil];

    [NSApp stopModal];
}

#pragma mark NSTableView delegate and datasource methods
- (id)tableView:(NSTableView *)aTableView
    objectValueForTableColumn:(NSTableColumn *)aTableColumn
                          row:(NSInteger)rowIndex
{
#pragma unused( aTableView )
    bool valueColumn = [aTableColumn.identifier isEqualToString:@"optionValue"];
    NSUInteger idx = (NSUInteger)rowIndex;
    return valueColumn ? mOptionsValues[idx] : mOptionsKeys[idx];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView
{
#pragma unused( aTableView )
    return (NSInteger)mOptionsKeys.count;
}

// Intercept the request to select a new row.  Update the popup's values.
- (BOOL)tableView:(NSTableView *)aTableView shouldSelectRow:(NSInteger)rowIndex
{
#pragma unused( aTableView )
    // Clear out the options popup menu
    [mOptionsPopUp removeAllItems];

    // Get the key for the selected table row
    NSString *key = mOptionsKeys[(NSUInteger)rowIndex];

    // Grab a copy of the selected RenderSystem name in String format
    if( mRenderSystemsPopUp.numberOfItems > 0 )
    {
        String selectedRenderSystemName = String( mRenderSystemsPopUp.selectedItem.title.UTF8String );
        const ConfigOptionMap &opts =
            Root::getSingleton().getRenderSystemByName( selectedRenderSystemName )->getConfigOptions();

        // Add the available options
        // Select the item that is the current config option, if there is no current setting, just pick
        // the top of the list
        ConfigOptionMap::const_iterator it = opts.find( key.UTF8String );
        if( it != opts.end() )
        {
            for( const String &possibleValue : it->second.possibleValues )
                [mOptionsPopUp addItemWithTitle:@( possibleValue.c_str() )];

            [mOptionsPopUp selectItemWithTitle:@( it->second.currentValue.c_str() )];
        }

        if( mOptionsPopUp.indexOfSelectedItem < 0 )
            [mOptionsPopUp selectItemAtIndex:0];

        // Always allow the new selection
        return YES;
    }
    else
    {
        return NO;
    }
}

#pragma mark Getters and Setters
- (NSWindow *)getConfigWindow
{
    return mConfigWindow;
}

- (void)setConfigWindow:(NSWindow *)window
{
    mConfigWindow = window;
}

- (NSPopUpButton *)getRenderSystemsPopUp
{
    return mRenderSystemsPopUp;
}

- (void)setRenderSystemsPopUp:(NSPopUpButton *)button
{
    mRenderSystemsPopUp = button;
}

- (void)setOgreLogo:(NSImageView *)image
{
    mOgreLogo = image;
}

- (NSImageView *)getOgreLogo
{
    return mOgreLogo;
}

- (void)setOptionsTable:(NSTableView *)table
{
    mOptionsTable = table;
}

- (NSTableView *)getOptionsTable
{
    return mOptionsTable;
}

- (void)setOptionsPopUp:(NSPopUpButton *)button
{
    mOptionsPopUp = button;
}

- (NSPopUpButton *)getOptionsPopUp
{
    return mOptionsPopUp;
}

@end
