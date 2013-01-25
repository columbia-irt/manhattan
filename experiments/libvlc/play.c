 #include <stdio.h>
 #include <stdlib.h>
 #include <vlc/vlc.h>
 
static void raise(libvlc_exception_t * ex)
{
    if (libvlc_exception_raised (ex))
    {
         fprintf (stderr, "error: %s\n", libvlc_exception_get_message(ex));
         exit (-1);
    }
}
int main(int argc, char* argv[])
{
    const char * const vlc_args[] = {
              "-I", "dummy", /* Don't use any interface */
              "--ignore-config", /* Don't use VLC's config */
              "--plugin-path=/set/your/path/to/libvlc/module/if/you/are/on/windows/or/macosx" };
    libvlc_exception_t ex;
    libvlc_instance_t * inst;
    libvlc_media_player_t *mp;
    libvlc_media_t *m;
    
    libvlc_exception_init (&ex);
    /* init vlc modules, should be done only once */
    inst = libvlc_new (sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args, &ex);
    raise (&ex);
 
    /* Create a new item */
    m = libvlc_media_new (inst, "http://mycool.movie.com/test.mov", &ex);
    raise (&ex);
   
    /* XXX: demo art and meta information fetching */
   
    /* Create a media player playing environement */
    mp = libvlc_media_player_new_from_media (m, &ex);
    raise (&ex);
    
    /* No need to keep the media now */
    libvlc_media_release (m);

#if 0
    /* This is a non working code that show how to hooks into a window,
     * if we have a window around */
     libvlc_drawable_t drawable = xdrawable;
    /* or on windows */
     libvlc_drawable_t drawable = hwnd;

     libvlc_media_player_set_drawable (mp, drawable, &ex);
     raise (&ex);
#endif

    /* play the media_player */
    libvlc_media_player_play (mp, &ex);
    raise (&ex);
   
    sleep (10); /* Let it play a bit */
   
    /* Stop playing */
#warning There is known deadlock bug here. Please update to LibVLC 1.1!
    libvlc_media_player_stop (mp, &ex);

    /* Free the media_player */
    libvlc_media_player_release (mp);

    libvlc_release (inst);
    raise (&ex);

    return 0;
}
