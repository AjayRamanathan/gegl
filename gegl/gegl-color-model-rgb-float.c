#include "gegl-color-model-rgb-float.h"
#include "gegl-color-model-rgb.h"
#include "gegl-color-model.h"
#include "gegl-object.h"
#include "gegl-color.h"
#include <string.h>

static void class_init (GeglColorModelRgbFloatClass * klass);
static void init (GeglColorModelRgbFloat * self, GeglColorModelRgbFloatClass * klass);
static GObject* constructor (GType type, guint n_props, GObjectConstructParam *props);
static GeglColorModelType color_model_type (GeglColorModel * self_color_model);
static void set_color (GeglColorModel * self_color_model, GeglColor * color, GeglColorConstant constant);
static char * get_convert_interface_name (GeglColorModel * self_color_model);
static void convert_to_xyz (GeglColorModel * self_color_model, gfloat ** xyz_data, guchar ** data, gint width);
static void convert_from_xyz (GeglColorModel * self_color_model, guchar ** data, gfloat ** xyz_data, gint width);
static void from_rgb_u8 (GeglColorModel * self_color_model, GeglColorModel * src_color_model, guchar ** data, guchar ** src_data, gint width);
static void from_gray_float (GeglColorModel * self_color_model, GeglColorModel * src_color_model, guchar ** data, guchar ** src_data, gint width);
static void from_rgb_float (GeglColorModel * self_color_model, GeglColorModel * src_color_model, guchar ** data, guchar ** src_data, gint width);

static gpointer parent_class = NULL;

GType
gegl_color_model_rgb_float_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglColorModelRgbFloatClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglColorModelRgbFloat),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_COLOR_MODEL_RGB, "GeglColorModelRgbFloat", &typeInfo, 0);
    }
    return type;
}

static void 
class_init (GeglColorModelRgbFloatClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GeglColorModelClass *color_model_class = GEGL_COLOR_MODEL_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->constructor = constructor; 

  color_model_class->color_model_type = color_model_type;
  color_model_class->set_color = set_color;
  color_model_class->get_convert_interface_name = get_convert_interface_name;
  color_model_class->convert_to_xyz = convert_to_xyz;
  color_model_class->convert_from_xyz = convert_from_xyz;

  return;
}

static void 
init (GeglColorModelRgbFloat * self, 
      GeglColorModelRgbFloatClass * klass)
{
  GeglColorModel *self_color_model = GEGL_COLOR_MODEL (self);

  self_color_model->data_type = GEGL_FLOAT;
  self_color_model->channel_data_type_name = NULL;
  self_color_model->bytes_per_channel = sizeof(float);

  /* These are the color models we can convert from directly */
  gegl_object_add_interface (GEGL_OBJECT(self), "FromRgbFloat", from_rgb_float);
  gegl_object_add_interface (GEGL_OBJECT(self), "FromRgbU8", from_rgb_u8);
  gegl_object_add_interface (GEGL_OBJECT(self), "FromGrayFloat", from_gray_float);

  return;
}

static GObject*        
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
  GObject *gobject = G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
  GeglColorModel *self_color_model = GEGL_COLOR_MODEL(gobject);
  self_color_model->bytes_per_pixel = self_color_model->bytes_per_channel * 
                                      self_color_model->num_channels;
  return gobject;
}

static GeglColorModelType 
color_model_type (GeglColorModel * self_color_model)
{
  return self_color_model->has_alpha ?
           GEGL_COLOR_MODEL_TYPE_RGBA_FLOAT:
           GEGL_COLOR_MODEL_TYPE_RGB_FLOAT;
}

static void 
set_color (GeglColorModel * self_color_model, 
           GeglColor * color, 
           GeglColorConstant constant)
{
  GeglChannelValue * channel_values = gegl_color_get_channel_values(color); 
  gboolean has_alpha = gegl_color_model_has_alpha (self_color_model); 
  GeglColorModelRgb * cm_rgb = GEGL_COLOR_MODEL_RGB(self_color_model);
  
  gint r = gegl_color_model_rgb_get_red_index (cm_rgb);
  gint g = gegl_color_model_rgb_get_green_index (cm_rgb);
  gint b = gegl_color_model_rgb_get_blue_index (cm_rgb);
  gint a = gegl_color_model_alpha_channel (GEGL_COLOR_MODEL(cm_rgb));

  switch (constant) 
    { 
      case GEGL_COLOR_WHITE:
        channel_values[r].f = 1.0;
        channel_values[g].f = 1.0;
        channel_values[b].f = 1.0;
        if (has_alpha)
          channel_values[a].f = 1.0;
        break;
      case GEGL_COLOR_BLACK:
        channel_values[r].f = 0.0;
        channel_values[g].f = 0.0;
        channel_values[b].f = 0.0;
        if (has_alpha)
          channel_values[a].f = 1.0;
        break;
      case GEGL_COLOR_RED:
        channel_values[r].f = 1.0;
        channel_values[g].f = 0.0;
        channel_values[b].f = 0.0;
        if (has_alpha)
          channel_values[a].f = 1.0;
        break;
      case GEGL_COLOR_GREEN:
        channel_values[r].f = 0.0;
        channel_values[g].f = 1.0;
        channel_values[b].f = 0.0;
        if (has_alpha)
          channel_values[a].f = 1.0;
        break;
      case GEGL_COLOR_BLUE:
        channel_values[r].f = 0.0;
        channel_values[g].f = 0.0;
        channel_values[b].f = 1.0;
        if (has_alpha)
          channel_values[a].f = 1.0;
        break;
      case GEGL_COLOR_GRAY:
      case GEGL_COLOR_HALF_WHITE:
        channel_values[r].f = 0.5;
        channel_values[g].f = 0.5;
        channel_values[b].f = 0.5;
        if (has_alpha)
          channel_values[a].f = 1.0;
        break;
      case GEGL_COLOR_WHITE_TRANSPARENT:
        channel_values[r].f = 1.0;
        channel_values[g].f = 1.0;
        channel_values[b].f = 1.0;
        if (!has_alpha)
          channel_values[a].f = 0.0;
        break;
      case GEGL_COLOR_TRANSPARENT:
      case GEGL_COLOR_BLACK_TRANSPARENT:
        channel_values[r].f = 0.0;
        channel_values[g].f = 0.0;
        channel_values[b].f = 0.0;
        if (has_alpha)
          channel_values[a].f = 0.0;
        break;
    }
}

static char * 
get_convert_interface_name (GeglColorModel * self_color_model)
{
  return g_strdup ("FromRgbFloat"); 
}

static void 
convert_to_xyz (GeglColorModel * self_color_model, 
                gfloat ** xyz_data, 
                guchar ** data, 
                gint width)
{
  /* convert from float rgb to float xyz */

  /*  
    [ X ] [ 0.412453  0.357580  0.180423 ] [ R709 ] 
    [ Y ]=[ 0.212671  0.715160  0.072169 ]*[ G709 ] 
    [ Z ] [ 0.019334  0.119193  0.950227 ] [ B709 ]

  */
  gfloat m00 = 0.412453;
  gfloat m10 = 0.357580;
  gfloat m20 = 0.180423;
  gfloat m01 = 0.212671;
  gfloat m11 = 0.715160;
  gfloat m21 = 0.072169;
  gfloat m02 = 0.019334;
  gfloat m12 = 0.119193;
  gfloat m22 = 0.950227;

  gfloat *r, *g, *b; 
  gfloat *a = NULL;
  gfloat *x, *y, *z; 
  gfloat *xyz_a = NULL;

  gboolean has_alpha = gegl_color_model_has_alpha(self_color_model);

  r = (gfloat*) data[0];
  g = (gfloat*) data[1];
  b = (gfloat*) data[2];       

  x = xyz_data[0];
  y = xyz_data[1];
  z = xyz_data[2];

  if (has_alpha)
    {
     xyz_a = xyz_data[3];
     a = (gfloat*) data[3];
    }

  while (width--)
    {
     *x++ = m00 * *r + m10 * *g + m20 * *b;
     *y++ = m01 * *r + m11 * *g + m21 * *b;
     *z++ = m02 * *r++ + m12 * *g++ + m22 * *b++;
     
     if (has_alpha)
       *xyz_a++ = *a++;
    }
}

static void 
convert_from_xyz (GeglColorModel * self_color_model, 
                  guchar ** data, 
                  gfloat ** xyz_data, 
                  gint width)
{
  /* convert from float xyz to float rgb */

  /*
    [ R709 ] [ 3.240479 -1.53715  -0.498535 ] [ X ] 
    [ G709 ]=[-0.969256  1.875991  0.041556 ]*[ Y ] 
    [ B709 ] [ 0.055648 -0.204043  1.057311 ] [ Z ] 
  */

  gfloat m00 = 3.240479;
  gfloat m10 = -1.53715;
  gfloat m20 = -0.498535;
  gfloat m01 = -0.969256;
  gfloat m11 = 1.875991;
  gfloat m21 = 0.041556; 
  gfloat m02 = 0.055648;
  gfloat m12 = -0.204043;
  gfloat m22 = 1.057311;

  gfloat *x, *y, *z; 
  gfloat *xyz_a = NULL;
  gfloat *r, *g, *b; 
  gfloat *a = NULL;
  
  gboolean has_alpha = gegl_color_model_has_alpha(self_color_model);

  x = xyz_data[0];
  y = xyz_data[1];
  z = xyz_data[2];

  r = (gfloat*) data[0];
  g = (gfloat*) data[1];
  b = (gfloat*) data[2];

  if (has_alpha) 
    {
     a = (gfloat*) data[3];
     xyz_a = xyz_data[3];
    }

  while (width--)
    {
     *r++ = m00 * *x + m10 * *y + m20 * *z;
     *g++ = m01 * *x + m11 * *y + m21 * *z;
     *b++ = m02 * *x++ + m12 * *y++ + m22 * *z++;
      if (has_alpha)
       *xyz_a++ = *a++;
    }
}

/**
 * from_rgb_u8:
 * @self_color_model: RGB_FLOAT #GeglColorModel.
 * @src_color_model: RGB_U8 #GeglColorModel.
 * @data: pointers to the dest rgb float data.
 * @src_data: pointers to the src rgb u8 data.
 * @width: number of pixels to process.
 *
 * Converts from RGB_U8 to RGB_FLOAT.
 *
 **/
static void 
from_rgb_u8 (GeglColorModel * self_color_model, 
             GeglColorModel * src_color_model, 
             guchar ** data, 
             guchar ** src_data, 
             gint width)
{
  gfloat          *r, *g, *b; 
  gfloat          *a = NULL;
  guint8          *src_r, *src_g, *src_b;
  guint8          *src_a = NULL;
  gboolean         has_alpha = gegl_color_model_has_alpha(self_color_model);
  gboolean         src_has_alpha = gegl_color_model_has_alpha(src_color_model);
  
  r = (gfloat*)data[0];
  g = (gfloat*)data[1];
  b = (gfloat*)data[2];
  if (has_alpha)
    a = (gfloat*)data[3];
  
  src_r = (guint8*) src_data[0];
  src_g = (guint8*) src_data[1];
  src_b = (guint8*) src_data[2];
  if (src_has_alpha)
    src_a = (guint8*) src_data[3];

  while (width--)
    {
     *r++ = *src_r++ / 255.0;
     *g++ = *src_g++ / 255.0;
     *b++ = *src_b++ / 255.0;

     if (src_has_alpha && has_alpha)
       *a++ = *src_a++ / 255.0;
     else if (has_alpha) 
       *a++ = 1.0;
    } 
}

/**
 * from_gray_float:
 * @self_color_model: RGB_FLOAT #GeglColorModel.
 * @src_color_model: GRAY_FLOAT #GeglColorModel.
 * @data: pointers to the dest rgb float data.
 * @src_data: pointers to the src gray float data.
 * @width: number of pixels to process.
 *
 * Converts from GRAY_FLOAT to RGB_FLOAT.
 *
 **/
static void 
from_gray_float (GeglColorModel * self_color_model, 
                 GeglColorModel * src_color_model, 
                 guchar ** data, 
                 guchar ** src_data, 
                 gint width)
{
  gfloat          *r, *g, *b; 
  gfloat          *a = NULL;
  gfloat          *src_g;
  gfloat          *src_a = NULL;
  gboolean         has_alpha = gegl_color_model_has_alpha(self_color_model);
  gboolean         src_has_alpha = gegl_color_model_has_alpha(src_color_model);
  
  r = (gfloat*)data[0];
  g = (gfloat*)data[1];
  b = (gfloat*)data[2];
  if (has_alpha)
    a = (gfloat*)data[3];

  src_g = (gfloat*) src_data[0];
  if (src_has_alpha)
    src_a = (gfloat*) src_data[1];

  while (width--)
    {
     *r++ = *src_g;
     *g++ = *src_g;
     *b++ = *src_g++;

     if (src_has_alpha && has_alpha)
       *a++ = *src_a++;
     else if (has_alpha) 
       *a++ = 1.0;
    } 
}

/**
 * from_rgb_float:
 * @self_color_model: RGB_FLOAT #GeglColorModel.
 * @src_color_model: RGB_FLOAT #GeglColorModel.
 * @data: pointers to dest rgb float data.
 * @src_data: pointers to src rgb float data.
 * @width: number of pixels to process.
 *
 * Copies from RGB_FLOAT to RGB_FLOAT.
 *
 **/
static void 
from_rgb_float (GeglColorModel * self_color_model, 
                GeglColorModel * src_color_model, 
                guchar ** data, 
                guchar ** src_data, 
                gint width)
{
  gfloat          *r, *g, *b; 
  gfloat          *a = NULL;
  gfloat          *src_r, *src_g, *src_b; 
  gfloat          *src_a = NULL;
  gboolean         has_alpha = gegl_color_model_has_alpha(self_color_model);
  gboolean         src_has_alpha = gegl_color_model_has_alpha(src_color_model);
  gint             row_bytes = width * gegl_color_model_bytes_per_channel(self_color_model); 
  
  r = (gfloat*)data[0];
  g = (gfloat*)data[1];
  b = (gfloat*)data[2];
  if (has_alpha)
    a = (gfloat*)data[3];

  src_r = (gfloat*)src_data[0];
  src_g = (gfloat*)src_data[1];
  src_b = (gfloat*)src_data[2];
  if (src_has_alpha)
    src_a = (gfloat*)src_data[3];

  if (src_has_alpha && has_alpha)
    {
      memcpy(r, src_r, row_bytes);  
      memcpy(g, src_g, row_bytes);  
      memcpy(b, src_b, row_bytes);  
      memcpy(a, src_a, row_bytes);  
    }
  else if (has_alpha) 
    {
      memcpy(r, src_r, row_bytes);  
      memcpy(g, src_g, row_bytes);  
      memcpy(b, src_b, row_bytes);  

          while(width--)
      *a++ = 1.0;
    }
  else 
    {
      memcpy(r, src_r, row_bytes);  
      memcpy(g, src_g, row_bytes);  
      memcpy(b, src_b, row_bytes);  
    }
}
