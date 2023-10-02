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

    int w = 640;
    int h = 480;
    vec4 pixel = vec4(0.7, 0.4, 0.0, 1.0);
    uint x = gl_GlobalInvocationID.x;

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
