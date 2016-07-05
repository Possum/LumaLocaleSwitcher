# LumaLocaleSwitcher

LumaLocaleSwitcher can be used to manage per-title locales for Luma3ds (and
compatible forks such as SaltFW).

**Installation**

Install the .cia file using a CIA installer such as
[FBI](https://github.com/Steveice10/FBI/releases).

**Compiling**

Requires [devkitARM](http://sourceforge.net/projects/devkitpro/files/devkitARM/)
and [citro3d](https://github.com/fincs/citro3d) to build. You also need
[ctrulib](https://github.com/smealum/ctrulib).

To build, just call `git submodule sync` (to pull in
[svchax](https://github.com/aliaspider/svchax) and
[buildtools](git://github.com/Steveice10/buildtools)) and `make` and you should
be good.

**Set up**

If you use Luma3DS you can just select "Titles" and make your changes.
Otherwise, you can choose from the list in the app or write your own custom
directory to /locales.conf

**Known Limitations**

Currently requires the parent path of the locales directory to exist (i.e., it
will not create the directory for you). If it fails to set the region for a
title, that is the most likely reason. If you are using Luma, and everything is
properly set up, this shouldn't affect you, since the "/luma" directory should
exist anyway.

There is no file chooser built in, so if you need to use a custom directory for
some reason, you will have to write the path to /locales.conf manually.

**Bugs**

Please report bugs at https://github.com/Possum/LumaLocaleSwitcher/

**Credits**

This is largely based on [FBI](https://github.com/Steveice10/FBI) by
[Steveice10](https://github.com/Steveice10).

This also would not be possible without
[AuroraWright](https://github.com/AuroraWright)'s work on the region emulation
feature in [Luma3DS](https://github.com/AuroraWright/Luma3DS/).

***Contributors***
[Possum](https://github.com/Possum) - project maintainer
CouldBeWolf - beta tester

Also thanks to the 3DS homebrew/CFW community.

**License**

This software is provided under the MIT license. Please see LICENSE.txt for full
details fo the license.
