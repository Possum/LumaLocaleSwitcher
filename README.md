# LumaLocaleSwitcher

LumaLocaleSwitcher can be used to manage per-title locales for Luma3ds (and
compatible forks such as SaltFW).

## Compiling

Requires [devkitARM](http://sourceforge.net/projects/devkitpro/files/devkitARM/)
and [citro3d](https://github.com/fincs/citro3d) to build. You also need
[ctrulib](https://github.com/smealum/ctrulib).

To build, just call `git submodule sync` (to pull in
[buildtools](git://github.com/Steveice10/buildtools)) and `make` and you should
be good.

##  Set up

Luma3DS Only: you can just select "Titles" and make your changes.


## Known Limitations

Currently requires the parent path of the locales directory to exist (i.e., it
will not create the directory for you). If it fails to set the region for a
title, that is the most likely reason. If you are using Luma, and everything is
properly set up, this shouldn't affect you, since the "/luma" directory should
exist anyway.

There is no file chooser built in, so if you need to use a custom directory for
some reason, you will have to write the path to /locales.conf manually.

Changing the region emulation is not always a magic bullet, due to the way the
3ds services are set up. In particular, online play may not function even when
the region emulation is set up properly. If you are receiving 003-0399, either
give up or use a search engine to look for workarounds.

## Bugs

Please report bugs at https://github.com/Possum/LumaLocaleSwitcher/

## Credits

This is largely based on [FBI](https://github.com/Steveice10/FBI) by
[Steveice10](https://github.com/Steveice10).

This also would not be possible without
[AuroraWright](https://github.com/AuroraWright)'s work on the region emulation
feature in [Luma3DS](https://github.com/AuroraWright/Luma3DS/).

### Contributors

* [Possum](https://github.com/Possum) - project maintainer
* CouldBeWolf - beta tester

Also thanks to the 3DS homebrew/CFW community.

## License

This software is provided under the MIT license. Please see LICENSE.txt for full
details fo the license.
