
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

#include <math.h>
#include <vector>
#include <string>
#include <iostream>

static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }

GLFWwindow* window;

const ImVec2 NODE_WINDOW_PADDING(8.0f, 8.0f);
const float NODE_SLOT_RADIUS = 5.0f;

static int node_selected = -1;;
static bool show_grid = true;

struct Node
{
    int id = -1;
    const char* name = "noName";

    ImVec2 pos;
    ImVec2 size;

    std::vector<Node*> outputConnections;
};

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

void DrawNode(ImDrawList* draw_list, Node* node, ImVec2 offset, int& node_selected) // Todo - go back add connections and dragging
{
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
    pos.x = node_rect_min.x + (node->size.x / 2) - textSize.x / 2;

    ImGui::SetCursorScreenPos(pos);
    ImGui::Text("%s", node->name);

    pos.y += 20;

    char playerInput[16] = ""; // Todo - give text nodes their own char for storing text

    ImGui::SetCursorScreenPos(pos);

    std::string label = "###" + std::to_string(node->id);

    ImGui::InputText( label.c_str(),playerInput, IM_ARRAYSIZE(playerInput));

    bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());

    ImVec2 node_rect_max = node_rect_min + node->size;

    node_rect_max = node_rect_max + ImVec2(IM_ARRAYSIZE(playerInput) * 3.5, 0);

    draw_list->ChannelsSetCurrent(0); // set to background

    ImGui::SetCursorScreenPos(node_rect_min);
    ImGui::InvisibleButton("node", node->size);

    if (ImGui::IsItemHovered())
    {
        node_hovered_in_scene = node->id;
        open_context_menu |= ImGui::IsMouseClicked(1);
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
    draw_list->AddRect(node_rect_min, node_rect_max, ImColor(100, 100, 100), 4.0f);

    draw_list->ChannelsSetCurrent(1); // set to background

    ImColor color = IM_COL32_WHITE;

    if (ImRect((node_rect_min + ImVec2(128, 8)), (node_rect_min + ImVec2(135, 15))).Contains(mouse))
    {
        color = IM_COL32_BLACK;

        if (!connection_selected && ImGui::IsMouseClicked(0))
        {
            connection_selected = std::unique_ptr<Node>(node);

            std::cout << "node" << connection_selected->id << std::endl;
        }
        else if (connection_selected && ImGui::IsMouseReleased(0) && connection_selected.get() != node)
        {
            connection_selected->outputConnections.push_back(node);

            connection_selected.release();
        }
    }

    if(node == connection_selected.get())
    {
        color = ImColor(150,150,150);
    }
    
    draw_list->AddCircleFilled(node_rect_min + ImVec2(133,12), NODE_SLOT_RADIUS, color);

    //offset.y += 40.0f;
    //
    //offset = offset + node_rect_min;
    //
    //for (Connection* con : node->inputConnections)
    //{
    //    ImGui::SetCursorScreenPos(offset + ImVec2(10.0f, 0));
    //    //ImGui::Text("%s", con->desc.name);
    //
    //    ImColor conColor = ImColor(150, 150, 150);
    //
    //    if (IsConnectorHovered(con, node_rect_min))
    //        conColor = ImColor(200, 200, 200);
    //
    //    draw_list->AddCircleFilled(node_rect_min + con->pos, NODE_SLOT_RADIUS, conColor);
    //
    //    offset.y += textSize.y + 2.0f;
    //}
    //
    //offset = node_rect_min;
    //offset.y += 40.0f;
    //
    //for (Connection* con : node->outputConnections)
    //{
    //    //textSize = ImGui::CalcTextSize(con->desc.name);
    //
    //    ImGui::SetCursorScreenPos(offset + ImVec2(con->pos.x - (textSize.x + 10.0f), 0));
    //    //ImGui::Text("%s", con->desc.name);
    //
    //    ImColor conColor = ImColor(150, 150, 150);
    //
    //    if (IsConnectorHovered(con, node_rect_min))
    //        conColor = ImColor(200, 200, 200);
    //
    //    draw_list->AddCircleFilled(node_rect_min + con->pos, NODE_SLOT_RADIUS, conColor);
    //
    //    offset.y += textSize.y + 2.0f;
    //}

    if (node_widgets_active || node_moving_active)
        node_selected = node->id;

    if (node_moving_active && ImGui::IsMouseDragging(0))
        node->pos = node->pos + ImGui::GetIO().MouseDelta;

    ImGui::PopID();
}

void RenderLines(ImDrawList* draw_list, ImVec2 offset)
{
    draw_list->ChannelsSetCurrent(0); // set to background

    for (Node* node : nodes)
    {
        for (Node* con : node->outputConnections)
        {
            DrawHermiteCurve(draw_list, offset + node->pos + ImVec2(385, 75), offset + con->pos + ImVec2(250, 75), 20);
        }
    }
}

static Node* CreateNode(int id, const char* name, ImVec2 size, ImVec2 pos = {0,0})
{
    Node* node = new Node();

    node->id = id;
    node->name = name;
    node->size = size;
    node->pos = pos;

    ImVec2 titleSize = ImGui::CalcTextSize(node->name);

    titleSize.y *= 3;

    //for (Connection* c : node->inputConnections)
    //{
    //    c->pos = ImVec2(0.0f, titleSize.y / 2.0f);

    //    c->pos.y += 3;
    //}

    //// set the positions for the output nodes when we know where the place them
    //for (Connection* c : node->outputConnections)
    //{
    //    c->pos = ImVec2(node->size.x, titleSize.y / 2.0f);

    //    c->pos.x *= 1.7f;
    //    c->pos.y += 3;
    //}

    return node;
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
            nodes.push_back(CreateNode(nodes.size(), "Name Node", { 80,50 }, { 25, 40 }));
            nodes.push_back(CreateNode(nodes.size(), "Name Node", { 80,50 }, { 250, 80 }));
            nodes.push_back(CreateNode(nodes.size(), "Name Node", { 80,50 }, { 500, 120 }));

            nodes[0]->outputConnections.push_back(nodes[1]);
            nodes[1]->outputConnections.push_back(nodes[2]);
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
            int node_hovered_in_list = -1;
            int node_hovered_in_scene = -1;

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
            draw_list->ChannelsSplit(2);
            //draw_list->ChannelsSetCurrent(0); // Background

            //Display Nodes
            for (Node* node : nodes)
            {
                DrawNode(draw_list, node, offset,node_selected);
            }

            draw_list->ChannelsSetCurrent(0); // set to background
            //UpdateDragging(scrolling);
            RenderLines(draw_list, scrolling);

            draw_list->ChannelsMerge();

            // Open context menu
            if (!ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1))
            {
                node_selected = node_hovered_in_list = node_hovered_in_scene = -1;
                open_context_menu = true;
            }
            if (open_context_menu)
            {
                ImGui::OpenPopup("context_menu");
                if (node_hovered_in_list != -1)
                    node_selected = node_hovered_in_list;
                if (node_hovered_in_scene != -1)
                    node_selected = node_hovered_in_scene;
            }

            // Draw context menu
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
            if (ImGui::BeginPopup("context_menu"))
            {
                if (ImGui::MenuItem("Add Node"))
                {
                    ImVec2 mousePos = ImGui::GetMousePos() - scrolling;
                    mousePos.x -= 325;
                    mousePos.y -= 100;

                    nodes.push_back(CreateNode(nodes.size(), "Name Node", { 80,50 }, mousePos));
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

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

	return 0;
}