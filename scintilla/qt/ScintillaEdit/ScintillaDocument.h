// ScintillaDocument.h
// Wrapper for Scintilla document object so it can be manipulated independently.
// Copyright (c) 2011 Archaeopteryx Software, Inc. d/b/a Wingware

#ifndef SCINTILLADOCUMENT_H
#define SCINTILLADOCUMENT_H

#include <QObject>

class WatcherHelper;

#ifndef EXPORT_IMPORT_API
#ifdef WIN32
#ifdef MAKING_LIBRARY
#define EXPORT_IMPORT_API __declspec(dllexport)
#else
// Defining dllimport upsets moc
#define EXPORT_IMPORT_API __declspec(dllimport)
//#define EXPORT_IMPORT_API
#endif
#else
#define EXPORT_IMPORT_API
#endif
#endif

class EXPORT_IMPORT_API ScintillaDocument : public QObject
{
    Q_OBJECT

    void *pdoc;
    WatcherHelper *docWatcher;

public:
    explicit ScintillaDocument(QObject *parent = 0, void *pdoc_=0);
    virtual ~ScintillaDocument();
    void *pointer();

    int line_from_position(int pos);
    bool is_cr_lf(int pos);
    bool delete_chars(int pos, int len);
    int undo();
    int redo();
    bool can_undo();
    bool can_redo();
    void delete_undo_history();
    bool set_undo_collection(bool collect_undo);
    bool is_collecting_undo();
    void begin_undo_action();
    void end_undo_action();
    void set_save_point();
    bool is_save_point();
    void set_read_only(bool read_only);
    bool is_read_only();
    void insert_string(int position, QByteArray &str);
    QByteArray get_char_range(int position, int length);
    char style_at(int position);
    int line_start(int lineno);
    int line_end(int lineno);
    int line_end_position(int pos);
    int length();
    int lines_total();
    void start_styling(int position);
    bool set_style_for(int length, char style);
    int get_end_styled();
    void ensure_styled_to(int position);
    void set_current_indicator(int indic);
    void decoration_fill_range(int position, int value, int fillLength);
    int decorations_value_at(int indic, int position);
    int decorations_start(int indic, int position);
    int decorations_end(int indic, int position);
    int get_code_page();
    void set_code_page(int code_page);
    int get_eol_mode();
    void set_eol_mode(int eol_mode);
    int move_position_outside_char(int pos, int move_dir, bool check_line_end);

    int get_character(int pos); // Calls GetCharacterAndWidth(pos, NULL)

signals:
    void modify_attempt();
    void save_point(bool atSavePoint);
    void modified(int position, int modification_type, const QByteArray &text, int length,
                  int linesAdded, int line, int foldLevelNow, int foldLevelPrev);
    void style_needed(int pos);
    void lexer_changed();
    void error_occurred(int status);

    friend class ::WatcherHelper;
};

#endif // SCINTILLADOCUMENT_H
