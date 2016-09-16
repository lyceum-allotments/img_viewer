#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <emscripten.h>
#include <html5.h>

// various states that the application can be in
enum input_state
{
    STATE_NOTHING = 0,
    STATE_ZOOM_OUT = 1,
    STATE_ZOOM_IN = 1<<1,
    STATE_PAN = 1<<2,
};

// constants
static const float MIN_ZOOM_LEVEL = 0.2;
static const float MAX_ZOOM_LEVEL = 1.4;
static const float ZOOM_INCREMENT = 1.1;

struct context
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    enum input_state active_state;

    SDL_Rect dest;
    SDL_Surface *image;
    SDL_Texture *img_tex;

    // mouse coords last frame
    Sint32 x_mouse_last;
    Sint32 y_mouse_last;

    // mouse coords this frame
    Sint32 x_mouse_this;
    Sint32 y_mouse_this;

    int non_fs_canvas_w;
    int non_fs_canvas_h;

    int canvas_w;
    int canvas_h;

    // original dimensions of the image
    int orig_w;
    int orig_h;

    // scaling factor
    float zoom_level;
};
// context has global scope, it persists between each call
// from JavaScript to 'load_image'
struct context ctx;

void process_input()
{
    SDL_Event event;

    ctx.active_state &= ~STATE_ZOOM_IN;
    ctx.active_state &= ~STATE_ZOOM_OUT;
    ctx.x_mouse_last = ctx.x_mouse_this;
    ctx.y_mouse_last = ctx.y_mouse_this;

    while (SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_MOUSEMOTION:
                ctx.x_mouse_this = event.motion.x;
                ctx.y_mouse_this = event.motion.y;
                break;
            case SDL_FINGERMOTION:
                ctx.x_mouse_this = event.tfinger.x * ctx.canvas_w;
                ctx.y_mouse_this = event.tfinger.y * ctx.canvas_h;
                break;
            case SDL_MOUSEWHEEL:
                if (event.wheel.y > 0)
                    ctx.active_state |= STATE_ZOOM_IN;
                if (event.wheel.y < 0)
                    ctx.active_state |= STATE_ZOOM_OUT;
                break;
            case SDL_MULTIGESTURE:
                if (fabs(event.mgesture.dDist) > 0.002)
                {
                    if (event.mgesture.dDist > 0)
                        ctx.active_state |= STATE_ZOOM_IN;
                    else if (event.mgesture.dDist < 0)
                        ctx.active_state |= STATE_ZOOM_OUT;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                ctx.active_state |= STATE_PAN;
                break;
            case SDL_FINGERDOWN:
                ctx.active_state |= STATE_PAN;
                ctx.x_mouse_last = ctx.x_mouse_this = event.tfinger.x * ctx.canvas_w;
                ctx.y_mouse_last = ctx.y_mouse_this = event.tfinger.y * ctx.canvas_h;
                break;
            case SDL_MOUSEBUTTONUP:
            case SDL_FINGERUP:
                ctx.active_state &= ~STATE_PAN;
                break;
        }
    }
}

// if the image is larger than the canvas, stop you from moving
// the edge of the image over the edge of the canvas
// r is the old position (x or y coordinate), new_r the position 
// the image is trying to move to, canvas_dim and tex_dim the 
// dimensions of the canvas and texture respectively in the same 
// coordinate as 'r'
void clamp_to_canvas_edge(int * r, int new_r, int canvas_dim, int tex_dim)
{
    if (tex_dim < canvas_dim)
        return;

    if (new_r > 0)
    {
        *r = 0;
    }
    else if (new_r < canvas_dim - tex_dim)
    {
        *r = canvas_dim - tex_dim;
    }
    else
    {
        *r = new_r;
    }
}

void loop_handler()
{
    process_input();

    float old_zoom_level = ctx.zoom_level;
    // handle the zooming
    if ((ctx.active_state & STATE_ZOOM_IN))
    {
        float new_zoom_level = ctx.zoom_level * ZOOM_INCREMENT;
        if (new_zoom_level < MAX_ZOOM_LEVEL)
        {
            ctx.zoom_level = new_zoom_level;
        }
        ctx.dest.w = (float)ctx.orig_w * ctx.zoom_level;
        ctx.dest.h = (float)ctx.orig_h * ctx.zoom_level;
    }
    if ((ctx.active_state & STATE_ZOOM_OUT))
    {
        float new_zoom_level = ctx.zoom_level / ZOOM_INCREMENT;
        if (new_zoom_level > MIN_ZOOM_LEVEL)
        {
            ctx.zoom_level = new_zoom_level;
        }
        ctx.zoom_level = (ctx.zoom_level < 0 ? 0 : ctx.zoom_level);
        ctx.dest.w = (float)ctx.orig_w * ctx.zoom_level;
        ctx.dest.h = (float)ctx.orig_h * ctx.zoom_level;

    }

    if ((ctx.active_state & (STATE_ZOOM_OUT | STATE_ZOOM_IN)))
    {
        // if the image is smaller than the canvas, just centre it
        if (ctx.dest.w < ctx.canvas_w)
        {
            ctx.dest.x = (ctx.canvas_w - ctx.dest.w)/2.;
        }
        else
        {
            // if it isn't, make sure it's side is clamped to the canvas edge
            // rather than being allowed to move over
            int new_x = ctx.dest.x - (ctx.x_mouse_this - ctx.dest.x) * (ctx.zoom_level/old_zoom_level - 1);
            clamp_to_canvas_edge(&ctx.dest.x, new_x, ctx.canvas_w, ctx.dest.w);
        }

       if (ctx.dest.h < ctx.canvas_h)
       {
           ctx.dest.y = (ctx.canvas_h - ctx.dest.h)/2.;
       }
       else
       {
         int new_y = ctx.dest.y - (ctx.y_mouse_this - ctx.dest.y) * (ctx.zoom_level/old_zoom_level - 1);
         clamp_to_canvas_edge(&ctx.dest.y, new_y, ctx.canvas_h, ctx.dest.h);
       }
    }

    if ((ctx.active_state & STATE_PAN))
    {
        // move the canvas in the direction that the mouse 
        // cursor is moving
        int new_x = ctx.dest.x + ctx.x_mouse_this - ctx.x_mouse_last;
        int new_y = ctx.dest.y + ctx.y_mouse_this - ctx.y_mouse_last;

        clamp_to_canvas_edge(&ctx.dest.x, new_x, ctx.canvas_w, ctx.dest.w);
        clamp_to_canvas_edge(&ctx.dest.y, new_y, ctx.canvas_h, ctx.dest.h);
    }

    SDL_RenderClear(ctx.renderer);
    SDL_RenderCopy(ctx.renderer, ctx.img_tex, NULL, &ctx.dest);
    SDL_RenderPresent(ctx.renderer);
}

void scale_to_canvas_size()
{
    float scale_x = (float)ctx.canvas_w / (float)ctx.dest.w;
    float scale_y = (float)ctx.canvas_h / (float)ctx.dest.h;

    if (scale_x < scale_y)
    {
        ctx.zoom_level = scale_x;
    }
    else
    {
        ctx.zoom_level = scale_y;
    }

    ctx.dest.w = (float)ctx.orig_w * ctx.zoom_level;
    ctx.dest.h = (float)ctx.orig_h * ctx.zoom_level;

    ctx.dest.x = (ctx.canvas_w - ctx.dest.w)/2.;
    ctx.dest.y = (ctx.canvas_h - ctx.dest.h)/2.;
}

EM_BOOL make_fullscreen(int eT, const EmscriptenFullscreenChangeEvent *change_event, void *ud);
void create_new_canvas(int w, int h)
{
    ctx.canvas_w = w;
    ctx.canvas_h = h;

    if (ctx.renderer)
        SDL_DestroyRenderer(ctx.renderer);
    if (ctx.img_tex)
        SDL_DestroyTexture(ctx.img_tex);

    SDL_CreateWindowAndRenderer(ctx.canvas_w, ctx.canvas_h, 0, &ctx.window, &ctx.renderer);
    SDL_SetRenderDrawColor(ctx.renderer, 255, 255, 255, 255);

    if (emscripten_set_fullscreenchange_callback(0, NULL, 1, make_fullscreen) != EMSCRIPTEN_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to set full-screen change callback\n");
    }
    if (ctx.img_tex)
    {
        ctx.img_tex = SDL_CreateTextureFromSurface(ctx.renderer, ctx.image);
        ctx.dest.w = ctx.image->w;
        ctx.dest.h = ctx.image->h;
        scale_to_canvas_size();
    }
}

EM_BOOL make_fullscreen(int eT, const EmscriptenFullscreenChangeEvent *change_event, void *ud)
{
    if (change_event->isFullscreen)
    {
        emscripten_cancel_main_loop();
        create_new_canvas(change_event->screenWidth, change_event->screenHeight);
        emscripten_set_main_loop(loop_handler, -1, 0);
    }
    else
    {
        emscripten_cancel_main_loop();
        create_new_canvas(ctx.non_fs_canvas_w, ctx.non_fs_canvas_h);
        emscripten_set_main_loop(loop_handler, -1, 0);
    }
    return EM_TRUE;
}

// to be called when Emscripten runtime is initialised,
// sets up the context
void setup_context(int w, int h)
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    ctx.active_state = STATE_NOTHING;
    ctx.zoom_level = 1;

    ctx.window = NULL;
    ctx.renderer = NULL;
    ctx.img_tex = NULL;
    ctx.non_fs_canvas_w = w;
    ctx.non_fs_canvas_h = h;
    create_new_canvas(w, h);

    if (emscripten_set_fullscreenchange_callback(0, NULL, 1, make_fullscreen) != EMSCRIPTEN_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to set full-screen change callback\n");
    }
    emscripten_set_main_loop(loop_handler, -1, 0);
}

// loads the image data pointed to by 'image_data' (of size 'size' bytes)
void load_image(void *image_data, int size)
{
    // if there is an existing texture we need to free it's memory
    if (ctx.img_tex)
    {
        SDL_DestroyTexture(ctx.img_tex);
        SDL_FreeSurface(ctx.image);
    }

    SDL_RWops *a = SDL_RWFromConstMem(image_data, size);
    ctx.image = IMG_Load_RW(a, 1);

    ctx.img_tex = SDL_CreateTextureFromSurface(ctx.renderer, ctx.image);
    ctx.dest.w = ctx.image->w;
    ctx.dest.h = ctx.image->h;
    ctx.orig_w = ctx.image->w;
    ctx.orig_h = ctx.image->h;
    scale_to_canvas_size();
}
