#include <QWidget>

#ifdef Q_OS_MACOS
#import <AppKit/AppKit.h>

void configureMacTitleBar(QWidget *window)
{
    NSView *view = reinterpret_cast<NSView *>(window->winId());
    if (!view) {
        return;
    }
    NSWindow *nsWindow = view.window;
    if (!nsWindow) {
        return;
    }
    nsWindow.titlebarAppearsTransparent = YES;
    nsWindow.titleVisibility = NSWindowTitleHidden;
    nsWindow.styleMask |= NSWindowStyleMaskFullSizeContentView;
}
#endif
