#ifndef SHAPES_2D_HPP
#define SHAPES_2D_HPP

#include <GL/glew.h>

struct Point2D
{
    GLfloat x;
    GLfloat y;
};

struct Rectangle2D 
{
    GLfloat height = 0;
    GLfloat width = 0;

    Point2D position { 0.0 };
    Point2D speed { 0.0 };

    GLfloat vertices[8] = {
        -0.5f, -0.5f,
        0.5f, -0.5f,
        0.5f,  0.5f,
        -0.5f,  0.5f
    };

    GLuint indices[6] = {
        0, 1, 3,
        1, 2, 3
    };

    int getNumVertices() { return 8; }
    int getNumIndices() { return 6; }

    int vertexBufferSize() { return getNumVertices() * sizeof(GLfloat); }
    int indexBufferSize() { return getNumIndices() * sizeof(GLuint); }

    Rectangle2D() = default;

    Rectangle2D(GLfloat width, GLfloat height, const Point2D &position) : width(width), height(height), position(position) {
        for (int i = 0; i < getNumVertices(); i+=2) vertices[i] *= width;
        for (int i = 1; i < getNumVertices(); i+=2) vertices[i] *= height;
    }
};

#endif