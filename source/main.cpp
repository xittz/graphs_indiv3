#include "platform.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <list>
#include <sstream>

#include "helpers/Program.h"
#include "helpers/Texture.h"
#include "helpers/Camera.h"

using namespace helpers;

// содержит шейдер, текстуру, VBO, VAO и параметры для glDrawArrays
struct ModelAsset {
    Program* shaders;
    Texture* texture;
    GLuint vbo;
    GLuint vao;
    GLenum drawType;
    GLint drawStart;
    GLint drawCount;
    GLfloat shininess;
    glm::vec3 specularColor;

    ModelAsset() :
        shaders(NULL),
        texture(NULL),
        vbo(0),
        vao(0),
        drawType(GL_TRIANGLES),
        drawStart(0),
        drawCount(0),
        shininess(0.0f),
        specularColor(1.0f, 1.0f, 1.0f)
    {}
};

// конкретный примитив - ссылка на модель и текущая матрица преобразований
struct ModelInstance {
    ModelAsset* asset;
    glm::mat4 transform;

    ModelInstance() :
        asset(NULL),
        transform()
    {}
};

// свет
struct Light {
    glm::vec4 position;
    glm::vec3 intensities; // интенсивности цветов
    float attenuation;
    float ambientCoefficient;
    float coneAngle;
    glm::vec3 coneDirection;
};

const glm::vec2 SCREEN_SIZE(1280, 720);

GLFWwindow* gWindow = NULL;
Camera gCamera;
ModelAsset gWoodenCube;
ModelAsset gGrassFloor;
ModelAsset gBrickWall;
std::list<ModelInstance> gInstances;
GLfloat gDegreesRotated = 0.0f;
std::vector<Light> gLights;


// считывает шейдеры и инициализирует их в программе
static Program* LoadShaders(const char* vertFilename, const char* fragFilename) {
    std::vector<Shader> shaders;
    shaders.push_back(Shader::shaderFromFile(ResourcePath(vertFilename), GL_VERTEX_SHADER));
    shaders.push_back(Shader::shaderFromFile(ResourcePath(fragFilename), GL_FRAGMENT_SHADER));
    return new Program(shaders);
}


// создает текстуру из файла (bitmap)
static Texture* LoadTexture(const char* filename) {
    Bitmap bmp = Bitmap::bitmapFromFile(ResourcePath(filename));
    bmp.flipVertically();
    return new Texture(bmp);
}


static void LoadWoodenCubeAsset() {
    gWoodenCube.shaders = LoadShaders("vertex-shader.txt", "fragment-shader.txt");
    gWoodenCube.drawType = GL_TRIANGLES;
    gWoodenCube.drawStart = 0;
    gWoodenCube.drawCount = 6*2*3;
    gWoodenCube.texture = LoadTexture("wooden-crate.jpg");
    gWoodenCube.shininess = 80.0;
    gWoodenCube.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    
	glGenBuffers(1, &gWoodenCube.vbo);
    glGenVertexArrays(1, &gWoodenCube.vao);
    glBindVertexArray(gWoodenCube.vao);
    glBindBuffer(GL_ARRAY_BUFFER, gWoodenCube.vbo);
	
	GLfloat vertexData[] = {
        //  X     Y     Z       U     V          Normal
        // нижняя грань
        -1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   0.0f, -1.0f, 0.0f,
         1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, -1.0f, 0.0f,
        -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   0.0f, -1.0f, 0.0f,
         1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, -1.0f, 0.0f,
         1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   0.0f, -1.0f, 0.0f,
        -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   0.0f, -1.0f, 0.0f,

        // вехрняя грань
        -1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   0.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 1.0f, 0.0f,
         1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
         1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 1.0f, 0.0f,
         1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 1.0f, 0.0f,

        // передняя грань
        -1.0f,-1.0f, 1.0f,   1.0f, 0.0f,   0.0f, 0.0f, 1.0f,
         1.0f,-1.0f, 1.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
         1.0f,-1.0f, 1.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
         1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,

        // задняя грань
        -1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   0.0f, 0.0f, -1.0f,
        -1.0f, 1.0f,-1.0f,   0.0f, 1.0f,   0.0f, 0.0f, -1.0f,
         1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 0.0f, -1.0f,
         1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 0.0f, -1.0f,
        -1.0f, 1.0f,-1.0f,   0.0f, 1.0f,   0.0f, 0.0f, -1.0f,
         1.0f, 1.0f,-1.0f,   1.0f, 1.0f,   0.0f, 0.0f, -1.0f,

        // левая грань
        -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
        -1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
        -1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
        -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
        -1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
        -1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   -1.0f, 0.0f, 0.0f,

        // правая грань
         1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
         1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   1.0f, 0.0f, 0.0f,
         1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
         1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
         1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
         1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   1.0f, 0.0f, 0.0f
    };
    
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

    glEnableVertexAttribArray(gWoodenCube.shaders->attrib("vert"));
    glVertexAttribPointer(gWoodenCube.shaders->attrib("vert"), 3, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), NULL);

    glEnableVertexAttribArray(gWoodenCube.shaders->attrib("vertTexCoord"));
    glVertexAttribPointer(gWoodenCube.shaders->attrib("vertTexCoord"), 2, GL_FLOAT, GL_TRUE,  8*sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));

    glEnableVertexAttribArray(gWoodenCube.shaders->attrib("vertNormal"));
    glVertexAttribPointer(gWoodenCube.shaders->attrib("vertNormal"), 3, GL_FLOAT, GL_TRUE,  8*sizeof(GLfloat), (const GLvoid*)(5 * sizeof(GLfloat)));

    glBindVertexArray(0);
}

static void LoadBrickWallAsset() {
	gBrickWall.shaders = LoadShaders("vertex-shader.txt", "fragment-shader.txt");
	gBrickWall.drawType = GL_TRIANGLES;
	gBrickWall.drawStart = 0;
	gBrickWall.drawCount = 6 * 2 * 3;
	gBrickWall.texture = LoadTexture("bricks.jpg");
	gBrickWall.shininess = 1000.0;
	gBrickWall.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	
	glGenBuffers(1, &gBrickWall.vbo);
	glGenVertexArrays(1, &gBrickWall.vao);
	glBindVertexArray(gBrickWall.vao);
	glBindBuffer(GL_ARRAY_BUFFER, gBrickWall.vbo);

	GLfloat vertexData[] = {
		-1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   0.0f, -1.0f, 0.0f,
		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, -1.0f, 0.0f,
		-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   0.0f, -1.0f, 0.0f,
		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, -1.0f, 0.0f,
		1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   0.0f, -1.0f, 0.0f,
		-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   0.0f, -1.0f, 0.0f,
		-1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   0.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 1.0f, 0.0f,
		1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
		1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 1.0f, 0.0f,
		-1.0f,-1.0f, 1.0f,   1.0f, 0.0f,   0.0f, 0.0f, 1.0f,
		1.0f,-1.0f, 1.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
		1.0f,-1.0f, 1.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   0.0f, 0.0f, -1.0f,
		-1.0f, 1.0f,-1.0f,   0.0f, 1.0f,   0.0f, 0.0f, -1.0f,
		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 0.0f, -1.0f,
		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 0.0f, -1.0f,
		-1.0f, 1.0f,-1.0f,   0.0f, 1.0f,   0.0f, 0.0f, -1.0f,
		1.0f, 1.0f,-1.0f,   1.0f, 1.0f,   0.0f, 0.0f, -1.0f,	
		-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
		-1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
		-1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
		-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
		-1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
		-1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
		1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   1.0f, 0.0f, 0.0f,
		1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
		1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
		1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   1.0f, 0.0f, 0.0f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(gBrickWall.shaders->attrib("vert"));
	glVertexAttribPointer(gBrickWall.shaders->attrib("vert"), 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), NULL);

	glEnableVertexAttribArray(gBrickWall.shaders->attrib("vertTexCoord"));
	glVertexAttribPointer(gBrickWall.shaders->attrib("vertTexCoord"), 2, GL_FLOAT, GL_TRUE, 8 * sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));

	glEnableVertexAttribArray(gBrickWall.shaders->attrib("vertNormal"));
	glVertexAttribPointer(gBrickWall.shaders->attrib("vertNormal"), 3, GL_FLOAT, GL_TRUE, 8 * sizeof(GLfloat), (const GLvoid*)(5 * sizeof(GLfloat)));

	glBindVertexArray(0);
}

static void LoadGrassFloorAsset() {
	gGrassFloor.shaders = LoadShaders("vertex-shader.txt", "fragment-shader.txt");
	gGrassFloor.drawType = GL_TRIANGLES;
	gGrassFloor.drawStart = 0;
	gGrassFloor.drawCount = 6 * 2 * 3;
	gGrassFloor.texture = LoadTexture("grass4k.jpg");
	gGrassFloor.shininess = 2000.0;
	gGrassFloor.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	
	glGenBuffers(1, &gGrassFloor.vbo);
	glGenVertexArrays(1, &gGrassFloor.vao);
	glBindVertexArray(gGrassFloor.vao);
	glBindBuffer(GL_ARRAY_BUFFER, gGrassFloor.vbo);

	GLfloat vertexData[] = {
		-1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   0.0f, -1.0f, 0.0f,
		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, -1.0f, 0.0f,
		-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   0.0f, -1.0f, 0.0f,
		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, -1.0f, 0.0f,
		1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   0.0f, -1.0f, 0.0f,
		-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   0.0f, -1.0f, 0.0f,
		-1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   0.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 1.0f, 0.0f,
		1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
		1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 1.0f, 0.0f,
		-1.0f,-1.0f, 1.0f,   1.0f, 0.0f,   0.0f, 0.0f, 1.0f,
		1.0f,-1.0f, 1.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
		1.0f,-1.0f, 1.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   0.0f, 0.0f, -1.0f,
		-1.0f, 1.0f,-1.0f,   0.0f, 1.0f,   0.0f, 0.0f, -1.0f,
		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 0.0f, -1.0f,
		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 0.0f, -1.0f,
		-1.0f, 1.0f,-1.0f,   0.0f, 1.0f,   0.0f, 0.0f, -1.0f,
		1.0f, 1.0f,-1.0f,   1.0f, 1.0f,   0.0f, 0.0f, -1.0f,
		-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
		-1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
		-1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
		-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
		-1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
		-1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
		1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   1.0f, 0.0f, 0.0f,
		1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
		1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
		1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   1.0f, 0.0f, 0.0f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(gGrassFloor.shaders->attrib("vert"));
	glVertexAttribPointer(gGrassFloor.shaders->attrib("vert"), 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), NULL);

	glEnableVertexAttribArray(gGrassFloor.shaders->attrib("vertTexCoord"));
	glVertexAttribPointer(gGrassFloor.shaders->attrib("vertTexCoord"), 2, GL_FLOAT, GL_TRUE, 8 * sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));

	glEnableVertexAttribArray(gGrassFloor.shaders->attrib("vertNormal"));
	glVertexAttribPointer(gGrassFloor.shaders->attrib("vertNormal"), 3, GL_FLOAT, GL_TRUE, 8 * sizeof(GLfloat), (const GLvoid*)(5 * sizeof(GLfloat)));

	glBindVertexArray(0);
}

glm::mat4 translate(GLfloat x, GLfloat y, GLfloat z) {
    return glm::translate(glm::mat4(), glm::vec3(x,y,z));
}

glm::mat4 scale(GLfloat x, GLfloat y, GLfloat z) {
    return glm::scale(glm::mat4(), glm::vec3(x,y,z));
}

static void CreateScene() {
    ModelInstance p1Vert;
    p1Vert.asset = &gWoodenCube;
    p1Vert.transform = translate(0,0,0) * scale(0.8f, 2.5,1);
    gInstances.push_back(p1Vert);

	ModelInstance p2Vert;
	p2Vert.asset = &gWoodenCube;
	p2Vert.transform = translate(6, 0, 0) * scale(0.8f, 2.5, 1);
	gInstances.push_back(p2Vert);

	ModelInstance p1Hor;
	p1Hor.asset = &gWoodenCube;
	p1Hor.transform = translate(0, 0, 0) * scale(2.5, 0.8f, 0.8f);
	gInstances.push_back(p1Hor);

	ModelInstance p2Hor;
	p2Hor.asset = &gWoodenCube;
	p2Hor.transform = translate(6, 0, 0) * scale(2.5, 0.8f, 0.8f);
	gInstances.push_back(p2Hor);

    ModelInstance cLeft;
    cLeft.asset = &gWoodenCube;
    cLeft.transform = translate(-8,0,0) * scale(1,6, 0.8f);
    gInstances.push_back(cLeft);

    ModelInstance cTop;
    cTop.asset = &gWoodenCube;
    cTop.transform = translate(-6,5,0) * scale(3,1,1);
    gInstances.push_back(cTop);

	ModelInstance cBot;
	cTop.asset = &gWoodenCube;
	cTop.transform = translate(-6, -5, 0) * scale(3, 1, 1);
	gInstances.push_back(cTop);

	ModelInstance wall;
	wall.asset = &gBrickWall;
	wall.transform = translate(-3, 1, -10) * scale(20, 10, 1);
	gInstances.push_back(wall);

	ModelInstance floor;
	floor.asset = &gGrassFloor;
	floor.transform = translate(-3, -7, 10) * scale(20, 1, 25);
	gInstances.push_back(floor);
}

template <typename T>
void SetLightUniform(Program* shaders, const char* propertyName, size_t lightIndex, const T& value) {
    std::ostringstream ss;
    ss << "allLights[" << lightIndex << "]." << propertyName;
    std::string uniformName = ss.str();

    shaders->setUniform(uniformName.c_str(), value);
}

static void RenderInstance(const ModelInstance& inst) {
    ModelAsset* asset = inst.asset;
    Program* shaders = asset->shaders;

    shaders->use();
    shaders->setUniform("camera", gCamera.matrix());
    shaders->setUniform("model", inst.transform);
    shaders->setUniform("materialTex", 0);
    shaders->setUniform("materialShininess", asset->shininess);
    shaders->setUniform("materialSpecularColor", asset->specularColor);
    shaders->setUniform("cameraPosition", gCamera.position());
    shaders->setUniform("numLights", (int)gLights.size());

    for(size_t i = 0; i < gLights.size(); ++i){
        SetLightUniform(shaders, "position", i, gLights[i].position);
        SetLightUniform(shaders, "intensities", i, gLights[i].intensities);
        SetLightUniform(shaders, "attenuation", i, gLights[i].attenuation);
        SetLightUniform(shaders, "ambientCoefficient", i, gLights[i].ambientCoefficient);
        SetLightUniform(shaders, "coneAngle", i, gLights[i].coneAngle);
        SetLightUniform(shaders, "coneDirection", i, gLights[i].coneDirection);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, asset->texture->object());
	glBindVertexArray(asset->vao);
    glDrawArrays(asset->drawType, asset->drawStart, asset->drawCount);
	glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    shaders->stopUsing();
}

static void Render() {
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	std::list<ModelInstance>::const_iterator it;
    for(it = gInstances.begin(); it != gInstances.end(); ++it)
        RenderInstance(*it);

	// отображает изменения
    glfwSwapBuffers(gWindow);
}


// обновление сцены в зависимости от времени прошедшего с момента последнего апдейта
static void Update(float secondsElapsed) {
    const float moveSpeed = 4.0;
    if(glfwGetKey(gWindow, 'S')){
        gCamera.offsetPosition(secondsElapsed * moveSpeed * -gCamera.forward());
    } else if(glfwGetKey(gWindow, 'W')){
        gCamera.offsetPosition(secondsElapsed * moveSpeed * gCamera.forward());
    }
    if(glfwGetKey(gWindow, 'A')){
        gCamera.offsetPosition(secondsElapsed * moveSpeed * -gCamera.right());
    } else if(glfwGetKey(gWindow, 'D')){
        gCamera.offsetPosition(secondsElapsed * moveSpeed * gCamera.right());
    }
    if(glfwGetKey(gWindow, 'Z')){
        gCamera.offsetPosition(secondsElapsed * moveSpeed * -glm::vec3(0,1,0));
    } else if(glfwGetKey(gWindow, 'X')){
        gCamera.offsetPosition(secondsElapsed * moveSpeed * glm::vec3(0,1,0));
    }

    if(glfwGetKey(gWindow, '1')){
        gLights[0].position = glm::vec4(gCamera.position(), 1.0);
        gLights[0].coneDirection = gCamera.forward();
    }

	if (glfwGetKey(gWindow, '2'))
		gLights[0].intensities = glm::vec3(5, 0, 0); //red
	else if (glfwGetKey(gWindow, '3'))
		gLights[0].intensities = glm::vec3(0, 5, 0); //green
	else if (glfwGetKey(gWindow, '4'))
		gLights[0].intensities = glm::vec3(0, 0, 5); // blue
    else if(glfwGetKey(gWindow, '5'))
        gLights[0].intensities = glm::vec3(2, 2, 2); //white


    const float mouseSensitivity = 0.1f;
    double mouseX, mouseY;
    glfwGetCursorPos(gWindow, &mouseX, &mouseY);
    gCamera.offsetOrientation(mouseSensitivity * (float)mouseY, mouseSensitivity * (float)mouseX);
	glfwSetCursorPos(gWindow, 0, 0);
}

void OnError(int errorCode, const char* msg) {
	throw std::runtime_error(msg);
}

int InitGlfw() {
	glfwSetErrorCallback(OnError);
	if (!glfwInit()) {
		std::cout << "glfwInit failed" << std::endl;
		return 1;
	}

	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	gWindow = glfwCreateWindow((int)SCREEN_SIZE.x, (int)SCREEN_SIZE.y, "OpenGL Tutorial", NULL, NULL);
	if (!gWindow) {
		std::cout << "glfwCreateWindow failed" << std::endl;
		return 1;
	}

	glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPos(gWindow, 0, 0);
	glfwMakeContextCurrent(gWindow);
}

void InitGlew() {
	glewExperimental = GL_TRUE;
	glewInit();
	while (glGetError() != GL_NO_ERROR) {}

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void InitCamera() {
	gCamera.setPosition(glm::vec3(-4, 0, 17));
	gCamera.setViewportAspectRatio(SCREEN_SIZE.x / SCREEN_SIZE.y);
	gCamera.setNearAndFarPlanes(0.5f, 100.0f);
}

void InitLights() {
	Light spotlight;
	spotlight.position = glm::vec4(2, 0, 10, 1);
	spotlight.intensities = glm::vec3(2, 2, 2); //strong white light
	spotlight.attenuation = 0.1f;
	spotlight.ambientCoefficient = 0.0f; //no ambient light
	spotlight.coneAngle = 20.0f;
	spotlight.coneDirection = glm::vec3(0, 0, -1);

	Light directionalLight;
	directionalLight.position = glm::vec4(1, 0.8, 0.6, 0); //w == 0 indications a directional light
	directionalLight.intensities = glm::vec3(0.4, 0.3, 0.1); //weak yellowish light
	directionalLight.ambientCoefficient = 0.06f;

	gLights.push_back(spotlight);
	gLights.push_back(directionalLight);
}

int ProgramCycle() {
	double lastTime = glfwGetTime();
	while (!glfwWindowShouldClose(gWindow)) {
		glfwPollEvents();

		double thisTime = glfwGetTime();
		Update((float)(thisTime - lastTime));
		lastTime = thisTime;

		Render();

		GLenum error = glGetError();
		if (error != GL_NO_ERROR) {
			std::cerr << "OpenGL Error " << error << std::endl;
			return 1;
		}

		if (glfwGetKey(gWindow, GLFW_KEY_ESCAPE))
			glfwSetWindowShouldClose(gWindow, GL_TRUE);
	}
	
	return 0;
}

int main(int argc, char *argv[]) {
	InitGlfw();
	InitGlew();

	// инициализация моделей
	LoadWoodenCubeAsset();
	LoadBrickWallAsset();
	LoadGrassFloorAsset();

	// создание сцены на основе моделей
	CreateScene();

	InitCamera();
	InitLights();

	auto status = ProgramCycle();

	glfwTerminate();

	return status;
}
