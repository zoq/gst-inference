/* Copyright (C) 2019 RidgeRun, LLC (http://www.ridgerun.com)
 * All Rights Reserved.
 *
 * The contents of this software are proprietary and confidential to RidgeRun,
 * LLC.  No part of this program may be photocopied, reproduced or translated
 * into another programming language without prior written consent of
 * RidgeRun, LLC.  The user is free to modify the source code after obtaining
 * a software license from RidgeRun.  All source code changes must be provided
 * back to RidgeRun without any encumbrance.
*/

#include "gstinferencemeta.h"

#include <gst/video/video.h>
#include <string.h>

static gboolean gst_inference_meta_init (GstMeta * meta,
    gpointer params, GstBuffer * buffer);
static void gst_inference_meta_free (GstMeta * meta, GstBuffer * buffer);
static gboolean gst_inference_clean_nodes (GNode * node, gpointer data);
static gboolean gst_inference_transfer_prediction (GNode * node, gpointer data);
static gboolean gst_inference_meta_transform (GstBuffer * transbuf,
    GstMeta * meta, GstBuffer * buffer, GQuark type, gpointer data);
/* static void gst_inference_children_copy (GNode * node, gpointer data); */
static gboolean gst_inference_meta_transfer (GstBuffer * transbuf,
    GstMeta * meta, GstBuffer * buffer, GstVideoMetaTransform * data);

static void gst_detection_meta_free (GstMeta * meta, GstBuffer * buffer);
static gboolean gst_detection_meta_init (GstMeta * meta,
    gpointer params, GstBuffer * buffer);
static gboolean gst_detection_meta_transform (GstBuffer * transbuf,
    GstMeta * meta, GstBuffer * buffer, GQuark type, gpointer data);
static gboolean gst_detection_meta_copy (GstBuffer * transbuf,
    GstMeta * meta, GstBuffer * buffer);
static gboolean gst_detection_meta_scale (GstBuffer * transbuf,
    GstMeta * meta, GstBuffer * buffer, GstVideoMetaTransform * data);

static gboolean gst_classification_meta_transform (GstBuffer * dest,
    GstMeta * meta, GstBuffer * buffer, GQuark type, gpointer data);
static gboolean gst_classification_meta_copy (GstBuffer * transbuf,
    GstMeta * meta, GstBuffer * buffer);
static gboolean gst_classification_meta_init (GstMeta * meta,
    gpointer params, GstBuffer * buffer);
static void gst_classification_meta_free (GstMeta * meta, GstBuffer * buffer);

GType
gst_inference_meta_api_get_type (void)
{
  static volatile GType type = 0;
  static const gchar *tags[] =
      { GST_META_TAG_VIDEO_STR, GST_META_TAG_VIDEO_ORIENTATION_STR,
    GST_META_TAG_VIDEO_SIZE_STR, NULL
  };

  if (g_once_init_enter (&type)) {
    GType _type = gst_meta_api_type_register ("GstInferenceMetaAPI", tags);
    g_once_init_leave (&type, _type);
  }
  return type;
}

/* inference metadata */
const GstMetaInfo *
gst_inference_meta_get_info (void)
{
  static const GstMetaInfo *inference_meta_info = NULL;

  if (g_once_init_enter (&inference_meta_info)) {
    const GstMetaInfo *meta = gst_meta_register (GST_INFERENCE_META_API_TYPE,
        "GstInferenceMeta", sizeof (GstInferenceMeta),
        gst_inference_meta_init, gst_inference_meta_free,
        gst_inference_meta_transform);
    g_once_init_leave (&inference_meta_info, meta);
  }
  return inference_meta_info;
}

GType
gst_embedding_meta_api_get_type (void)
{
  static volatile GType type = 0;
  static const gchar *tags[] = { GST_META_TAG_VIDEO_STR, NULL };

  if (g_once_init_enter (&type)) {
    GType _type = gst_meta_api_type_register ("GstEmbeddingMetaAPI", tags);
    g_once_init_leave (&type, _type);
  }
  return type;
}

/* embedding metadata: As per now the embedding meta is ABI compatible
 * with classification. Reuse the meta methods.
 */
const GstMetaInfo *
gst_embedding_meta_get_info (void)
{
  static const GstMetaInfo *embedding_meta_info = NULL;

  if (g_once_init_enter (&embedding_meta_info)) {
    const GstMetaInfo *meta = gst_meta_register (GST_EMBEDDING_META_API_TYPE,
        "GstEmbeddingMeta", sizeof (GstEmbeddingMeta),
        gst_classification_meta_init, gst_classification_meta_free,
        gst_classification_meta_transform);
    g_once_init_leave (&embedding_meta_info, meta);
  }
  return embedding_meta_info;
}

GType
gst_classification_meta_api_get_type (void)
{
  static volatile GType type = 0;
  static const gchar *tags[] = { GST_META_TAG_VIDEO_STR, NULL };

  if (g_once_init_enter (&type)) {
    GType _type = gst_meta_api_type_register ("GstClassificationMetaAPI", tags);
    g_once_init_leave (&type, _type);
  }
  return type;
}

/* classification metadata */
const GstMetaInfo *
gst_classification_meta_get_info (void)
{
  static const GstMetaInfo *classification_meta_info = NULL;

  if (g_once_init_enter (&classification_meta_info)) {
    const GstMetaInfo *meta =
        gst_meta_register (GST_CLASSIFICATION_META_API_TYPE,
        "GstClassificationMeta", sizeof (GstClassificationMeta),
        gst_classification_meta_init, gst_classification_meta_free,
        gst_classification_meta_transform);
    g_once_init_leave (&classification_meta_info, meta);
  }
  return classification_meta_info;
}

GType
gst_detection_meta_api_get_type (void)
{
  static volatile GType type = 0;
  static const gchar *tags[] =
      { GST_META_TAG_VIDEO_STR, GST_META_TAG_VIDEO_ORIENTATION_STR,
    GST_META_TAG_VIDEO_SIZE_STR, NULL
  };

  if (g_once_init_enter (&type)) {
    GType _type = gst_meta_api_type_register ("GstDetectionMetaAPI", tags);
    g_once_init_leave (&type, _type);
  }
  return type;
}

/* detection metadata */
const GstMetaInfo *
gst_detection_meta_get_info (void)
{
  static const GstMetaInfo *detection_meta_info = NULL;

  if (g_once_init_enter (&detection_meta_info)) {
    const GstMetaInfo *meta =
        gst_meta_register (GST_DETECTION_META_API_TYPE, "GstDetectionMeta",
        sizeof (GstDetectionMeta), gst_detection_meta_init,
        gst_detection_meta_free,
        gst_detection_meta_transform);
    g_once_init_leave (&detection_meta_info, meta);
  }
  return detection_meta_info;
}

static gboolean
gst_classification_meta_init (GstMeta * meta, gpointer params,
    GstBuffer * buffer)
{
  GstClassificationMeta *cmeta = (GstClassificationMeta *) meta;

  cmeta->label_probs = NULL;
  cmeta->num_labels = 0;

  return TRUE;
}

static void
gst_classification_meta_free (GstMeta * meta, GstBuffer * buffer)
{
  GstClassificationMeta *class_meta = (GstClassificationMeta *) meta;

  g_return_if_fail (meta != NULL);
  g_return_if_fail (buffer != NULL);

  if (class_meta->num_labels != 0) {
    g_free (class_meta->label_probs);
  }
}

static gboolean
gst_detection_meta_init (GstMeta * meta, gpointer params, GstBuffer * buffer)
{
  GstDetectionMeta *dmeta = (GstDetectionMeta *) meta;

  dmeta->boxes = NULL;
  dmeta->num_boxes = 0;

  return TRUE;
}

static void
gst_detection_meta_free (GstMeta * meta, GstBuffer * buffer)
{
  GstDetectionMeta *detect_meta = (GstDetectionMeta *) meta;

  g_return_if_fail (meta != NULL);
  g_return_if_fail (buffer != NULL);

  if (detect_meta->num_boxes != 0) {
    g_free (detect_meta->boxes);
  }
}

static gboolean
gst_detection_meta_copy (GstBuffer * dest, GstMeta * meta, GstBuffer * buffer)
{
  GstDetectionMeta *dmeta, *smeta;
  gsize raw_size;

  g_return_val_if_fail (dest, FALSE);
  g_return_val_if_fail (meta, FALSE);
  g_return_val_if_fail (buffer, FALSE);

  smeta = (GstDetectionMeta *) meta;
  dmeta =
      (GstDetectionMeta *) gst_buffer_add_meta (dest, GST_DETECTION_META_INFO,
      NULL);
  if (!dmeta) {
    GST_ERROR ("Unable to add meta to buffer");
    return FALSE;
  }

  dmeta->num_boxes = smeta->num_boxes;
  raw_size = dmeta->num_boxes * sizeof (BBox);
  dmeta->boxes = (BBox *) g_malloc (raw_size);
  memcpy (dmeta->boxes, smeta->boxes, raw_size);

  return TRUE;
}

static gboolean
gst_detection_meta_scale (GstBuffer * dest,
    GstMeta * meta, GstBuffer * buffer, GstVideoMetaTransform * trans)
{
  GstDetectionMeta *dmeta, *smeta;
  gsize raw_size;
  gint ow, oh, nw, nh;
  gdouble hfactor, vfactor;

  g_return_val_if_fail (dest, FALSE);
  g_return_val_if_fail (meta, FALSE);
  g_return_val_if_fail (buffer, FALSE);
  g_return_val_if_fail (trans, FALSE);

  smeta = (GstDetectionMeta *) meta;
  dmeta =
      (GstDetectionMeta *) gst_buffer_add_meta (dest, GST_DETECTION_META_INFO,
      NULL);

  if (!dmeta) {
    GST_ERROR ("Unable to add meta to buffer");
    return FALSE;
  }

  ow = GST_VIDEO_INFO_WIDTH (trans->in_info);
  nw = GST_VIDEO_INFO_WIDTH (trans->out_info);
  oh = GST_VIDEO_INFO_HEIGHT (trans->in_info);
  nh = GST_VIDEO_INFO_HEIGHT (trans->out_info);

  g_return_val_if_fail (ow, FALSE);
  g_return_val_if_fail (oh, FALSE);

  dmeta->num_boxes = smeta->num_boxes;
  raw_size = dmeta->num_boxes * sizeof (BBox);
  dmeta->boxes = (BBox *) g_malloc (raw_size);

  hfactor = nw * 1.0 / ow;
  vfactor = nh * 1.0 / oh;

  GST_DEBUG ("Scaling detection metadata %dx%d -> %dx%d", ow, oh, nw, nh);
  for (gint i = 0; i < dmeta->num_boxes; ++i) {
    dmeta->boxes[i].x = smeta->boxes[i].x * hfactor;
    dmeta->boxes[i].y = smeta->boxes[i].y * vfactor;

    dmeta->boxes[i].width = smeta->boxes[i].width * hfactor;
    dmeta->boxes[i].height = smeta->boxes[i].height * vfactor;

    dmeta->boxes[i].label = smeta->boxes[i].label;
    dmeta->boxes[i].prob = smeta->boxes[i].prob;

    GST_LOG ("scaled bbox %d: %fx%f@%fx%f -> %fx%f@%fx%f",
        smeta->boxes[i].label, smeta->boxes[i].x, smeta->boxes[i].y,
        smeta->boxes[i].width, smeta->boxes[i].height, dmeta->boxes[i].x,
        dmeta->boxes[i].y, dmeta->boxes[i].width, dmeta->boxes[i].height);
  }
  return TRUE;
}

static gboolean
gst_detection_meta_transform (GstBuffer * dest, GstMeta * meta,
    GstBuffer * buffer, GQuark type, gpointer data)
{
  GST_LOG ("Transforming detection metadata");

  if (GST_META_TRANSFORM_IS_COPY (type)) {
    GST_LOG ("Copy detection metadata");
    return gst_detection_meta_copy (dest, meta, buffer);
  }

  if (GST_VIDEO_META_TRANSFORM_IS_SCALE (type)) {
    GstVideoMetaTransform *trans = (GstVideoMetaTransform *) data;
    return gst_detection_meta_scale (dest, meta, buffer, trans);
  }

  /* No transform supported */
  return FALSE;
}

static gboolean
gst_classification_meta_copy (GstBuffer * dest,
    GstMeta * meta, GstBuffer * buffer)
{
  GstClassificationMeta *dmeta, *smeta;
  gsize raw_size;

  smeta = (GstClassificationMeta *) meta;
  dmeta =
      (GstClassificationMeta *) gst_buffer_add_meta (dest,
      GST_CLASSIFICATION_META_INFO, NULL);

  if (!dmeta) {
    GST_ERROR ("Unable to add meta to buffer");
    return FALSE;
  }

  GST_LOG ("Copy classification metadata");
  dmeta->num_labels = smeta->num_labels;
  raw_size = dmeta->num_labels * sizeof (gdouble);
  dmeta->label_probs = (gdouble *) g_malloc (raw_size);
  memcpy (dmeta->label_probs, smeta->label_probs, raw_size);

  return TRUE;
}

static gboolean
gst_classification_meta_transform (GstBuffer * dest, GstMeta * meta,
    GstBuffer * buffer, GQuark type, gpointer data)
{
  GST_LOG ("Transforming detection metadata");

  if (GST_META_TRANSFORM_IS_COPY (type)) {
    return gst_classification_meta_copy (dest, meta, buffer);
  }

  /* No transform supported */
  return FALSE;
}

/* inference metadata functions */
static gboolean
gst_inference_meta_init (GstMeta * meta, gpointer params, GstBuffer * buffer)
{
  GstInferenceMeta *imeta = (GstInferenceMeta *) meta;
  Prediction *root;

  /* Create root Prediction */
  root = g_malloc (sizeof (Prediction));
  root->id = rand ();
  root->enabled = TRUE;
  root->box = NULL;
  root->classifications = NULL;
  root->node = g_node_new (root);

  imeta->prediction = root;

  return TRUE;
}

static void
gst_inference_clean_classifications (gpointer data)
{
  Classification *class = (Classification *) data;

  g_return_if_fail (class != NULL);

  /* Delete class */
  if (class->classes_probs)
    g_free (class->classes_probs);

  if (class->class_label)
    g_free (class->class_label);

  g_free (class);
}

static gboolean
gst_inference_clean_nodes (GNode * node, gpointer data)
{
  Prediction *predict = (Prediction *) node->data;

  g_return_val_if_fail (predict != NULL, TRUE);

  /* Delete the Box in the Prediction */
  if (predict->box != NULL) {
    g_free (predict->box);
    predict->box = NULL;
  }
  /* Delete classifications */
  if (predict->classifications != NULL) {
    g_list_free_full (predict->classifications,
        gst_inference_clean_classifications);
  }
  /* Delete the prediction */
  g_free (predict);

  return FALSE;
}

static void
gst_inference_meta_free (GstMeta * meta, GstBuffer * buffer)
{
  GstInferenceMeta *imeta = NULL;
  Prediction *root = NULL;

  g_return_if_fail (meta != NULL);
  g_return_if_fail (buffer != NULL);

  imeta = (GstInferenceMeta *) meta;
  root = imeta->prediction;

  g_node_traverse (root->node, G_LEVEL_ORDER, G_TRAVERSE_ALL, -1,
      gst_inference_clean_nodes, NULL);
  g_node_destroy (root->node);
}

static void
gst_inference_class_copy (gpointer data, gpointer user_data)
{
  Classification *class = (Classification *) data;
  Prediction *target = (Prediction *) user_data;
  Classification *new_class = NULL;

  g_return_if_fail (class != NULL);
  g_return_if_fail (target != NULL);

  /* Allocate new Classification */
  new_class = g_malloc (sizeof (Classification));
  new_class->class_id = class->class_id;
  new_class->class_prob = class->class_prob;
  new_class->num_classes = class->num_classes;
  new_class->class_label = g_strdup (class->class_label);
  if (class->num_classes > 0) {
    new_class->classes_probs = g_malloc (class->num_classes * sizeof (gdouble));
    memcpy (new_class->classes_probs, class->classes_probs,
        class->num_classes * sizeof (gdouble));
  }
}

static Prediction *
gst_inference_prediction_copy (Prediction * src, Prediction * dest)
{
  g_return_val_if_fail (src != NULL, FALSE);

  if (!dest) {
    /* Allocate new Prediction */
    dest = g_malloc (sizeof (Prediction));
    dest->box = NULL;
    dest->classifications = NULL;
    dest->node = g_node_new (dest);
  }

  dest->id = src->id;
  dest->enabled = src->enabled;

  /* Copy BBox if needed */
  if (src->box) {
    if (dest->box)
      g_free (dest->box);
    dest->box = g_malloc (sizeof (BBox));
    memcpy (dest->box, src->box, sizeof (BBox));
  }

  if (src->classifications) {
    g_list_foreach (src->classifications, gst_inference_class_copy,
        (gpointer) dest);
  }

  return dest;
}

static void
gst_inference_children_copy (GNode * node, gpointer data)
{
  GNode *dest_node = (GNode *) data;
  GNode *new_node = NULL;
  Prediction *new_root = NULL;

  g_return_if_fail (dest_node != NULL);

  new_root =
      gst_inference_prediction_copy ((Prediction *) node->data, new_root);

  if (new_root) {
    new_node = new_root->node;
    g_node_append (dest_node, new_node);

    /* Copy node children recursively */
    g_node_children_foreach (node, G_TRAVERSE_ALL, gst_inference_children_copy,
        new_node);
  }
}

static gboolean
gst_inference_transfer_prediction (GNode * node, gpointer data)
{
  GNode *snode = (GNode *) data;
  Prediction *sroot;
  Prediction *droot = (Prediction *) node->data;

  g_return_val_if_fail (snode != NULL, TRUE);
  g_return_val_if_fail (droot != NULL, TRUE);

  sroot = (Prediction *) snode->data;

  /* Transfer prediction only if ID is the same */
  if (sroot->id == droot->id) {
    guint src_n_child = g_node_n_children (snode);
    guint dest_n_child = g_node_n_children (node);

    node->data = (gpointer) gst_inference_prediction_copy (sroot, droot);

    /* Transfer children nodes */
    for (gint i = 0; i < src_n_child; i++) {
      gboolean exists = FALSE;
      GNode *src_nchild = g_node_nth_child (snode, i);
      Prediction *src_pchild = (Prediction *) src_nchild->data;

      for (gint n = 0; i < dest_n_child; i++) {
        GNode *dest_nchild = g_node_nth_child (node, n);
        Prediction *dest_pchild = (Prediction *) dest_nchild->data;

        /* Check if destination node already has this child, if not, we copy it */
        if (dest_pchild->id == src_pchild->id) {
          exists = TRUE;
          break;
        }
      }
      if (!exists) {
        Prediction *new_prediction = NULL;

        /* Copy prediction and add it to parent node */
        new_prediction =
            gst_inference_prediction_copy (src_pchild, new_prediction);
        g_node_append (node, new_prediction->node);
      }
    }
  }

  return FALSE;
}


static gboolean
gst_inference_meta_transfer (GstBuffer * dest,
    GstMeta * meta, GstBuffer * buffer, GstVideoMetaTransform * trans)
{
  GstInferenceMeta *dmeta, *smeta;
  Prediction *droot, *sroot;

  g_return_val_if_fail (dest, FALSE);
  g_return_val_if_fail (meta, FALSE);
  g_return_val_if_fail (buffer, FALSE);
  g_return_val_if_fail (trans, FALSE);

  smeta = (GstInferenceMeta *) meta;
  sroot = smeta->prediction;
  dmeta =
      (GstInferenceMeta *) gst_buffer_get_meta (dest,
      GST_INFERENCE_META_API_TYPE);

  if (!dmeta) {
    /* If meta doesn't exist, copy it */
    dmeta =
        (GstInferenceMeta *) gst_buffer_add_meta (dest, GST_INFERENCE_META_INFO,
        NULL);

    /* Copy root Prediction first */
    dmeta->prediction =
        gst_inference_prediction_copy (sroot, dmeta->prediction);

    if (!dmeta->prediction) {
      GST_ERROR ("Prediction copy failed");
      return FALSE;
    } else {
      Prediction *droot = dmeta->prediction;

      /* Copy node children recursively */
      g_node_children_foreach (sroot->node, G_TRAVERSE_ALL,
          gst_inference_children_copy, droot->node);
    }
  } else {
    /* Transfer the meta */
    droot = dmeta->prediction;

    g_node_traverse (droot->node, G_LEVEL_ORDER, G_TRAVERSE_ALL, -1,
        gst_inference_transfer_prediction, sroot);
  }

  return TRUE;
}


static gboolean
gst_inference_meta_transform (GstBuffer * dest, GstMeta * meta,
    GstBuffer * buffer, GQuark type, gpointer data)
{
  GST_LOG ("Transforming inference metadata");

  if (GST_VIDEO_META_TRANSFORM_IS_SCALE (type)) {
    GstVideoMetaTransform *trans = (GstVideoMetaTransform *) data;
    return gst_inference_meta_transfer (dest, meta, buffer, trans);
  }

  /* No transform supported */
  return FALSE;
}
