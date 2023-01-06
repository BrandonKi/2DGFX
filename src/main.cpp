#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <glm.hpp>

#include "Bitmap.h"
#include "Painter.h"

static void render_line(Bitmap& bitmap, Painter& painter, glm::ivec2 start, glm::ivec2 end) {
    auto d = end - start;

	i64 step;
    if (glm::abs(d.x) >= glm::abs(d.y))
        step = glm::abs(d.x);
    else
        step = glm::abs(d.y);

    float dx = (float)d.x / step;
    float dy = (float)d.y / step;
    float x = start.x;
    float y = start.y;

    for(int i = 1; i <= step; ++i) {
        painter.fill_pixel(glm::round(x), glm::round(y));
	    x = x + dx;
        y = y + dy;
    }
}

static auto rfract(auto x) {
    return 1 - glm::fract(x);
}

static void render_AA_line(Bitmap& bitmap, Painter& painter, glm::ivec2 start, glm::ivec2 end) {
    bool steep = glm::abs(end.y - start.y) > glm::abs(end.x - start.x);
    
    if(steep) {
        std::swap(start.x, start.y);
        std::swap(end.x, end.y);
    }

    if(start.x > end.x) {
        std::swap(start.x, end.x);
        std::swap(start.y, end.y);
    }
    
    auto dx = end.x - start.x;
    auto dy = end.y - start.y;

    float gradient = 1.0;
    if(dx != 0) {
        gradient = (float)dy / dx;
    }

    // handle first endpoint
    auto xend = glm::round(start.x);
    auto yend = start.y + gradient * (xend - start.x);
    auto xgap = rfract(start.x + 0.5);
    auto xpxl1 = xend; // this will be used in the main loop
    auto ypxl1 = glm::floor(yend);
    if(steep) {
        painter.fill_pixel(ypxl1,   xpxl1, rfract(yend) * xgap * 255);
        painter.fill_pixel(ypxl1+1, xpxl1,  glm::fract(yend) * xgap * 255);
    }
    else {
        painter.fill_pixel(xpxl1, ypxl1  , rfract(yend) * xgap * 255);
        painter.fill_pixel(xpxl1, ypxl1+1,  glm::fract(yend) * xgap * 255);
    }
    auto intery = yend + gradient; // first y-intersection for the main loop
    
    // handle second endpoint
    xend = glm::round(end.x);
    yend = end.y + gradient * (xend - end.x);
    xgap = glm::fract(end.x + 0.5);
    auto xpxl2 = xend; //this will be used in the main loop
    auto ypxl2 = glm::floor(yend);
    if(steep) {
        painter.fill_pixel(glm::round(ypxl2)  , glm::round(xpxl2), rfract(yend) * xgap * 255);
        painter.fill_pixel(glm::round(ypxl2+1), glm::round(xpxl2),  glm::fract(yend) * xgap * 255);
    }
    else {
        painter.fill_pixel(glm::round(xpxl2), glm::round(ypxl2),  rfract(yend) * xgap * 255);
        painter.fill_pixel(glm::round(xpxl2), glm::round(ypxl2+1), glm::fract(yend) * xgap * 255);
    }
    
    // main loop
    if(steep) {
        for(int x = xpxl1 + 1; x < xpxl2; ++x) {
            painter.fill_pixel(glm::floor(intery)  , x, rfract(intery) * 255);
            painter.fill_pixel(glm::floor(intery)+1, x,  glm::fract(intery) * 255);
            intery = intery + gradient;
        }
    }
    else {
        for(int x = xpxl1 + 1; x < xpxl2; ++x) {
            painter.fill_pixel(x, glm::floor(intery),  rfract(intery) * 255);
            painter.fill_pixel(x, glm::floor(intery)+1, glm::fract(intery) * 255);
            intery = intery + gradient;
        }
    }
}

static auto lerp(auto x, auto y, auto t) {
    return x + (y - x) * t;
}

static bool quadratic_bezier_curve_is_flat_enough(glm::vec2 start, glm::vec2 end, glm::vec2 control) {
    float tolerance = 0.5f;

    auto p1x = 3 * control.x - 2 * start.x - end.x;
    auto p1y = 3 * control.y - 2 * start.y - end.y;
    auto p2x = 3 * control.x - 2 * end.x - start.x;
    auto p2y = 3 * control.y - 2 * end.y - start.y;

    p1x = p1x * p1x;
    p1y = p1y * p1y;
    p2x = p2x * p2x;
    p2y = p2y * p2y;

    return glm::max(p1x, p2x) + glm::max(p1y, p2y) <= tolerance;
}

struct QuadraticBezierSegment {
    glm::vec2 p1;
    glm::vec2 p2;
    glm::vec2 control;
};

static glm::vec2 midpoint(glm::vec2 start, glm::vec2 end) {
    return {(start.x + end.x)/2, (start.y + end.y)/2};
}

static std::array<QuadraticBezierSegment, 2> split_quadratic_bezier_curve(glm::vec2 start, glm::vec2 end, glm::vec2 control) {
    glm::vec2 m1 = midpoint(start, control);
    glm::vec2 m2 = midpoint(control, end);
    glm::vec2 m3 = midpoint(m1, m2);

    return {
        QuadraticBezierSegment{start, m3, m1},
        QuadraticBezierSegment{m3, end, m2}
    };
}

static void render_quadratic_bezier_curve(Bitmap& bitmap, Painter& painter, glm::vec2 start, glm::vec2 end, glm::vec2 control) {
    if(quadratic_bezier_curve_is_flat_enough(start, end, control)) {
        render_line(bitmap, painter, start, end);
    } else {
        auto curves = split_quadratic_bezier_curve(start, end, control);
        render_quadratic_bezier_curve(bitmap, painter, curves[0].p1, curves[0].p2, curves[0].control);
        render_quadratic_bezier_curve(bitmap, painter, curves[1].p1, curves[1].p2, curves[1].control);
    }
}

int main(void) {
    GLFWwindow* window;

    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
	
	/* Initialize the library */
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
    }

    Bitmap bitmap(640, 480);
	Painter painter(bitmap, Color{0xed, 0x0d, 0x92, 0});
	// render_AA_line(bitmap, painter, {100, 300}, {150, 100});
	// render_AA_line(bitmap, painter, {150, 100}, {200, 300});
	// render_AA_line(bitmap, painter, {200, 300}, {250, 100});
	// render_AA_line(bitmap, painter, {250, 100}, {300, 300});
    render_quadratic_bezier_curve(bitmap, painter, {300, 300}, {100, 100}, {200, 300});

	bitmap.serialize_to_bmp("test.bmp");
	
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    //ImGui::StyleColorsDark();
    ImGui::StyleColorsLight();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
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
}
