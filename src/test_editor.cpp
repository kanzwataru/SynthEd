#include "main_test.h"
#include <imgui.h>
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

static NoteBlock section_notes[] = {
    {{N_C, 4}, 0, L_WHOLE},
    {{N_A, 4}, 4, L_HALF}
};

void test_editor()
{
    static float s_zoom = 1.0f;

    ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_NoScrollbar);
    ImGui::SliderFloat("Zoom", &s_zoom, 0.1f, 6.0f);
    ImDrawList *root_draw_list = ImGui::GetWindowDrawList();

    // piano
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 40.0f);

    ImGui::BeginChild("EditorArea", {0, 0}, true, ImGuiWindowFlags_HorizontalScrollbar);
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const ImU32 grey_col = ImColor(0.25f, 0.25f, 0.25f, 0.5f);
    const ImU32 white_key_bg_col = ImColor(0.15f, 0.15f, 0.15f, 0.5f);
    const ImU32 bold_col = ImColor(0.5f, 0.5f, 0.5f, 0.75f);
    const ImGuiIO &io = ImGui::GetIO();
    const ImVec2 p = ImGui::GetCursorScreenPos();

    // grid
    const int notes = N_TOTAL;
    const int octaves = 8;

    const int whole_note_count = 256;
    const float cell_height = 20.0f;
    const float cell_width  = 92.0f * s_zoom;
    const float cell_single = cell_width / L_WHOLE;

    auto to_note = [](int n) { return (N_TOTAL - 1) - n; };

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

            root_draw_list->AddText(
                {p.x - 40.0f, p.y + y * cell_height + 2.0f},
                ImGui::GetColorU32(ImGuiCol_Text),
                str
            );
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
    for(size_t i = 0; i < countof(section_notes); ++i) {
        const auto &note = section_notes[i];

        const ImVec2 lp = ImGui::GetCursorPos();
        const float octave_offset = (N_TOTAL * (octaves - note.note.octave));
        const ImVec2 minp = {
            lp.x + note.time * cell_single,
            lp.y + (to_note(note.note.note) + octave_offset) * cell_height
        };
        const ImVec2 maxp = {
            lp.x + (note.time * cell_single) + (note.duration * cell_single),
            lp.y + (to_note(note.note.note) + 1 + octave_offset) * cell_height
        };
        const ImVec2 textp = {minp.x + 4, minp.y + 4};

        ImGui::PushID(i); // TODO: global offset table/enums?

        const float rounding = 15.0f;
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, rounding);
        ImGui::PushStyleColor(ImGuiCol_Button,        ImU32(ImColor{0.75f, 0.45f, 0.35f, 1.0f}));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImU32(ImColor{0.75f, 0.25f, 0.15f, 1.0f}));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImU32(ImColor{0.95f, 0.55f, 0.45f, 1.0f}));

        ImGui::SetCursorPos(minp);
        ImGui::Button(note_name_table[note.note.note], {maxp.x - minp.x, maxp.y - minp.y});
        if(ImGui::IsItemActive()) {
            const ImVec2 offset = {
                (io.MouseClickedPos[0].x - ((p.x - lp.x) + minp.x)),
                (io.MouseClickedPos[0].y - ((p.y - lp.y) + minp.y))
            };
            const ImVec2 startpos = {
                io.MousePos.x - offset.x,
                io.MousePos.y - offset.y
            };
            const ImVec2 endpos = {
                startpos.x + (note.duration * cell_single),
                startpos.y + (cell_height)
            };

            draw_list->AddRect(startpos, endpos, ImGui::GetColorU32(ImGuiCol_Button), rounding);
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImGui::SetCursorPos(lp);
        ImGui::PopID();
    }

    ImGui::Dummy({(whole_note_count * cell_width), (notes * octaves) * cell_height});
    ImGui::EndChild();

    ImGui::End();
}
