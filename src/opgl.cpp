#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <windows.h>
#include <gdiplus.h>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <vector>

// Vertex Shader Source
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
void main() {
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    TexCoord = aTexCoord;
}
)";

// Fragment Shader Source
const char* fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
uniform sampler2D texture1;
out vec4 FragColor;
void main() {
    FragColor = texture(texture1, TexCoord);
}
)";

GLuint loadTexture(const wchar_t* path) {
    Gdiplus::Bitmap image(path);
    if (image.GetLastStatus() != Gdiplus::Ok) {
        std::wcerr << L"Failed to load texture: " << path << std::endl;
        return 0;
    }

    const UINT width = image.GetWidth();
    const UINT height = image.GetHeight();
    Gdiplus::Rect rect(0, 0, width, height);
    Gdiplus::BitmapData bitmapData{};
    if (image.LockBits(&rect, Gdiplus::ImageLockModeRead,
                       PixelFormat32bppARGB, &bitmapData) != Gdiplus::Ok) {
        std::wcerr << L"Failed to read texture pixels: " << path << std::endl;
        return 0;
    }

    std::vector<unsigned char> pixels(
        static_cast<size_t>(width) * height * 4);
    const auto* source = static_cast<const unsigned char*>(bitmapData.Scan0);
    for (UINT row = 0; row < height; ++row) {
        const auto* sourceRow = source + static_cast<ptrdiff_t>(row) * bitmapData.Stride;
        auto* destinationRow = pixels.data() +
            static_cast<size_t>(height - 1 - row) * width * 4;
        std::memcpy(destinationRow, sourceRow, static_cast<size_t>(width) * 4);
    }
    image.UnlockBits(&bitmapData);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                 GL_BGRA, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    return texture;
}

// Window resize callback
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

int main() {
    // 1. Initialize GLFW and request OpenGL 3.3 Core Profile
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. Create the window
    GLFWwindow* window = glfwCreateWindow(800, 600, "Step 1: Modern OpenGL Triangle", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 3. Load function pointers via GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Gdiplus::Ok) {
        std::cerr << "Failed to initialize GDI+" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // 4. Compile Vertex Shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // 5. Compile Fragment Shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // 6. Link Shaders into a Shader Program
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Delete individual shaders now that they're linked into the program
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    const GLuint texture = loadTexture(L"..\\assets\\brickwall.png");
    if (texture == 0) {
        glDeleteProgram(shaderProgram);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

    // 7. Define triangle coordinates in Normalized Device Coordinates (NDC)
    float vertices[] = {
    // Positions      // Texture Coords
    -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, // Bottom Left
     0.5f, -0.5f, 0.0f,  1.0f, 0.0f, // Bottom Right
     0.0f,  0.5f, 0.0f,  0.5f, 1.0f  // Top Center
};

    // 8. Generate and bind VAO & VBO
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // Bind VAO first, then bind and configure VBO
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Configure position and texture-coordinate attributes.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Unbind buffer and array object to prevent unintended modification
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // 9. Main Render Loop
    while (!glfwWindowShouldClose(window)) {
        // Handle input (close window on ESC)
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // Clear screen with a dark grey background
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw triangle using our shader program and VAO
        glUseProgram(shaderProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // Swap buffers and poll window events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 10. Clean up GPU memory
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteTextures(1, &texture);
    glDeleteProgram(shaderProgram);

    Gdiplus::GdiplusShutdown(gdiplusToken);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}