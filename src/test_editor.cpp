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

    ImGui::BeginChild("EditorArea", {0, 0}, true, ImGuiWindowFlags_HorizontalScrollbar);
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const ImU32 grey_col = ImColor(0.25f, 0.25f, 0.25f, 0.5f);
    const ImU32 bold_col = ImColor(0.5f, 0.5f, 0.5f, 0.75f);
    const ImVec2 p = ImGui::GetCursorScreenPos();

    // grid
    const int notes = 12;
    const int octaves = 7;

    const int whole_note_count = 256;
    const float cell_height = 20.0f;
    const float cell_width  = 92.0f * s_zoom;
    const float cell_single = cell_width / L_WHOLE;

    for(int y = 0; y < notes * octaves; ++y) {
        draw_list->AddLine(
            {p.x + 0, p.y + y * cell_height},
            {p.x + whole_note_count * cell_width, p.y + y * cell_height},
            grey_col
        );
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
    /*
    draw_list->AddRectFilled(
        {p.x + 0, p.y + 0},
        {p.x + cell_width, p.y + cell_height},
        ImColor{0.75f, 0.45f, 0.35f, 1.0f},
        0.25f
    );
    */

#define AS_BUTTON
    for(size_t i = 0; i < countof(section_notes); ++i) {
        const auto &note = section_notes[i];
#ifdef AS_BUTTON
        const ImVec2 lp = ImGui::GetCursorPos();
#else
        const ImVec2 lp = p;
#endif
        const ImVec2 minp = {lp.x + note.time * cell_single, lp.y + (N_TOTAL - note.note.note) * cell_height};
        const ImVec2 maxp = {lp.x + (note.time * cell_single) + (note.duration * cell_single), lp.y + (N_TOTAL - note.note.note + 1) * cell_height};
        const ImVec2 textp = {minp.x + 4, minp.y + 4};

#ifdef AS_BUTTON
        ImGui::PushID(i); // TODO: global offset table/enums?

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 15.0f);
        ImGui::PushStyleColor(ImGuiCol_Button,        ImU32(ImColor{0.75f, 0.45f, 0.35f, 1.0f}));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImU32(ImColor{0.75f, 0.25f, 0.15f, 1.0f}));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImU32(ImColor{0.95f, 0.55f, 0.45f, 1.0f}));

        ImGui::SetCursorPos(minp);
        ImGui::Button(note_name_table[note.note.note], {maxp.x - minp.x, maxp.y - minp.y});
        ImGui::SetCursorPos(lp);

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImGui::PopID();
#else
        draw_list->AddRectFilled(
            minp,
            maxp,
            ImColor{0.75f, 0.45f, 0.35f, 1.0f},
            15.0f
        );
        draw_list->AddText(textp, ImColor{0.15f, 0.15f, 0.15f, 0.75f}, note_name_table[note.note.note]);
#endif
    }

    ImGui::Dummy({(whole_note_count * cell_width), (notes * octaves) * cell_height});
    ImGui::EndChild();

    //ImGui::PushStyleVar(ImGuiStyleVar_)


    ImGui::End();
}
