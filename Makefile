CC = emcc
all: src/img_viewer.c src/post_img_viewer.js
	$(CC) src/img_viewer.c -O2 -s USE_SDL=2  -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]' \
	 -s EXPORTED_FUNCTIONS="['_setup_context', '_load_image']" \
	 -s MODULARIZE=1 -s EXPORT_NAME="'EmImgViewer'" -s TOTAL_MEMORY=40000000 -o js/img_viewer.js 
	cat src/post_img_viewer.js >> js/img_viewer.js

clean: 
	rm -r js/*
