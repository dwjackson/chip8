all:
	cd src && $(MAKE)

clean:
	cd src && $(MAKE) clean

.PHONY: all clean
