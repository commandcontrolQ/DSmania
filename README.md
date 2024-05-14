## DSmania - Stepmania/DDR clone for the Nintendo DS (originally created by [qgerman2](https://github.com/qgerman2))

This project aims to recreate a (Stepmania/DDR)-based rhythm game with **ideally no compromises**: this means no need for specific sound file formats or specific image dimensions.

In theory, every song should work just like the other games that support them (by just copying them over), the only limitations being video backgrounds or lua scripts.

## How to build
You should compile this project with the latest devkitPro release (see https://devkitpro.org/wiki/Getting_Started).

See the third-party directory for building the required third-party libraries that devkitPro does not bundle by default:
* libmad : MPEG Audio Decoder
* libogg and libremor: Integer-only Ogg vorbis decoder
* libjpeg-turbo : jpeg image decoder
* zlib : data compression, dependency for libpng
* libpng : png image decoder


Once everything is set up, running `make` should generate a .nds file.

## Licensing

According to qgerman2:

> i haven't bothered with figuring out which license to use, i realize that the game currently uses some assets ripped from other games and the fact that the third-party libraries should be compatible with whatever license i choose puts some constraints in my options.
>
> if possible the code for dds should be considered public domain or WTFPL.

For now, I have made the decision to license this project under the CC BY-NC 4.0 license. In short,
* Give credit to the original creator, qgerman2 (and me but you dont have to)
* You can't make any profit from this

Originally I _was_ going to use WTFPL, however allowing people to profit from a game which uses ripped assets could be problematic.
