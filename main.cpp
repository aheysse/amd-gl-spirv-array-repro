// Draws the same fragment shader twice, once loaded as SPIR-V (glShaderBinary + glSpecializeShader) and once
// compiled from GLSL source (glShaderSource), and compares the two.
//
//   array      samples layer 1 of a two-layer GL_TEXTURE_2D_ARRAY, which is solid green
//   control2d  samples a GL_TEXTURE_2D, which is solid yellow
//
// Every texture unit has both textures bound, each on its own target. --no-2d leaves the GL_TEXTURE_2D target of
// every unit empty.
//
// Usage: repro [array | control2d] [--no-2d] [--no-wait]

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {

	constexpr int WindowWidth = 800;
	constexpr int WindowHeight = 400;
	constexpr int TextureSize = 4;
	constexpr int LayerCount = 2;

	std::vector<char> ReadFile(const std::string& path) {
		std::ifstream file(path, std::ios::binary);
		if (!file) {
			std::printf("Could not open %s\n", path.c_str());
		}
		return std::vector<char>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	}

	void PrintShaderLog(const GLuint shader, const char* const label) {
		GLint compiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		char log[1024] = {};
		glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
		if (compiled == 0 || log[0] != '\0') {
			std::printf("%s: compiled=%d log=%s\n", label, compiled, log);
		}
	}

	GLuint LinkProgram(const GLuint vertexShader, const GLuint fragmentShader, const char* const label) {
		const GLuint program = glCreateProgram();
		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);
		glLinkProgram(program);
		GLint linked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &linked);
		char log[1024] = {};
		glGetProgramInfoLog(program, sizeof(log), nullptr, log);
		if (linked == 0 || log[0] != '\0') {
			std::printf("%s: linked=%d log=%s\n", label, linked, log);
		}
		return program;
	}

	GLuint LoadSpirvShader(const GLenum stage, const std::string& path) {
		const std::vector<char> binary = ReadFile(path);
		const GLuint shader = glCreateShader(stage);
		glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, binary.data(), static_cast<GLsizei>(binary.size()));
		glSpecializeShader(shader, "main", 0, nullptr, nullptr);
		PrintShaderLog(shader, path.c_str());
		return shader;
	}

	GLuint LoadGlslShader(const GLenum stage, const std::string& path) {
		const std::vector<char> source = ReadFile(path);
		const char* const sourcePointer = source.data();
		const GLint sourceLength = static_cast<GLint>(source.size());
		const GLuint shader = glCreateShader(stage);
		glShaderSource(shader, 1, &sourcePointer, &sourceLength);
		glCompileShader(shader);
		PrintShaderLog(shader, path.c_str());
		return shader;
	}

	GLuint CreateArrayTexture() {
		// layer 0 is red, layer 1 is green
		std::vector<std::uint8_t> texels(TextureSize * TextureSize * LayerCount * 4);
		for (int layer = 0; layer < LayerCount; ++layer) {
			for (int texel = 0; texel < TextureSize * TextureSize; ++texel) {
				std::uint8_t* const rgba = &texels[(layer * TextureSize * TextureSize + texel) * 4];
				rgba[0] = layer == 0 ? 255 : 0;
				rgba[1] = layer == 1 ? 255 : 0;
				rgba[2] = 0;
				rgba[3] = 255;
			}
		}

		GLuint texture = 0;
		glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &texture);
		glTextureStorage3D(texture, 1, GL_RGBA8, TextureSize, TextureSize, LayerCount);
		glTextureSubImage3D(texture, 0, 0, 0, 0, TextureSize, TextureSize, LayerCount, GL_RGBA, GL_UNSIGNED_BYTE, texels.data());
		glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		return texture;
	}

	GLuint CreatePlainTexture() {
		// solid yellow, so it can't be mistaken for either array layer
		std::vector<std::uint8_t> texels(TextureSize * TextureSize * 4);
		for (int texel = 0; texel < TextureSize * TextureSize; ++texel) {
			std::uint8_t* const rgba = &texels[texel * 4];
			rgba[0] = 255;
			rgba[1] = 255;
			rgba[2] = 0;
			rgba[3] = 255;
		}

		GLuint texture = 0;
		glCreateTextures(GL_TEXTURE_2D, 1, &texture);
		glTextureStorage2D(texture, 1, GL_RGBA8, TextureSize, TextureSize);
		glTextureSubImage2D(texture, 0, 0, 0, TextureSize, TextureSize, GL_RGBA, GL_UNSIGNED_BYTE, texels.data());
		glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		return texture;
	}

	std::array<std::uint8_t, 4> ReadPixel(const int x, const int y) {
		std::array<std::uint8_t, 4> pixel{};
		glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
		return pixel;
	}
}

int main(const int argumentCount, const char* const* const arguments) {
	std::string fragmentName = "array";
	bool waitForClose = true;
	bool bindPlainTexture = true;
	for (int argumentIndex = 1; argumentIndex < argumentCount; ++argumentIndex) {
		const std::string argument = arguments[argumentIndex];
		if (argument == "--no-wait") {
			waitForClose = false;
		}
		else if (argument == "--no-2d") {
			bindPlainTexture = false;
		}
		else {
			fragmentName = argument;
		}
	}

	if (glfwInit() == GLFW_FALSE) {
		std::printf("glfwInit failed\n");
		return 2;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* const window = glfwCreateWindow(WindowWidth, WindowHeight, "left: SPIR-V    right: GLSL    (both halves should match)", nullptr, nullptr);
	if (window == nullptr) {
		std::printf("glfwCreateWindow failed\n");
		return 2;
	}
	glfwMakeContextCurrent(window);
	gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));

	std::printf("GL_RENDERER: %s\nGL_VERSION:  %s\n", glGetString(GL_RENDERER), glGetString(GL_VERSION));

	const std::string shaderDirectory = SHADER_DIR;
	const GLuint spirvProgram = LinkProgram(
		LoadSpirvShader(GL_VERTEX_SHADER, shaderDirectory + "/fullscreen.vert.spv"),
		LoadSpirvShader(GL_FRAGMENT_SHADER, shaderDirectory + "/" + fragmentName + ".frag.spv"), "SPIR-V program");
	const GLuint glslProgram = LinkProgram(
		LoadGlslShader(GL_VERTEX_SHADER, shaderDirectory + "/fullscreen.vert"),
		LoadGlslShader(GL_FRAGMENT_SHADER, shaderDirectory + "/" + fragmentName + ".frag"), "GLSL program");

	const GLuint arrayTexture = CreateArrayTexture();
	const GLuint plainTexture = CreatePlainTexture();

	GLuint vertexArray = 0;
	glCreateVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	int framebufferWidth = 0;
	int framebufferHeight = 0;
	glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
	const int halfWidth = framebufferWidth / 2;

	int exitCode = 0;
	bool reported = false;
	while (glfwWindowShouldClose(window) == GLFW_FALSE) {
		glViewport(0, 0, framebufferWidth, framebufferHeight);
		glClearColor(0.f, 0.f, 1.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);

		// every unit, so the result doesn't depend on which unit a sampler ends up reading
		for (GLuint unit = 0; unit < 8; ++unit) {
			glBindTextureUnit(unit, arrayTexture);
			if (bindPlainTexture) {
				glBindTextureUnit(unit, plainTexture);
			}
		}

		glViewport(0, 0, halfWidth, framebufferHeight);
		glUseProgram(spirvProgram);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glViewport(halfWidth, 0, framebufferWidth - halfWidth, framebufferHeight);
		glUseProgram(glslProgram);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		if (!reported) {
			glFinish();
			const std::array<std::uint8_t, 4> spirvPixel = ReadPixel(halfWidth / 2, framebufferHeight / 2);
			const std::array<std::uint8_t, 4> glslPixel = ReadPixel(halfWidth + halfWidth / 2, framebufferHeight / 2);
			std::printf("fragment shader: %s%s\n", fragmentName.c_str(), bindPlainTexture ? "" : " (GL_TEXTURE_2D targets left empty)");
			std::printf("SPIR-V program: %u %u %u %u\n", spirvPixel[0], spirvPixel[1], spirvPixel[2], spirvPixel[3]);
			std::printf("GLSL program:   %u %u %u %u\n", glslPixel[0], glslPixel[1], glslPixel[2], glslPixel[3]);
			const bool matches = spirvPixel == glslPixel;
			std::printf("%s\n", matches ? "PASS: the SPIR-V and GLSL programs match" : "FAIL: the SPIR-V program does not match the GLSL program");
			std::fflush(stdout);
			exitCode = matches ? 0 : 1;
			reported = true;
			if (!waitForClose) {
				break;
			}
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return exitCode;
}
