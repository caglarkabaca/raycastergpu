#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <math.h>

const char *str_computeShader = R"(
    #version 450 core
    layout(local_size_x = 1, local_size_y = 1) in;
    layout(rgba32f, binding = 0) uniform image2D img_output;
    
    layout(std430, binding = 1) buffer WorldMapArray {
        int worldMap[];
    };

    layout(std430, binding = 2) buffer DatasArray {
        float datas[];
    };

    layout(binding = 3, rgba32f) readonly uniform image2D wall_output;

    void main() {

        vec4 pixel = vec4(0.7, 0.4, 0.0, 1.0);
        uint x = gl_GlobalInvocationID.x;
        int w = 512;
        int h = 512;

        vec2 pos = vec2(datas[0], datas[1]);
        vec2 dir = vec2(datas[2], datas[3]);
        vec2 plane = vec2(datas[4], datas[5]);

        float cameraX = 2 * x / float(w) - 1; //x-coordinate in camera space
        float rayDirX = dir.x + plane.x * cameraX;
        float rayDirY = dir.y + plane.y * cameraX;
        //which box of the map we're in
        int mapX = int(pos.x);
        int mapY = int(pos.y);

        //length of ray from current position to next x or y-side
        float sideDistX;
        float sideDistY;

        //length of ray from one x or y-side to next x or y-side
        //these are derived as:
        //deltaDistX = sqrt(1 + (rayDirY * rayDirY) / (rayDirX * rayDirX))
        //deltaDistY = sqrt(1 + (rayDirX * rayDirX) / (rayDirY * rayDirY))
        //which can be simplified to abs(|rayDir| / rayDirX) and abs(|rayDir| / rayDirY)
        //where |rayDir| is the length of the vector (rayDirX, rayDirY). Its length,
        //unlike (dirX, dirY) is not 1, however this does not matter, only the
        //ratio between deltaDistX and deltaDistY matters, due to the way the DDA
        //stepping further below works. So the values can be computed as below.
        // Division through zero is prevented, even though technically that's not
        // needed in C++ with IEEE 754 floating point values.
        float deltaDistX = (rayDirX == 0) ? 1e30 : abs(1 / rayDirX);
        float deltaDistY = (rayDirY == 0) ? 1e30 : abs(1 / rayDirY);

        float perpWallDist;

        //what direction to step in x or y-direction (either +1 or -1)
        int stepX;
        int stepY;

        int hit = 0; //was there a wall hit?
        int side; //was a NS or a EW wall hit?
        //calculate step and initial sideDist
        if(rayDirX < 0)
        {
            stepX = -1;
            sideDistX = (pos.x - mapX) * deltaDistX;
        }
        else
        {
            stepX = 1;
            sideDistX = (mapX + 1.0 - pos.x) * deltaDistX;
        }
        if(rayDirY < 0)
        {
            stepY = -1;
            sideDistY = (pos.y - mapY) * deltaDistY;
        }
        else
        {
            stepY = 1;
            sideDistY = (mapY + 1.0 - pos.y) * deltaDistY;
        }
        //perform DDA
        while(hit == 0)
        {

            //jump to next map square, either in x-direction, or in y-direction
            if(sideDistX < sideDistY)
            {
            sideDistX += deltaDistX;
            mapX += stepX;
            side = 0;
            }
            else
            {
            sideDistY += deltaDistY;
            mapY += stepY;
            side = 1;
            }
            //Check if ray has hit a wall
            if(worldMap[mapX * 10 + mapY] > 0) hit = 1;
        }
        //Calculate distance projected on camera direction. This is the shortest distance from the point where the wall is
        //hit to the camera plane. Euclidean to center camera point would give fisheye effect!
        //This can be computed as (mapX - posX + (1 - stepX) / 2) / rayDirX for side == 0, or same formula with Y
        //for size == 1, but can be simplified to the code below thanks to how sideDist and deltaDist are computed:
        //because they were left scaled to |rayDir|. sideDist is the entire length of the ray above after the multiple
        //steps, but we subtract deltaDist once because one step more into the wall was taken above.
        if(side == 0) perpWallDist = (sideDistX - deltaDistX);
        else          perpWallDist = (sideDistY - deltaDistY);

        //Calculate height of line to draw on screen
        int lineHeight = int(h / perpWallDist);

        //calculate lowest and highest pixel to fill in current stripe
        int drawStart = -lineHeight / 2 + h / 2;
        if(drawStart < 0) drawStart = 0;
        int drawEnd = lineHeight / 2 + h / 2;
        if(drawEnd >= h) drawEnd = h - 1;

        // TEXTURE
        ivec2 imgSize = imageSize(wall_output);
        int texWidth = imgSize.x;
        int texHeight = imgSize.y;

        //calculate value of wallX
        float wallX; //where exactly the wall was hit
        if (side == 0) wallX = pos.y + perpWallDist * rayDirY;
        else           wallX = pos.x + perpWallDist * rayDirX;
        wallX -= floor((wallX));

        //x coordinate on the texture
        int texX = int(wallX * float(texWidth));
        if(side == 0 && rayDirX > 0) texX = texWidth - texX - 1;
        if(side == 1 && rayDirY < 0) texX = texWidth - texX - 1;

        vec4 texture[1024];
        for (int i = 0; i < texHeight; i++) {
            texture[i] = imageLoad(wall_output, ivec2(texX, i));
        }

        float step = 1.0 * texHeight / lineHeight;
        // Starting texture coordinate
        float texPos = (drawStart - h / 2 + lineHeight / 2) * step;
        for(int y = drawStart; y < drawEnd; y++)
        {
            // Cast the texture coordinate to integer, and mask with (texHeight - 1) in case of overflow
            int texY = int(texPos) & (texHeight - 1);
            texPos += step;
            vec3 color = texture[texY].rgb;
            imageStore(img_output, ivec2(x, y), vec4(color, 1.0));
        }

        // SADECE RENK
        // switch (worldMap[mapX * 10 + mapY]) {
        //     case 1:
        //         pixel = vec4(1.0);
        //         break;
        //     case 2:
        //         pixel = vec4(1.0, 0.0, 0.0, 1.0);
        //         break;
        //     default:
        //         pixel = vec4(0.0, 1.0, 0.0, 1.0);
        //         break;
        // }

        // for (int i = drawStart; i < drawEnd; i++) {
        //     imageStore(img_output, ivec2(x, i), pixel);
        // }
    }

)";

const char *str_vertexShader = R"(
    #version 450 core
    layout(location = 0) in vec2 vertexPosition; 
    out vec2 texCoord;

    void main() {
        gl_Position = vec4(vertexPosition, 0.0, 1.0);
        texCoord = (vertexPosition + 1.0) * 0.5;
    }
)";

const char *str_fragmentShader = R"(
    #version 450 core
    in vec2 texCoord;
    out vec4 FragColor;

    uniform sampler2D textureSampler;

    void main() {
        FragColor = texture(textureSampler, texCoord);
    }
)";

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