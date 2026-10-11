#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

class LineRenderer {
public:
	bool initialize() {
		const char* vertexSource = R"(#version 330 core
			layout (location = 0) in vec2 position;
			void main() { gl_Position = vec4(position, 0.0, 1.0); }
		)";
		const char* fragmentSource = R"(#version 330 core
			out vec4 fragmentColor;
			uniform vec3 lineColor;
			void main() { fragmentColor = vec4(lineColor, 1.0); }
		)";

		const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
		const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
		if (!vertexShader || !fragmentShader) {
			if (vertexShader) glDeleteShader(vertexShader);
			if (fragmentShader) glDeleteShader(fragmentShader);
			return false;
		}

		program_ = glCreateProgram();
		glAttachShader(program_, vertexShader);
		glAttachShader(program_, fragmentShader);
		glLinkProgram(program_);
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		GLint linked = GL_FALSE;
		glGetProgramiv(program_, GL_LINK_STATUS, &linked);
		if (!linked) {
			GLint length = 0;
			glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &length);
			char log[1024]{};
			glGetProgramInfoLog(program_, length, nullptr, log);
			std::cerr << "Shader link error: " << log << '\n';
			glDeleteProgram(program_);
			program_ = 0;
			return false;
		}

		colorLocation_ = glGetUniformLocation(program_, "lineColor");
		glGenVertexArrays(1, &vao_);
		glGenBuffers(1, &vbo_);
		glBindVertexArray(vao_);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4, nullptr, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);
		return true;
	}

	// Coordinates use OpenGL normalized device space: [-1, 1].
	void draw(float x1, float y1, float x2, float y2,
			  float red = 1.0f, float green = 1.0f, float blue = 1.0f) const {
		const float points[] = {x1, y1, x2, y2};
		glUseProgram(program_);
		glUniform3f(colorLocation_, red, green, blue);
		glBindVertexArray(vao_);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
		glDrawArrays(GL_LINES, 0, 2);
		glBindVertexArray(0);
	}

	void shutdown() {
		if (vbo_) glDeleteBuffers(1, &vbo_);
		if (vao_) glDeleteVertexArrays(1, &vao_);
		if (program_) glDeleteProgram(program_);
		vbo_ = vao_ = program_ = 0;
	}

private:
	static GLuint compileShader(GLenum type, const char* source) {
		const GLuint shader = glCreateShader(type);
		glShaderSource(shader, 1, &source, nullptr);
		glCompileShader(shader);
		GLint compiled = GL_FALSE;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (!compiled) {
			GLint length = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
			char log[1024]{};
			glGetShaderInfoLog(shader, length, nullptr, log);
			std::cerr << "Shader compile error: " << log << '\n';
			glDeleteShader(shader);
			return 0;
		}
		return shader;
	}

	GLuint vao_ = 0;
	GLuint vbo_ = 0;
	GLuint program_ = 0;
	GLint colorLocation_ = -1;
};

int main() {
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW\n";
		return 1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Line Renderer", nullptr, nullptr);
	if (!window) {
		std::cerr << "Failed to create window\n";
		glfwTerminate();
		return 1;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
		std::cerr << "Failed to initialize GLAD\n";
		glfwDestroyWindow(window);
		glfwTerminate();
		return 1;
	}

	glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int width, int height) {
		glViewport(0, 0, width, height);
	});
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	LineRenderer renderer;
	if (!renderer.initialize()) {
		glfwDestroyWindow(window);
		glfwTerminate();
		return 1;
	}

	while (!glfwWindowShouldClose(window)) {
		glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		renderer.draw(-0.7f, -0.5f, 0.7f, 0.5f, 0.2f, 0.8f, 1.0f);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	renderer.shutdown();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
