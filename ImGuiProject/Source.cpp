
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdio.h>

#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>    // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>    // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>  // Initialize with gladLoadGL()
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <math.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <set>

#include <windows.h>
#include <shobjidl.h> 
#include <shlobj.h>
#include <filesystem>

#include "node.cpp"
#include "Source.h"

static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }

GLFWwindow* window;

const float NODE_SLOT_RADIUS = 5.0f;
const ImVec2 NODE_WINDOW_PADDING(8.0f, 8.0f);

ImVec2 scrolling = ImVec2(0.0f, 0.0f);
ImVec2 selectionStartPos = ImVec2(0, 0);
ImVec2 selectionEndPos = ImVec2(0, 0);

static std::set<int> node_selected = {};
static int nodeNum = 0;

bool show_grid = true;
bool connectionHovered = false;
bool resizeHovered = false;
bool isSelectionBox = false;

std::string filePath;
std::string relativePath;

std::unique_ptr<Node> connection_selected;
std::unique_ptr<Node> resize_selected;

static std::vector<Node*> nodes = {};

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

static Node* CreateNode(int id, std::string name, ImVec2 size, ImVec2 pos = { 0,0 }, NodeType nodeType = DEFAULT)
{
    Node* node;

    switch (nodeType)
    {
    case DIALOGUE:
        node = new DialogueNode();
        break;
    case CHOICE:
        node = new ChoiceNode();
        break;
    case CONDITION:
        node = new ConditionNode();
        break;
    case VALUE:
        node = new ValueNode();
        break;
    default:
        node = new Node();
        break;
    }

    node->id = id;
    node->name = name;
    node->size = size;
    node->pos = pos;
    node->type = nodeType;

    std::cout << "Created Node with the name: " << name << ", " << node->name << "ID: " << node->id << std::endl;

    ImVec2 titleSize = ImGui::CalcTextSize(node->name.c_str());

    titleSize.y *= 3;

    //node->size.x = titleSize.x + NODE_WINDOW_PADDING.x;

    return node;
}

void DrawHermiteCurve(ImDrawList* drawList, ImVec2 p1, ImVec2 p2, int STEPS) // this is the line between two nodes
{
    ImVec2 t1 = ImVec2(+80.0f, 0.0f);
    ImVec2 t2 = ImVec2(+80.0f, 0.0f);

    for (int step = 0; step <= STEPS; step++)
    {
        float t = (float)step / (float)STEPS;
        float h1 = +2 * t * t * t - 3 * t * t + 1.0f;
        float h2 = -2 * t * t * t + 3 * t * t;
        float h3 = t * t * t - 2 * t * t + t;
        float h4 = t * t * t - t * t;
        drawList->PathLineTo(ImVec2(h1 * p1.x + h2 * p2.x + h3 * t1.x + h4 * t2.x, h1 * p1.y + h2 * p2.y + h3 * t1.y + h4 * t2.y));
    }

    drawList->PathStroke(ImColor(200, 200, 100), false, 3.0f);
}

//Calls each node draw function to get its unique elements
void DrawNodeFeatures(Node* node, ImVec2& pos, ImVec2& node_rect_min)
{
    pos.y += 20;

    node->DrawNode(node, pos, node_rect_min);
}

//Draws the connection point and handles its behaviour
void DrawConnection(ImVec2& node_rect_min, Node*& node, ImVec2& mouse, ImVec2& pos, ImDrawList* draw_list)
{
    ImColor connectionColor = IM_COL32_WHITE;

    ImVec2 conMin = ImVec2(node_rect_min.x + node->size.x - 3.5f, node_rect_min.y + (node->size.y / 2.5f) - 3.5f);
    ImVec2 conMax = ImVec2(node_rect_min.x + node->size.x + 3.5f, node_rect_min.y + (node->size.y / 2.5f) + 3.5f);

    if (ImRect((conMin), (conMax)).Contains(mouse))
    {
        connectionHovered = true;

        connectionColor = IM_COL32_BLACK;

        if (!connection_selected && ImGui::IsMouseDown(0) && !ImGui::IsMouseDragging(0))
        {
            connection_selected = std::unique_ptr<Node>(node);
        }
        else if (connection_selected && ImGui::IsMouseReleased(0) && connection_selected.get() == node)
        {
            connection_selected.release();
        }
    }

    if (node == connection_selected.get())
    {
        connectionColor = ImColor(150, 150, 150);
    }

    pos = node_rect_min + node->size;
    pos.y = node_rect_min.y + (node->size.y / 2.5f);

    draw_list->AddCircleFilled(pos, NODE_SLOT_RADIUS, connectionColor);
}

//Handles node resizing each frame
void ResizeNode(ImVec2& node_rect_min, Node*& node, ImVec2& mouse, ImVec2& pos, ImDrawList* draw_list)
{
    pos.x = node_rect_min.x + (node->size.x - NODE_SLOT_RADIUS);
    pos.y = node_rect_min.y + (node->size.y - NODE_SLOT_RADIUS);

    ImColor connectionColor = ImColor(150, 150, 150,150);
    
    ImVec2 p1 = pos + ImVec2(-NODE_SLOT_RADIUS * 2, NODE_SLOT_RADIUS);
    ImVec2 p2 = pos + ImVec2(NODE_SLOT_RADIUS, -NODE_SLOT_RADIUS * 2);
    ImVec2 p3 = pos + ImVec2(NODE_SLOT_RADIUS, NODE_SLOT_RADIUS);

    if (ImTriangleContainsPoint(p1, p2, p3, mouse))
    {
        resizeHovered = true;
        connectionColor = IM_COL32_BLACK;

        if (!resize_selected && ImGui::IsMouseDown(0) && !ImGui::IsMouseDragging(0))
        {
            resize_selected = std::unique_ptr<Node>(node);
        }
    }
    else resizeHovered = false;

    if (resize_selected && ImGui::IsMouseReleased(0))
    {
        resize_selected.release();
    }
    else if (resize_selected.get() == node && !ImGui::IsMouseReleased(0))
    {
        float newX = std::fmax(120,mouse.x - node_rect_min.x);
        float newY = std::fmax(50,mouse.y - node_rect_min.y);

        resize_selected->size = ImVec2(newX,newY);
        connectionColor = ImColor(150, 150, 150);
    }

    draw_list->AddTriangleFilled(pos + ImVec2(-10, NODE_SLOT_RADIUS), pos + ImVec2(5, -10), pos + ImVec2(NODE_SLOT_RADIUS, NODE_SLOT_RADIUS), connectionColor);
}

//Draws the generic aspects of the nodes
void DrawNodeGeneral(Node*& node, ImVec2& offset, ImDrawList* draw_list)
{
    int node_hovered_in_scene = -1;
    bool open_node_menu = false;
    bool node_moving_active = false;
    bool isSelected = false;

    ImVec2 mouse = ImGui::GetIO().MousePos;

    ImVec2 node_rect_min = offset + node->pos;
    ImVec2 node_rect_max = node_rect_min + node->size;
    ImVec2 pos = node_rect_min + NODE_WINDOW_PADDING;
    ImVec2 textSize = ImGui::CalcTextSize(node->name.c_str());

    ImGuiIO& io = ImGui::GetIO();

    pos.x = node_rect_min.x + (node->size.x / 2) - (textSize.x / 2);

    // Get id & node start position
    ImGui::PushID(node->id);

    draw_list->ChannelsSetCurrent(1); // set channel to foreground

    ImGui::BeginGroup();
    ImGui::SetCursorScreenPos(pos);
    ImGui::Text(node->name.c_str());

    DrawNodeFeatures(node, pos, node_rect_min);

    draw_list->ChannelsSetCurrent(0); // set to background

    ImGui::SetCursorScreenPos(node_rect_min + ImVec2(NODE_WINDOW_PADDING.x, 0));
    ImGui::InvisibleButton("node", node->size - ImVec2(NODE_WINDOW_PADDING.x * 2, NODE_WINDOW_PADDING.y));

    if (ImGui::IsItemHovered())
    {
        node_hovered_in_scene = node->id;
        open_node_menu |= ImGui::IsMouseClicked(1);

        if (connection_selected && ImGui::IsMouseReleased(0) && connection_selected.get() != node)
        {
            std::vector<Node*>::iterator it = std::find(connection_selected->outputConnections.begin(), connection_selected->outputConnections.end(), node);

            if (it == connection_selected->outputConnections.end())
            {
                connection_selected->outputConnections.push_back(node);
            }

            connection_selected.release();
        }

        if (open_node_menu)
        {
            if (node_hovered_in_scene != -1)
                node_selected.insert(node_hovered_in_scene);// selects node to open context menu for
        }
    }

    if (ImGui::IsItemActive())
        node_moving_active = true;

    auto search = node_selected.find(node->id);
    if (search != node_selected.end())
        isSelected = true;

    ImU32 node_bg_color = isSelected || node_hovered_in_scene == node->id ? node->hoverBgColor : node->bgColor; // changes color on hover
    draw_list->AddRectFilled(node_rect_min, node_rect_max, node_bg_color, 3.5f);
    draw_list->AddRect(node_rect_min, node_rect_max, node->borderColor, 3.5f, 15, 2);

    ImGui::EndGroup();

    draw_list->ChannelsSetCurrent(1); // set to background

    DrawConnection(node_rect_min, node, mouse, pos, draw_list);
    ResizeNode(node_rect_min, node, mouse, pos, draw_list);

    if (node_moving_active && !io.KeyCtrl) // checks if grabbing node
    {
        node_selected.clear();
        node_selected.insert(node->id);
    }
    else if(node_moving_active && io.KeyCtrl)
        node_selected.insert(node->id);

    if (isSelected && ImGui::IsMouseDragging(0))
        node->pos = node->pos + io.MouseDelta;

    ImGui::PopID();
}

void DrawNode(ImDrawList* draw_list, Node* node, ImVec2 offset) // Todo - go back add connections and dragging
{
    draw_list->ChannelsSplit(2);

    DrawNodeGeneral(node, offset, draw_list);

    draw_list->ChannelsMerge();
}

//Draws the lines connecting each node and its connections
void RenderLines(ImDrawList* draw_list, ImVec2 offset)
{
    for (Node* node : nodes)
    {
        for (Node* con : node->outputConnections)
        {
            if (con != nullptr)
            {
                ImVec2 p1 = (offset + node->pos + ImVec2(node->size.x + 4, node->size.y / 2.5f));
                ImVec2 p2 = (offset + con->pos + ImVec2(0, con->size.y / 2));
                DrawHermiteCurve(draw_list, p1, p2, 20);
            }
        }
    }

    if (connection_selected && ImGui::IsMouseDown(0))
    {
        ImVec2 mouse = ImGui::GetIO().MousePos;

        DrawHermiteCurve(draw_list, offset + connection_selected->pos + ImVec2(connection_selected->size.x + 4, connection_selected->size.y / 2.5f), mouse, 15);
    }
} // TODO - add way to remove lines/connections & fix leftover lines after delete

void ShowDemoWindows(bool& show_demo_window, bool& show_another_window, ImVec4& clear_color)
{
    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (show_another_window)
    {

        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }
}

//Converts each json child back into a node
void SaveChild(json& nodeJson, int i, Node* currentNode)
{
    nodeJson[i]["ID"] = currentNode->id;
    nodeJson[i]["Name"] = currentNode->name;

    nodeJson[i]["Position"][0] = currentNode->pos.x;
    nodeJson[i]["Position"][1] = currentNode->pos.y;
    nodeJson[i]["Size"][0] = currentNode->size.x;
    nodeJson[i]["Size"][1] = currentNode->size.y;

    nodeJson[i]["Type"] = currentNode->type;

    currentNode->SaveTypeData(nodeJson, i);

    for (int j = 0; j < currentNode->outputConnections.size(); ++j)
    {
        SaveChild(nodeJson[i]["Children"], j, currentNode->outputConnections[j]);
    }
}

//Converts each node into json 
void LoadChild(json& nodeJson, int i, Node* parentNode)
{
    int nodeID = nodeJson[i]["ID"];
    std::string nodeName = nodeJson[i]["Name"];
    float nodePosX = nodeJson[i]["Position"][0];
    float nodePosY = nodeJson[i]["Position"][1];
    float nodeSizeX = nodeJson[i]["Size"][0];
    float nodeSizeY = nodeJson[i]["Size"][1];
    NodeType nodeType = nodeJson[i]["Type"];

    Node* node = CreateNode(nodeID, nodeName, { nodeSizeX, nodeSizeY }, { nodePosX, nodePosY }, nodeType);

    node->LoadTypeData(nodeJson,i);

    bool exists = false;
    for (int n = 0; n < nodes.size(); ++n)
    {
        if (node->id == nodes[n]->id)
        {
            exists = true;
            std::cout << "already exists" << std::endl;
            parentNode->outputConnections.push_back(nodes[n]);
        }
    }

    if (!exists)
    {
        if (nodeNum < nodeID)
            nodeNum = nodeID + 1;

        nodes.push_back(node);
        parentNode->outputConnections.push_back(node);

        std::cout << nodeID << ", " << nodeName << ", " << nodeType << "Pos:[" << nodePosX << "," << nodePosY << "]" << std::endl;

        for (int j = 0; j < nodeJson[i]["Children"].size(); ++j)
        {
            LoadChild(nodeJson[i]["Children"], j, node);
        }
    }
}

void SaveToJson(std::string _filePath)
{
    json nodeJson;

    nodeJson["Root"]["Children"];

    std::vector<Node*> currentNodes = nodes[0]->outputConnections;

    for (int i = 0; i < currentNodes.size(); ++i)
    {
        SaveChild(nodeJson["Root"]["Children"], i, currentNodes[i]);
    }

    std::ofstream file(_filePath);
    file << nodeJson;
}

void LoadFromJson(std::string _filePath)
{
    node_selected.clear();
    nodeNum = 0;
    connectionHovered = false;
    connection_selected.reset();

    for (Node* node : nodes)
    {
        delete node;
    }

    nodes = {};

    json nodeJson;

    std::ifstream file(_filePath);
    file >> nodeJson;

    //nodeJson["Root"]["Children"];

    Node* rootNode = CreateNode(-1, "Default Node", { 160,50 }, { 25, 40 }, DEFAULT); //todo check if node already exist and don't create again if so

    nodes.push_back(rootNode);

    for (int i = 0; i < nodeJson.size(); ++i)
    {
        LoadChild(nodeJson["Root"]["Children"], i, rootNode);
    }
}

void NewFile()
{
    node_selected.clear();
    nodeNum = 0;
    connectionHovered = false;
    connection_selected.reset();
    filePath = "";

    for (Node* node : nodes)
    {
        delete node;
    }

    nodes = {};

    nodes.push_back(CreateNode(0  , "Default Node", { 160,50 }, { 25, 40 }, DEFAULT));
    nodes.push_back(CreateNode(++nodeNum, "Example Dialogue Node", { 400,110 }, { 250, 80 }, DIALOGUE));

    nodes[0]->outputConnections.push_back(nodes[1]);
}

void LoadFile()
{
    std::string jsonType = "JSON";
    std::wstring ws;
    ws.assign(jsonType.begin(), jsonType.end());
    LPCWSTR name = ws.c_str();

    COMDLG_FILTERSPEC rgSpec[] =
    {
        { name, L"*.json;" },
    };

    //<SnippetRefCounts>
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
        COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileOpenDialog* pFileOpen;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
            IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

        if (SUCCEEDED(hr))
        {
            DWORD dwFlags;

            hr = pFileOpen->GetOptions(&dwFlags);

            if (SUCCEEDED(hr))
            {
                hr = pFileOpen->SetOptions(dwFlags | FOS_STRICTFILETYPES);

                if (SUCCEEDED(hr))
                {
                    hr = pFileOpen->SetFileTypes(ARRAYSIZE(rgSpec), rgSpec);

                    if (SUCCEEDED(hr))
                    {
                        hr = pFileOpen->SetDefaultExtension(L"json");

                        if (SUCCEEDED(hr))
                        {
                            std::wstring stemp = std::wstring(relativePath.begin(), relativePath.end());

                            PCWSTR folderpath = stemp.c_str();

                            IShellItem* psi = NULL;
                            hr = SHCreateItemFromParsingName(folderpath,NULL,IID_PPV_ARGS(&psi));
                            if (SUCCEEDED(hr))
                            {
                                hr = pFileOpen->SetDefaultFolder(psi);
                                if (SUCCEEDED(hr))
                                {
                                    // Show the Open dialog box.
                                    hr = pFileOpen->Show(NULL);

                                    // Get the file name from the dialog box.
                                    if (SUCCEEDED(hr))
                                    {
                                        IShellItem* pItem;
                                        hr = pFileOpen->GetResult(&pItem);
                                        if (SUCCEEDED(hr))
                                        {
                                            PWSTR pszFilePath;
                                            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                                            // Display the file name to the user.
                                            if (SUCCEEDED(hr))
                                            {
                                                size_t i;
                                                char str[128];
                                                wcstombs_s(&i, str, pszFilePath, 128);

                                                MessageBox(NULL, pszFilePath, L"File Path", MB_OK);
                                                CoTaskMemFree(pszFilePath);

                                                filePath.assign(str);

                                                LoadFromJson(filePath);
                                            }
                                            pItem->Release();
                                        }
                                    }
                                    pFileOpen->Release();
                                }
                            }
                        }
                    }
                }
            }
        }

        CoUninitialize();
    }
}

void SaveFile()
{
    std::string jsonType = "JSON";
    std::wstring ws;
    ws.assign(jsonType.begin(), jsonType.end());
    LPCWSTR name = ws.c_str();

    COMDLG_FILTERSPEC rgSpec[] =
    {
        { name, L"*.json;" },
    };

    //<SnippetRefCounts>
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
        COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileSaveDialog* pFileSave;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL,
            IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSave));

        if (SUCCEEDED(hr))
        {
            DWORD dwFlags;

            hr = pFileSave->GetOptions(&dwFlags);

            if (SUCCEEDED(hr))
            {
                hr = pFileSave->SetOptions(dwFlags | FOS_STRICTFILETYPES);

                if (SUCCEEDED(hr))
                {
                    hr = pFileSave->SetFileTypes(ARRAYSIZE(rgSpec), rgSpec);

                    if (SUCCEEDED(hr))
                    {
                        hr = pFileSave->SetDefaultExtension(L"json");

                        std::wstring stemp = std::wstring(relativePath.begin(), relativePath.end());

                        PCWSTR folderpath = stemp.c_str();

                        IShellItem* psi = NULL;
                        hr = SHCreateItemFromParsingName(folderpath, NULL, IID_PPV_ARGS(&psi));
                        if (SUCCEEDED(hr))
                        {
                            hr = pFileSave->SetDefaultFolder(psi);
                            if (SUCCEEDED(hr))
                            {
                                // Show the Open dialog box.
                                hr = pFileSave->Show(NULL);

                                // Get the file name from the dialog box.
                                if (SUCCEEDED(hr))
                                {
                                    IShellItem* pItem;
                                    hr = pFileSave->GetResult(&pItem);
                                    if (SUCCEEDED(hr))
                                    {
                                        PWSTR pszFilePath;
                                        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                                        // Display the file name to the user.
                                        if (SUCCEEDED(hr))
                                        {
                                            size_t i;
                                            char str[128];
                                            wcstombs_s(&i, str, pszFilePath, 128);

                                            MessageBox(NULL, pszFilePath, L"File Path", MB_OK);
                                            CoTaskMemFree(pszFilePath);

                                            filePath.assign(str);

                                            SaveToJson(filePath);
                                        }
                                        pItem->Release();
                                    }
                                }
                                pFileSave->Release();
                            }
                        }
                    }
                }
            }
        }

        CoUninitialize();
    }
}

//Handles the visuals of the menu and handles the menu item's actions
void DrawContextMenu(ImVec2& scrolling)
{
    // Draw context menu
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    if (ImGui::BeginPopup("context_menu"))
    {
        if (ImGui::MenuItem("Add Dialogue Node"))
        {
            ImVec2 mousePos = ImGui::GetMousePos() - scrolling;
            mousePos.x -= 325;
            mousePos.y -= 100;

            nodes.push_back(CreateNode(++nodeNum, "Dialogue Node", { 400,110 }, mousePos, DIALOGUE));
        }
        else if (ImGui::MenuItem("Add Choice Node"))
        {
            ImVec2 mousePos = ImGui::GetMousePos() - scrolling;
            mousePos.x -= 325;
            mousePos.y -= 100;

            nodes.push_back(CreateNode(++nodeNum, "Choice Node", { 200,80 }, mousePos, CHOICE));
        }
        else if (ImGui::MenuItem("Add Condition Node"))
        {
            ImVec2 mousePos = ImGui::GetMousePos() - scrolling;
            mousePos.x -= 325;
            mousePos.y -= 100;

            nodes.push_back(CreateNode(++nodeNum, "Condition Node", { 200,80 }, mousePos, CONDITION));
        }
        else if (ImGui::MenuItem("Add Value Node"))
        {
            ImVec2 mousePos = ImGui::GetMousePos() - scrolling;
            mousePos.x -= 325;
            mousePos.y -= 100;

            nodes.push_back(CreateNode(++nodeNum, "Value Node", { 200,80 }, mousePos, VALUE));
        }
        else if (ImGui::MenuItem("Save To File"))
        {
            SaveToJson(filePath);
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();

    // Draw context menu
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    if (ImGui::BeginPopup("node_menu"))
    {
        if (ImGui::MenuItem("Delete Node"))
        {
            struct HasId
            {
                bool operator()(const Node* n)
                {
                    return node_selected.find(n->id) != node_selected.end();
                }
            };

            nodes.erase(std::remove_if(nodes.begin(), nodes.end(), HasId()), nodes.end());

            for (int i = 0; i < nodes.size(); ++i)
            {
                nodes[i]->outputConnections.erase(std::remove_if(nodes[i]->outputConnections.begin(), nodes[i]->outputConnections.end(), HasId()), nodes[i]->outputConnections.end());
            }
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

//Opens the context menu when the player right clicks
void OpenContextMenu(int& node_hovered_in_list, int& node_hovered_in_scene, bool& open_context_menu, bool& open_node_menu)
{
    // Open context menu
    if (!ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1))
    {
        node_selected.clear();
        node_hovered_in_list = node_hovered_in_scene = -1; // resets selected to none
        open_context_menu = true;
    }
    else if (ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1))
    {
        if (!node_selected.empty())
            open_node_menu = true;
    }

    if (open_context_menu)
    {
        ImGui::OpenPopup("context_menu");
    }
    else if (open_node_menu)
    {
        ImGui::OpenPopup("node_menu");
    }
}

// Draws the background grid
void DrawGrid(ImVec2& scrolling, ImDrawList* draw_list)
{
    if (true) // draws grid into window
    {
        ImU32 GRID_COLOR = IM_COL32(200, 200, 200, 40);
        float GRID_SZ = 64.0f;
        ImVec2 win_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_sz = ImGui::GetWindowSize();
        for (float x = fmodf(scrolling.x, GRID_SZ); x < canvas_sz.x; x += GRID_SZ)
            draw_list->AddLine(ImVec2(x, 0.0f) + win_pos, ImVec2(x, canvas_sz.y) + win_pos, GRID_COLOR);
        for (float y = fmodf(scrolling.y, GRID_SZ); y < canvas_sz.y; y += GRID_SZ)
            draw_list->AddLine(ImVec2(0.0f, y) + win_pos, ImVec2(canvas_sz.x, y) + win_pos, GRID_COLOR);
    }
}

// Draws a list of all the nodes on the left
void NodeListRender(int& node_hovered_in_list, bool& open_node_menu)
{
    ImGui::SetNextWindowBgAlpha(0.5f);
    ImGui::BeginChild("node_list", ImVec2(150, 0), false);
    ImGui::Text("Nodes");
    ImGui::Separator();

    for (size_t i = 0; i < nodes.size(); ++i)
    {
        Node* node = nodes[i];

        ImGui::PushID(node->id);

        bool isSelected = false;
        auto search = node_selected.find(nodes[i]->id);
        if (search != node_selected.end())
            isSelected = true;

        if (ImGui::Selectable(node->name.c_str(), isSelected))
        {
            node_selected.clear();
            node_selected.insert(node->id);
        }
        if (ImGui::IsItemHovered())
        {
            node_hovered_in_list = node->id;
            //open_node_menu |= ImGui::IsMouseClicked(1);
        }

        ImGui::PopID();
    }

    ImGui::EndChild();
}

//Draws node window, calls nodes draw function, handles context menues, and scrolling
void HandleNodes()
{
    bool open_context_menu = false;
    bool open_node_menu = false;
    int node_hovered_in_list = -1;
    int node_hovered_in_scene = -1;

    ImVec2 mouse = ImGui::GetIO().MousePos;

    connectionHovered = false;

    NodeListRender(node_hovered_in_list, open_node_menu);

    ImGui::SetCursorPos({ 170,55 });
    ImGui::BeginGroup();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, IM_COL32(60, 60, 70, 200));
    ImGui::BeginChild("scrolling_region", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
    ImGui::PushItemWidth(120.0f);

    ImVec2 offset = ImGui::GetCursorScreenPos() + scrolling;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    DrawGrid(scrolling, draw_list);

    RenderLines(draw_list, offset);

    //Display Nodes
    for (Node* node : nodes)
    {
        DrawNode(draw_list, node, offset);
    }

    if (connection_selected && connectionHovered == false && ImGui::IsMouseReleased(0))
    {
        connection_selected.release();
    }

    if (!node_selected.empty() && ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered())
    {
        node_selected.clear();
    }
    else if (!isSelectionBox && !connection_selected && !resize_selected && ImGui::IsMouseDown(0) && !ImGui::IsAnyItemHovered())
    {
        isSelectionBox = true;
        selectionStartPos = mouse;
        selectionEndPos = mouse;
    }

    if (isSelectionBox && ImGui::IsMouseDragging(0))
    {
        selectionEndPos = mouse;
        draw_list->AddRect(selectionStartPos, selectionEndPos, ImColor(20, 20, 255,150),0,15,3);
        draw_list->AddRectFilled(selectionStartPos, selectionEndPos, ImColor(50, 50, 255,20));
    }
    else if (isSelectionBox && ImGui::IsMouseReleased(0))
    {
        if (selectionStartPos.x > selectionEndPos.x)
        {
            auto temp = selectionStartPos;
            selectionStartPos = selectionEndPos;
            selectionEndPos = temp;
        }

        node_selected.clear();

        for (Node* node : nodes)
        {
            if (ImRect(selectionStartPos, selectionEndPos).Contains(node->pos + offset))
            {
                node_selected.insert(node->id);
            }
        }

        isSelectionBox = false;
    }


    OpenContextMenu(node_hovered_in_list, node_hovered_in_scene, open_context_menu, open_node_menu);

    DrawContextMenu(scrolling);

    // Scrolling
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(2, 0.0f))
        scrolling = scrolling + ImGui::GetIO().MouseDelta;

    ImGui::PopItemWidth();
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    ImGui::EndGroup();
}

int main()
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";

    // Create window with graphics context
    window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#else
    bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    char rootFilePath[MAX_PATH];

    GetModuleFileNameA(NULL, rootFilePath, MAX_PATH);

    relativePath = rootFilePath;

    size_t pos = relativePath.find("ImGuiProject.exe");

    if (pos != std::string::npos)
    {
        // If found then erase it from string
        relativePath.erase(pos, 17);
    }

    relativePath += "Json";

    std::cout << relativePath <<  std::endl;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (nodes.size() == 0)
        {
            nodes.push_back(CreateNode(++nodeNum, "Start", { 160,50 }, { 25, 40 }, DEFAULT));
            nodes.push_back(CreateNode(++nodeNum, "Example Dialogue Node", { 400,110 }, { 250, 80 }, DIALOGUE));

            nodes[0]->outputConnections.push_back(nodes[1]);
        }

        //ShowDemoWindows(show_demo_window, show_another_window, clear_color);

        ImGui::Begin("##AllWindow");

        ImVec2 cursorPos = ImGui::GetCursorPos();

        ImGui::SetCursorPos({ 170, cursorPos.y });
        ImGui::BeginChild("TopBar", {500, 40});
        if (ImGui::Button("New File", { 100,20 }))
            NewFile();
        ImGui::SameLine();
        if (ImGui::Button("Save File", { 100,20 }))
            SaveFile();
        ImGui::SameLine();
        if (ImGui::Button("Load File", { 100,20 }))
            LoadFile();
        ImGui::EndChild();
        ImGui::SetCursorPos(cursorPos);

        HandleNodes();

        ImGui::End();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    for (Node* node : nodes)
    {
        delete node;
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

	return 0;
}