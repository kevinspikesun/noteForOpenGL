// ����GLEW�� ���徲̬����
#define GLEW_STATIC
#include <GLEW/glew.h>
// ����GLFW��
#include <GLFW/glfw3.h>
// ����SOIL��
#include <SOIL/SOIL.h>
// ����GLM��
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

#include <iostream>
#include <sstream>
#include <vector>

// ������ɫ�����ؿ�
#include "shader.h"
// ����������Ƹ�����
#include "camera.h"
// �����������ظ�����
#include "texture.h"
// ��ʱ��������
#include "Timer.h"
// �������������
#include "font.h"

// ���̻ص�����ԭ������
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
// ����ƶ��ص�����ԭ������
void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
// �����ֻص�����ԭ������
void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// �������ƶ�
void do_movement();

// ���������
const int WINDOW_WIDTH = 800, WINDOW_HEIGHT = 600;
// ���������������
GLfloat lastX = WINDOW_WIDTH / 2.0f, lastY = WINDOW_HEIGHT / 2.0f;
bool firstMouseMove = true;
bool keyPressedStatus[1024]; // ���������¼
GLfloat deltaTime = 0.0f; // ��ǰ֡����һ֡��ʱ���
GLfloat lastFrame = 0.0f; // ��һ֡ʱ��
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
glm::vec3 lampPos(0.5f, 0.5f, 0.3f);

GLuint quadVAOId, quadVBOId;
GLuint textVAOId, textVBOId;
GLuint PBOIds[2];
void setupQuadVAO();
void preparePBO();
// ͼƬ����
const int    IMAGE_WIDTH = 1024;
const int    IMAGE_HEIGHT = 1024;
const int DATA_SIZE = IMAGE_WIDTH * IMAGE_HEIGHT * 4;
const int    TEXT_WIDTH = 8;
const int    TEXT_HEIGHT = 30;
const GLenum PIXEL_FORMAT = GL_BGRA;
void updatePixels(GLubyte* dst, int size);
GLubyte* imageData = 0; // ͼƬ����
void initImageData();
void releaseImageData();

int pboMode = 0;
Timer timer, t1, t2;
float copyTime, updateTime;
GLuint textureId;
void initTexture();

void renderInfo(Shader& shader);
void renderScene(Shader& shader);
void renderText(Shader &shader, std::wstring text,
	GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color);
void printTransferRate();
std::map<wchar_t, FontCharacter> unicodeCharacters;

int main(int argc, char** argv)
{

	if (!glfwInit())	// ��ʼ��glfw��
	{
		std::cout << "Error::GLFW could not initialize GLFW!" << std::endl;
		return -1;
	}

	// ����OpenGL 3.3 core profile
	std::cout << "Start OpenGL core profile version 3.3" << std::endl;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// ��������
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
		"Demo of PBO(unpack stream texture)", NULL, NULL);
	if (!window)
	{
		std::cout << "Error::GLFW could not create winddow!" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwSetWindowPos(window, 300, 100);
	// �����Ĵ��ڵ�contextָ��Ϊ��ǰcontext
	glfwMakeContextCurrent(window);

	// ע�ᴰ�ڼ����¼��ص�����
	glfwSetKeyCallback(window, key_callback);
	// ע������¼��ص�����
	glfwSetCursorPosCallback(window, mouse_move_callback);
	// ע���������¼��ص�����
	glfwSetScrollCallback(window, mouse_scroll_callback);
	// ��겶�� ͣ���ڳ�����
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// ��ʼ��GLEW ��ȡOpenGL����
	glewExperimental = GL_TRUE; // ��glew��ȡ������չ����
	GLenum status = glewInit();
	if (status != GLEW_OK)
	{
		std::cout << "Error::GLEW glew version:" << glewGetString(GLEW_VERSION)
			<< " error string:" << glewGetErrorString(status) << std::endl;
		glfwTerminate();
		return -1;
	}

	// �����ӿڲ���
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

	// Section1 ׼����������
	setupQuadVAO();
	preparePBO();
	initImageData();
	initTexture();

	// Section2 ��������
	FontResourceManager::getInstance().loadFont("arial", "../../resources/fonts/arial.ttf");
	FontResourceManager::getInstance().loadASCIIChar("arial", 38, unicodeCharacters);
	// Section3 ׼����ɫ������
	Shader shader("scene.vertex", "scene.frag");
	Shader textShader("text.vertex", "text.frag");

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// ��ʼ��Ϸ��ѭ��
	while (!glfwWindowShouldClose(window))
	{
		GLfloat currentFrame = (GLfloat)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		glfwPollEvents(); // ����������� ���̵��¼�
		do_movement(); // �����û�������� �����������

		// �����ɫ������ ����Ϊָ����ɫ
		glClearColor(0.18f, 0.04f, 0.14f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 model;
		// ������д�������ƴ���
		// �Ȼ�������ͼƬ
		shader.use();
		glUniformMatrix4fv(glGetUniformLocation(shader.programId, "projection"),
			1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(shader.programId, "view"),
			1, GL_FALSE, glm::value_ptr(view));
		model = glm::mat4();
		glUniformMatrix4fv(glGetUniformLocation(shader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		renderScene(shader);
		// �ڻ�����Ϣ����
		
		textShader.use();
		projection = glm::ortho(0.0f, (GLfloat)(WINDOW_WIDTH),
		0.0f, (GLfloat)WINDOW_HEIGHT);
		view = glm::mat4();
		glUniformMatrix4fv(glGetUniformLocation(textShader.programId, "projection"),
			1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(textShader.programId, "view"),
			1, GL_FALSE, glm::value_ptr(view));
		model = glm::mat4();
		glUniformMatrix4fv(glGetUniformLocation(textShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		renderInfo(textShader);

		printTransferRate();

		glBindVertexArray(0);
		glUseProgram(0);
		glfwSwapBuffers(window); // ��������
	}
	// �ͷ���Դ
	glDeleteVertexArrays(1, &quadVAOId);
	glDeleteBuffers(1, &quadVBOId);
	glDeleteVertexArrays(1, &textVAOId);
	glDeleteBuffers(1, &textVBOId);
	glDeleteBuffers(2, PBOIds);  // ע���ͷ�PBO
	releaseImageData();

	glfwTerminate();
	return 0;
}
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			keyPressedStatus[key] = true;
		else if (action == GLFW_RELEASE)
			keyPressedStatus[key] = false;
	}
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE); // �رմ���
	}
	else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
	{
		pboMode = (pboMode + 1) % 3;
	}
}
void mouse_move_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouseMove) // �״�����ƶ�
	{
		lastX = xpos;
		lastY = ypos;
		firstMouseMove = false;
	}

	GLfloat xoffset = xpos - lastX;
	GLfloat yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	camera.handleMouseMove(xoffset, yoffset);
}
// ����������ദ�������ֿ���
void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.handleMouseScroll(yoffset);
}
// ����������ദ�����̿���
void do_movement()
{

	if (keyPressedStatus[GLFW_KEY_W])
		camera.handleKeyPress(FORWARD, deltaTime);
	if (keyPressedStatus[GLFW_KEY_S])
		camera.handleKeyPress(BACKWARD, deltaTime);
	if (keyPressedStatus[GLFW_KEY_A])
		camera.handleKeyPress(LEFT, deltaTime);
	if (keyPressedStatus[GLFW_KEY_D])
		camera.handleKeyPress(RIGHT, deltaTime);
}

// ׼��PBO
void preparePBO()
{
	glGenBuffers(2, PBOIds);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBOIds[0]);
	//  GL_STREAM_DRAW���ڴ������ݵ�texture object GL_STREAM_READ ���ڶ�ȡFBO������
	glBufferData(GL_PIXEL_UNPACK_BUFFER, DATA_SIZE, NULL, GL_STREAM_DRAW);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBOIds[1]);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, DATA_SIZE, NULL, GL_STREAM_DRAW);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

// ���д����������ɫ
void updatePixels(GLubyte* dst, int size)
{
	static int color = 0;

	if (!dst)
		return;

	int* ptr = (int*)dst;  // ÿ�δ���4���ֽ�

	for (int i = 0; i < IMAGE_WIDTH; ++i)
	{
		for (int j = 0; j < IMAGE_HEIGHT; ++j)
		{
			*ptr = color;
			++ptr;
		}
		color += 257;   // ��������� ���ֵ������
	}
	++color;
}

// �������ε�VAO����
void setupQuadVAO()
{
	// ������������ λ�� ����
	GLfloat quadVertices[] = {
		-1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	};
	glGenVertexArrays(1, &quadVAOId);
	glGenBuffers(1, &quadVBOId);
	glBindVertexArray(quadVAOId);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBOId);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
		5 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
		5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));

	glGenVertexArrays(1, &textVAOId);
	glGenBuffers(1, &textVBOId);
	glBindVertexArray(textVAOId);
	glBindBuffer(GL_ARRAY_BUFFER, textVBOId);
	// �������ֵľ��ζ����������� λ�� ���� �Ƕ�̬���������
	// ����Ԥ����ռ�
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)* 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	// xy ��ʾλ�� zw��ʾ��������
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
	glBindVertexArray(0);
}

void renderScene(Shader& shader)
{
	
	static int index = 0;				// ���ڴ�PBO�������ص���������
	int nextIndex = 0;                  // ָ����һ��PBO ���ڸ���PBO������
	glActiveTexture(GL_TEXTURE0);
	if (pboMode > 0)
	{
		if (pboMode == 1)
		{
			// ֻ��һ��ʱ ʹ��0��PBO
			index = nextIndex = 0;
		}
		else if (pboMode == 2)
		{
			index = (index + 1) % 2;
			nextIndex = (index + 1) % 2;
		}

		// ��ʼPBO��texture object�����ݸ��� unpack����
		t1.start();

		// ������ ��PBO
		glBindTexture(GL_TEXTURE_2D, textureId);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBOIds[index]);

		// ��PBO���Ƶ�texture object ʹ��ƫ���� ������ָ��
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, IMAGE_WIDTH,
			IMAGE_HEIGHT, PIXEL_FORMAT, GL_UNSIGNED_BYTE, 0);

		// ���㸴����������ʱ��
		t1.stop();
		copyTime = t1.getElapsedTimeInMilliSec();


		// ��ʼ�޸�nextIndexָ���PBO������
		t1.start();

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBOIds[nextIndex]);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, DATA_SIZE, 0, GL_STREAM_DRAW);
		// ��PBOӳ�䵽�û��ڴ�ռ� Ȼ���޸�PBO������
		GLubyte* ptr = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
		if (ptr)
		{
			// ����ӳ�����ڴ�����
			updatePixels(ptr, DATA_SIZE);
			glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // �ͷ�ӳ����û��ڴ�ռ�
		}

		// �����޸�PBO��������ʱ��
		t1.stop();
		updateTime = t1.getElapsedTimeInMilliSec();

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	}
	else
	{
		// ��ʹ��PBO�ķ�ʽ ���û��ڴ渴�Ƶ�texture object
		t1.start();

		glBindTexture(GL_TEXTURE_2D, textureId);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 
			IMAGE_WIDTH, IMAGE_HEIGHT, PIXEL_FORMAT, GL_UNSIGNED_BYTE, (GLvoid*)imageData);

		t1.stop();
		copyTime = t1.getElapsedTimeInMilliSec();

		// �޸��ڴ�����
		t1.start();
		updatePixels(imageData, DATA_SIZE);
		t1.stop();
		updateTime = t1.getElapsedTimeInMilliSec();
	}
	glUniform1i(glGetUniformLocation(shader.programId, "randomText"), 0);
	
	glBindVertexArray(quadVAOId);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void renderInfo(Shader& shader)
{
	float color[4] = { 1, 1, 1, 1 };

	std::wstringstream ss;
	ss << "PBO: ";
	if (pboMode == 0)
		ss << "off" << std::ends;
	else if (pboMode == 1)
		ss << "1 PBO" << std::ends;
	else if (pboMode == 2)
		ss << "2 PBOs" << std::ends;

	renderText(shader, ss.str(), 1, WINDOW_HEIGHT - TEXT_HEIGHT, 0.4f, glm::vec3(0.0f, 0.0f, 1.0f));
	ss.str(L"");

	ss << std::fixed << std::setprecision(3);
	ss << "Updating Time: " << updateTime << " ms" << std::ends;
	renderText(shader, ss.str(), 1, WINDOW_HEIGHT - (2 * TEXT_HEIGHT), 0.4f, glm::vec3(0.0f, 0.0f, 1.0f));
	ss.str(L"");

	ss << "Copying Time: " << copyTime << " ms" << std::ends;
	renderText(shader, ss.str(), 1, WINDOW_HEIGHT - (3 * TEXT_HEIGHT), 0.4f, glm::vec3(0.0f, 0.0f, 1.0f));
	ss.str(L"");

	ss << "Press SPACE key to toggle PBO on/off." << std::ends;
	renderText(shader, ss.str(), 1, 1, 0.8f, glm::vec3(0.0f, 0.0f, 1.0f));

	ss << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield);
}
void printTransferRate()
{
	const double INV_MEGA = 1.0 / (1024 * 1024);
	static Timer timer;
	static int count = 0;
	static std::stringstream ss;
	double elapsedTime;

	// ѭ��ֱ��1sʱ�䵽
	elapsedTime = timer.getElapsedTime();
	if (elapsedTime < 1.0)
	{
		++count;
	}
	else
	{
		std::cout << std::fixed << std::setprecision(1);
		std::cout << "Transfer Rate: " << (count / elapsedTime) * DATA_SIZE * INV_MEGA << " MB/s. (" << count / elapsedTime << " FPS)\n";
		std::cout << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield);
		count = 0;                      // ���ü�����
		timer.start();                  // ���¿�ʼ��ʱ��
	}
}
void initImageData()
{
	imageData = new GLubyte[DATA_SIZE];
	memset(imageData, 0, DATA_SIZE);
}
void initTexture()
{
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, IMAGE_WIDTH, IMAGE_HEIGHT, 0, PIXEL_FORMAT, GL_UNSIGNED_BYTE, (GLvoid*)imageData);
	glBindTexture(GL_TEXTURE_2D, 0);
}
void releaseImageData()
{
	delete[] imageData;
}
void renderText(Shader &shader, std::wstring text,
	GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color)
{
	shader.use();
	glUniform3f(glGetUniformLocation(shader.programId, "textColor"), color.x, color.y, color.z);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(textVAOId);

	// ���������ַ������ַ�
	std::wstring::const_iterator c;
	for (c = text.begin(); c != text.end(); c++)
	{
		if (unicodeCharacters.count(*c) <= 0)
		{
			continue;
		}
		FontCharacter ch = unicodeCharacters[*c];

		GLfloat xpos = x + ch.bearing.x * scale;
		GLfloat ypos = y - (ch.size.y - ch.bearing.y) * scale;

		GLfloat w = ch.size.x * scale;
		GLfloat h = ch.size.y * scale;
		// ����������ζ�Ӧ�ľ���λ������
		GLfloat vertices[6][4] = {
			{ xpos, ypos + h, 0.0, 0.0 },
			{ xpos, ypos, 0.0, 1.0 },
			{ xpos + w, ypos, 1.0, 1.0 },

			{ xpos, ypos + h, 0.0, 0.0 },
			{ xpos + w, ypos, 1.0, 1.0 },
			{ xpos + w, ypos + h, 1.0, 0.0 }
		};
		// ������ַ���Ӧ������
		glBindTexture(GL_TEXTURE_2D, ch.textureId);
		// ��̬����VBO������ ��Ϊ�����ʾ���ֵľ���λ�������б䶯 ��Ҫ��̬����
		glBindBuffer(GL_ARRAY_BUFFER, textVBOId);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		// ��Ϊadvance ��1/64���ر�ʾ���뵥λ ����������λ���ʾһ�����ؾ���
		x += (ch.advance >> 6) * scale;
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}