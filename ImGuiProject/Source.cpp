
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "stdio.h"

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

#include "node.cpp"

static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }

GLFWwindow* window;

const ImVec2 NODE_WINDOW_PADDING(8.0f, 8.0f);
const float NODE_SLOT_RADIUS = 5.0f;

static int node_selected = -1;;
static bool show_grid = true;

static int nodeNum = 0;

bool connectionHovered = false;

std::unique_ptr<Node> connection_selected;

static std::vector<Node*> nodes = {};

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
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

//void UpdateDragging(ImVec2 offset)
//{
//    switch (s_dragState)
//    {
//    case DragState_Default:
//    {
//        ImVec2 pos;
//        Connection* con = GetHoverCon(offset, &pos);
//
//        if (con)
//        {
//            s_dragNode.con = con;
//            s_dragNode.pos = pos;
//            s_dragState = DragState_Hover;
//            return;
//        }
//
//        break;
//    }
//
//    case DragState_Hover:
//    {
//        ImVec2 pos;
//        Connection* con = GetHoverCon(offset, &pos);
//
//        // Make sure we are still hovering the same node
//
//        if (con != s_dragNode.con)
//        {
//            s_dragNode.con = 0;
//            s_dragState = DragState_Default;
//            return;
//        }
//
//        if (ImGui::IsMouseClicked(0) && s_dragNode.con)
//            s_dragState = DragState_Draging;
//
//        break;
//    }
//
//    case DragState_BeginDrag:
//    {
//        break;
//    }
//
//    case DragState_Draging:
//    {
//        ImDrawList* drawList = ImGui::GetWindowDrawList();
//
//        drawList->ChannelsSetCurrent(0); // Background
//
//        DrawHermiteCurve(drawList, s_dragNode.pos, ImGui::GetIO().MousePos, 12);
//
//        if (!ImGui::IsMouseDown(0))
//        {
//            ImVec2 pos;
//            Connection* con = GetHoverCon(offset, &pos);
//
//            // Make sure we are still hovering the same node
//
//            if (con == s_dragNode.con)
//            {
//                s_dragNode.con = 0;
//                s_dragState = DragState_Default;
//                return;
//            }
//
//            // Lets connect the nodes.
//            // TODO: Make sure we connect stuff in the correct way!
//
//            con->input = s_dragNode.con;
//            s_dragNode.con = 0;
//            s_dragState = DragState_Default;
//        }
//
//        break;
//    }
//
//    case DragState_Connect:
//    {
//        break;
//    }
//    }
//}

void DrawNodeFeatures(Node* node, ImVec2& pos, ImVec2& node_rect_min)
{
    node->DrawNode(node, pos, node_rect_min);
}

void DrawNode(ImDrawList* draw_list, Node* node, ImVec2 offset, int& node_selected) // Todo - go back add connections and dragging
{
    draw_list->ChannelsSplit(2);

    int node_hovered_in_scene = -1;
    bool open_context_menu = false;

    ImVec2 mouse = ImGui::GetIO().MousePos;

    // Get id & node start position
    ImGui::PushID(node->id);
    ImVec2 node_rect_min = offset + node->pos;

    draw_list->ChannelsSetCurrent(1); // set channel to foreground
    bool old_any_active = ImGui::IsAnyItemActive();

    ImVec2 textSize = ImGui::CalcTextSize(node->name);

    ImVec2 pos = node_rect_min + NODE_WINDOW_PADDING;
    pos.x = node_rect_min.x + (node->size.x / 2) - (textSize.x / 2);

    ImVec2 node_rect_max = node_rect_min + node->size;

    ImGui::BeginGroup();
    ImGui::SetCursorScreenPos(pos);
    ImGui::Text(node->name);

    pos.y += 20;

    DrawNodeFeatures(node, pos, node_rect_min);

    bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());

    draw_list->ChannelsSetCurrent(0); // set to background

    ImGui::SetCursorScreenPos(node_rect_min + ImVec2(NODE_WINDOW_PADDING.x, 0));
    ImGui::InvisibleButton("node", node->size - ImVec2(NODE_WINDOW_PADDING.x * 2, NODE_WINDOW_PADDING.y));

    if (ImGui::IsItemHovered())
    {
        node_hovered_in_scene = node->id;
        open_context_menu |= ImGui::IsMouseClicked(1);

        if (connection_selected && ImGui::IsMouseReleased(0) && connection_selected.get() != node)
        {
            std::vector<Node*>::iterator it = std::find(connection_selected->outputConnections.begin(), connection_selected->outputConnections.end(), node);

            if (it == connection_selected->outputConnections.end())
            {
                connection_selected->outputConnections.push_back(node);
            }

            connection_selected.release();
        }

        if (open_context_menu)
        {
            if (node_hovered_in_scene != -1)
                node_selected = node_hovered_in_scene;
        }
    }

    bool node_moving_active = false;

    if (ImGui::IsItemActive()) // & !s_dragNode.con
        node_moving_active = true;

    ImU32 node_bg_color = node_hovered_in_scene == node->id ? ImColor(75, 75, 75) : ImColor(60, 60, 60); // changes color on hover
    draw_list->AddRectFilled(node_rect_min, node_rect_max, node_bg_color, 4.0f);

    ImVec2 titleArea = node_rect_max;
    titleArea.y = node_rect_min.y + 30.0f;

    // Draw text bg area
    //draw_list->AddRectFilled(node_rect_min + ImVec2(1, 1), titleArea, ImColor(100, 0, 0), 4.0f);
    draw_list->AddRect(node_rect_min, node_rect_max, ImColor(100, 100, 100), 4.0f, 15, 2);

    ImGui::EndGroup();

    draw_list->ChannelsSetCurrent(1); // set to background

    ImColor color = IM_COL32_WHITE;

    ImVec2 conMin = ImVec2(node_rect_min.x + node->size.x - 3.5f, node_rect_min.y + (node->size.y / 2.5f) - 3.5f);
    ImVec2 conMax = ImVec2(node_rect_min.x + node->size.x + 3.5f, node_rect_min.y + (node->size.y / 2.5f) + 3.5f);

    if (ImRect((conMin), (conMax)).Contains(mouse))
    {
        connectionHovered = true;

        color = IM_COL32_BLACK;

        if (!connection_selected && ImGui::IsMouseDown(0) && !ImGui::IsMouseDragging(0))
        {
            connection_selected = std::unique_ptr<Node>(node);

            std::cout << "node" << connection_selected->id << std::endl;
        }
        
        if (connection_selected && ImGui::IsMouseReleased(0) && connection_selected.get() == node)
        {
            connection_selected.release();
        }
    }

    if(node == connection_selected.get())
    {
        color = ImColor(150,150,150);
    }
    
    pos = node_rect_min + node->size;
    pos.y = node_rect_min.y + (node->size.y / 2.5f);

    draw_list->AddCircleFilled(pos, NODE_SLOT_RADIUS, color);

    if (node_widgets_active || node_moving_active)
        node_selected = node->id;

    if (node_moving_active && ImGui::IsMouseDragging(0))
        node->pos = node->pos + ImGui::GetIO().MouseDelta;

    ImGui::PopID();

    draw_list->ChannelsMerge();
}

void RenderLines(ImDrawList* draw_list, ImVec2 offset)
{
    //draw_list->ChannelsSetCurrent(0); // set to background

    for (Node* node : nodes)
    {
        for (Node* con : node->outputConnections)
        {
            if (con != nullptr)
            {
                ImVec2 p1 = (offset + node->pos + ImVec2(node->size.x + 4, node->size.y / 2.5f));
                ImVec2 p2 = (offset + con->pos + ImVec2(0, 25));
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

static Node* CreateNode(int id, const char* name, ImVec2 size, ImVec2 pos = {0,0}, NodeType nodeType = DEFAULT)
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

    ImVec2 titleSize = ImGui::CalcTextSize(node->name);

    titleSize.y *= 3;

    //node->size.x = titleSize.x + NODE_WINDOW_PADDING.x;

    return node;
}

void SaveChild(json& nodeJson, int i, Node* currentNode)
{
    nodeJson[i]["ID"] = currentNode->id;
    nodeJson[i]["Name"] = currentNode->name;
    //nodeJson[i]["Position"][0] = currentNode->pos.x;
    //nodeJson[i]["Position"][1] = currentNode->pos.y;

    nodeJson[i]["Type"] = currentNode->type;

    currentNode->SaveTypeData(nodeJson, i);

    for (int j = 0; j < currentNode->outputConnections.size(); ++j)
    {
        SaveChild(nodeJson[i]["Children"], j, currentNode->outputConnections[j]);
    }
}

void SaveToJson()
{
    json nodeJson;

    nodeJson["Root"]["Children"];

    std::vector<Node*> currentNodes = nodes[0]->outputConnections;

    for (int i = 0; i < currentNodes.size(); ++i)
    {
        SaveChild(nodeJson["Root"]["Children"], i, currentNodes[i]);
    }

    std::ofstream file("nodes.json");
    file << nodeJson;
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

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (nodes.size() == 0)
        {
            nodes.push_back(CreateNode(-1, "Default Node", { 160,50 }, { 25, 40 }, DEFAULT));
            nodes.push_back(CreateNode(++nodeNum, "Example Dialogue Node", { 400,110 }, { 250, 80 }, DIALOGUE));

            nodes[0]->outputConnections.push_back(nodes[1]);
        }

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

        {
            bool open_context_menu = false;
            bool open_node_menu = false;
            int node_hovered_in_list = -1;
            int node_hovered_in_scene = -1;

            connectionHovered = false;

            static ImVec2 scrolling = ImVec2(0.0f, 0.0f);

            ImGui::BeginChild("node_list", ImVec2(100, 0));
            ImGui::Text("Nodes");
            ImGui::Separator();

            for (size_t i = 0; i < nodes.size(); ++i)
            {
                Node* node = nodes[i];

                ImGui::PushID(node->id);

                if (ImGui::Selectable(node->name, node->id == node_selected))
                    node_selected = node->id;
                if (ImGui::IsItemHovered())
                {
                    node_hovered_in_list = node->id;
                    open_context_menu |= ImGui::IsMouseClicked(1);
                }

                ImGui::PopID();
            }

            ImGui::EndChild();

            ImGui::SameLine();
            ImGui::BeginGroup();

            // Create our child canvas
            //ImGui::Text("Hold middle mouse button to scroll (%.2f,%.2f)", scrolling.x, scrolling.y);
            //ImGui::SameLine(ImGui::GetWindowWidth() - 100);
            //ImGui::Checkbox("Show grid", &show_grid);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, IM_COL32(60, 60, 70, 200));
            ImGui::BeginChild("scrolling_region", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
            ImGui::PushItemWidth(120.0f);

            ImVec2 offset = ImGui::GetCursorScreenPos() + scrolling;
            ImDrawList* draw_list = ImGui::GetWindowDrawList();

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

            // Display links
            //draw_list->ChannelsSplit(2);
            //draw_list->ChannelsSetCurrent(0); // Background

            RenderLines(draw_list, offset);

            //Display Nodes
            for (Node* node : nodes)
            {
                DrawNode(draw_list, node, offset,node_selected);
            }

            //draw_list->ChannelsSetCurrent(0); // set to background
            //UpdateDragging(scrolling);


            if (connectionHovered == false && ImGui::IsMouseReleased(0))
            {
                connection_selected.release();
            }

            // Open context menu
            if (!ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1))
            {
                node_selected = node_hovered_in_list = node_hovered_in_scene = -1;
                open_context_menu = true;
            }
            else if (ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1))
            {
                if (node_selected != -1)
                    open_node_menu = true;
            }
            if (open_context_menu)
            {
                ImGui::OpenPopup("context_menu");
            }
            else if(open_node_menu)
            {
                ImGui::OpenPopup("node_menu");
            }

            // Draw context menu
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
            if (ImGui::BeginPopup("context_menu"))
            {
                if (ImGui::MenuItem("Add Dialogue Node"))
                {
                    ImVec2 mousePos = ImGui::GetMousePos() - scrolling;
                    mousePos.x -= 325;
                    mousePos.y -= 100;

                    nodes.push_back(CreateNode(++nodeNum, "Dialogue Node", { 395,105 }, mousePos, DIALOGUE));
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
                    SaveToJson();
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
                    for (int i = 0; i < nodes.size(); ++i)
                    {
                        if (nodes[i]->id == node_selected)
                        {
                            nodes.erase(nodes.begin() + i);
                            continue;
                        }
                        
                        for (int j = 0; j < nodes[i]->outputConnections.size(); ++j)
                        {
                            if (nodes[i]->outputConnections[j]->id == node_selected)
                            {
                                nodes[i]->outputConnections.erase(nodes[i]->outputConnections.begin() + j);
                            }
                        }
                    }
                }
                ImGui::EndPopup();
            }
            ImGui::PopStyleVar();

            // Scrolling
            if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(2, 0.0f))
                scrolling = scrolling + ImGui::GetIO().MouseDelta;

            ImGui::PopItemWidth();
            ImGui::EndChild();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar(2);
            ImGui::EndGroup();
        }

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