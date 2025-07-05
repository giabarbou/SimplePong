#include "BufferHandler.hpp"

BufferHandler::BufferHandler()
{

}

void BufferHandler::generateBuffers()
{
	glGenVertexArrays(1, &id_vao);
	glGenBuffers(1, &id_ibo);
	glGenBuffers(1, &id_vbo);
}

void BufferHandler::addIndexData(GLuint *indices_arr, GLuint size)
{
	for (size_t i = 0; i < size; i++) {
		indices.push_back(indices_arr[i]);
	}
	index_sizes.push_back(size);
}

void BufferHandler::addVertexData(GLfloat *vertices_arr, GLuint size)
{
	for (size_t i = 0; i < size; i++) {
		vertices.push_back(vertices_arr[i]);
	}
	vertex_sizes.push_back(size);
}

void BufferHandler::bindIndexBuffer()
{
	glBindVertexArray(id_vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_ibo);
}

void BufferHandler::unbindIndexBuffer()
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void BufferHandler::loadDataToGPU() 
{
	glBindVertexArray(id_vao);

	glBindBuffer(GL_ARRAY_BUFFER, id_vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), 0, GL_STATIC_DRAW);

	size_t offset = 0;

	for (size_t i = 0; i < vertex_sizes.size(); i++) {
		glBufferSubData(GL_ARRAY_BUFFER, offset * sizeof(GLfloat), vertex_sizes[i] * sizeof(GLfloat), &vertices[offset]);
		offset += vertex_sizes[i];
	}

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), 0, GL_STATIC_DRAW);

	offset = 0;

	for (size_t i = 0; i < index_sizes.size(); i++) {
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset * sizeof(GLuint), index_sizes[i] * sizeof(GLuint), &indices[offset]);
		offset += index_sizes[i];
	}

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);
}