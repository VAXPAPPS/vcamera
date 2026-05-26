#include "take_photo.h"
#include <gio/gio.h>

void take_photo_execute(GdkTexture *texture) {
    if (!texture) return;
    
    // Save image to User's Pictures directory
    const char *pictures_dir = g_get_user_special_dir(G_USER_DIRECTORY_PICTURES);
    if (!pictures_dir) pictures_dir = g_get_home_dir();
    
    // Generate a unique filename using timestamp
    g_autofree char *filename = g_strdup_printf("%s/vcamera_img_%ld.png", pictures_dir, g_get_real_time());
    
    // Save the texture to file
    gdk_texture_save_to_png(texture, filename);
    g_print("تم حفظ الصورة بنجاح في: %s\n", filename);
}
