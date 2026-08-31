INFORM = inform
OZMOO = /home/johan/commodore/ozmoo-z6
PUNY = /home/johan/commodore/punyinform

XMEGA65 = xemu-xmega65 -besure
X16 = /home/johan/commodore/ozmoo/x16-emulator46/x16emu
# The lines above are the ones in use. SDL_AUDIODRIVER=pulseaudio used to be
# mandatory -- SDL did not get on with pipewire on Fedora 44/KDE and both
# emulators came up silent with no error at all -- and a Fedora update fixed it
# in August 2026. Swap in the lines below if an emulator is ever silent again:
# that is still the first thing to try, since the failure has no diagnostic.
#XMEGA65 = SDL_AUDIODRIVER=pulseaudio xemu-xmega65
#X16 = SDL_AUDIODRIVER=pulseaudio /home/johan/commodore/ozmoo/x16-emulator46/x16emu

# --xscale 2 --yscale 2 is not optional here: sfrotz's screen is
# always 640x400 and it draws pictures at 1:1 unless it detects
# that the game is one of ARTHUR/JOURNEY/SHOGUN/ZORK_ZERO
SFROTZ = sfrotz --xscale 2 --yscale 2 

# make.rb anchors everything it reads to its own directory (asm/, tools/, temp/,
# exomizer) and writes the finished disk image into the CURRENT directory, so it
# can be called from here with plain relative paths -- no cd into $(OZMOO), no
# copying the image back. It also leaves the X16 build unpacked in
# x16_vanyar/ beside the zip, so there is nothing to unzip either.
OZMOOBUILD = ruby $(OZMOO)/make.rb

PICSRC   = resources/contents.yaml $(wildcard resources/*.png)
STORYSRC = vanyar.inf ../lib/ext_z6graphics.h $(wildcard $(PUNY)/lib/*.h)

all: test

z5-debug:
	$(INFORM) +$(PUNY)/lib -v5 -es -D vanyar.inf

z5-release:
	$(INFORM) +$(PUNY)/lib -v5 -es vanyar.inf

vanyar.z6: $(STORYSRC) vanyar.blb
	$(INFORM) +$(PUNY)/lib -v6 -es vanyar.inf

z6: vanyar.z6

vanyar.blb: $(PICSRC)
	python $(OZMOO)/tools/make_blorb.py resources

blorb: vanyar.blb

x16_vanyar.zip: vanyar.blb vanyar.z6
	$(OZMOOBUILD) -t:x16 -asw resources -pics vanyar.blb vanyar.z6

x16: x16_vanyar.zip
	# the emulator must run from inside the game directory
	cd x16_vanyar && $(X16) -prg WYRMWARD.PRG -run

mega65_vanyar.d81: vanyar.blb vanyar.z6 $(WAVS)
	$(OZMOOBUILD) -t:mega65 -asw resources -fcm -pics vanyar.blb vanyar.z6

mega65: mega65_vanyar.d81
	$(XMEGA65) -8 mega65_vanyar.d81

c64: z6
	$(OZMOOBUILD) -s vanyar.z6

.PHONY: all z5-release z5-debug blorb z6 x16 mega65 test frotz sfrotz release clean

test: z5-debug
	rm -f vanyar.scr vanyar.qzl vanyar.cur
	frotz vanyar.z5 < vanyar.cmd
	grep -v "Serial number" vanyar.scr > vanyar.cur
	rm -rf vanyar.scr
	meld vanyar.cur vanyar.txt

z6test:
	$(INFORM) -v6 --define Z6_TESTPROGRAM  $(PUNY)/lib/ext_z6graphics.h
	sfrotz ext_z6graphics.z6

frotz: z5-debug
	frotz -d vanyar.z5

sfrotz: vanyar.z6 vanyar.blb
	$(SFROTZ) vanyar.z6

release: z5-release
	frotz -d vanyar.z5

clean:
	rm -rf vanyar.z5 vanyar.z6 vanyar.blb vanyar.scr vanyar.cur pics *.d81 x16_vanyar* *qzl  sounds/*small.wav
