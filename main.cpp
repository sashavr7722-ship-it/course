#include <gtk/gtk.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <thread>
#include <mutex>
#include <ctime>
#include <algorithm>
#include "checker.h"

struct AppData {
    GtkWidget *window;
    GtkWidget *file_chooser_button;
    GtkWidget *login_entry;
    GtkWidget *check_single_button;
    GtkWidget *result_textview;
    GtkWidget *status_label;
    GtkWidget *security_level_box;
    GtkWidget *security_level_label;
    GtkWidget *security_level_indicator;
    GtkTextBuffer *text_buffer;
    std::string current_file;
    std::mutex text_mutex;
};

struct SecurityColors {
    static const char* GREEN_BG;
    static const char* YELLOW_BG;
    static const char* RED_BG;
    static const char* GREEN_TEXT;
    static const char* YELLOW_TEXT;
    static const char* RED_TEXT;
};

const char* SecurityColors::GREEN_BG = "#4CAF50";
const char* SecurityColors::YELLOW_BG = "#f8bc07";
const char* SecurityColors::RED_BG = "#F44336";
const char* SecurityColors::GREEN_TEXT = "БЕЗОПАСНО";
const char* SecurityColors::YELLOW_TEXT = "СРЕДНИЙ РИСК";
const char* SecurityColors::RED_TEXT = "ВЫСОКИЙ РИСК";

void updateSecurityIndicator(AppData *app_data, int weight) {
    GdkRGBA color;
    
    if (weight == 0) {
        gdk_rgba_parse(&color, SecurityColors::GREEN_BG);
        gtk_widget_override_background_color(app_data->security_level_indicator, GTK_STATE_FLAG_NORMAL, &color);
        gtk_label_set_text(GTK_LABEL(app_data->security_level_label), SecurityColors::GREEN_TEXT);
    } else if (weight < 20) {
        gdk_rgba_parse(&color, SecurityColors::YELLOW_BG);
        gtk_widget_override_background_color(app_data->security_level_indicator, GTK_STATE_FLAG_NORMAL, &color);
        gtk_label_set_text(GTK_LABEL(app_data->security_level_label), SecurityColors::YELLOW_TEXT);
    } else {
        gdk_rgba_parse(&color, SecurityColors::RED_BG);
        gtk_widget_override_background_color(app_data->security_level_indicator, GTK_STATE_FLAG_NORMAL, &color);
        gtk_label_set_text(GTK_LABEL(app_data->security_level_label), SecurityColors::RED_TEXT);
    }
}

void updateResults(AppData *app_data, 
    const std::vector<std::tuple<std::string, std::string, int>>& results, int weight) {
    
    std::lock_guard<std::mutex> lock(app_data->text_mutex);
    
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(app_data->text_buffer, &iter);
    
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    char timestamp[100];
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm);
    
    gtk_text_buffer_insert(app_data->text_buffer, &iter, 
        "═══════════════════════════════════════════════════════════════\n", -1);
    gtk_text_buffer_insert(app_data->text_buffer, &iter, "🕐 ", -1);
    gtk_text_buffer_insert(app_data->text_buffer, &iter, timestamp, -1);
    gtk_text_buffer_insert(app_data->text_buffer, &iter, "\n", -1);
    
    for (const auto& result : results) {
        std::string line = "• " + std::get<0>(result) + ": " + std::get<1>(result) + "\n";
        
        if (std::get<2>(result) >= 8) {
            line = "🔴 " + line;
        } else if (std::get<2>(result) >= 4) {
            line = "🟡 " + line;
        } else if (std::get<2>(result) > 0) {
            line = "🟢 " + line;
        } else {
            line = "⚪ " + line;
        }
        
        gtk_text_buffer_insert(app_data->text_buffer, &iter, line.c_str(), -1);
    }
    
    gtk_text_buffer_insert(app_data->text_buffer, &iter, "\n", -1);
    
    updateSecurityIndicator(app_data, weight);
}

void on_check_single_clicked(GtkWidget *widget, gpointer data) {
    AppData *app_data = static_cast<AppData*>(data);
    
    const char *login = gtk_entry_get_text(GTK_ENTRY(app_data->login_entry));
    if (strlen(login) == 0) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app_data->window),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_WARNING,
            GTK_BUTTONS_OK,
            "⚠️ Введите логин для проверки!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    std::string login_copy(login);
    
    std::thread([app_data, login_copy]() {
        auto [results, weight] = checkSingleLogin(login_copy);
        
        struct CallbackData {
            AppData *app_data;
            std::vector<std::tuple<std::string, std::string, int>> results;
            int weight;
        };
        
        CallbackData *cb_data = new CallbackData{app_data, results, weight};
        
        g_idle_add([](gpointer user_data) -> gboolean {
            CallbackData *data = static_cast<CallbackData*>(user_data);
            updateResults(data->app_data, data->results, data->weight);
            delete data;
            return FALSE;
        }, cb_data);
    }).detach();
}

void on_check_all_clicked(GtkWidget *widget, gpointer data) {
    AppData *app_data = static_cast<AppData*>(data);
    
    if (app_data->current_file.empty()) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app_data->window),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_WARNING,
            GTK_BUTTONS_OK,
            "⚠️ Сначала выберите файл!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    std::string filename_copy = app_data->current_file;
    
    std::thread([app_data, filename_copy]() {
        auto [results, weight] = checkAllLoginsInFile(filename_copy);
        
        struct CallbackData {
            AppData *app_data;
            std::vector<std::tuple<std::string, std::string, int>> results;
            int weight;
        };
        
        CallbackData *cb_data = new CallbackData{app_data, results, weight};
        
        g_idle_add([](gpointer user_data) -> gboolean {
            CallbackData *data = static_cast<CallbackData*>(user_data);
            updateResults(data->app_data, data->results, data->weight);
            
            // Показываем уведомление о сохранении файла
            std::string status = "✅ Проверка завершена! Результаты сохранены в файл отчёта.";
            gtk_label_set_text(GTK_LABEL(data->app_data->status_label), status.c_str());
            
            delete data;
            return FALSE;
        }, cb_data);
    }).detach();
}

void on_clear_clicked(GtkWidget *widget, gpointer data) {
    AppData *app_data = static_cast<AppData*>(data);
    gtk_text_buffer_set_text(app_data->text_buffer, "", -1);
    updateSecurityIndicator(app_data, 0);
    gtk_label_set_text(GTK_LABEL(app_data->status_label), "🧹 Результаты очищены");
}

void on_file_selected(GtkFileChooserButton *button, gpointer data) {
    AppData *app_data = static_cast<AppData*>(data);
    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(button));
    app_data->current_file = filename;
    g_free(filename);
    
    std::string status = "📁 Выбран файл: " + app_data->current_file;
    gtk_label_set_text(GTK_LABEL(app_data->status_label), status.c_str());
}

void apply_css_style() {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "button {"
        "   background-image: none;"
        "   border-radius: 5px;"
        "   padding: 8px 15px;"
        "   font-weight: bold;"
        "}"
        "button:hover {"
        "   background-color: #45a049;"
        "}"
        "entry {"
        "   border-radius: 5px;"
        "   padding: 8px;"
        "   border: 2px solid #ddd;"
        "}"
        "entry:focus {"
        "   border-color: #4CAF50;"
        "}"
        "textview {"
        "   font-family: monospace;"
        "   font-size: 10pt;"
        "}"
        ".security-indicator {"
        "   border-radius: 10px;"
        "   padding: 10px;"
        "   font-weight: bold;"
        "   color: white;"
        "}", -1, NULL);
    
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    apply_css_style();
    
    AppData *app_data = new AppData();
    
    app_data->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app_data->window), "Login Security Checker");
    gtk_window_set_default_size(GTK_WINDOW(app_data->window), 900, 700);
    gtk_container_set_border_width(GTK_CONTAINER(app_data->window), 15);
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(app_data->window), vbox);
    
    GtkWidget *title_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title_label), 
        "<span size='xx-large' weight='bold'>🛡️ Проверка логина на социальную инженерию</span>");
    gtk_box_pack_start(GTK_BOX(vbox), title_label, FALSE, FALSE, 10);
    
    app_data->security_level_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), app_data->security_level_box, FALSE, FALSE, 5);
    
    GtkWidget *security_frame = gtk_frame_new("Уровень безопасности");
    gtk_box_pack_start(GTK_BOX(app_data->security_level_box), security_frame, TRUE, TRUE, 0);
    
    app_data->security_level_indicator = gtk_event_box_new();
    gtk_widget_set_size_request(app_data->security_level_indicator, -1, 50);
    gtk_container_add(GTK_CONTAINER(security_frame), app_data->security_level_indicator);
    
    app_data->security_level_label = gtk_label_new(SecurityColors::GREEN_TEXT);
    gtk_label_set_markup(GTK_LABEL(app_data->security_level_label), 
        "<span size='large' weight='bold'>БЕЗОПАСНО</span>");
    gtk_container_add(GTK_CONTAINER(app_data->security_level_indicator), app_data->security_level_label);
    
    GtkWidget *control_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), control_box, FALSE, FALSE, 5);
    
    GtkWidget *file_frame = gtk_frame_new("Файл с логинами");
    gtk_box_pack_start(GTK_BOX(control_box), file_frame, TRUE, TRUE, 0);
    
    GtkWidget *file_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(file_frame), file_vbox);
    
    app_data->file_chooser_button = gtk_file_chooser_button_new("Выберите файл",
        GTK_FILE_CHOOSER_ACTION_OPEN);
    gtk_box_pack_start(GTK_BOX(file_vbox), app_data->file_chooser_button, TRUE, TRUE, 0);
    
    GtkWidget *login_frame = gtk_frame_new("Логин для проверки");
    gtk_box_pack_start(GTK_BOX(control_box), login_frame, TRUE, TRUE, 0);
    
    GtkWidget *login_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(login_frame), login_vbox);
    
    app_data->login_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app_data->login_entry), "Введите логин...");
    gtk_box_pack_start(GTK_BOX(login_vbox), app_data->login_entry, TRUE, TRUE, 0);
    
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 5);
    
    app_data->check_single_button = gtk_button_new_with_label("🔍 Проверить один логин");
    gtk_box_pack_start(GTK_BOX(button_box), app_data->check_single_button, TRUE, TRUE, 0);
    
    GtkWidget *check_all_button = gtk_button_new_with_label("📊 Проверить ВСЕ логины в файле");
    gtk_box_pack_start(GTK_BOX(button_box), check_all_button, TRUE, TRUE, 0);
    
    GtkWidget *clear_button = gtk_button_new_with_label("🗑️ Очистить");
    gtk_box_pack_start(GTK_BOX(button_box), clear_button, FALSE, FALSE, 0);
    
    GtkWidget *results_frame = gtk_frame_new("Результаты проверки");
    gtk_box_pack_start(GTK_BOX(vbox), results_frame, TRUE, TRUE, 5);
    
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(results_frame), scrolled);
    
    app_data->result_textview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app_data->result_textview), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(app_data->result_textview), GTK_WRAP_WORD);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(app_data->result_textview), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled), app_data->result_textview);
    
    app_data->text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app_data->result_textview));
    
    app_data->status_label = gtk_label_new("👋 Добро пожаловать! Выберите файл или введите логин для проверки");
    gtk_label_set_xalign(GTK_LABEL(app_data->status_label), 0);
    gtk_box_pack_start(GTK_BOX(vbox), app_data->status_label, FALSE, FALSE, 5);
    
    g_signal_connect(app_data->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(app_data->file_chooser_button, "file-set", G_CALLBACK(on_file_selected), app_data);
    g_signal_connect(app_data->check_single_button, "clicked", G_CALLBACK(on_check_single_clicked), app_data);
    g_signal_connect(check_all_button, "clicked", G_CALLBACK(on_check_all_clicked), app_data);
    g_signal_connect(clear_button, "clicked", G_CALLBACK(on_clear_clicked), app_data);
    
    updateSecurityIndicator(app_data, 0);
    
    gtk_widget_show_all(app_data->window);
    
    gtk_main();
    
    delete app_data;
    return 0;
}