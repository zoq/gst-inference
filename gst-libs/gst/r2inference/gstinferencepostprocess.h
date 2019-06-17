/*
 * GStreamer
 * Copyright (C) 2019 RidgeRun
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <gst/r2inference/gstvideoinference.h>
#include <gst/r2inference/gstinferencemeta.h>

#ifndef __GST_INFERENCE_POSTPROCESS_H__
#define __GST_INFERENCE_POSTPROCESS_H__

G_BEGIN_DECLS

/**
 * \brief Fill all the classification meta with predictions
 *
 * \param class_meta Meta to fill
 * \param prediction Value of the prediction
 * \param predsize Size of the prediction
 */

gboolean gst_fill_classification_meta(GstClassificationMeta *class_meta, const gpointer prediction,
    gsize predsize);

/**
 * \brief Fill all the detection meta with the boxes
 *
 * \param prediction Value of the prediction
 * \param detect_meta Meta to fill
 * \param resulting_boxes The output boxes of the prediction
 * \param elements The number of objects
 * \param obj_thresh Objectness threshold
 * \param prob_thresh Class probability threshold
 * \param iou_thresh Intersection over union threshold
 */
gboolean gst_create_boxes (const gpointer prediction,
    GstDetectionMeta *detect_meta, BBox ** resulting_boxes,
    gint * elements, gfloat obj_thresh, gfloat prob_thresh, gfloat iou_thresh);

/**
 * \brief Fill all the detection meta with the boxes
 *
 * \param prediction Value of the prediction
 * \param detect_meta Meta to fill
 * \param resulting_boxes The output boxes of the prediction
 * \param elements The number of objects
 * \param obj_thresh Objectness threshold
 * \param prob_thresh Class probability threshold
 * \param iou_thresh Intersection over union threshold
 */
gboolean gst_create_boxes_float (const gpointer prediction,
    GstDetectionMeta *detect_meta, BBox ** resulting_boxes,
    gint * elements, gdouble obj_thresh, gdouble prob_thresh, gdouble iou_thresh);

G_END_DECLS

#endif
