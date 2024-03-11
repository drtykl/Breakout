#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "texture.h"
#include "shader.h"


class Mytest
{
public:
    // Constructor (inits shaders/shapes)
    Mytest(Shader shader);
    // Destructor
    ~Mytest();
    // Renders a defined quad textured with given sprite
    void DrawSprite(Texture2D texture);
private:
    // Render state
    Shader       shader;
    unsigned int quadVAO;
    // Initializes and configures the quad's buffer and vertex attributes
    void initRenderData();
};

