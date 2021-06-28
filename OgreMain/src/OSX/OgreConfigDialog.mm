

/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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

#import "OgreLogManager.h"
#import "OgreConfigDialog.h"

using namespace Ogre;

namespace Ogre {

	ConfigDialog* dlg = NULL;

	ConfigDialog::ConfigDialog()
	{
		dlg = this;
	}
	
	ConfigDialog::~ConfigDialog()
	{
        mWindowDelegate = nil;
	}
	
	void ConfigDialog::initialise()
	{
        mWindowDelegate = [[OgreConfigWindowDelegate alloc] init];

        if (!mWindowDelegate)
            OGRE_EXCEPT (Exception::ERR_INTERNAL_ERROR, "Could not load config dialog",
                         "ConfigDialog::initialise");
	}

	bool ConfigDialog::display()
	{
        // Select previously selected rendersystem
        mSelectedRenderSystem = Root::getSingleton().getRenderSystem();
        long retVal = 0;
        
        @autoreleasepool {
            initialise();

            // Run a modal dialog, Abort means cancel, Stop means Ok
            NSModalSession modalSession = [NSApp beginModalSessionForWindow:[mWindowDelegate getConfigWindow]];
            for (;;) {
                retVal = [NSApp runModalSession:modalSession];

                // User pressed a button
                if (retVal != NSRunContinuesResponse)
                    break;
            }
            [NSApp endModalSession:modalSession];

            // Set the rendersystem
            String selectedRenderSystemName = String([[[[mWindowDelegate getRenderSystemsPopUp] selectedItem] title]    UTF8String]);
            RenderSystem *rs = Root::getSingleton().getRenderSystemByName(selectedRenderSystemName);
            Root::getSingleton().setRenderSystem(rs);
        
            // Relinquish control of the table
            [[mWindowDelegate getOptionsTable] setDataSource:nil];
            [[mWindowDelegate getOptionsTable] setDelegate:nil];
        }

        return (retVal == NSRunStoppedResponse) ? true : false;
	}

}

@implementation OgreConfigWindowDelegate

- (id)init
{
    if((self = [super init]))
    {
        // This needs to be called in order to use Cocoa from a Carbon app
        NSApplicationLoad();

        // Construct the window manually
        mConfigWindow = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 512, 512)
                                                    styleMask:(NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask)
                                                      backing:NSBackingStoreBuffered
                                                        defer:NO];

        // Make ourselves the delegate
        [mConfigWindow setDelegate:self];

        // First do the buttons
        mOkButton = [[NSButton alloc] initWithFrame:NSMakeRect(414, 12, 84, 32)];
        [mOkButton setButtonType:NSMomentaryPushInButton];
        [mOkButton setBezelStyle:NSRoundedBezelStyle];
        [mOkButton setTitle:NSLocalizedString(@"OK", @"okButtonString")];
        [mOkButton setAction:@selector(okButtonPressed:)];
        [mOkButton setTarget:self];
        [mOkButton setKeyEquivalent:@"\r"];
        [[mConfigWindow contentView] addSubview:mOkButton];

        mCancelButton = [[NSButton alloc] initWithFrame:NSMakeRect(330, 12, 84, 32)];
        [mCancelButton setButtonType:NSMomentaryPushInButton];
        [mCancelButton setBezelStyle:NSRoundedBezelStyle];
        [mCancelButton setAction:@selector(cancelButtonPressed:)];
        [mCancelButton setTarget:self];
        [mCancelButton setKeyEquivalent:@"\e"];
        [mCancelButton setTitle:NSLocalizedString(@"Cancel", @"cancelButtonString")];
        [[mConfigWindow contentView] addSubview:mCancelButton];

        // Then the Ogre logo out of the framework bundle
        mOgreLogo = [[NSImageView alloc] initWithFrame:NSMakeRect(0, 295, 512, 220)];
        NSMutableString *logoPath = [[[NSBundle bundleForClass:[self class]] resourcePath] mutableCopy];
        [logoPath appendString:@"/ogrelogo.png"];

        NSImage *image = [[NSImage alloc] initWithContentsOfFile:logoPath];
        [mOgreLogo setImage:image];
        [mOgreLogo setImageScaling:NSScaleToFit];
        [mOgreLogo setEditable:NO];
        [[mConfigWindow contentView] addSubview:mOgreLogo];

        // Popup menu for rendersystems.  On OS X this is always OpenGL
        mRenderSystemsPopUp = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(168, 259, 327, 26) pullsDown:NO];
        [[mConfigWindow contentView] addSubview:mRenderSystemsPopUp];
        [mOptionsPopUp setAction:@selector(renderSystemChanged:)];
        [mOptionsPopUp setTarget:self];

        NSTextField *renderSystemLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(18, 265, 148, 17)];
        [renderSystemLabel setStringValue:NSLocalizedString(@"Rendering Subsystem", @"renderingSubsystemString")];
        [renderSystemLabel setEditable:NO];
        [renderSystemLabel setSelectable:NO];
        [renderSystemLabel setDrawsBackground:NO];
        [renderSystemLabel setAlignment:NSNaturalTextAlignment];
        [renderSystemLabel setBezeled:NO];
        [[mConfigWindow contentView] addSubview:renderSystemLabel];

        // The pretty box to contain the table and options
        NSBox *tableBox = [[NSBox alloc] initWithFrame:NSMakeRect(19, 54, 477, 203)];
        [tableBox setTitle:NSLocalizedString(@"Rendering System Options", @"optionsBoxString")];
        [tableBox setContentViewMargins:NSMakeSize(0, 0)];
        [tableBox setFocusRingType:NSFocusRingTypeNone];
        [tableBox setBorderType:NSLineBorder];

        // Set up the tableview
        mOptionsTable = [[NSTableView alloc] init];
        [mOptionsTable setDelegate:self];
        [mOptionsTable setDataSource:self];
        [mOptionsTable setHeaderView:nil];
        [mOptionsTable setUsesAlternatingRowBackgroundColors:YES];
        [mOptionsTable sizeToFit];
        
        // Table column to hold option names
        NSTableColumn *column = [[NSTableColumn alloc] initWithIdentifier: @"optionName"];
        [column setEditable:NO];
        [column setMinWidth:237];
        [mOptionsTable addTableColumn:column];
        NSTableColumn *column2 = [[NSTableColumn alloc] initWithIdentifier: @"optionValue"];
        [column2 setEditable:NO];
        [column2 setMinWidth:200];
        [mOptionsTable addTableColumn:column2];

        // Scroll view to hold the table in case the list grows some day
        NSScrollView *scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(22, 42, 439, 135)];
        [scrollView setBorderType:NSBezelBorder];
        [scrollView setAutoresizesSubviews:YES];
        [scrollView setAutohidesScrollers:YES];
        [scrollView setDocumentView:mOptionsTable];
        
        [[tableBox contentView] addSubview:scrollView];

        mOptionLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(15, 15, 173, 17)];
        [mOptionLabel setStringValue:NSLocalizedString(@"Select an Option", @"optionLabelString")];
        [mOptionLabel setEditable:NO];
        [mOptionLabel setSelectable:NO];
        [mOptionLabel setDrawsBackground:NO];
        [mOptionLabel setAlignment:NSRightTextAlignment];
        [mOptionLabel setBezeled:NO];
        [[tableBox contentView] addSubview:mOptionLabel];

        mOptionsPopUp = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(190, 10, 270, 26) pullsDown:NO];
        [[tableBox contentView] addSubview:mOptionsPopUp];
        [mOptionsPopUp setAction:@selector(popUpValueChanged:)];
        [mOptionsPopUp setTarget:self];

        [[mConfigWindow contentView] addSubview:tableBox];
        
        // Add renderers to the drop down
        const RenderSystemList& renderers = Root::getSingleton().getAvailableRenderers();
        for (RenderSystemList::const_iterator pRend = renderers.begin(); pRend != renderers.end(); ++pRend)
        {
            NSString *renderSystemName = [NSString stringWithUTF8String:(*pRend)->getName().c_str()];
            [mRenderSystemsPopUp addItemWithTitle:renderSystemName];
        }

        // Initalise storage for the table data source
        mOptionsKeys = [[NSMutableArray alloc] init];
        mOptionsValues = [[NSMutableArray alloc] init];

        [self refreshConfigOptions];
    }
    return self;
}

-(void) refreshConfigOptions
{
    [mOptionsKeys removeAllObjects];
    [mOptionsValues removeAllObjects];

    // Get detected option values and add them to our config dictionary
    String selectedRenderSystemName = String([[[mRenderSystemsPopUp selectedItem] title] UTF8String]);
    RenderSystem *rs = Root::getSingleton().getRenderSystemByName(selectedRenderSystemName);
    const ConfigOptionMap& opts = rs->getConfigOptions();
    for (ConfigOptionMap::const_iterator pOpt = opts.begin(); pOpt != opts.end(); ++pOpt)
    {
        [mOptionsKeys addObject:[NSString stringWithUTF8String:pOpt->first.c_str()]];
        [mOptionsValues addObject:[NSString stringWithUTF8String:pOpt->second.currentValue.c_str()]];
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
#pragma unused(sender)
    // Grab a copy of the selected RenderSystem name in String format
    String selectedRenderSystemName = String([[[mRenderSystemsPopUp selectedItem] title] UTF8String]);
    
    // Save the current config value
    if((0 <= [mOptionsTable selectedRow]) && [mOptionsPopUp selectedItem])
    {
        String value = String([[[mOptionsPopUp selectedItem] title] UTF8String]);
        String name = String([[mOptionsKeys objectAtIndex:[mOptionsTable selectedRow]] UTF8String]);
        
        Root::getSingleton().getRenderSystemByName(selectedRenderSystemName)->setConfigOption(name, value);
        
        [self refreshConfigOptions];
    }
}

- (BOOL)windowShouldClose:(id)sender
{
#pragma unused(sender)
    // Hide the window
    [mConfigWindow orderOut:nil];
    
    [NSApp abortModal];

    return true;
}

- (void)cancelButtonPressed:(id)sender
{
#pragma unused(sender)
    // Hide the window
    [mConfigWindow orderOut:nil];

    [NSApp abortModal];
    [NSApp terminate:nil];
}

- (void)okButtonPressed:(id)sender
{
#pragma unused(sender)
    // Hide the window
    [mConfigWindow orderOut:nil];

    [NSApp stopModal];
}

#pragma mark NSTableView delegate and datasource methods
- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
#pragma unused(aTableView)
    bool valueColumn = [aTableColumn.identifier isEqualToString:@"optionValue"];
    return valueColumn ? [mOptionsValues objectAtIndex:rowIndex] : [mOptionsKeys objectAtIndex:rowIndex];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView
{
#pragma unused(aTableView)
    return [mOptionsKeys count];
}

// Intercept the request to select a new row.  Update the popup's values.
- (BOOL)tableView:(NSTableView *)aTableView shouldSelectRow:(NSInteger)rowIndex
{
#pragma unused(aTableView)
    // Clear out the options popup menu
    [mOptionsPopUp removeAllItems];
    
    // Get the key for the selected table row
    NSString *key = [mOptionsKeys objectAtIndex:rowIndex];
    
    // Grab a copy of the selected RenderSystem name in String format
    if([mRenderSystemsPopUp numberOfItems] > 0)
    {
        String selectedRenderSystemName = String([[[mRenderSystemsPopUp selectedItem] title] UTF8String]);
        const ConfigOptionMap& opts = Root::getSingleton().getRenderSystemByName(selectedRenderSystemName)->getConfigOptions();

        // Add the available options
        // Select the item that is the current config option, if there is no current setting, just pick the top of the list
        ConfigOptionMap::const_iterator it = opts.find([key UTF8String]);
        if (it != opts.end())
        {
            for(uint i = 0; i < it->second.possibleValues.size(); i++)
            {
                NSString *value = [NSString stringWithUTF8String:it->second.possibleValues[i].c_str()];
                [mOptionsPopUp addItemWithTitle:value];
            }

            [mOptionsPopUp selectItemWithTitle:[NSString stringWithCString:it->second.currentValue.c_str()
                                     encoding:NSASCIIStringEncoding]];
        }

        if([mOptionsPopUp indexOfSelectedItem] < 0)
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
