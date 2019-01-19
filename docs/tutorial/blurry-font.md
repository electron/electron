# Fixing Blurry Fonts

If fonts look blurry for you, set the window background to a non transparent color.
Do this in the constructor for [BrowserWindow][browser-window]:

    new BrowserWindow({width: 1000, height: 750, backgroundColor: '#fff'});
    //                                           ^ here                ^

The reason for why the font may look blurry is that [sub pixel anti-aliasing](http://alienryderflex.com/sub_pixel/)
will be deactivated when the background is transparent. See [this issue](https://github.com/electron/electron/issues/6344#issuecomment-420371918).

Screenshot of the result:

![subpixel rendering example](https://i.imgur.com/CjNQaeR.gif)

[browser-window]: ../api/browser-window.md
