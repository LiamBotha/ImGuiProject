#pragma once

#ifndef NODE
#define NODE

#include <imgui.h>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

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
    std::string name = "noName";

    ImVec2 pos;
    ImVec2 size;

    NodeType type = DEFAULT;

    ImColor bgColor;
    ImColor hoverBgColor;
    ImColor borderColor;
    ImVec4 textboxColor;

    std::vector<Node*> outputConnections;

    Node()
    {
        bgColor = ImColor(60, 60, 60);
        hoverBgColor = ImColor(75, 75, 75);
        borderColor = ImColor(100, 100, 100);
        textboxColor = ImColor(80, 80, 80);
    }

    virtual void DrawNode(Node* node, ImVec2& pos, ImVec2& node_rect_min)
    {

    }

    virtual void SaveTypeData(nlohmann::json& nodeJson, int i)
    {

    }

    virtual void LoadTypeData(nlohmann::json& nodeJson, int i)
    {

    }

}; //TODO - Convert to class with subnode classes

class DialogueNode : public Node
{
public:
    char dialogueLine[320] = "";

    DialogueNode()
    {
        bgColor = ImColor(0, 28, 112);
        hoverBgColor = ImColor(10, 38, 112);
        borderColor = ImColor(100, 100, 100);
        textboxColor = ImColor(0, 13, 54);
    }

    void DrawNode(Node* node, ImVec2& pos, ImVec2& node_rect_min)
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, textboxColor);

        pos.x = node_rect_min.x + (node->size.x / 2) - (375 / 2);

        ImGui::SetCursorScreenPos(pos);

        std::string label = "###" + std::to_string(node->id);

        //ImGui::SetNextItemWidth(IM_ARRAYSIZE(dialogueLine) * 7);
        ImGui::InputTextMultiline(label.c_str(), dialogueLine, IM_ARRAYSIZE(dialogueLine), ImVec2(375,65), ImGuiInputTextFlags_NoHorizontalScroll);

        ImGui::PopStyleColor();
    }

    void SaveTypeData(nlohmann::json& nodeJson, int i)
    {
        nodeJson[i]["Dialogue"] = dialogueLine;
    }

    void LoadTypeData(nlohmann::json& nodeJson, int i)
    {
        std::string str = nodeJson[i]["Dialogue"];

        for(int j = 0; j < str.size(); ++j)
        {
            dialogueLine[j] = str[j];
        }
    }
};

class ChoiceNode : public Node
{
public:
    char choiceLine[100] = "";

    ChoiceNode()
    {
        bgColor = ImColor(62, 32, 99);
        hoverBgColor = ImColor(70, 42, 100);
        borderColor = ImColor(100, 100, 100);
        textboxColor = ImColor(45, 23, 71);
    }

    void DrawNode(Node* node, ImVec2& pos, ImVec2& node_rect_min)
    {
        pos.x = node_rect_min.x + (node->size.x / 2) - (180 / 2);

        ImGui::SetCursorScreenPos(pos);

        std::string label = "###" + std::to_string(node->id);

        //ImGui::InputText(label.c_str(), choiceLine, IM_ARRAYSIZE(choiceLine));

        ImGui::PushStyleColor(ImGuiCol_FrameBg, textboxColor);
        ImGui::InputTextMultiline(label.c_str(), choiceLine, IM_ARRAYSIZE(choiceLine), ImVec2(180, 40), ImGuiInputTextFlags_NoHorizontalScroll);
        ImGui::PopStyleColor();
    }

    void SaveTypeData(nlohmann::json& nodeJson, int i)
    {
        nodeJson[i]["Choice"] = choiceLine;
    }
};

class ConditionNode : public Node
{
public:
    char playerInput[16] = "";
    char conditionValue[16] = "";

    ConditionNode()
    {
        bgColor = ImColor(120, 37, 72);
        hoverBgColor = ImColor(130, 47, 73);
        borderColor = ImColor(100, 100, 100);
        textboxColor = ImColor(87, 27, 52);
    }
    
    void DrawNode(Node* node, ImVec2& pos, ImVec2& node_rect_min)
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, textboxColor);

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

        ImGui::PopStyleColor();
    }

    void SaveTypeData(nlohmann::json& nodeJson, int i)
    {
        nodeJson[i]["Condition Variable"] = playerInput;
        nodeJson[i]["Condition Value"] = conditionValue;
    }
};

class ValueNode : public Node
{
public:
    char playerInput[16] = "";
    char conditionValue[16] = "";

    ValueNode()
    {
        bgColor = ImColor(26, 102, 84);
        hoverBgColor = ImColor(36, 112, 85);
        borderColor = ImColor(100, 100, 100);
        textboxColor = ImColor(18, 77, 63);
    }

    void DrawNode(Node* node, ImVec2& pos, ImVec2& node_rect_min)
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, textboxColor);

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

        ImGui::PopStyleColor();
    }

    void SaveTypeData(nlohmann::json& nodeJson, int i)
    {
        nodeJson[i]["Condition Variable"] = playerInput;
        nodeJson[i]["Condition Value"] = conditionValue;
    }
};

#endif // !NODE
