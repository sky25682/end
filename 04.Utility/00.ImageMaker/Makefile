all: pyImageMaker clean
ImageMaker: ImageMaker.c
	gcc -o $@ $<
	cp $@ ../../
pyImageMaker:
	@echo ==================== pyImageMaker Build ====================
	cp ImageMaker.py ImageMaker
	chmod +x ./ImageMaker
	mv ./ImageMaker ../../



clean:
	rm -f ImageMaker