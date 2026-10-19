#pragma once
#include "my_texture.h"
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "shader.h"
#include <string>
#include <functional>

using std::cout, std::endl, std::vector, std::string;

enum Button_Shape {
	RECTANGLE,
	CIRCLE
};

class UI;

unsigned int screenWidth = 800;
unsigned int screenHeight= 600;

float clickPosX = 1.0f;
float clickPosY = 1.0f;
bool mouseClicked = false;

vector<UI*> ui_elements;

class UI {
public:
	float xPos   = 0.0f;
	float yPos   = 0.0f;
	float width  = 0.8f;
	float height = 0.8f;
	float radius;


	unsigned int Texture = 0;

	glm::vec4 color = glm::vec4(1.0f);
	glm::vec4 boundingBox = glm::vec4(1.0f);

	bool clickable = true;
	std::function<void()> onClick;
	Shader buttonShader;	
	


	UI(unsigned int VAO, Shader shader, std::function<void()> onClickFunction = nullptr): buttonShader(shader), onClick(onClickFunction) {

		quadVAO = VAO;

		//convert the position to pixels
		xPos = xPos * screenWidth / 2;
		yPos = yPos * screenHeight / 2;
		//convert the sides to pixels
		width = width * screenWidth;
		height = height * screenHeight;
		//calculate the bounding box and store in a vec4(LEFT, RIGHT, DOWN, UP)
		boundingBox = glm::vec4(xPos - width/2, xPos + width/2, yPos - height/2, yPos + height/2);
		

		//convert the position to clip space coords
		xPos = xPos * 2 / screenWidth;
		yPos = yPos * 2 / screenHeight;

		//convert the sides to normalized
		width = width / screenWidth;
		height = height / screenHeight;

		model = glm::translate(model, glm::vec3(xPos, yPos, 0.0f));
		model = glm::scale(model, glm::vec3(width, height, 0.0f));

		buttonShader.use();
		buttonShader.setMat4("model", model);
		buttonShader.setVec4("color", color);

		if (Texture){ 
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, Texture);

			buttonShader.setBool("hasTexture", true); 
			buttonShader.setInt("screenTexture", 0);
		}

		else { 
			buttonShader.setBool("hasTexture", false); 
		}
		

		ui_elements.push_back(this);
	}

	void RefreshVars() {

		//convert the position to pixels
		xPos = xPos * screenWidth / 2;
		yPos = yPos * screenHeight / 2;
		//convert the sides to pixels
		width = width * screenWidth;
		height = height * screenHeight;
		//calculate the bounding box and store in a vec4(LEFT, RIGHT, DOWN, UP)
		boundingBox = glm::vec4(xPos - width / 2, xPos + width / 2, yPos - height / 2, yPos + height / 2);
		


		//convert the position to clip space coords
		xPos = xPos * 2 / screenWidth;
		yPos = yPos * 2 / screenHeight;

		//convert the sides to normalized
		width = width / screenWidth;
		height = height / screenHeight;

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(xPos, yPos, 0.0f));
		model = glm::scale(model, glm::vec3(width, height, 0.0f));
		buttonShader.use();
		buttonShader.setMat4("model", model);
	}

	void convertPosition(bool normalize = false) {
		if (normalize) {
			//convert the sides to normalized
			width = width / screenWidth;
			height = height / screenHeight;
		}
		else {
			//convert the sides to pixels
			width = width * screenWidth / 4;
			height = height * screenHeight / 4;
		}
	}

	void SetPosition(float localxPos, float localyPos, bool normalized = true) {
		if (normalized)
		{
			xPos = localxPos;
			yPos = localyPos;
		}
		else 
		{
			xPos = localxPos * 1 / screenWidth;
			yPos = localyPos * 1 / screenHeight;
						
		}
		RefreshVars();

	}

	void SetDimensions(float localWidth, float localHeight, bool normalized = true) {
		if (normalized)
		{
			width = localWidth;
			height = localHeight;
		}
		else
		{
			width = localWidth * 1 / screenWidth;
			height = localHeight * 1 / screenHeight;

		}
		
		RefreshVars();

	}

void SetTexture(string imagePath) {
		
			unsigned int textureID;
			glGenTextures(1, &textureID);
			glBindTexture(GL_TEXTURE_2D, textureID);

			stbi_set_flip_vertically_on_load(false);

			int width, height, nrChannels;
			
				unsigned char* data = stbi_load(imagePath.c_str(), &width, &height, &nrChannels, 0);
				if (data) {
					GLenum format = GL_RGB;

					if (nrChannels == 1) { format = GL_RED; glPixelStorei(GL_UNPACK_ALIGNMENT, 1); }
					else if (nrChannels == 3) { format = GL_RGB; glPixelStorei(GL_UNPACK_ALIGNMENT, 1); }
					else if (nrChannels == 4) { format = GL_RGBA; glPixelStorei(GL_UNPACK_ALIGNMENT, 4); }
					glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
				}
					
				else
				{
					std::cout << "UI button texture failed to load"  << std::endl;
					stbi_image_free(data);
				}
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

			stbi_set_flip_vertically_on_load(true);

			Texture = textureID;
		
	}
	

	void DrawButton() {
		buttonShader.use();

		if (Texture) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, Texture);

			buttonShader.setBool("hasTexture", true);
			buttonShader.setInt("screenTexture", 0);
		}

		else {
			buttonShader.setBool("hasTexture", false);
		}

		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	void CheckForCLick() {
		float localX = clickPosX - screenWidth / 2;
		float localY = screenHeight / 2 - clickPosY;

		if (localX > boundingBox.x && localX < boundingBox.y && localY > boundingBox.z && localY < boundingBox.w && mouseClicked) {
			if (onClick)
			{
				onClick();
			}
			mouseClicked = false;
		}
	}

private:
	unsigned int quadVAO;

	glm::mat4 model = glm::mat4(1.0f);





};