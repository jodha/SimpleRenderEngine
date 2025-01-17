#include <string.h>
#include <imgui.h>
#include <sre/imgui_addon.hpp>

namespace ImGui {

//=========================== ImGui Functions ==================================

bool
ShowMessage(const std::string& message,
			const std::string& title,
			// Last two arguments are for a modal "process dialog"
			// without buttons, which needs a bool to be closed
			// Should really make a seperate function for a popup without
			// a ok button -- it would keep things cleaner
			const bool& showOk,
			bool* show)
// Use the PostMessage function instead of this function, since it provides the
// same functionality and is much more robust for general use.
//
// This function shows a message and returns true if acknowledged, but can only
// be called from within the rendering code
{
    bool acknowledged = false;
	// OpenPopup should not be called every frame! Figure out how to only call
	// it when opening (can check whether Popup is open already
    ImGui::OpenPopup(title.c_str());
    // Always center this window when appearing
    ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f,
                    ImGui::GetIO().DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(title.c_str(), NULL,
								ImGuiWindowFlags_AlwaysAutoResize)) {
		int height = ImGui::GetFrameHeight();
		if (!showOk) {
			ImGui::Dummy(ImVec2(height, height));
		}
		ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 365);
        ImGui::Text("%s", message.c_str());
		assert(!(!showOk && show == nullptr));
        if (showOk) {
			if (ImGui::Button("OK", ImVec2(120, 0))) {
	            acknowledged = true;
				ImGui::CloseCurrentPopup();
			}
        } else {
			ImGui::Dummy(ImVec2(height, height));
			if (!*show) {
				acknowledged = true;
				ImGui::CloseCurrentPopup();
			}
		}
        ImGui::EndPopup();
    }
    return acknowledged;
}

void
ToggleButton(const char* str_id, bool* selected, ImVec2 size)
{
	// Initialize and store variables used
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImGuiStyle& imGuiStyle = ImGui::GetStyle();
	ImVec4* colors = imGuiStyle.Colors;
    float prevFrameRounding = imGuiStyle.FrameRounding;
	imGuiStyle.FrameRounding = 0.0;
    float prevFrameBorderSize = imGuiStyle.FrameBorderSize;
	imGuiStyle.FrameBorderSize = 1.0;
	ImVec2 pIn = ImGui::GetCursorScreenPos();
	ImVec2 p = pIn;
	if (size.y == 0) {
		size.y = ImGui::GetFrameHeight();
	}
	float thick  = 0.1f*size.y;
	float width = size.x + 2.0f*thick;
	float height = size.y + 2.0f*thick;

	// Add rectangle border on top of button
	draw_list->AddRectFilled(ImVec2(p.x, p.y),
				ImVec2(p.x + width, p.y + thick),
				ImGui::GetColorU32(*selected ? colors[ImGuiCol_ButtonActive]
				: colors[ImGuiCol_Button]), ImGui::GetStyle().FrameRounding);
	// Add rectangle border on bottom of button
	draw_list->AddRectFilled(ImVec2(p.x, p.y + height),
				ImVec2(p.x + width, p.y + height - thick),
				ImGui::GetColorU32(*selected ? colors[ImGuiCol_ButtonActive]
				: colors[ImGuiCol_Button]), ImGui::GetStyle().FrameRounding);
	// Add rectangle border on left side of button
	draw_list->AddRectFilled(ImVec2(p.x, p.y + thick),
				ImVec2(p.x + thick, p.y + height - thick),
				ImGui::GetColorU32(*selected ? colors[ImGuiCol_ButtonActive]
				: colors[ImGuiCol_Button]), ImGui::GetStyle().FrameRounding);
	// Add rectangle border on right side of button
	draw_list->AddRectFilled(ImVec2(p.x + width - thick, p.y + thick),
				ImVec2(p.x + width, p.y + height - thick),
				ImGui::GetColorU32(*selected ? colors[ImGuiCol_ButtonActive]
				: colors[ImGuiCol_Button]), ImGui::GetStyle().FrameRounding);

	// Add button to the center of the border
	p = {p.x + thick, p.y + thick};
	ImGui::SetCursorScreenPos(p);
    if (ImGui::Button(str_id, ImVec2(size.x, size.y))) {
		*selected = !*selected;
	}
	// Return style properties to their previous values
	imGuiStyle.FrameBorderSize = prevFrameBorderSize;
	imGuiStyle.FrameRounding = prevFrameRounding;

	// Advance ImGui cursor according to actual size of full toggle button
	ImGui::SetCursorScreenPos(pIn);
	ImGui::Dummy(ImVec2(width, height));
}

} // namespace ImGui
