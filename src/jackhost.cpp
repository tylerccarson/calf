/* Calf DSP Library
 * Standalone application module wrapper example.
 * Copyright (C) 2007 Krzysztof Foltman
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <config.h>
#include <jack/jack.h>
#include <calf/giface.h>
#include <calf/modules.h>
#include <gtk/gtk.h>
#include <phat/phat.h>

using namespace synth;

class jack_host_gui;

struct jack_host_param
{
    jack_host_gui *gui;
    int param_no;
    GtkWidget *label;
};

class jack_host_gui
{
protected:
    int param_count;
public:
    GtkWidget *toplevel;
    GtkWidget *table;
    synth::jack_host_base *host;
    jack_host_param *params;

    jack_host_gui(GtkWidget *_toplevel);
    void create(synth::jack_host_base *_host, const char *title);
    static void knob_value_changed(PhatKnob *widget, gpointer value);
    static void combo_value_changed(GtkComboBox *widget, gpointer value);
};

jack_host_gui::jack_host_gui(GtkWidget *_toplevel)
: toplevel(_toplevel)
{
    
}

void jack_host_gui::knob_value_changed(PhatKnob *widget, gpointer value)
{
    jack_host_param *jhp = (jack_host_param *)value;
    jack_host_base &jh = *jhp->gui->host;
    const parameter_properties &props = jh.get_param_props()[jhp->param_no];
    int nparam = jhp->param_no;
    float cvalue = props.from_01 (phat_knob_get_value (widget));
    jh.get_params()[nparam] = cvalue;
    // printf("%d -> %f\n", nparam, cvalue);
    jh.set_params();
    gtk_label_set_text (GTK_LABEL (jhp->label), props.to_string(cvalue).c_str());
}

void jack_host_gui::combo_value_changed(GtkComboBox *widget, gpointer value)
{
    jack_host_param *jhp = (jack_host_param *)value;
    jack_host_base &jh = *jhp->gui->host;
    int nparam = jhp->param_no;
    jh.get_params()[nparam] = gtk_combo_box_get_active (widget) + jh.get_param_props()[jhp->param_no].min;
    jh.set_params();
}

void jack_host_gui::create(synth::jack_host_base *_host, const char *title)
{
    host = _host;
    param_count = host->get_param_count();
    params = new jack_host_param[param_count];
    
    for (int i = 0; i < param_count; i++) {
        params[i].gui = this;
        params[i].param_no = i;
    }

    table = gtk_table_new (param_count + 1, 3, FALSE);
    
    GtkWidget *title_label = gtk_label_new ("");
    gtk_label_set_markup (GTK_LABEL (title_label), (string("<b>")+title+"</b>").c_str());
    gtk_table_attach (GTK_TABLE (table), title_label, 0, 3, 0, 1, GTK_EXPAND, GTK_EXPAND, 2, 2);

    for (int i = 0; i < param_count; i++) {
        GtkWidget *label = gtk_label_new (host->get_param_names()[host->get_input_count() + host->get_output_count() + i]);
        gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, i + 1, i + 2, GTK_FILL, GTK_EXPAND, 2, 2);
        
        parameter_properties &props = host->get_param_props()[i];
        
        GtkWidget *widget;
        
        if ((props.flags & PF_TYPEMASK) == PF_ENUM && 
            (props.flags & PF_CTLMASK) == PF_CTL_COMBO)
        {
            widget  = gtk_combo_box_new_text ();
            for (int j = (int)props.min; j <= (int)props.max; j++)
                gtk_combo_box_append_text (GTK_COMBO_BOX (widget), props.choices[j]);
            gtk_combo_box_set_active (GTK_COMBO_BOX (widget), (int)host->get_params()[i] - (int)props.min);
            gtk_signal_connect (GTK_OBJECT (widget), "changed", G_CALLBACK (combo_value_changed), (gpointer)&params[i]);
            gtk_table_attach (GTK_TABLE (table), widget, 1, 3, i + 1, i + 2, GTK_EXPAND, GTK_EXPAND, 0, 0);
        }
        else
        {
            GtkWidget *knob = phat_knob_new_with_range (host->get_param_props()[i].to_01 (host->get_params()[i]), 0, 1, 0.01);
            gtk_signal_connect (GTK_OBJECT (knob), "value-changed", G_CALLBACK (knob_value_changed), (gpointer)&params[i]);
            gtk_table_attach (GTK_TABLE (table), knob, 1, 2, i + 1, i + 2, GTK_EXPAND, GTK_EXPAND, 0, 0);

            widget  = gtk_label_new ("");
            gtk_label_set_width_chars (GTK_LABEL (widget), 12);
            gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
            gtk_table_attach (GTK_TABLE (table), widget, 2, 3, i + 1, i + 2, GTK_FILL, GTK_EXPAND, 0, 0);
            params[i].label = widget;
            // gtk_widget_set_size_request (widget, widget->allocation.width, widget->allocation.height);
            // gtk_widget_set_size_request (widget, 100, 10);
            
            knob_value_changed (PHAT_KNOB (knob), (gpointer)&params[i]);
        }
    }
    gtk_container_add (GTK_CONTAINER (toplevel), table);
}

// I don't need anyone to tell me this is stupid. I already know that :)
jack_host_gui *gui;

//float par_values[1] = { 0.3 };

void destroy(GtkWindow *window, gpointer data)
{
    gtk_main_quit();
}

void make_gui(jack_host_base *jh, const char *title, const char *effect)
{
    GtkWidget *toplevel = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (toplevel), title);
        
    gui = new jack_host_gui(toplevel);
    gui->create(jh, effect);

    gtk_signal_connect (GTK_OBJECT(toplevel), "destroy", G_CALLBACK(destroy), NULL);
    gtk_widget_show_all(toplevel);
    
}

static struct option long_options[] = {
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'v'},
    {"client", 1, 0, 'c'},
    {"effect", 1, 0, 'e'},
    {"input", 1, 0, 'i'},
    {"output", 1, 0, 'o'},
    {0,0,0,0},
};

const char *effect_name = "flanger";
const char *client_name = "calfhost";
const char *input_name = "input";
const char *output_name = "output";

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);
    while(1) {
        int option_index;
        int c = getopt_long(argc, argv, "c:e:i:o:hv", long_options, &option_index);
        if (c == -1)
            break;
        switch(c) {
            case 'h':
            case '?':
                printf("JACK host for Calf effects\n"
                    "Syntax: %s [--effect reverb|flanger|filter] [--client <name>] [--input <name>] [--output <name>] [--help] [--version]\n", 
                    argv[0]);
                return 0;
            case 'v':
                printf("%s\n", PACKAGE_STRING);
                return 0;
            case 'e':
                effect_name = optarg;
                break;
            case 'c':
                client_name = optarg;
                break;
            case 'i':
                input_name = optarg;
                break;
            case 'o':
                output_name = optarg;
                break;
        }
    }
    try {
        jack_host_base *jh = NULL;
        if (!strcmp(effect_name, "reverb"))
            jh = new jack_host<reverb_audio_module>();
        else if (!strcmp(effect_name, "flanger"))
            jh = new jack_host<flanger_audio_module>();
        else if (!strcmp(effect_name, "filter"))
            jh = new jack_host<filter_audio_module>();
        else {
            fprintf(stderr, "Unknown filter name; allowed are: reverb, flanger, filter\n");
            return 1;
        }
        jh->open(client_name, input_name, output_name);
        make_gui(jh, client_name, effect_name);
        gtk_main();
        delete gui;
        jh->close();
        delete jh;
    }
    catch(std::exception &e)
    {
        fprintf(stderr, "%s\n", e.what());
        exit(1);
    }
    return 0;
}
