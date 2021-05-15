#pragma once

struct Vertex
{
    float pos[3] = {0.0f, 0.0f, 0.0f};
    float col[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float normal[3] = {0.0f, 0.0f, 1.0f};
    float uv[2] = {0.0f, 0.0f};
};