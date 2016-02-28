DEBUG     =
DEATHGAME = /Users/roman/Library/Developer/Xcode/DerivedData/The_Game_of_Death-awnvmnldwvvzflbsyyosswenwoxi/Build/Products/Debug/The\ Game\ of\ Death
DEATHAC   = /Users/roman/Library/Developer/Xcode/DerivedData/The_Game_of_Death-awnvmnldwvvzflbsyyosswenwoxi/Build/Products/Debug/deathac

all: green.dasm red.dasm yellow.dasm blue.dasm run

%.dasm: %.dac
	$(DEATHAC) $< $@

run:
	$(DEBUG) $(DEATHGAME) green red yellow blue

clean:
	rm -f count *.dasm
