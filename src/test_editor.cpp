#include "main_test.h"
#include <imgui.h>

void test_editor()
{
    ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_HorizontalScrollbar);

    // grid
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const ImU32 grey_col = ImColor(0.25f, 0.25f, 0.25f, 0.5f);
    const ImU32 bold_col = ImColor(0.5f, 0.5f, 0.5f, 0.75f);
    const ImVec2 p = ImGui::GetCursorScreenPos();

    const int notes = 12;
    const int octaves = 7;

    const int whole_note_count = 256;
    const float cell_height = 20.0f;
    const float cell_width  = 92.0f;

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

    ImGui::Dummy({(whole_note_count * cell_width), (notes * octaves) * cell_height});
    ImGui::End();
}
