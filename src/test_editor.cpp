#include "main_test.h"
#include <imgui.h>

void test_editor()
{
    ImGui::Begin("Editor");

    // grid
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const ImU32 col = ImColor(0.15f, 0.25f, 0.25f);
    const ImVec2 p = ImGui::GetCursorScreenPos();

    for(int y = 0; y < 40; ++y) {
        draw_list->AddLine({p.x + 0, p.y + y * 40.0f}, {p.x + 40, p.y + y * 40.0f}, col);
    }

    ImGui::Dummy({40, 4 * 40.0f});
    ImGui::End();
}
