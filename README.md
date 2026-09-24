# A PunyInform game with optional graphics in z6 mode

When an ancient diamond falls into the hands of a rebellious lord and his dragon, a kingdom's fate rests on your blade and an elven sage's quiet magic. Experience a authentic retro journey; a resurrected 1980s BASIC text adventure complete with nostalgia-fueled PETSCII graphics.   

![Vanyar picture mode](https://github.com/johanberntsson/Vanyar/blob/main/screenshots/vanyar-firstroom.png?raw=true) 
![Vanyar map mode](https://github.com/johanberntsson/Vanyar/blob/main/screenshots/vanyar-map.png?raw=true) 

## I just want to play the game!

Download precompiled binaries on my itch page: [https://johanberntsson.itch.io/vanyar](https://johanberntsson.itch.io/vanyar)

## Building the text only version (z5)

Wyrmwatd is a PunyInform game and requires the PunyInform library, and the Inform 6 compiler.

### Get PunyInform

Go to [https://github.com/johanberntsson/PunyInform/releases](https://github.com/johanberntsson/PunyInform/releases) and download the newest version of PunyInform (the version that comes first on the page) by clicking on “Source code (zip)” under Assets. Extract the contents in a suitable location. You can always rename and/or move the folder later.

### Get the Inform 6 compiler

PunyInform is a library for the Inform 6 programming language. To use it, you will need to download the compiler, called “inform” or “inform6”. PunyInform requires at least version 6.44 of the compiler, but you typically want the newest version available at [http://www.ifarchive.org/indexes/if-archiveXinfocomXcompilersXinform6Xexecutables.html](http://www.ifarchive.org/indexes/if-archiveXinfocomXcompilersXinform6Xexecutables.html).

### Compile Vanyar

Either use the attached Makefile or issue the build command manually. You need to update the INFORM and PUNY definitions in the Makefile and then type "make release" to build the vanyar.z5 file.

You can also build it manually

    > inform -v5 +<path to punyinform>/lib vanyar.inf

To test the new file you can use frotz with "frotz vanyar.z5" or use Windows Frotz.

## Building the graphical version (z6)

You will need Ozmoo to create the asset file for graphics, in addition to the Inform compiler and PunyInform library that we installed for the z5 version. To get Ozmoo, either clone its github archive on [https://github.com/johanberntsson/ozmoo](https://github.com/johanberntsson/ozmoo), or download the assets from latest release.

To use Ozmoo some additional software is needed; the ACME cross assembler, the Exomizer file compression program, Ruby and Python. For more instructions, refer to Ozmoo's README and manual.

Once Ozmoo and its requirements are installed, then for Linux, update the OZMOO variable in Makefile, and type "make z6". This will create vanyar.z6 and vanyar.blb, and can be run with for example sfrotz with "sfrotz vanyar.z6 vanyar.blb"

It's also possible to create these files manually

    > inform -v6 +<path to punyinform>/lib vanyar.inf
    > python <path to ozmoo>/tools/make_blorb.py resources

## Building MEGA65 and X16 versions with Ozmoo

Since we already have Ozmoo installed we can create graphical versions for MEGA65 and X16. There are already targets for this: "make mega65" and "make x16". If the MEGA65 and X16 emulators are installed, then the game will be launched automatically.

## How to change or add resources (graphics)

The resources folder contains graphics (png files) and a index file called contents.yaml. The make_blorb.py script will read the configuration file and create a blorb file as defined by the contents. The png files are just normal png files, but make_blorb.py will convert these to indexed png files with a 16 colour palette, adjusting colours if needed. Because of this, the resulting picture may look slightly different from the original files. In addition, they will be scaled to fit the 320x200 standard size of the Ozmoo z6 screen model, and may be cropped further by adding optional max height and width parameters.

The config file consists of these fields: blorb, outdir, pictures[]. Each picture needs two fields; id, file (filename), and can also have these optional fields; name, height, width, location.

Add assets as needed, update contents.yaml, and run make_blorb.py to create a blorb with new assets.

## Licensing

- **Source Code:** Distributed under the [MIT License](LICENSE).
- **Game Assets (Graphics, Music, Story Text):** Distributed under [Creative Commons Attribution 4.0 International (CC BY 4.0)](https://creativecommons.org/licenses/by/4.0/).
