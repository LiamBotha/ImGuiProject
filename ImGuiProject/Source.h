#pragma once

void ShowDemoWindows(bool& show_demo_window, bool& show_another_window, ImVec4& clear_color);

void DrawConnection(ImVec2& node_rect_min, Node*& node, ImVec2& mouse, ImVec2& pos, ImDrawList* draw_list);

void NodeListRender(const ImGuiWindowFlags& window_flags, int& node_hovered_in_list, bool& open_context_menu);

void DrawContextMenues(ImVec2& scrolling);

void OpenContextMenu(int& node_hovered_in_list, int& node_hovered_in_scene, bool& open_context_menu, bool& open_node_menu);

void DrawGrid(ImVec2& scrolling, ImDrawList* draw_list);

void HandleNodes();

int main();
