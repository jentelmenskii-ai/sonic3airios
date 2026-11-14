// Native iOS entry point. SDL2main owns UIApplicationMain and invokes SDL_main.
#import <UIKit/UIKit.h>
#include <SDL_syswm.h>
#include "sonic3air/pch.h"
#include "sonic3air/EngineDelegate.h"
#include "sonic3air/GameArgumentsReader.h"
#include "oxygen/platform/PlatformFunctions.h"

// Present Files at the actual writable folder used by the engine.
// The picker is only for browsing/managing files: selecting an item imports nothing.
static void openIOSDirectory(const std::wstring& path)
{
    NSString* folderPath = [NSString stringWithUTF8String:rmx::convertToUTF8(path).c_str()];
    dispatch_async(dispatch_get_main_queue(), ^{
        NSURL* folderURL = [NSURL fileURLWithPath:folderPath isDirectory:YES];
        NSError* error = nil;
        if (![NSFileManager.defaultManager createDirectoryAtURL:folderURL
                withIntermediateDirectories:YES attributes:nil error:&error]) {
            SDL_Log("Cannot open mods folder: %s", error.localizedDescription.UTF8String);
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Mods folder",
                error.localizedDescription.UTF8String, nullptr);
            return;
        }
        SDL_Window* window = SDL_GetKeyboardFocus();
        if (!window) return;
        SDL_SysWMinfo info;
        SDL_VERSION(&info.version);
        if (!SDL_GetWindowWMInfo(window, &info) || info.subsystem != SDL_SYSWM_UIKIT) return;
        UIViewController* presenter = info.info.uikit.window.rootViewController;
        if (!presenter || presenter.presentedViewController) return;
        UIDocumentPickerViewController* picker = [[UIDocumentPickerViewController alloc]
            initWithDocumentTypes:@[@"public.item"] inMode:UIDocumentPickerModeOpen];
        picker.directoryURL = folderURL;
        picker.title = @"Mods folder";
        picker.modalPresentationStyle = UIModalPresentationFullScreen;
        [presenter presentViewController:picker animated:YES completion:nil];
    });
}

static bool isCompatibleROM(NSData* data)
{
    if (data.length != 0x400000) return false;
    // Apply the same compatibility bytes as ConfigurationImpl::fillDefaultGameProfile.
    NSMutableData* normalized = [data mutableCopy];
    static_cast<uint8*>(normalized.mutableBytes)[0x2001f0] = 0x4a;
    return rmx::getMurmur2_64(static_cast<const uint8*>(normalized.bytes), normalized.length) == 0x344983ffcfeff8cbULL;
}

@interface AIRImportController : UIViewController <UIDocumentPickerDelegate>
@property(nonatomic, copy) NSURL* destination;
@property(nonatomic) BOOL ready;
@property(nonatomic, strong) UILabel* message;
@end

@implementation AIRImportController
- (void)viewDidLoad
{
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor colorWithRed:0.03 green:0.06 blue:0.12 alpha:1];
    UILabel* title = [UILabel new];
    title.text = @"Sonic 3 A.I.R.";
    title.font = [UIFont boldSystemFontOfSize:32];
    title.textColor = UIColor.whiteColor;
    title.textAlignment = NSTextAlignmentCenter;
    self.message = [UILabel new];
    self.message.text = @"Choose your Sonic 3 & Knuckles ROM (.bin).\nThe original game ROM is not included.\nYou can also copy it into this app's folder using Files.";
    self.message.font = [UIFont systemFontOfSize:17];
    self.message.textColor = UIColor.whiteColor;
    self.message.numberOfLines = 0;
    self.message.textAlignment = NSTextAlignmentCenter;
    UIButton* button = [UIButton buttonWithType:UIButtonTypeSystem];
    [button setTitle:@"Choose ROM…" forState:UIControlStateNormal];
    button.titleLabel.font = [UIFont boldSystemFontOfSize:22];
    [button addTarget:self action:@selector(chooseROM) forControlEvents:UIControlEventTouchUpInside];
    UIStackView* stack = [[UIStackView alloc] initWithArrangedSubviews:@[title, self.message, button]];
    stack.axis = UILayoutConstraintAxisVertical;
    stack.spacing = 20;
    stack.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:stack];
    [NSLayoutConstraint activateConstraints:@[
        [stack.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
        [stack.centerYAnchor constraintEqualToAnchor:self.view.centerYAnchor],
        [stack.widthAnchor constraintLessThanOrEqualToConstant:620],
        [stack.leadingAnchor constraintGreaterThanOrEqualToAnchor:self.view.safeAreaLayoutGuide.leadingAnchor constant:24],
        [stack.trailingAnchor constraintLessThanOrEqualToAnchor:self.view.safeAreaLayoutGuide.trailingAnchor constant:-24]
    ]];
}
- (void)chooseROM
{
    UIDocumentPickerViewController* picker = [[UIDocumentPickerViewController alloc] initWithDocumentTypes:@[@"public.data"] inMode:UIDocumentPickerModeImport];
    picker.delegate = self;
    [self presentViewController:picker animated:YES completion:nil];
}
- (void)documentPicker:(UIDocumentPickerViewController*)controller didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls
{
    NSURL* source = urls.firstObject;
    if (!source) return;
    BOOL scoped = [source startAccessingSecurityScopedResource];
    NSError* error = nil;
    NSData* data = [NSData dataWithContentsOfURL:source options:0 error:&error];
    if (scoped) [source stopAccessingSecurityScopedResource];
    if (!data) {
        self.message.text = error.localizedDescription ?: @"Could not read this file. Try another location.";
        return;
    }
    if (!isCompatibleROM(data)) {
        self.message.text = @"This ROM is not compatible. Choose the unmodified 4 MB Sonic 3 & Knuckles ROM supported by Sonic 3 A.I.R.";
        return;
    }
    if (![data writeToURL:self.destination options:NSDataWritingAtomic error:&error]) {
        self.message.text = error.localizedDescription;
        return;
    }
    // Wait for the picker to finish dismissing before starting SDL's game window.
    [self dismissViewControllerAnimated:YES completion:^{ self.ready = YES; }];
}
@end

int main(int argc, char** argv)
{
    @autoreleasepool {
        NSFileManager* files = NSFileManager.defaultManager;
        NSError* error = nil;
        NSURL* documents = [files URLForDirectory:NSDocumentDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:&error];
        if (!documents) {
            SDL_Log("Cannot open Documents: %s", error.localizedDescription.UTF8String);
            return 1;
        }
        [files createDirectoryAtURL:[documents URLByAppendingPathComponent:@"mods" isDirectory:YES] withIntermediateDirectories:YES attributes:nil error:nil];
        NSURL* rom = [documents URLByAppendingPathComponent:@"Sonic_Knuckles_wSonic3.bin"];
        NSData* existing = [NSData dataWithContentsOfURL:rom];
        if (!isCompatibleROM(existing)) {
            UIWindow* setupWindow = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
            AIRImportController* setup = [AIRImportController new];
            setup.destination = rom;
            setupWindow.rootViewController = setup;
            [setupWindow makeKeyAndVisible];
            // UIKit continues servicing document-provider and lifecycle events here.
            while (!setup.ready) {
                [NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
            }
            setupWindow.hidden = YES;
            setupWindow.rootViewController = nil;
        }

        EngineMain::earlySetup();
        SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
        EngineDelegate delegate;
        GameArgumentsReader arguments;
        arguments.read(argc, argv);
        EngineMain engine(delegate, arguments);
        Configuration& config = Configuration::instance();
        config.mAppDataPath = String(documents.path.UTF8String).toStdWString() + L"/";
        PlatformFunctions::mExAppDataPath = config.mAppDataPath;
        PlatformFunctions::mOpenDirectoryHandler = openIOSDirectory;
        config.mRomPath = String(rom.path.UTF8String).toStdWString();
        config.mSaveStatesDir = config.mAppDataPath + L"savestates/";
        NSString* resources = [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"content/sonic3air"];
        if (![files changeCurrentDirectoryPath:resources]) {
            SDL_Log("Missing bundled game resources");
            return 1;
        }
        randomize();
        try { engine.execute(); }
        catch (const std::exception& e) {
            SDL_Log("Unhandled game error: %s", e.what());
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Sonic 3 A.I.R.", e.what(), nullptr);
            return 1;
        }
        return 0;
    }
}
