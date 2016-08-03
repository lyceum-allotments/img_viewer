# img_viewer
JavaScript zoomable image viewer.

Compilation and Installation Instructions
-----------------------------------------

1. Run make (requires Emscripten to be installed on the system)
2. Copy files in `js` directory to where you wish to serve them
3. Include `img_viewer.js` as script element in your HTML page

Usage Instructions
------------------

* `ImgViewer(img, canvas)` turns the HTML canvas element `canvas` into a
   zoomable image viewer showing the image `img`. `img` can be a path to an
   image to be shown, or a HTML image element

* `viewer.change_image(img)` changes the image shown by the `ImgViewer`
   object `viewer` into `img`, where `img` can be a path to an image, or a
   HTML image element

* `viewer.fullScreen()` makes the `ImgViewer`, `viewer`, full screen

An example of usage is in eg/newsham_park
