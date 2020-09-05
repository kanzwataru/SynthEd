#include "main_test.h"
#include "notes.h"
#include "playback.h"

#include <imgui.h>
#include <SDL.h>

#include <cmath>
#include <cstdio>
#include <algorithm>
#include <thread>
#include <atomic>
#include <vector>

#define countof(x) sizeof((x)) / sizeof(x[0])

enum Lengths {
    L_LEAST = 1,

    L_SIXT  = L_LEAST,
    L_EIGT  = L_SIXT * 2,
    L_QUART = L_EIGT * 2,
    L_HALF  = L_QUART * 2,
    L_WHOLE = L_HALF * 2
};

struct NoteBlock {
    Note note;
    int  time;
    int  duration;
};

struct NoteEdit {
    NoteBlock *block;
    NoteBlock prev_block;
    NoteBlock new_block;
};

/*
static NoteBlock section_notes[] = {
    {{N_C, 4}, 0, L_WHOLE},
    {{N_A, 4}, 4, L_HALF},
    {{N_B, 7}, 10, L_WHOLE}
};
*/

// TEMP test song
#define X(o, n, l) {{n, o}, 0, l}
static std::vector<NoteBlock> song = {
    X(3, N_B,  L_EIGT + L_EIGT),
    X(4, N_CS, L_EIGT),
    X(4, N_D,  L_EIGT),
    X(4, N_A,  L_EIGT),
    X(4, N_FS, L_QUART),
    X(4, N_FS, L_EIGT),
    X(4, N_E,  L_SIXT),
    X(4, N_FS, L_SIXT + L_HALF + L_QUART),

    X(4, N_E,  L_EIGT),
    X(4, N_FS, L_EIGT),
    X(4, N_A,  L_EIGT), //
    X(4, N_FS, L_EIGT),
    X(4, N_CS, L_EIGT),
    X(4, N_D,  L_EIGT),
    X(4, N_CS, L_QUART),
    X(4, N_CS, L_EIGT),
    X(3, N_B,  L_SIXT),
    X(3, N_FS, L_SIXT + L_HALF + L_QUART + L_EIGT),

    X(3, N_B,  L_EIGT + L_EIGT),
    X(4, N_CS, L_EIGT),
    X(4, N_D,  L_EIGT),
    X(4, N_A,  L_EIGT),
    X(4, N_FS, L_QUART),
    X(4, N_FS, L_EIGT),
    X(4, N_E,  L_SIXT),
    X(4, N_FS, L_SIXT + L_HALF + L_QUART),

    X(4, N_E,  L_EIGT),
    X(4, N_FS, L_EIGT),
    X(4, N_A,  L_EIGT), //
    X(4, N_FS, L_EIGT),
    X(4, N_A,  L_EIGT),
    X(5, N_D,  L_EIGT),
    X(5, N_CS, L_EIGT + L_SIXT),
    X(4, N_B,  L_EIGT + L_SIXT),
    X(4, N_FS, L_EIGT + L_QUART + L_EIGT),
};
#undef X

static NoteEdit edit = {};
static bool currently_editing = false;

static int to_note(int n) { return (N_TOTAL - 1) - n; }
static float zoom = 1.0f;

static Instrument instruments[VOICE_COUNT];

static std::thread playback_thread;
static std::atomic<bool> playing;
static std::atomic<int> playhead;

static const int note_count = N_TOTAL;
static const int octave_count = 8;

static size_t find_closest_note(const NoteBlock *notes, size_t count, int time)
{
    assert(count > 0);

    int delta = time - notes[0].time;
    size_t idx = 0;

    for(size_t i = 0; i < count; ++i) {
        int this_delta = time - notes[i].time;
        if(abs(this_delta) < abs(delta)) {
            delta = this_delta;
            idx = i;
        }
    }

    return idx;
}

static inline bool operator<(const NoteBlock &lhs, const NoteBlock &rhs)
{
    return lhs.time < rhs.time;
}

static void play_song(const NoteBlock *notes, size_t count)
{
    if(count == 0) {
        playing = false;
        return;
    }

    //TODO: make this less awkward and don't use STL maybe?
    std::vector<NoteBlock> sorted;
    sorted.resize(count);
    memcpy(sorted.data(), notes, count * sizeof(*notes));
    std::sort(sorted.begin(), sorted.end());
    notes = sorted.data();

    int track_end = notes[count - 1].time + notes[count - 1].duration;
    const NoteBlock *n = &notes[find_closest_note(notes, count, playhead)];

    while(playing && playhead < track_end && n < (notes + count)) {
        if(playhead == n->time) {
            playback_play_note(0, n->note);
        }
        else if(playhead == n->time + n->duration) {
            ++n;

            if(n < (notes + count)) {
                if(playhead == n->time) {
                    playback_play_note(0, n->note);
                }
                else {
                    playback_stop(0);
                }
            }
            else {
                playback_stop(0);
                break;
            }
        }

        SDL_Delay(105);
        ++playhead;
    }

    playback_stop(0);
    playing = false;
}

static void ui_top_bar()
{
    ImGui::SliderFloat("Zoom", &zoom, 0.1f, 6.0f);

    const ImVec2 button_dim = {48.0f, 32.0f};
    if(ImGui::Button("|<", button_dim)) {
        playhead = 0;
    }
    ImGui::SameLine();

    if(playing) {
        if(ImGui::Button("||", button_dim)) {
            playing = false;
            if(playback_thread.joinable()) {
                playback_thread.join();
            }
            playback_stop(0);
        }
    }
    else {
        if(ImGui::Button(">", button_dim)) {
            playing = true;
            playback_thread = std::thread([](){
                play_song(song.data(), song.size());
            });
        }
    }
    ImGui::SameLine();

    ImGui::Button(">|", button_dim);
}

void test_editor_init()
{
    playback_registers_clear();
    for(int i = 0; i < VOICE_COUNT; ++i) {
        playback_instrument_set(instruments[i], i);
    }

    // TEMP: lay out notes in test song
    int time = 0;
    for(auto &block : song) {
        block.time = time;
        time += block.duration;
    }
}

struct UINoteMetrics {
    int whole_note_count;
    float cell_height;
    float cell_width;
    float cell_single;

    //TODO: should style not be in a seperate struct?
    float note_rounding;
};

struct UINoteMouseCoords {
    ImVec2 offset;
    float button_percent;

    ImVec2 startpos;
    ImVec2 endpos;
    int block_x;
    int block_y;
};

struct UICtx {
    ImVec2 p; // cursor
    ImDrawList *draw_list;
};

static UINoteMouseCoords calc_mouse_note_coords(int duration, const ImVec2 &lp, const UICtx &ui, const UINoteMetrics &metrics, const ImVec2 &minp = {0.0f, 0.0f}, const ImVec2 &maxp = {0.0f, 0.0f})
{
    ImGuiIO &io = ImGui::GetIO();
    UINoteMouseCoords mouse;

    const float delta = maxp.x - minp.x;

    if(std::abs(delta) > 0.001f) {
        mouse.offset = {
            (io.MouseClickedPos[0].x - ((ui.p.x - lp.x) + minp.x)),
            (io.MouseClickedPos[0].y - ((ui.p.y - lp.y) + minp.y))
        };
        mouse.button_percent = mouse.offset.x / (maxp.x - minp.x);
    }
    else {
        // this isn't quite the same behaviour though...
        mouse.offset = {0.0f, 0.0f};
        mouse.button_percent = 0.0f;
    }

    mouse.startpos = {
        io.MousePos.x - mouse.offset.x,
        io.MousePos.y - mouse.offset.y
    };
    mouse.endpos = {
        mouse.startpos.x + (duration * metrics.cell_single),
        mouse.startpos.y + (metrics.cell_height)
    };

    mouse.block_x = std::round((mouse.startpos.x - ui.p.x) / (metrics.cell_single));
    mouse.block_y = std::round((mouse.startpos.y - ui.p.y) / (metrics.cell_height));

    return mouse;
}

static void note_from_mouse_pos(NoteBlock *note, int block_x, int block_y)
{
    note->note.note = N_TOTAL - 1 - (block_y % N_TOTAL);
    note->note.octave = octave_count - 1 - (block_y / N_TOTAL);
    note->time = block_x;
    assert(note->note.note >= 0 && note->note.note < N_TOTAL);
    assert(note->note.octave >= 0 && note->note.octave < octave_count);
}

static void calc_note_extents(ImVec2 &minp, ImVec2 &maxp, const NoteBlock &note, const ImVec2 &lp, const UINoteMetrics &metrics)
{
    const float octave_offset = (N_TOTAL * (octave_count - 1 - note.note.octave));

    minp = {
        lp.x + note.time * metrics.cell_single,
        lp.y + (to_note(note.note.note) + octave_offset) * metrics.cell_height
    };
    maxp = {
        lp.x + (note.time * metrics.cell_single) + (note.duration * metrics.cell_single),
        lp.y + (to_note(note.note.note) + 1 + octave_offset) * metrics.cell_height
    };
}

static void draw_note_drag(float duration, const UINoteMouseCoords &mouse, const UICtx &ui, const UINoteMetrics &metrics)
{
    ui.draw_list->AddRectFilled(
        {ui.p.x + (mouse.block_x * metrics.cell_single), ui.p.y + mouse.block_y * metrics.cell_height},
        {ui.p.x + ((mouse.block_x + duration) * metrics.cell_single), ui.p.y + (mouse.block_y + 1) * metrics.cell_height},
        ImU32(ImColor{0.35f, 0.35f, 0.35f, 0.5f}),
        metrics.note_rounding
    );
}

//TODO: Clip timing to beg/end of canvas
static inline bool is_in_song_extents(int block_x, int block_y)
{
    return block_y >= 0 && block_y < note_count * octave_count;
}

static bool ui_note_with_edit(NoteBlock *note, const UINoteMetrics &metrics, const UICtx &ui)
{
    ImGuiIO &io = ImGui::GetIO();

    bool edited = false; // return value

    // compute extents for box
    const ImVec2 lp = ImGui::GetCursorPos();
    ImVec2 minp, maxp;
    calc_note_extents(minp, maxp, *note, lp, metrics);

    // style
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, metrics.note_rounding);
    ImGui::PushStyleColor(ImGuiCol_Button,        ImU32(ImColor{0.75f, 0.45f, 0.35f, 1.0f}));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImU32(ImColor{0.75f, 0.25f, 0.15f, 1.0f}));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImU32(ImColor{0.95f, 0.55f, 0.45f, 1.0f}));

    ImGui::SetCursorPos(minp);
    ImGui::Button(note_name_table[note->note.note], {maxp.x - minp.x, maxp.y - minp.y});

    // set mouse cursor for resizing
    const float drag_handle_begin = 0.25f;
    const float drag_handle_end = 0.85f;
    if(ImGui::IsItemHovered() && !ImGui::IsItemActive()) {
        UINoteMouseCoords mouse = calc_mouse_note_coords(0, lp, ui, metrics, minp, maxp);

        if(mouse.button_percent >= drag_handle_end || mouse.button_percent <= drag_handle_begin) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }
    }

    // drag for EITHER move or resize
    if(ImGui::IsItemActive()) {
        UINoteMouseCoords mouse = calc_mouse_note_coords(note->duration, lp, ui, metrics, minp, maxp);

        edited = true;

        const bool should_move = mouse.button_percent > drag_handle_begin && mouse.button_percent < drag_handle_end;
        if(should_move) {
            // move note
            if(is_in_song_extents(mouse.block_x, mouse.block_y)) {
                edit.block = note;
                edit.prev_block = *note;
                edit.new_block = *note;

                note_from_mouse_pos(&edit.new_block, mouse.block_x, mouse.block_y);
                draw_note_drag(note->duration, mouse, ui, metrics);
            }
        }
        else {
            // resize note
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

            const ImVec2 delta = ImGui::GetMouseDragDelta();
            const int note_delta = round(delta.x / metrics.cell_single);

            edit.block = note;
            edit.prev_block = *note;
            edit.new_block = *note;
            if(mouse.button_percent >= drag_handle_end) {
                edit.new_block.duration = std::max(1, note->duration + note_delta);
            }
            else if(mouse.button_percent <= drag_handle_end){
                edit.new_block.duration = std::max(1, note->duration - note_delta);
                edit.new_block.time = std::max(0, std::min(note->time + note_delta, note->time + note->duration));
            }

            //TODO: clean up copy-pasta
            const ImVec2 minp_screen = {
                ui.p.x + edit.new_block.time * metrics.cell_single,
                ui.p.y + (to_note(edit.new_block.note.note) + (N_TOTAL * (octave_count - 1 - edit.new_block.note.octave))) * metrics.cell_height
            };
            const ImVec2 maxp_screen = {
                ui.p.x + (edit.new_block.time * metrics.cell_single) + (edit.new_block.duration * metrics.cell_single),
                ui.p.y + (to_note(edit.new_block.note.note) + 1 + (N_TOTAL * (octave_count - 1 - edit.new_block.note.octave))) * metrics.cell_height
            };

            ui.draw_list->AddRectFilled(
                minp_screen,
                maxp_screen,
                ImU32(ImColor{0.35f, 0.35f, 0.35f, 0.75f}),
                metrics.note_rounding
            );
        }
    }

    ImGui::SetCursorPos(lp);

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    return edited;
}

static bool ui_note_add(NoteBlock *note, const UINoteMetrics &metrics, const UICtx &ui)
{
    ImGuiIO &io = ImGui::GetIO();

    const bool adding = io.KeyCtrl;;
    const bool done_adding = adding && io.MouseReleased[0];

    if(adding) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);

        const ImVec2 lp = ImGui::GetCursorPos();
        auto mouse = calc_mouse_note_coords(note->duration, lp, ui, metrics);

        note_from_mouse_pos(note, mouse.block_x, mouse.block_y);
        draw_note_drag(note->duration, mouse, ui, metrics);

        return done_adding;
    }

    return false;
}

void test_editor()
{
    ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_NoScrollbar);

    ui_top_bar();

    ImDrawList *root_draw_list = ImGui::GetWindowDrawList();

    const float piano_width = 50.0f;

    const ImU32 grey_col = ImColor(0.25f, 0.25f, 0.25f, 0.5f);
    const ImU32 white_key_bg_col = ImColor(0.15f, 0.15f, 0.15f, 0.5f);
    const ImU32 bold_col = ImColor(0.5f, 0.5f, 0.5f, 0.75f);
    const ImGuiIO &io = ImGui::GetIO();
    const ImVec2 orig_pos = ImGui::GetCursorPos();
    const ImVec2 orig_pos_screen = ImGui::GetCursorScreenPos();

    UINoteMetrics metrics;
    metrics.whole_note_count = 256;
    metrics.cell_height = 20.0f;
    metrics.cell_width  = 92.0f * zoom;
    metrics.cell_single = metrics.cell_width / L_WHOLE;
    metrics.note_rounding = 8.0f; // style stuff, should be in a different struct?

    const ImVec2 overall_size = {(metrics.whole_note_count * metrics.cell_width), (note_count * octave_count) * metrics.cell_height};

    ImGui::SetCursorPos({orig_pos.x + piano_width, orig_pos.y});

    // * grid
    ImGui::BeginChild("EditorArea", {0, 0}, true, ImGuiWindowFlags_HorizontalScrollbar);

    UICtx ui;
    ui.draw_list = ImGui::GetWindowDrawList();
    ui.p = ImGui::GetCursorScreenPos();

    // background fill
    for(int note = 0; note < note_count; ++note) {
        for(int octave = 0; octave < octave_count; ++octave) {
            int y = note + (note_count * octave);

            ui.draw_list->AddLine(
                {ui.p.x + 0, ui.p.y + y * metrics.cell_height},
                {ui.p.x + metrics.whole_note_count * metrics.cell_width, ui.p.y + y * metrics.cell_height},
                grey_col
            );

            if(note_key_type_table[to_note(note)] == KEY_WHITE) {
                ui.draw_list->AddRectFilled(
                    {ui.p.x + 0, ui.p.y + y * metrics.cell_height},
                    {ui.p.x + metrics.whole_note_count * metrics.cell_width, ui.p.y + (y + 1) * metrics.cell_height},
                    white_key_bg_col
                );
            }
        }
    }

    // minor lines
    for(int x = 0; x < metrics.whole_note_count; ++x) {
        ui.draw_list->AddLine(
            {ui.p.x + x * metrics.cell_width, ui.p.y + 0},
            {ui.p.x + x * metrics.cell_width, ui.p.y + (note_count * octave_count * metrics.cell_height)},
            bold_col
        );
    }

    // major lines
    for(int x = 0; x < metrics.whole_note_count * 4; ++x) {
        ui.draw_list->AddLine(
            {ui.p.x + x * metrics.cell_width / 4, ui.p.y + 0},
            {ui.p.x + x * metrics.cell_width / 4, ui.p.y + (note_count * octave_count * metrics.cell_height)},
            grey_col
        );
    }

    // * notes
    // show notes and allow editing
    bool any_is_editing = false;
    for(size_t i = 0; i < song.size(); ++i) {
        ImGui::PushID(5000 + i); // TODO: global offset table/enums? Is this even necessary?

        const bool edited = ui_note_with_edit(&song[i], metrics, ui);
        any_is_editing = edited ? true : any_is_editing;

        // cleanup
        ImGui::PopID();
    }

    if(any_is_editing) {
        currently_editing = true;
    }
    else if(!any_is_editing && currently_editing) {
        if(edit.block) {
            *edit.block = edit.new_block;
            edit.block = nullptr;
            currently_editing = false;
        }
    }

    // add note
    NoteBlock note_to_add;
    note_to_add.duration = song.size() > 0 ? song.back().duration : L_WHOLE;

    const bool added_note = ui_note_add(&note_to_add, metrics, ui);
    if(added_note) {
        song.push_back(note_to_add);
    }

    // * playhead
    ImVec2 lp = ImGui::GetCursorPos();
    ui.p = ImGui::GetCursorScreenPos();

    const float playhead_offset = (playhead * metrics.cell_single);
    const ImVec2 playhead_dim = {metrics.cell_single * L_QUART, overall_size.y};
    ImGui::SetCursorPos({lp.x + playhead_offset, lp.y});

    ImGui::PushStyleColor(ImGuiCol_Button,        (ImU32)ImColor(0.50f, 0.50f, 0.50f, 0.35f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  (ImU32)ImColor(0.45f, 0.45f, 0.45f, 0.35f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImU32)ImColor(0.60f, 0.60f, 0.60f, 0.35f));
    ImGui::Button("", playhead_dim);
    ImGui::PopStyleColor(3);

    if(ImGui::IsItemActive() && !ImGui::IsItemHovered()) {
        // needs minp???
        const float mouse_offset = io.MousePos.x - ((ui.p.x - lp.x));

        const ImVec2 delta = ImGui::GetMouseDragDelta();
        if(abs(delta.x) > metrics.cell_single) {
            const int t_next = mouse_offset / metrics.cell_single;
            const int max_time = metrics.whole_note_count * L_WHOLE;
            if(t_next >= 0 && t_next < max_time) {
                playhead = t_next;
            }

            ImGui::ResetMouseDragDelta();
        }
    }

    //root_draw_list->AddLine({p.x + playhead_offset, p.y}, {p.x + playhead_offset, p.y + overall_size.y}, (ImU32)ImColor(1.0f, 0.0f, 0.0f, 1.0f), 2.0f);

    ImGui::SetCursorPos(lp);

    ImGui::Dummy(overall_size);

    const float scroll_y = ImGui::GetScrollY();

    ImGui::EndChild();
    ImGui::SameLine();

    // * piano
    root_draw_list->PushClipRect(
        orig_pos_screen,
        {orig_pos_screen.x + ImGui::GetWindowSize().x, orig_pos_screen.y + ImGui::GetWindowSize().y},
        true
    );

    ImGui::SetCursorPos({orig_pos.x, orig_pos.y - scroll_y + (metrics.cell_height / 2) - 1});
    lp = ImGui::GetCursorPos();
    ui.p = ImGui::GetCursorScreenPos();

    root_draw_list->AddRectFilled(
        ui.p,
        {ui.p.x + piano_width, ui.p.y + overall_size.y},
        ImU32(ImColor{0.65f, 0.65f, 0.65f, 1.0f})
    );

    for(int note = 0; note < note_count; ++note) {
        for(int octave = 0; octave < octave_count; ++octave) {
            int y = note + (note_count * octave);

            const char *note_name = note_name_table[to_note(note)];
            char str[5];
            if(to_note(note) == N_C) {
                assert(octave < 10);
                str[0] = '0' + (octave_count - 1 - octave);
            }
            else {
                str[0] = ' ';
            }
            str[1] = ' ';
            str[2] = note_name[0];
            str[3] = note_name[1];
            str[4] = '\0';

            /*
            root_draw_list->AddText(
                {p.x, p.y + y * cell_height + 2.0f},
                ImGui::GetColorU32(ImGuiCol_Text),
                str
            );
            */

            // TODO: Maybe need to do PushID???
            ImGui::PushID(y);
            ImGui::SetCursorPos({lp.x, lp.y + (y * metrics.cell_height)});
            float width = piano_width;

            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, {0.0f, 0.5f});
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
            if(note_key_type_table[to_note(note)] == KEY_WHITE) {
                ImGui::PushStyleColor(ImGuiCol_Button,        ImU32(ImColor{0.65f, 0.65f, 0.65f, 1.0f}));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImU32(ImColor{0.45f, 0.45f, 0.45f, 1.0f}));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImU32(ImColor{0.75f, 0.75f, 0.75f, 1.0f}));
                ImGui::PushStyleColor(ImGuiCol_Text,          ImU32(ImColor{0.05f, 0.05f, 0.05f, 1.0f}));
            }
            else {
                width *= 0.75f;
                ImGui::PushStyleColor(ImGuiCol_Button,        ImU32(ImColor{0.15f, 0.15f, 0.15f, 1.0f}));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImU32(ImColor{0.05f, 0.05f, 0.05f, 1.0f}));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImU32(ImColor{0.25f, 0.25f, 0.25f, 1.0f}));
                ImGui::PushStyleColor(ImGuiCol_Text,          ImU32(ImColor{0.95f, 0.95f, 0.95f, 1.0f}));
            }

            ImGui::Button(str, {width, metrics.cell_height});

            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar(2);
            ImGui::SetCursorPos(lp);
            ImGui::PopID();
        }
    }
    root_draw_list->PopClipRect();

    ImGui::End();

    // * join thread if needed
    if(!playing && playback_thread.joinable()) {
        playback_thread.join();
    }
}
