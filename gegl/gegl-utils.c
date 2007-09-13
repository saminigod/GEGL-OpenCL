/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2003 Calvin Williamson
 */

#include "config.h"

#include <glib-object.h>

#include "gegl-types.h"

#include "gegl-utils.h"
#include "gegl-types.h"


void gegl_log         (GLogLevelFlags  level,
                       gchar          *file,
                       gint            line,
                       gchar          *function,
                       gchar          *format,
                       ...);
void gegl_logv        (GLogLevelFlags  level,
                       gchar          *file,
                       gint            line,
                       gchar          *function,
                       gchar          *format,
                       va_list         args);
void gegl_direct_log  (GLogLevelFlags  level,
                       gchar          *format,
                       ...);
void gegl_direct_logv (GLogLevelFlags  level,
                       gchar          *format,
                       va_list         args);


inline gint
_gegl_float_epsilon_zero (float value)
{
  return value > -GEGL_FLOAT_EPSILON && value < GEGL_FLOAT_EPSILON;
}

inline gint
_gegl_float_epsilon_equal (float v1, float v2)
{
  register float diff = v1 - v2;

  return diff > -GEGL_FLOAT_EPSILON && diff < GEGL_FLOAT_EPSILON;
}

void
gegl_rectangle_set (GeglRectangle *r,
                    gint           x,
                    gint           y,
                    guint          w,
                    guint          h)
{
  r->x      = x;
  r->y      = y;
  r->width  = w;
  r->height = h;
}

void
gegl_rectangle_bounding_box (GeglRectangle       *dest,
                             const GeglRectangle *src1,
                             const GeglRectangle *src2)
{
  gboolean s1_has_area = src1->width && src1->height;
  gboolean s2_has_area = src2->width && src2->height;

  if (!s1_has_area && !s2_has_area)
    gegl_rectangle_set (dest, 0, 0, 0, 0);
  else if (!s1_has_area)
    gegl_rectangle_copy (dest, src2);
  else if (!s2_has_area)
    gegl_rectangle_copy (dest, src1);
  else
    {
      gint x1 = MIN (src1->x, src2->x);
      gint x2 = MAX (src1->x + src1->width, src2->x + src2->width);
      gint y1 = MIN (src1->y, src2->y);
      gint y2 = MAX (src1->y + src1->height, src2->y + src2->height);

      dest->x      = x1;
      dest->y      = y1;
      dest->width  = x2 - x1;
      dest->height = y2 - y1;
    }
}

gboolean
gegl_rectangle_intersect (GeglRectangle       *dest,
                          const GeglRectangle *src1,
                          const GeglRectangle *src2)
{
  gint x1, x2, y1, y2;

  x1 = MAX (src1->x, src2->x);
  x2 = MIN (src1->x + src1->width, src2->x + src2->width);

  if (x2 <= x1)
    {
      if (dest)
        gegl_rectangle_set (dest, 0, 0, 0, 0);
      return FALSE;
    }

  y1 = MAX (src1->y, src2->y);
  y2 = MIN (src1->y + src1->height, src2->y + src2->height);

  if (y2 <= y1)
    {
      if (dest)
        gegl_rectangle_set (dest, 0, 0, 0, 0);
      return FALSE;
    }

  if (dest)
    gegl_rectangle_set (dest, x1, y1, x2 - x1, y2 - y1);
  return TRUE;
}

void
gegl_rectangle_copy (GeglRectangle       *to,
                     const GeglRectangle *from)
{
  to->x      = from->x;
  to->y      = from->y;
  to->width  = from->width;
  to->height = from->height;
}

gboolean
gegl_rectangle_contains (const GeglRectangle *r,
                         const GeglRectangle *s)
{
  if (s->x >= r->x &&
      s->y >= r->y &&
      (s->x + s->width) <= (r->x + r->width) &&
      (s->y + s->height) <= (r->y + r->height))
    return TRUE;
  else
    return FALSE;
}

gboolean
gegl_rectangle_equal (const GeglRectangle *r,
                      const GeglRectangle *s)
{
  if (r->x == s->x &&
      r->y == s->y &&
      r->width == s->width &&
      r->height == s->height)
    return TRUE;
  else
    return FALSE;
}

gboolean
gegl_rectangle_equal_coords (const GeglRectangle *r,
                             gint                 x,
                             gint                 y,
                             gint                 w,
                             gint                 h)
{
  if (r->x == x &&
      r->y == y &&
      r->width == w &&
      r->height == h)
    return TRUE;
  else
    return FALSE;
}

#define GEGL_LOG_DOMAIN    "Gegl"

void gegl_log_debug (gchar *file,
                     gint   line,
                     gchar *function,
                     gchar *format,
                     ...)
{
  va_list args;

  va_start (args, format);
  gegl_logv (G_LOG_LEVEL_DEBUG, file, line, function, format, args);
  va_end (args);
}

void gegl_log_info (gchar *file,
                    gint   line,
                    gchar *function,
                    gchar *format,
                    ...)
{
  va_list args;

  va_start (args, format);
  gegl_logv (G_LOG_LEVEL_INFO, file, line, function, format, args);
  va_end (args);
}

void gegl_log_message (gchar *file,
                       gint   line,
                       gchar *function,
                       gchar *format,
                       ...)
{
  va_list args;

  va_start (args, format);
  gegl_logv (G_LOG_LEVEL_MESSAGE, file, line, function, format, args);
  va_end (args);
}

void gegl_log_direct (gchar *format,
                      ...)
{
  va_list args;

  va_start (args, format);
  gegl_direct_logv (G_LOG_LEVEL_DEBUG, format, args);
  va_end (args);
}

void
gegl_log (GLogLevelFlags level,
          gchar         *file,
          gint           line,
          gchar         *function,
          gchar         *format,
          ...)
{
  va_list args;

  va_start (args, format);
  gegl_logv (level, file, line, function, format, args);
  va_end (args);
}

void
gegl_logv (GLogLevelFlags level,
           gchar         *file,
           gint           line,
           gchar         *function,
           gchar         *format,
           va_list        args)
{
  if (g_getenv ("GEGL_LOG_ON"))
    {
      gchar *tabbed = NULL;

      /* log the file and line */
      g_log (GEGL_LOG_DOMAIN, level, "%s:  %s:%d:", function, file, line);

      /* move the regular output over a bit. */
      tabbed = g_strconcat ("   ", format, NULL);
      g_logv (GEGL_LOG_DOMAIN, level, tabbed, args);
      g_log (GEGL_LOG_DOMAIN, level, "        ");
      g_free (tabbed);
    }
}

void
gegl_direct_log (GLogLevelFlags level,
                 gchar         *format,
                 ...)
{
  va_list args;

  va_start (args, format);
  gegl_direct_logv (level, format, args);
  va_end (args);
}

void
gegl_direct_logv (GLogLevelFlags level,
                  gchar         *format,
                  va_list        args)
{
  if (g_getenv ("GEGL_LOG_ON"))
    {
      gchar *tabbed = NULL;
      tabbed = g_strconcat ("   ", format, NULL);
      g_logv (GEGL_LOG_DOMAIN, level, tabbed, args);
      g_free (tabbed);
    }
}


static GeglRectangle *
gegl_rectangle_dup (const GeglRectangle *rectangle)
{
  GeglRectangle *result = g_new (GeglRectangle, 1);

  *result = *rectangle;

  return result;
}

GType
gegl_rectangle_get_type (void)
{
  static GType our_type = 0;

  if (our_type == 0)
    our_type = g_boxed_type_register_static (g_intern_static_string ("GeglRectangle"),
                                             (GBoxedCopyFunc) gegl_rectangle_dup,
                                             (GBoxedFreeFunc) g_free);
  return our_type;
}


