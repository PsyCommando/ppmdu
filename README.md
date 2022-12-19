# Moved to a New Repository!
Since I basically can't build the project on recent MSVC, I decided to port the build system over to cmake instead. The current build system is a frankeinstein mess of hacks and failed experiments to circumvent various ancient bugs with older versions of Visual Studio. And now I can't be assed anymore to try to maintain that mess. So I'm going to archive this repository and any new development will happen on the new repository: https://github.com/PsyCommando/ppmdu_2

[![forthebadge](http://forthebadge.com/images/badges/60-percent-of-the-time-works-every-time.svg)](http://forthebadge.com)
[![forthebadge](http://forthebadge.com/images/badges/compatibility-club-penguin.svg)](http://forthebadge.com)
[![forthebadge](http://forthebadge.com/images/badges/built-with-science.svg)](http://forthebadge.com)
[![forthebadge](http://forthebadge.com/images/badges/built-with-love.svg)](http://forthebadge.com)

# Pokemon Mystery Dungeon Utilities
Combined code for several utilities used to export and import things to and from the "Pokemon Mystery Dungeon : Explorers of" games on the NDS. 

I used this as a personal test ground for a lot of little things, so the code isn't very well organized yet, and you'll probably spot a lot of weirdness and incoherences.
However I try as much as possible to avoid dangerous practices, macros and etc. And I've been trying to use as much as possible the subset of C++11 that Visual Studio offers (Which is ridiculously limited and buggy)
So you'll see very little dynamic memory allocation in here, and mainly RAII managed pointers / templates when possible.

With that said, there is a lot of room for improvement, and I'll eventually try to fix most of those issues.(I wouldn't mind some help from time to time) 
And keep in mind that, math isn't natural to me, so I often overlook easy ways to calculate things.(I wouldn't mind help with that either..) And also, I'm constantly looking to improve my coding, considering I've been taught C++03 first, and I'm on my own learning C++11.. 

Oh, and the revision number system I'm using is complete BS. I was going to go with the one suggested by github, but I couldn't find a way to figure if a release was a breaking change considering the codebase is pretty "immature" and disorganized as I haven't yet completely figured out the game's inner working, and things changes as I discover new details..

Here's my development thread on Project Pokemon:  
http://projectpokemon.org/forums/showthread.php?40199-Pokemon-Mystery-Dungeon-2-Psy_commando-s-Tools-and-research-notes

And here's the wiki for the file formats in PMD2:  
http://projectpokemon.org/wiki/Pokemon_Mystery_Dungeon_Explorers_of_Sky

## Utilities Info:

* **AudioUtil:** *WIP*   
  A tool for exporting and importing all the various audio formats used by Pokemon Mystery Dungeon.
  It will most likely work with any other games that uses the DSE sound driver, by Procyon Studios.
  
  Most of the conversion process happens in the files with the prefix "dse_", or with the names "smdl", "swdl", and "sedl".
  
* **DoPX:**  
  A utility to compress files into "PX" compressed containers such as "at4px" and "pkdpx". (The actual name of the compression format is unknown. But it looks like a variant of LZ.)
  
* **UnPX:**  
  A utility to decompress "at4px", "pkdpx", and their SIR0 wrapped form.
  
* **GFXCrunch:** *WIP*  
  A utility meant to support import/export of every single graphic formats used in PMD2. 
  
* **PaletteTool:**  
  This utility is meant to be used alongside GFXUtil. It allows to extract, inject, or shift the color palette of a PNG/BMP file. Its ideally meant to be used for preparing sprites to be imported into the game with GFXUtil.
  
* **KaoUtil:**  
  A tool meant for exporting/importing pokemon portraits. It will eventually be replaced by GFX Crunch.
  
* **PackFileUtil:**  
  A tool meant mostly for research. It extracts a "pack file"'s content, or rebuild one. The functionality is automated in other tools, so its mostly unnecessary now!
  
* **StatsUtil:** *WIP*  
  This tool exports/imports game statistics files. Things such as game text, pokemon and item names, pokemon stats, stats growth, item and move data, dungeon stats, etc..
  It exports/imports all that data(besides game strings) to XML files as an intermediate format. XML files are easily editable by anyone, and 3rd party tools supporting XML.
  The game strings are extracted to a single simple .txt file, where each line is a C-String, and special characters are represented as escape sequences. The encoding is properly determined by using a XML configuration file that users can use to add the encoding string for a specific game version/language.

## Future:
  Eventually more tools will come. 
  Even possibly full-fledged editors with a user interface! Ideally, I'd let 3rd parties make their own editors that uses my tools, as it lets me focus on research, while they can focus on making a good GUI. But we'll see.
  
## Portability:
  This was mainly made on several version of Visual Studio. So while I tried to keep everything portable, and use portable 
  libraries, there might be some issues from time to time compiling on GCC or using Clang+LLVM. 
  
### Mingw
  If you're using Mingw on windows, I suggest you use MSys for building the dependencies. Especially since POCO for some reasons
  won't compile on Mingw without some messing around. Unless if ran in MSys using ./configure and make. 
  The line I used to configure it is (Feel free to remove --static or --shared or add --no-tests and play around with it): 
  ./configure --prefix=INSERT_PATH_TO_LIB_RELEASE_HERE --no-samples --no-tests --static --shared --omit=Crypto,NetSSL_OpenSSL,Zip,Data,Data/SQLite,Data/ODBC,Data/MySQL,MongoDB,PDF,CppParser,PageCompiler,Net
  
  Then I just ran "make -j 8"(-j 8 is for 8 core machines) and "make install". The --minimal switch makes the whole thing only compile the 
  
## License:
  Those tools and their source code, excluding the content of the /lib/ directory (present only for convenience), is [Creative Common 0](https://creativecommons.org/publicdomain/zero/1.0/), AKA Public Domain. 
  Do what you want with it. Use the code in your coding horror museum, copy-paste it in your pmd2 tools, anything.. XD 
  You don't have to credit me, but its always appreciated if you do ! And I'd love to see what people will do with this code.
  
  However, right now, the libraries within the "lib" folders each have their own licenses, all which allow redistribution. 
  I eventually intend to link those library dynamically once/before we reach 1.0, once I can figure out a few major issues(Issues that would probably be solved if I'd distribute the thing as a DLL/SO itslelf.. Or if I'd make an editor..), to respect their licenses better (The viral GPL/LGPL clause is problematic, as it requires code that it statically links to, to be GPL/LGPL as well. Even though my CC0 license is immensly more permissive/practical. That's copyrights in a nutshell.. ) even though I give away my sources freely and met most of the LGPL requirements
  
  As required by some of the licenses of the libraries in here, their respective licenses are in their folders, and for those that required it, in the LICENSE file in the root of the repository.
