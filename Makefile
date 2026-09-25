INFORM = inform
OZMOO = /home/johan/commodore/ozmoo
PUNY = /home/johan/commodore/punyinform

SFROTZ = sfrotz
XMEGA65 = xemu-xmega65 -besure
X16 = /home/johan/commodore/ozmoo/x16-emulator46/x16emu

# make.rb anchors everything it reads to its own directory (asm/, tools/, temp/,
# exomizer) and writes the finished disk image into the CURRENT directory, so it
# can be called from here with plain relative paths -- no cd into $(OZMOO), no
# copying the image back. It also leaves the X16 build unpacked in
# x16_vanyar/ beside the zip, so there is nothing to unzip either.
OZMOOBUILD = ruby $(OZMOO)/make.rb

PICSRC   = resources/contents.yaml $(wildcard resources/*.png)
STORYSRC = vanyar.inf ext_z6map.h ../lib/ext_z6graphics.h $(wildcard $(PUNY)/lib/*.h)

all: test

z5-debug:
	$(INFORM) +$(PUNY)/lib -v5 -es -D vanyar.inf

z5-release:
	$(INFORM)  --opt OMIT_SYMBOL_TABLE=1 --define RUNTIME_ERRORS=0 +$(PUNY)/lib -v5 -es vanyar.inf

vanyar.z6: $(STORYSRC) vanyar.blb
	$(INFORM) --opt OMIT_SYMBOL_TABLE=1 --define RUNTIME_ERRORS=0  +$(PUNY)/lib -v6 -es vanyar.inf

z6: vanyar.z6

vanyar.blb: $(PICSRC)
	python $(OZMOO)/tools/make_blorb.py resources

blorb: vanyar.blb

# Vanyar has pictures but no sound effects, so there is no -asw here (Ozmoo
# stops with "No sound files found" if it is given a folder with no wavs).
x16_vanyar.zip: vanyar.blb vanyar.z6
	$(OZMOOBUILD) -t:x16 -pics vanyar.blb vanyar.z6

x16: x16_vanyar.zip
	# the emulator must run from inside the game directory
	cd x16_vanyar && $(X16) -prg VANYAR.PRG -run

mega65_vanyar.d81: vanyar.blb vanyar.z6
	$(OZMOOBUILD) -t:mega65 -fcm -pics vanyar.blb vanyar.z6

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
	# the blorb has to be named: sfrotz does not pick it up from the story name
	$(SFROTZ) vanyar.z6 vanyar.blb

release: z5-release z6 mega65_vanyar.d81 x16_vanyar.zip
	$(OZMOOBUILD) -ch vanyar.z6
	$(OZMOOBUILD) -ch -t:c128 vanyar.z6
	$(OZMOOBUILD) -ch -t:plus4 vanyar.z6
	zip release1.zip vanyar.z5 vanyar.z6 vanyar.blb mega65_vanyar.d81 x16_vanyar.zip c64_vanyar.d64 c128_vanyar.d71 plus4_vanyar.d64

clean:
	rm -rf vanyar.z5 vanyar.z6 vanyar.blb vanyar.scr vanyar.cur pics *.d64 *.d71 *.d81 x16_vanyar* *qzl
