#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <math.h>

#include "utils.h"
#include <string>

std::string* string_compute = readFile("shaders/compute.glsl");
const char *str_computeShader = string_compute->c_str();

std::string* string_vertex = readFile("shaders/vertex.glsl");
const char *str_vertexShader = string_vertex->c_str();

std::string* string_fragment = readFile("shaders/fragment.glsl");
const char *str_fragmentShader = string_fragment->c_str();

GLfloat vertices[] = {
    -1.f,  1.f,  0.f,  0.f,
     1.f,  1.f,  1.f,  0.f,
     1.f, -1.f,  1.f,  1.f,

    -1.f,  1.f,  0.f,  0.f,
    -1.f, -1.f,  0.f,  1.f,
     1.f, -1.f,  1.f,  1.f,
};

int map[10][10] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 2, 2, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 2, 2, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 3, 3, 3, 0, 0, 1},
    {1, 0, 0, 3, 3, 0, 3, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
};

#define SHEESH_ILERI 0
#define SHEESH_GERI 1
#define SHEESH_SAG 2
#define SHEESH_SOL 3

class Sheesh
{
private:
    GLuint wall_output;

    GLuint tex_output;
    GLuint ray_program, quad_program;
    GLuint quad_vao;

    GLuint map_ssbo;
    GLuint posdirplane_ssbo;

    float *datas;
    int tex_w, tex_h;

public:
    float posX = 3, posY = 3;      // x and y start position
    float dirX = -1, dirY = 0;       // initial direction vector
    float planeX = 0, planeY = 0.44; // the 2d raycaster version of camera plane

    void init();
    void debugWorksizes();
    void initRayProgram();
    void loop();
    void move(int dir, double frameTime);
};

void Sheesh::init()
{
    datas = (float*)calloc(6, sizeof(float));
    datas[0] = posX;
    datas[1] = posY;
    datas[2] = dirX;
    datas[3] = dirY;
    datas[4] = planeX;
    datas[5] = planeY;

    quad_program = glCreateProgram();
    GLuint quad_vertex = glCreateShader(GL_VERTEX_SHADER);
    GLuint quad_fragment = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(quad_vertex, 1, &str_vertexShader, NULL);
    glCompileShader(quad_vertex);

    GLint compileStatus;
    glGetShaderiv(quad_vertex, GL_COMPILE_STATUS, &compileStatus);

    if (compileStatus != GL_TRUE)
    {
        GLchar infoLog[512];
        glGetShaderInfoLog(quad_vertex, sizeof(infoLog), NULL, infoLog);
        std::cerr << "VShader derleme hatası: " << infoLog << std::endl;
    }

    glShaderSource(quad_fragment, 1, &str_fragmentShader, NULL);
    glCompileShader(quad_fragment);

    glGetShaderiv(quad_fragment, GL_COMPILE_STATUS, &compileStatus);

    if (compileStatus != GL_TRUE)
    {
        GLchar infoLog[512];
        glGetShaderInfoLog(quad_fragment, sizeof(infoLog), NULL, infoLog);
        std::cerr << "FShader derleme hatası: " << infoLog << std::endl;
    }

    glAttachShader(quad_program, quad_vertex);
    glAttachShader(quad_program, quad_fragment);
    glLinkProgram(quad_program);

    GLuint quad_vbo;
    glGenVertexArrays(1, &quad_vao);
    glGenBuffers(1, &quad_vbo);

    glBindVertexArray(quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &map_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, map_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(map), map, GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    
    glGenBuffers(1, &posdirplane_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, posdirplane_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(double) * 6, datas, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    tex_w = 640;
    tex_h = 480;
    glGenTextures(1, &tex_output);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_output);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tex_w, tex_h, 0, GL_RGBA, GL_FLOAT,
                 NULL);
    glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    unsigned char* wallData;
    int w, h, numC;
    stbi_set_flip_vertically_on_load(true);
    wallData = stbi_load("wall.png", &w, &h, &numC, 0);

    if (!wallData)
        std::cout << "walldata failed" << std::endl;
    else
        std::cout << "w: " << w << " h: " << h << " numC: " << numC << std::endl;

    glGenTextures(1, &wall_output);
    glBindTexture(GL_TEXTURE_2D, wall_output);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, wallData);
    stbi_image_free(wallData);

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "OpenGL Hata Kodu: " << error << std::endl;
    }

    glBindImageTexture(3, wall_output, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

    debugWorksizes();
    initRayProgram();
}

void Sheesh::debugWorksizes()
{
    int work_grp_cnt[3];

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);

    std::cout << "max global (total) work group counts x:" << work_grp_cnt[0] << " y:" << work_grp_cnt[1] << " z:" << work_grp_cnt[2] << std::endl;

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_cnt[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_cnt[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_cnt[2]);

    std::cout << "max local (in one shader) work group sizes x:" << work_grp_cnt[0] << " y:" << work_grp_cnt[1] << " z:" << work_grp_cnt[2] << std::endl;

    int buffer;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &buffer);
    std::cout << "max local work group invocations " << buffer << std::endl;
}

void Sheesh::initRayProgram()
{
    GLuint ray_shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(ray_shader, 1, &str_computeShader, NULL);
    glCompileShader(ray_shader);

    GLint compileStatus;
    glGetShaderiv(ray_shader, GL_COMPILE_STATUS, &compileStatus);

    if (compileStatus != GL_TRUE)
    {
        GLchar infoLog[512];
        glGetShaderInfoLog(ray_shader, sizeof(infoLog), NULL, infoLog);
        std::cerr << "CShader derleme hatası: " << infoLog << std::endl;
    }

    ray_program = glCreateProgram();
    glAttachShader(ray_program, ray_shader);
    glLinkProgram(ray_program);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, (GLuint)1, map_ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, (GLuint)2, posdirplane_ssbo);

    glDeleteShader(ray_shader);
}

void Sheesh::loop()
{
    glClearTexImage(tex_output, 0, GL_RGBA, GL_FLOAT, NULL);

    glUseProgram(ray_program);
    glDispatchCompute((GLuint)tex_w, 1, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    {
        glUseProgram(quad_program);
        glBindVertexArray(quad_vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_output);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
}

void Sheesh::move(int dir, double frameTime) {
    double moveSpeed = 1.2f * frameTime;
    double rotSpeed = 1.4f * frameTime;
    switch (dir) {
        case SHEESH_ILERI:
            if(map[int(posX + dirX * moveSpeed)][int(posY)] == false)
                posX += dirX * moveSpeed;
            if(map[int(posX)][int(posY + dirY * moveSpeed)] == false)
                posY += dirY * moveSpeed;
            break;
        case SHEESH_GERI:
            if(map[int(posX - dirX * moveSpeed)][int(posY)] == false)
                posX -= dirX * moveSpeed;
            if(map[int(posX)][int(posY - dirY * moveSpeed)] == false) 
                posY -= dirY * moveSpeed;
            break;
        case SHEESH_SOL:
        {
            double oldDirX = dirX;
            dirX = dirX * cos(-rotSpeed) - dirY * sin(-rotSpeed);
            dirY = oldDirX * sin(-rotSpeed) + dirY * cos(-rotSpeed);
            double oldPlaneX = planeX;
            planeX = planeX * cos(-rotSpeed) - planeY * sin(-rotSpeed);
            planeY = oldPlaneX * sin(-rotSpeed) + planeY * cos(-rotSpeed);
        }
            break;
        case SHEESH_SAG:
        {
            double oldDirX = dirX;
            dirX = dirX * cos(rotSpeed) - dirY * sin(rotSpeed);
            dirY = oldDirX * sin(rotSpeed) + dirY * cos(rotSpeed);
            double oldPlaneX = planeX;
            planeX = planeX * cos(rotSpeed) - planeY * sin(rotSpeed);
            planeY = oldPlaneX * sin(rotSpeed) + planeY * cos(rotSpeed);
        }
            break;
    }

    datas[0] = posX;
    datas[1] = posY;
    datas[2] = dirX;
    datas[3] = dirY;
    datas[4] = planeX;
    datas[5] = planeY;

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, posdirplane_ssbo);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(double) * 6, datas);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}