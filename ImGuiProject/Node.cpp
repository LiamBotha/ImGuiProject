#pragma once

#ifndef NODE
#define NODE

#include <imgui.h>
#include <vector>
#include <string>

enum NodeType
{
    DEFAULT,
    DIALOGUE,   // Stores normal dialogue
    CHOICE,     // Stores choice text
    CONDITION,  // Checks if value meets condition - text for con name and value field for value
    VALUE       // Sets variable value - text for name and field for value to set it to
};

class Node
{
public:

    int id = -1;
    const char* name = "noName";

    ImVec2 pos;
    ImVec2 size;

    NodeType type = DEFAULT;

    std::vector<Node*> outputConnections;

    virtual void DrawNode(Node* node, ImVec2& pos, ImVec2& node_rect_min)
    {

    }
}; //TODO - Convert to class with subnode classes

class DialogueNode : public Node
{
public:
    char dialogueLine[320] = "";

    void DrawNode(Node* node, ImVec2& pos, ImVec2& node_rect_min)
    {
        pos.x = node_rect_min.x + (node->size.x / 2) - (375 / 2);

        ImGui::SetCursorScreenPos(pos);

        std::string label = "###" + std::to_string(node->id);

        //ImGui::SetNextItemWidth(IM_ARRAYSIZE(dialogueLine) * 7);
        ImGui::InputTextMultiline(label.c_str(), dialogueLine, IM_ARRAYSIZE(dialogueLine), ImVec2(375,65), ImGuiInputTextFlags_NoHorizontalScroll);
    }
};

class ChoiceNode : public Node
{
public:
    char choiceLine[100] = "";

    void DrawNode(Node* node, ImVec2& pos, ImVec2& node_rect_min)
    {
        pos.x = node_rect_min.x + (node->size.x / 2) - (180 / 2);

        ImGui::SetCursorScreenPos(pos);

        std::string label = "###" + std::to_string(node->id);

        //ImGui::InputText(label.c_str(), choiceLine, IM_ARRAYSIZE(choiceLine));
        ImGui::InputTextMultiline(label.c_str(), choiceLine, IM_ARRAYSIZE(choiceLine), ImVec2(180, 40), ImGuiInputTextFlags_NoHorizontalScroll);
    }
};

class ConditionNode : public Node
{
    char playerInput[16] = "";
    char conditionValue[16] = "";

    void DrawNode(Node* node, ImVec2& pos, ImVec2& node_rect_min)
    {
        pos.x = node_rect_min.x + (node->size.x / 2) - (IM_ARRAYSIZE(playerInput) * 3.8) - ImGui::CalcTextSize("aaaa ").x / 2;

        ImGui::SetCursorScreenPos(pos);

        std::string label = "###" + std::to_string(node->id);

        ImGui::Text("Var ");
        pos.x += + ImGui::CalcTextSize("Var ").x;
        ImGui::SetCursorScreenPos(pos);

        ImGui::InputText(label.c_str(), playerInput, IM_ARRAYSIZE(playerInput));

        pos.x = node_rect_min.x + (node->size.x / 2) - (IM_ARRAYSIZE(playerInput) * 3.8) - ImGui::CalcTextSize("aaaa ").x / 2;
        pos.y += 20;

        ImGui::SetCursorScreenPos(pos);

        label = "###" + std::to_string(node->id) + "Val";

        ImGui::Text("Val ");
        pos.x += ImGui::CalcTextSize("Val ").x;
        ImGui::SetCursorScreenPos(pos);
        ImGui::InputText(label.c_str(), conditionValue, IM_ARRAYSIZE(conditionValue));
    }
};

class ValueNode : public Node
{
    char playerInput[16] = "";
    char conditionValue[16] = "";

    void DrawNode(Node* node, ImVec2& pos, ImVec2& node_rect_min)
    {
        pos.x = node_rect_min.x + (node->size.x / 2) - (IM_ARRAYSIZE(playerInput) * 3.8) - ImGui::CalcTextSize("aaaa ").x / 2;

        ImGui::SetCursorScreenPos(pos);

        std::string label = "###" + std::to_string(node->id);

        ImGui::Text("Var ");
        pos.x += +ImGui::CalcTextSize("Var ").x;
        ImGui::SetCursorScreenPos(pos);

        ImGui::InputText(label.c_str(), playerInput, IM_ARRAYSIZE(playerInput));

        pos.x = node_rect_min.x + (node->size.x / 2) - (IM_ARRAYSIZE(playerInput) * 3.8) - ImGui::CalcTextSize("aaaa ").x / 2;
        pos.y += 20;

        ImGui::SetCursorScreenPos(pos);

        label = "###" + std::to_string(node->id) + "Val";

        ImGui::Text("Val ");
        pos.x += ImGui::CalcTextSize("Val ").x;
        ImGui::SetCursorScreenPos(pos);
        ImGui::InputText(label.c_str(), conditionValue, IM_ARRAYSIZE(conditionValue));
    }
};

#endif // !NODE
