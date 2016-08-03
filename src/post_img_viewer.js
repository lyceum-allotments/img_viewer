(function(){
    ImgViewer = function(image, canvas_element) {
        // Uint8Array of bytes of image will be stored here
        // previously loaded images are cached under the 
        // src of the image
        var image_bytes_cache = {};

        // Emscripten exported function for loading an image into
        // the viewer will be stored here
        var loadViewerImage = 0;

        var image_src = "";

        var loadImage = function(image) {
            // image argument can be a src path
            if (typeof image == "string") {
                image_src = image;

                // if image has been cached load image into viewer
                if (image_bytes_cache.hasOwnProperty(image_src)) {
                    if(loadViewerImage)
                        loadViewerImage(image_bytes_cache[image_src], image_bytes_cache[image_src].length);
                    return;
                }

                // if it hasn't load the image
                var im = new Image();

                im.src = image;
                im.onload = function() {
                    loadImageBytes(this);

                    // on load, load image into viewer
                    if(loadViewerImage)
                        loadViewerImage(image_bytes_cache[image_src], image_bytes_cache[image_src].length);
                };
                return;
            }

            // image argument is an HTMLImageElement
            image_src = image.src;

            // if image hasn't been cached load the image bytes
            if (!image_bytes_cache.hasOwnProperty(image_src)) {
                loadImageBytes(image);
            }

            // load the image into the viewer
            if(loadViewerImage)
                loadViewerImage(image_bytes_cache[image_src], image_bytes_cache[image_src].length);
        };

        var loadImageBytes = function(image) {
            // create a temporary canvas element for this task
            var canvas = document.createElement('canvas');

            // set it to have same size of underlying image
            canvas.width = image.naturalWidth;
            canvas.height = image.naturalHeight;

            // load the image in the canvas
            canvas.getContext('2d').drawImage(image, 0, 0);

            // convert the image to a data URI.
            // A data URI has a prefix describing the type of data the string 
            // following contains, we don't need this so just replace it with nothing
            var string_data = canvas.toDataURL('image/png').replace(/^data:image\/(png|jpg);base64,/, '');

            // change the data URI from a base64 to an ASCII encoded string
            var decoded_data = atob(string_data);

            // make an unsigned 8 bit int typed array large enough to hold image data
            var array_data = new Uint8Array(decoded_data.length);

            // convert ASCII string to unsigned 8 bit int array
            for (var i = 0; i < decoded_data.length; i++)
            {
                array_data[i] = decoded_data.charCodeAt(i);
            }

            image_bytes_cache[image_src] = array_data;
        };

        var options = {
          preRun: [],
          postRun: [],
          print: function(text){console.log(text)},
          printErr: function(text) {console.error(text)},
          canvas: (function() {
              return canvas_element;
          })(),
          onRuntimeInitialized: function() {
              this.ccall('setup_context', null, ['number', 'number'], [canvas.width, canvas.height]);
              loadViewerImage = this.cwrap('load_image', null, ['array', 'number']);
              if (image_bytes_cache.hasOwnProperty(image_src)) // if an image has already been loaded by the time
                               // this call back is called, load the image bytes
                               // into the viewer
              {
                loadViewerImage(image_bytes_cache[image_src], image_bytes_cache[image_src].length);
              }
          },
        };

        // load the initial image
        loadImage(image);

        // create an image viewer with the options
        var viewer_module = EmImgViewer(options);

        return {
            'changeImage' : function(image) { // method to change viewer's image
                                              // to the HTTPImageElement `image`
                loadImage(image);
            },
            'fullScreen'  : function() {
                // viewer_module.requestFullScreen(false, true);
                viewer_module.requestFullScreen(false, true);
            }
        };
    };
})();
