#ifndef BUFFER_HANDLER_HPP
#define BUFFER_HANDLER_HPP

#include <vector>
#include <GL/glew.h>

class BufferHandler
{
public:
    BufferHandler();
    void generateBuffers();
    void loadDataToGPU();
    void addIndexData(GLuint* indices_arr, GLuint size);
    void addVertexData(GLfloat* vertices_arr, GLuint size);
    void bindIndexBuffer();
    void unbindIndexBuffer();
private:
    std::vector<GLfloat> vertices;
    std::vector<GLuint> vertex_sizes;

    std::vector<GLuint> indices;
    std::vector<GLuint> index_sizes;

    GLuint id_ibo{};
    GLuint id_vbo{};
    GLuint id_vao{};
};

#endif