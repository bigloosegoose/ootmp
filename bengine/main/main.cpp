#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <map>
#include <random>
#include <memory>
#include <filesystem>
#include <algorithm>

#include <thread>
#include <queue>
#include <mutex>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "stb_image.h"
#include "my_texture.h"
#include "my_movieLoader.h"

#include "my_UI.h"
#include "model.h"
#include "camera.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


#include <soloud/soloud.h>
#include <soloud/soloud_wav.h>

using std::cout, std::endl, std::string, std::vector;

SoLoud::Soloud soloud; //audio engine
SoLoud::Wav sample;     //one wave file

std::mutex cmdMutex;
std::queue<string> cmdQueue;

GLFWwindow* window;
namespace fs = std::filesystem;

constexpr float PI = 3.1415926535f;

//movie vars
double playStart = 0.0;
double framePts = 0.0;
bool havePending = false;



//settings
int width = 800;
int height = 600;
bool isFullscreen = false;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

float lastX = width / 2.0f;
float lastY = height / 2.0f;
bool firstMouse = true;

bool cursorEnabled = false;

float quadVertices[] = {
	// positions	// texCoords
	-1.0f,  1.0f,	0.0f, 0.0f,
	-1.0f, -1.0f,	0.0f, 1.0f,
	 1.0f, -1.0f,	1.0f, 1.0f,
	-1.0f,  1.0f,	0.0f, 0.0f,
	 1.0f, -1.0f,	1.0f, 1.0f,
	 1.0f,  1.0f,	1.0f, 0.0f
};

string extensions[] = { ".mp4",".webm",".mov",".avi",".mkv",".wmv",".flv",".f4v",".avchd" };
string path;
fs::path currentDir = fs::current_path();
fs::path targetDir = currentDir;

//UI ELEMENTS
std::unique_ptr<UI> cross_button;

//functions
void processInput(GLFWwindow* window);

void framebuffer_size_callback(GLFWwindow* window, int fwidth, int fheight);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void toggleMouse(GLFWwindow* window);

void uiDefine(ImGuiIO io);
void uiDraw();
void uiRefresh();
void uiClickCheck();

void showFiles();
void prompt();
void changeDir(fs::directory_entry target);
//****************************************************************************************************************************************
int main() {

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	window = glfwCreateWindow(width, height, "screen", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	//important -> load all the function pointers for openGL before calling any of its functions
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {

		std::cout << "Failed to initialize GLAD" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwHideWindow(window);

	//setting up imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();
	//Setup platform for imgui
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	//setting up soloud(audio)
	soloud.init();
	sample.setVolume(1.0f);

	glfwSetKeyCallback(window, key_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_STENCIL_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);

	camera.SetCameraMode(FLY);




	//***********************************************************************************************
	//quad VAO, VBO
	unsigned int quadVAO, quadVBO;
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);

	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	//VertexPos Attribute
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	//texture attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	//***********************************************************************************************
	//frame buffers
	unsigned int FBO;
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	unsigned int texColorBuffer; 
	glGenTextures(1, &texColorBuffer);
	glBindTexture(GL_TEXTURE_2D, texColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0); // unbinding so its not interfered with / interfering
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColorBuffer, 0);


	unsigned int RBO;
	glGenRenderbuffers(1, &RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);
	
	Shader screenShader(R"(shaders/lab.vert)", R"(shaders/lab.frag)");
	Shader buttonShader(R"(shaders/lab.vert)", R"(shaders/lab.frag)");

	//to be executed last
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//***********************************************************************************************
	//movie Frame Tex
	glGenTextures(1, &movieTexture);
	glBindTexture(GL_TEXTURE_2D, movieTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	//***********************************************************************************************

	//unbind all the stuff
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glLinkProgram(0);

	//start stuff
	soloud.play(sample);
	camera.Position = glm::vec3(0.0f, -20.0f, 100.0f);

	//######################################################################################################
	//render loop
	cout << "all set!" << endl;
	glViewport(0, 0, width, height);
	while (!glfwWindowShouldClose(window)) {
		while (ended) {
			prompt();
			glfwHideWindow(window);
		}

		glfwShowWindow(window);

		float currentTime = (float)glfwGetTime();
		deltaTime = currentTime - lastFrame;
		lastFrame = currentTime;

		uiClickCheck();
		processInput(window);

		//matrices
		glm::mat4 model = glm::mat4(1.0f);
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)width/(float)height, 0.1f, 150.0f);
		glm::mat4 view = camera.GetViewMatrix();
		

		//first pass bruh
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		//making the cli first
		if (!havePending) {
			havePending = decodeNextFrame();
			if (havePending) framePts = frame->best_effort_timestamp * timeBase;
			else closeMovie();
		}
		double clock = hasAudio ? soloud.getStreamTime(audioHandle) : glfwGetTime() - playStart;
		if (havePending && clock >= framePts) {
			uploadFrame();
			havePending = false;
		}



		//second pass (not anymore)
		glDisable(GL_DEPTH_TEST);
		screenShader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, movieTexture);
		screenShader.setInt("screenTexture", 0);
		screenShader.setMat4("model", model);
		screenShader.setBool("hasTexture", true);

		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);


		//unbind stuff 
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glEnable(GL_DEPTH_TEST);
		
		////UI
		//uiDefine(io);	//Design
		//uiDraw();		//Render

		//swap
		glfwSwapBuffers(window);
		glfwPollEvents();


	}
//######################################################################################################

	soloud.deinit();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}

	if (key == GLFW_KEY_LEFT_ALT && action == GLFW_PRESS) {
		toggleMouse(window);
	}



}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {

		float xpos = static_cast<float>(xposIn);
		float ypos = static_cast<float>(yposIn);

		if (firstMouse) {
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos; //reversed, y goes bottom to top
		lastX = xpos;
		lastY = ypos;

		clickPosX = xpos;
		clickPosY = ypos;

		if (cursorEnabled == false)
		{
		camera.ProcessMouseMovement(xoffset, yoffset, deltaTime);
		}
}

void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS) {
		if (!isFullscreen) {
			glfwGetWindowPos(window, &width, &height);
			glfwGetWindowSize(window, &width, &height);

			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);

			// 3. Switch to full screen
			glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
			isFullscreen = true;
		}
		else {
			// 4. Restore to windowed mode (passing NULL for the monitor parameter)
			glfwSetWindowMonitor(window, NULL, width, height, width, height, 0);
			isFullscreen = false;
		}
	}
	
	
}

void framebuffer_size_callback(GLFWwindow* window, int fwidth, int fheight) {
	glViewport(0, 0, fwidth, fheight);
	if (fwidth == 0 || fheight == 0) {
		return;
	}

	width = fwidth;
	height = fheight;

	screenWidth = fwidth;
	screenHeight = fheight;

	uiRefresh();

}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS && cursorEnabled == true) 
	{
		mouseClicked = true;
		//cout << lastX - width/2 << "," <<height/2 - lastY<< endl;


	}
	else {
		mouseClicked = false;
	}
}

void toggleMouse(GLFWwindow* window) {
	if (cursorEnabled == true) {
		cursorEnabled = false;
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
	else {
		cursorEnabled = true;
		firstMouse = true;
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}

void uiDefine(ImGuiIO io) {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	{
		ImGui::Begin("Pending Panel");
		ImGui::Text("Application Average: %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		ImGui::End();
	}
}

void uiDraw() {
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	for (unsigned int i = 0; i < ui_elements.size(); i++) {
		ui_elements[i]->DrawButton();
	}
}

void uiRefresh() {

	for (unsigned int i = 0; i < ui_elements.size(); i++) {
		ui_elements[i]->RefreshVars();
	}

}

void uiClickCheck() {
	for (unsigned int i = 0; i < ui_elements.size(); i++) {
		ui_elements[i]->CheckForCLick();
	}
}



void prompt() {
	string prompt;
	string element = "";
	vector <string> command;

	cout << "ootmp " << currentDir << ">>> ";
	std::getline(std::cin, prompt);
	prompt += ' ';
	cout << endl;

	
	for (const auto& x : prompt) {
		if (x == ' ' && !element.empty()) {
			command.push_back(element);
			element = "";
		}
		else {
			element += x;
		}
	}

	if (command.size() < 2) { 
		command.push_back("");
		command.push_back("");
	}
	
	if (command[0] == "cd") changeDir(fs::directory_entry(command[1]));
	else if (command[0] == "dir") showFiles();
	else if (command[0] == "play") {
		if (openMovie(command[1]) == 0) {
			if (hasAudio) audioHandle = soloud.play(movieAudio);
			playStart = glfwGetTime();
			havePending = false;
		}
	}
	else if (command[0] == "help") {
		cout << "cd {target directory}				 Changes directory" << endl;
		cout << "dir								 Lists files and folders in the current directory" << endl;
		cout << "play {file path}					 Plays the video at the specified path"<< endl;
	}
	else cout << "\"" << command[0] << "\" is not a valid ootmp command!" << endl;
}
	

void showFiles() {
	cout << "[Parent Folder] .." << endl;
	for (const auto& entry : fs::directory_iterator(fs::current_path())) {
		if (entry.is_directory()) {
			cout << "[Folder] " << entry.path().filename().string() << endl;
		}
		else if (entry.is_regular_file() && std::find(std::begin(extensions), std::end(extensions), entry.path().extension()) != std::end(extensions)) {
			cout << "[File]" << entry.path().filename().string() << endl;

		}
	}
	cout << endl;
}

void changeDir(fs::directory_entry target) {
	if (target == fs::directory_entry("..")) {
		cout << "Changed directory to " << currentDir.parent_path() << endl;
		currentDir = currentDir.parent_path();
		fs::current_path(currentDir);
	}
	else if (target.is_directory()) {
		cout << "Changed directory to " << target << endl;
		currentDir += fs::directory_entry("\\" + target.path().filename().string());
		fs::current_path(currentDir);

	}
	else cout << "directory " << target << " not found!" << endl;
	cout << endl;
}
