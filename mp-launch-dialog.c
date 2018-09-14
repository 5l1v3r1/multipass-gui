#include "mp-launch-dialog.h"

struct _MpLaunchDialog
{
    GtkDialog     parent_instance;

    GtkComboBox  *image_combo;
    GtkListStore *image_model;
    GtkEntry     *name_entry;

    GCancellable *cancellable;
    MpClient     *client;
};

G_DEFINE_TYPE (MpLaunchDialog, mp_launch_dialog, GTK_TYPE_DIALOG)

static void
mp_launch_dialog_dispose (GObject *object)
{
    MpLaunchDialog *dialog = MP_LAUNCH_DIALOG (object);

    g_cancellable_cancel (dialog->cancellable);
    g_clear_object (&dialog->cancellable);
    g_clear_object (&dialog->client);

    G_OBJECT_CLASS (mp_launch_dialog_parent_class)->dispose (object);
}

void
mp_launch_dialog_class_init (MpLaunchDialogClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    object_class->dispose = mp_launch_dialog_dispose;

    gtk_widget_class_set_template_from_resource (widget_class, "/com/ubuntu/multipass/mp-launch-dialog.ui");

    gtk_widget_class_bind_template_child (widget_class, MpLaunchDialog, image_combo);
    gtk_widget_class_bind_template_child (widget_class, MpLaunchDialog, image_model);
    gtk_widget_class_bind_template_child (widget_class, MpLaunchDialog, name_entry);
}

void
mp_launch_dialog_init (MpLaunchDialog *dialog)
{
    gtk_widget_init_template (GTK_WIDGET (dialog));

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (dialog->image_combo), renderer, TRUE);
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (dialog->image_combo), renderer, "text", 0);
}

static void
find_cb (GObject *client, GAsyncResult *result, gpointer user_data)
{
    MpLaunchDialog *dialog = user_data;

    g_autoptr(GError) error = NULL;
    g_auto(GStrv) image_names = mp_client_find_finish (dialog->client, result, &error);
    if (image_names == NULL) {
        g_printerr ("Failed to get images: %s\n", error->message);
        return;
    }

    for (int i = 0; image_names[i] != NULL; i++) {
        GtkTreeIter iter;
        gtk_list_store_append (dialog->image_model, &iter);
        gtk_list_store_set (dialog->image_model, &iter, 0, image_names[i], -1);

        if (gtk_combo_box_get_active (dialog->image_combo) < 0)
            gtk_combo_box_set_active_iter (dialog->image_combo, &iter);
    }
}

MpLaunchDialog *
mp_launch_dialog_new (MpClient *client)
{
    MpLaunchDialog *dialog = g_object_new (MP_TYPE_LAUNCH_DIALOG, NULL);

    dialog->cancellable = g_cancellable_new ();
    dialog->client = g_object_ref (client);
    mp_client_find_async (dialog->client, dialog->cancellable, find_cb, dialog);

    return dialog;
}
