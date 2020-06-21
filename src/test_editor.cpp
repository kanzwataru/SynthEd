#include "main_test.h"
#include <imgui.h>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include "notes.h"

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

static NoteBlock section_notes[] = {
    {{N_C, 4}, 0, L_WHOLE},
    {{N_A, 4}, 4, L_HALF},
    {{N_B, 7}, 10, L_WHOLE}
};

static NoteEdit edit = {};
static bool was_editing = false;

static int to_note(int n) { return (N_TOTAL - 1) - n; }

void test_editor()
{
    static float s_zoom = 1.0f;

    ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_NoScrollbar);
    ImGui::SliderFloat("Zoom", &s_zoom, 0.1f, 6.0f);
    ImDrawList *root_draw_list = ImGui::GetWindowDrawList();

    const float piano_width = 50.0f;

    const ImU32 grey_col = ImColor(0.25f, 0.25f, 0.25f, 0.5f);
    const ImU32 white_key_bg_col = ImColor(0.15f, 0.15f, 0.15f, 0.5f);
    const ImU32 bold_col = ImColor(0.5f, 0.5f, 0.5f, 0.75f);
    const ImGuiIO &io = ImGui::GetIO();
    const ImVec2 orig_pos = ImGui::GetCursorPos();
    const ImVec2 orig_pos_screen = ImGui::GetCursorScreenPos();

    const int notes = N_TOTAL;
    const int octaves = 8;

    const int whole_note_count = 256;
    const float cell_height = 20.0f;
    const float cell_width  = 92.0f * s_zoom;
    const float cell_single = cell_width / L_WHOLE;
    const ImVec2 overall_size = {(whole_note_count * cell_width), (notes * octaves) * cell_height};

    ImGui::SetCursorPos({orig_pos.x + piano_width, orig_pos.y});

    // grid
    ImGui::BeginChild("EditorArea", {0, 0}, true, ImGuiWindowFlags_HorizontalScrollbar);
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();

    for(int note = 0; note < notes; ++note) {
        for(int octave = 0; octave < octaves; ++octave) {
            int y = note + (notes * octave);

            draw_list->AddLine(
                {p.x + 0, p.y + y * cell_height},
                {p.x + whole_note_count * cell_width, p.y + y * cell_height},
                grey_col
            );

            if(note_key_type_table[to_note(note)] == KEY_WHITE) {
                draw_list->AddRectFilled(
                    {p.x + 0, p.y + y * cell_height},
                    {p.x + whole_note_count * cell_width, p.y + (y + 1) * cell_height},
                    white_key_bg_col
                );
            }
        }
    }

    for(int x = 0; x < whole_note_count; ++x) {
        draw_list->AddLine(
            {p.x + x * cell_width, p.y + 0},
            {p.x + x * cell_width, p.y + (notes * octaves * cell_height)},
            bold_col
        );
    }

    for(int x = 0; x < whole_note_count * 4; ++x) {
        draw_list->AddLine(
            {p.x + x * cell_width / 4, p.y + 0},
            {p.x + x * cell_width / 4, p.y + (notes * octaves * cell_height)},
            grey_col
        );
    }

    // notes
    bool any_is_editing = false;
    for(size_t i = 0; i < countof(section_notes); ++i) {
        auto &note = section_notes[i];

        const ImVec2 lp = ImGui::GetCursorPos();
        const float octave_offset = (N_TOTAL * (octaves - 1 - note.note.octave));
        const ImVec2 minp = {
            lp.x + note.time * cell_single,
            lp.y + (to_note(note.note.note) + octave_offset) * cell_height
        };
        const ImVec2 maxp = {
            lp.x + (note.time * cell_single) + (note.duration * cell_single),
            lp.y + (to_note(note.note.note) + 1 + octave_offset) * cell_height
        };
        //const ImVec2 textp = {minp.x + 4, minp.y + 4};

        ImGui::PushID(5000 + i); // TODO: global offset table/enums?

        const float rounding = 15.0f;
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, rounding);
        ImGui::PushStyleColor(ImGuiCol_Button,        ImU32(ImColor{0.75f, 0.45f, 0.35f, 1.0f}));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImU32(ImColor{0.75f, 0.25f, 0.15f, 1.0f}));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImU32(ImColor{0.95f, 0.55f, 0.45f, 1.0f}));

        ImGui::SetCursorPos(minp);
        ImGui::Button(note_name_table[note.note.note], {maxp.x - minp.x, maxp.y - minp.y});

        const float drag_handle_begin = 0.25f;
        const float drag_handle_end = 0.85f;
        if(ImGui::IsItemHovered() && !ImGui::IsItemActive()) {
            //TODO: reduce copy-pasta?
            const ImVec2 mouse_offset = {
                (io.MousePos.x - ((p.x - lp.x) + minp.x)),
                (io.MousePos.y - ((p.y - lp.y) + minp.y))
            };

            const float hover_button_percent = mouse_offset.x / (maxp.x - minp.x);

            if(hover_button_percent >= drag_handle_end || hover_button_percent <= drag_handle_begin) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            }
        }

        if(ImGui::IsItemActive()) {
            const ImVec2 mouse_offset = {
                (io.MouseClickedPos[0].x - ((p.x - lp.x) + minp.x)),
                (io.MouseClickedPos[0].y - ((p.y - lp.y) + minp.y))
            };

            const float hover_button_percent = mouse_offset.x / (maxp.x - minp.x);

            const ImVec2 startpos = {
                io.MousePos.x - mouse_offset.x,
                io.MousePos.y - mouse_offset.y
            };
            const ImVec2 endpos = {
                startpos.x + (note.duration * cell_single),
                startpos.y + (cell_height)
            };

            const int block_x = std::round((startpos.x - p.x) / (cell_single));
            const int block_y = std::round((startpos.y - p.y) / (cell_height));

            //TODO: Clip timing to beg/end of canvas
            any_is_editing = true;
            if(hover_button_percent < drag_handle_end && hover_button_percent > drag_handle_begin) {
                if(block_y >= 0 && block_y < notes * octaves) {
                    edit.block = &note;
                    edit.prev_block = note;
                    edit.new_block = note;
                    edit.new_block.note.note = N_TOTAL - 1 - (block_y % N_TOTAL);
                    edit.new_block.note.octave = octaves - 1 - (block_y / N_TOTAL);
                    edit.new_block.time = block_x;
                    assert(edit.new_block.note.note >= 0 && edit.new_block.note.note < N_TOTAL);
                    assert(edit.new_block.note.octave >= 0 && edit.new_block.note.octave < octaves);

                    draw_list->AddRectFilled(
                        {p.x + (block_x * cell_single), p.y + block_y * cell_height},
                        {p.x + ((block_x + note.duration) * cell_single), p.y + (block_y + 1) * cell_height},
                        ImU32(ImColor{0.35f, 0.35f, 0.35f, 0.5f}),
                        rounding
                    );
                }
            }
            else {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

                const ImVec2 delta = ImGui::GetMouseDragDelta();
                const int note_delta = round(delta.x / cell_single);

                edit.block = &note;
                edit.prev_block = note;
                edit.new_block = note;
                if(hover_button_percent >= drag_handle_end) {
                    edit.new_block.duration = std::max(1, note.duration + note_delta);
                }
                else if(hover_button_percent <= drag_handle_end){
                    edit.new_block.duration = std::max(1, note.duration - note_delta);
                    edit.new_block.time = std::max(0, std::min(note.time + note_delta, note.time + note.duration));
                }

                //TODO: clean up copy-pasta
                const ImVec2 minp_screen = {
                    p.x + edit.new_block.time * cell_single,
                    p.y + (to_note(edit.new_block.note.note) + (N_TOTAL * (octaves - 1 - edit.new_block.note.octave))) * cell_height
                };
                const ImVec2 maxp_screen = {
                    p.x + (edit.new_block.time * cell_single) + (edit.new_block.duration * cell_single),
                    p.y + (to_note(edit.new_block.note.note) + 1 + (N_TOTAL * (octaves - 1 - edit.new_block.note.octave))) * cell_height
                };

                draw_list->AddRectFilled(
                    minp_screen,
                    maxp_screen,
                    ImU32(ImColor{0.35f, 0.35f, 0.35f, 0.75f}),
                    rounding
                );
            }

            //draw_list->AddRect(startpos, endpos, ImGui::GetColorU32(ImGuiCol_Button), rounding);
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImGui::SetCursorPos(lp);
        ImGui::PopID();
    }

    if(any_is_editing) {
        was_editing = true;
    }
    else if(!any_is_editing && was_editing) {
        if(edit.block) {
            *edit.block = edit.new_block;
            edit.block = nullptr;
            was_editing = false;
        }
    }

    ImGui::Dummy(overall_size);
    ImGui::EndChild();
    ImGui::SameLine();

    // piano
    root_draw_list->PushClipRect(
        orig_pos_screen,
        {orig_pos_screen.x + ImGui::GetWindowSize().x, ImGui::GetWindowSize().y},
        true
    );

    ImGui::SetCursorPos({orig_pos.x, p.y - cell_height});
    const ImVec2 lp = ImGui::GetCursorPos();
    p = ImGui::GetCursorScreenPos();

    root_draw_list->AddRectFilled(
        p,
        {p.x + piano_width, p.y + overall_size.y},
        ImU32(ImColor{0.65f, 0.65f, 0.65f, 1.0f})
    );

    for(int note = 0; note < notes; ++note) {
        for(int octave = 0; octave < octaves; ++octave) {
            int y = note + (notes * octave);

            const char *note_name = note_name_table[to_note(note)];
            char str[5];
            if(to_note(note) == N_C) {
                assert(octave < 10);
                str[0] = '0' + (octaves - 1 - octave);
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
            ImGui::SetCursorPos({lp.x, lp.y + (y * cell_height)});
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

            ImGui::Button(str, {width, cell_height});

            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar(2);
            ImGui::SetCursorPos(lp);
            ImGui::PopID();
        }
    }
    root_draw_list->PopClipRect();

    ImGui::End();
}
