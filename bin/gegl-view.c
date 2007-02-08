/* This file is part of GEGL editor -- a gtk frontend for GEGL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (C) 2003, 2004, 2006 Øyvind Kolås
 */

#include <glib-object.h>
#include "gegl.h"
#include <babl/babl.h>
#include "gegl-view.h"
#include "gegl-tree-editor.h"
#include "editor.h"
#include <math.h>

enum
{
  PROP_0,
  PROP_NODE,
  PROP_X,
  PROP_Y,
  PROP_SCALE
};


static void      gegl_view_class_init (GeglViewClass  *klass);
static void      gegl_view_init       (GeglView       *self);
static void      finalize             (GObject        *gobject);
static void      set_property         (GObject        *gobject,
                                       guint           prop_id,
                                       const GValue   *value,
                                       GParamSpec     *pspec);
static void      get_property         (GObject        *gobject,
                                       guint           prop_id,
                                       GValue         *value,
                                       GParamSpec     *pspec);
static gboolean  motion_notify_event  (GtkWidget      *widget,
                                       GdkEventMotion *event);
static gboolean  button_press_event   (GtkWidget      *widget,
                                       GdkEventButton *event);
static gboolean  expose_event         (GtkWidget      *widget,
                                       GdkEventExpose *event);


G_DEFINE_TYPE (GeglView, gegl_view, GTK_TYPE_DRAWING_AREA)


static void
gegl_view_class_init (GeglViewClass * klass)
{
  GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class  = GTK_WIDGET_CLASS (klass);

  gobject_class->finalize     = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  widget_class->motion_notify_event = motion_notify_event;
  widget_class->button_press_event  = button_press_event;
  widget_class->expose_event        = expose_event;

  g_object_class_install_property (gobject_class, PROP_X,
                                   g_param_spec_int ("x",
                                                     "X",
                                                     "X origin",
                                                     G_MININT, G_MAXINT, 0,
                                                     G_PARAM_CONSTRUCT |
                                                     G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_Y,
                                   g_param_spec_int ("y",
                                                     "Y",
                                                     "Y origin",
                                                     G_MININT, G_MAXINT, 0,
                                                     G_PARAM_CONSTRUCT |
                                                     G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_SCALE,
                                   g_param_spec_double ("scale",
                                                        "Scale",
                                                        "Zoom factor",
                                                        0.0, 100.0, 1.00,
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_NODE,
                                   g_param_spec_object ("node",
                                                        "Node",
                                                        "The node to render",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_READWRITE));
}

static void
gegl_view_init (GeglView *self)
{
  self->node        = NULL;
  self->x           = 0;
  self->y           = 0;
  self->prev_x      = -1;
  self->prev_y      = -1;
  self->scale       = 1.0;
  self->monitor_id  = 0;
  self->processor   = NULL;

  gtk_widget_add_events (GTK_WIDGET (self), (GDK_EXPOSURE_MASK     |
                                             GDK_BUTTON_PRESS_MASK |
                                             GDK_POINTER_MOTION_MASK));
}

static void
finalize (GObject *gobject)
{
  GeglView * self = GEGL_VIEW (gobject);

  if (self->node)
    g_object_unref (self->node);

  G_OBJECT_CLASS (gegl_view_parent_class)->finalize (gobject);
}

static void
computed_event (GeglNode      *self,
                GeglRectangle *rect,
                GeglView      *view)
{
  gint x = view->scale * (rect->x - view->x);
  gint y = view->scale * (rect->y - view->y);
  gint w = ceil (view->scale * rect->width );
  gint h = ceil (view->scale * rect->height);

  gtk_widget_queue_draw_area (GTK_WIDGET (view), x, y, w, h);
}


static void
invalidated_event (GeglNode      *self,
                   GeglRectangle *rect,
                   GeglView      *view)
{
  gegl_view_repaint (view);
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglView *self = GEGL_VIEW (gobject);

  switch (property_id)
    {
    case PROP_NODE:
      if (self->node)
        {
#if 0
          gegl_node_disable_cache (self->node); /* FIXME: should we really? */
#endif
          g_object_unref (self->node);
        }

      if (g_value_get_object (value))
        {
          self->node = GEGL_NODE (g_value_dup_object (value));

          g_signal_connect_object (self->node, "computed",
                                   G_CALLBACK (computed_event),
                                   self, 0);
          g_signal_connect_object (self->node, "invalidated",
                                   G_CALLBACK (invalidated_event),
                                   self, 0);
          gegl_view_repaint (self);
        }
      else
        {
          self->node = NULL;
        }
      break;
    case PROP_X:
      self->x = g_value_get_int (value);
      gtk_widget_queue_draw (GTK_WIDGET (self));
      break;
    case PROP_Y:
      self->y = g_value_get_int (value);
      gtk_widget_queue_draw (GTK_WIDGET (self));
      break;
    case PROP_SCALE:
      self->scale = g_value_get_double (value);
      gtk_widget_queue_draw (GTK_WIDGET (self));
      break;
    default:

      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
    }
}

static void
get_property (GObject      *gobject,
              guint         property_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglView *self = GEGL_VIEW (gobject);

  switch (property_id)
    {
    case PROP_NODE:
      g_value_set_object (value, self->node);
      break;
    case PROP_X:
      g_value_set_int (value, self->x);
      break;
    case PROP_Y:
      g_value_set_int (value, self->y);
      break;
    case PROP_SCALE:
      g_value_set_double (value, self->scale);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
    }
}

static gboolean
button_press_event (GtkWidget      *widget,
                    GdkEventButton *event)
{
  GeglView *view = GEGL_VIEW (widget);
  gint      x    = event->x;
  gint      y    = event->y;

  view->screen_x = x;
  view->screen_y = y;

  view->orig_x = view->x;
  view->orig_y = view->y;

  view->start_buf_x = view->x + x/view->scale;
  view->start_buf_y = view->y + y/view->scale;

  view->prev_x = x;
  view->prev_y = y;

  x = x / view->scale + view->x;
  y = y / view->scale + view->y;

  {
    GeglNode *detected = gegl_node_detect (view->node,
                                           view->x + event->x / view->scale,
                                           view->y + event->y / view->scale);
    if (detected)
      {
#if 0
        gchar *name;
        gchar *operation;

        gegl_node_get (detected, "name", &name, "operation", &operation, NULL);
        g_warning ("%s:%s(%p)", operation, name, detected);
        g_free (name);
        g_free (operation);
#endif

        tree_editor_set_active (editor.tree_editor, detected);
      }
  }

  return TRUE;
}

static gboolean
motion_notify_event (GtkWidget      *widget,
                     GdkEventMotion *event)
{
  GeglView *view = GEGL_VIEW (widget);
  gint      x    = event->x;
  gint      y    = event->y;

  if (event->state & GDK_BUTTON1_MASK)
    {
      gint pre_x = floor (view->x * view->scale);
      gint pre_y = floor (view->y * view->scale);

      view->x = view->orig_x + (view->screen_x-x) / view->scale;
      view->y = view->orig_y + (view->screen_y-y) / view->scale;

      gdk_window_scroll (widget->window, pre_x - floor (view->x * view->scale),
                                         pre_y - floor (view->y * view->scale));

      g_object_notify (G_OBJECT (view), "x");
      g_object_notify (G_OBJECT (view), "y");
    }
  else if (event->state & GDK_BUTTON3_MASK)
    {
      gint diff = view->prev_y - y;
      gint i;

      if (diff < 0)
        {
          for (i=0;i>diff;i--)
            {
              view->scale /= 1.006;
            }
        }
      else
        {
          for (i=0;i<diff;i++)
            {
              view->scale *= 1.006;
            }
        }

      view->x = view->start_buf_x - (view->screen_x) / view->scale;
      view->y = view->start_buf_y - (view->screen_y) / view->scale;

      gtk_widget_queue_draw (GTK_WIDGET (view));

      g_object_notify (G_OBJECT (view), "x");
      g_object_notify (G_OBJECT (view), "y");
      g_object_notify (G_OBJECT (view), "scale");
    }

  view->prev_x = x;
  view->prev_y = y;

  return TRUE;
}

static gboolean
expose_event (GtkWidget      *widget,
              GdkEventExpose *event)
{
  GeglView      *view = GEGL_VIEW (widget);
  GdkRectangle  *rectangles;
  gint           count;
  gint           i;

  if (! view->node)
    return FALSE;

  gdk_region_get_rectangles (event->region, &rectangles, &count);

  for (i=0; i<count; i++)
    {
      GeglRectangle  roi;
      guchar        *buf;

      roi.x = view->x + (rectangles[i].x / view->scale);
      roi.y = view->y + (rectangles[i].y / view->scale);
      roi.width  = rectangles[i].width;
      roi.height  = rectangles[i].height;

      buf = g_malloc ((roi.width +1) * (roi.height+1) * 3);
      /* FIXME: this padding should not be needed, but it avoids some segfaults */

      gegl_node_blit (view->node,
                      &roi,
                      view->scale,
                      babl_format ("R'G'B' u8"),
                      0, /* rowstride, currently unused */
                      (gpointer)buf,
                      GEGL_BLIT_CACHE|GEGL_BLIT_DIRTY);

      gdk_draw_rgb_image (widget->window,
                          widget->style->black_gc,
                          rectangles[i].x, rectangles[i].y,
                          rectangles[i].width, rectangles[i].height,
                          GDK_RGB_DITHER_NONE,
                          buf, roi.width  * 3);
      g_free (buf);
    }

  g_free (rectangles);

  gegl_view_repaint (view);

  return FALSE;
}

static gboolean
task_monitor (GeglView *view)
{
  if (gegl_processor_work (view->processor, NULL))
    return TRUE;

  view->monitor_id = 0;

  return FALSE;
}

void
gegl_view_repaint (GeglView *view)
{
  GtkWidget     *widget = GTK_WIDGET (view);
  GeglRectangle  roi    = { view->x, view->y,
                            widget->allocation.width / view->scale,
                            widget->allocation.height / view->scale };

#if 0
  /* forget all already queued repaints */
  gegl_cache_dequeue (gegl_node_get_cache (view->node), NULL);
  /* then enqueue our selves */
  gegl_cache_enqueue (gegl_node_get_cache (view->node), roi);
#endif

  if (view->monitor_id == 0)
    {
      view->monitor_id = g_idle_add_full (G_PRIORITY_LOW,
                                          (GSourceFunc) task_monitor, view,
                                          NULL);

      if (view->processor == NULL)
        {
          if (view->node)
            view->processor = gegl_node_new_processor (view->node, &roi);
        }
    }

  if (view->processor)
    gegl_processor_set_rectangle (view->processor, &roi);
}
