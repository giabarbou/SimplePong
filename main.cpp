#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <unordered_map>
#include <cmath>
#include "Shader.hpp"
#include "BufferHandler.hpp"
#include "chrono"
#include "thread"
#include "Shapes2D.hpp"

namespace GameConstants {

    constexpr GLfloat BALL_SPEED_INITIAL = 1.5f;

    constexpr GLfloat BALL_SPEED_REFLECT_PLAYER_1 = 2.5f;
    constexpr GLfloat BALL_SPEED_REFLECT_PLAYER_2 = BALL_SPEED_REFLECT_PLAYER_1;
    
    constexpr GLfloat PLAYER_1_SPEED = 2.5f;
    constexpr GLfloat PLAYER_2_SPEED = PLAYER_1_SPEED;

    constexpr GLfloat BALL_POS_INITIAL[2] = { 0.0f, 0.5f };

    constexpr GLfloat PLAYER_1_POS_INITIAL[2] = { -1.5f, 0.5f };
    constexpr GLfloat PLAYER_2_POS_INITIAL[2] = { 1.5f, 0.5f };

    constexpr GLfloat PLAYER_1_WIDTH = 0.1f;
    constexpr GLfloat PLAYER_1_HEIGHT = 0.5f;

    constexpr GLfloat PLAYER_2_WIDTH = PLAYER_1_WIDTH;
    constexpr GLfloat PLAYER_2_HEIGHT = PLAYER_1_HEIGHT;

    constexpr GLfloat LINES_WIDTH = 0.05f;
    constexpr GLfloat LINES_HEIGHT = 0.2f;
    constexpr GLint NUM_LINES = 20;

    constexpr GLfloat BALL_WIDTH = 0.1f;
    constexpr GLfloat BALL_HEIGHT = 0.1f;

    constexpr GLint WAIT_SECONDS_BEFORE_BALL_SPAWNS = 1;
};

struct GameContext
{
    const GLint win_width  = 1000;
    const GLint win_height = 800;

    glm::mat4 view_projection;
    GLuint index_offset = 0;

    GLFWwindow* main_window = nullptr;
    int buffer_width = 1;
    int buffer_height = 1;

    Rectangle2D p1;
    Rectangle2D p2;
    Rectangle2D ball;

    std::vector<Rectangle2D> lines;
    
    BufferHandler buffer_handler;    

    bool p1_scored = false;
    bool p2_scored = false;

    bool should_reset_ball = false;

    bool ball_out_of_bounds = false;

    ShaderHandler shader_handler;

    GLfloat proj_up = 2.0f;
    GLfloat proj_down = -2.0f;

    GLfloat proj_right = 2.0f;
    GLfloat proj_left = -2.0f;

    GLfloat proj_near = -1.0f;
    GLfloat proj_far = 1.0f;

    GLfloat last_frame_time = 0.0f;
    GLfloat delta_time = 0.0f;
};

// vertex shader
static const char* v_shader_code = "                                                                    \n\
#version 330                                                                                            \n\
                                                                                                        \n\
layout(location = 0) in vec2 pos;                                                                       \n\
                                                                                                        \n\
uniform vec2 offset;                                                                                    \n\
uniform mat4 view_projection;                                                                           \n\
                                                                                                        \n\
void main()                                                                                             \n\
{                                                                                                       \n\
    gl_Position = view_projection * vec4(offset.x + pos.x, offset.y + pos.y, 0.0, 1.0);                 \n\
}                                                                                                       \n\
";

// fragment shader
static const char* f_shader_code = "                            \n\
#version 330                                                    \n\
                                                                \n\
out vec4 color;                                                 \n\
                                                                \n\
void main()                                                     \n\
{                                                               \n\
    color = vec4(.7f, .7f, .7f, 1.0f);                          \n\
}                                                               \n\
";

static void loadRectanglesToBuffers(GameContext &ctx)
{
    ctx.buffer_handler.addVertexData(ctx.p1.vertices, ctx.p1.getNumVertices());
    ctx.buffer_handler.addVertexData(ctx.p2.vertices, ctx.p2.getNumVertices());
    ctx.buffer_handler.addVertexData(ctx.ball.vertices, ctx.ball.getNumVertices());

    for (Rectangle2D& line : ctx.lines) {
        ctx.buffer_handler.addVertexData(line.vertices, line.getNumVertices());
    }

    int i = 0;
    for (GLuint& index : ctx.p1.indices) index += i * 4; 
    i++;
    for (GLuint& index : ctx.p2.indices) index += i * 4; 
    i++;
    for (GLuint& index : ctx.ball.indices) index += i * 4; 
    i++;

    for (Rectangle2D& line : ctx.lines) {
        for (GLuint& index : line.indices) index += i * 4;
    }

    ctx.buffer_handler.addIndexData(ctx.p1.indices, ctx.p1.getNumIndices());
    ctx.buffer_handler.addIndexData(ctx.p2.indices, ctx.p2.getNumIndices());
    ctx.buffer_handler.addIndexData(ctx.ball.indices, ctx.ball.getNumIndices());

    for (Rectangle2D& line : ctx.lines) {
        ctx.buffer_handler.addIndexData(line.indices, line.getNumIndices());
    }

    ctx.buffer_handler.loadDataToGPU();
}

static std::vector<Rectangle2D> createLines(GLfloat width, GLfloat height, int num_lines)
{
    std::vector<Rectangle2D> line_rects;

    GLfloat gap = 0.2f;

    GLfloat curr_offset = 0.0f;

    for (int i = 0; i < num_lines; i++) {
        Rectangle2D rect(width, height, { 0.0f, 2.0f - height / 2.0f - curr_offset });
        line_rects.emplace_back(rect);
        curr_offset += height + gap;
    }

    return line_rects;
}

static void initWindowArgs()
{
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
}

static void initMainWindow(GLFWwindow *main_window, GameContext &ctx)
{
    glfwGetFramebufferSize(main_window, &ctx.buffer_width, &ctx.buffer_height);
    glfwMakeContextCurrent(main_window);

    glViewport(0, 0, ctx.buffer_width, ctx.buffer_height);

    glfwSetWindowUserPointer(main_window, &ctx);
}

void updatePosition(Rectangle2D &obj, const GameContext &ctx)
{
    obj.position.x += obj.speed.x * ctx.delta_time;
    obj.position.y += obj.speed.y * ctx.delta_time;
}

bool playerScored_player_1(const Rectangle2D &player, const GameContext &ctx)
{
    constexpr float ball_pass_through_lim = 0.6;
    return ctx.ball.position.x <= player.position.x - player.width * ball_pass_through_lim;
}

bool playerScored_player_2(const Rectangle2D &player, const GameContext &ctx)
{
    constexpr float ball_pass_through_lim = 0.6;
    return ctx.ball.position.x >= player.position.x - player.width * ball_pass_through_lim;
}

struct CollisionInfo
{
    GLfloat collision_x{};
    GLfloat lower_bound{};
    GLfloat upper_bound{};
};

bool hasCollided_player_1(const CollisionInfo &info, const GameContext &ctx)
{
    return ctx.ball.position.x <= info.collision_x && (ctx.ball.position.y >= info.lower_bound && ctx.ball.position.y <= info.upper_bound);
}

void updateBallDirection_player_1(GameContext& ctx, const CollisionInfo& info)
{
    constexpr GLfloat theta_max = 75.0f;

    ctx.ball.position.x = info.collision_x + FLT_MIN;

    GLfloat d = ctx.ball.position.y - ctx.p1.position.y;

    GLfloat angle_deg = 2 * theta_max / ctx.p2.height * d;
    GLfloat angle = glm::radians(angle_deg);

    Point2D straight_vec = { GameConstants::BALL_SPEED_REFLECT_PLAYER_1, 0.0f };

    GLfloat r_x = cos(angle) * straight_vec.x - sin(angle) * straight_vec.y;
    GLfloat r_y = sin(angle) * straight_vec.x + cos(angle) * straight_vec.y;

    Point2D rotated_vec = { r_x, r_y };

    ctx.ball.speed = rotated_vec;
}

void checkCollision_player_1(Rectangle2D& p, Rectangle2D& ball, GameContext &ctx)
{
    CollisionInfo info;

    info.collision_x = p.position.x + p.width * 0.5 + ball.width * 0.5;
    info.lower_bound = p.position.y - p.height * 0.5;
    info.upper_bound = p.position.y + p.height * 0.5;

    if (hasCollided_player_1(info, ctx)) {
        updateBallDirection_player_1(ctx, info);
    }
}

bool hasCollided_player_2(const CollisionInfo &info, const GameContext &ctx)
{
    return ctx.ball.position.x >= info.collision_x && (ctx.ball.position.y >= info.lower_bound && ctx.ball.position.y <= info.upper_bound);
}

void updateBallDirection_player_2(GameContext &ctx, const CollisionInfo &info)
{
    constexpr GLfloat theta_max = 75.0f;

    ctx.ball.position.x = info.collision_x - FLT_MIN;

    GLfloat d = ctx.ball.position.y - ctx.p2.position.y;

    GLfloat angle_deg = 2 * theta_max / ctx.p2.height * d;
    GLfloat angle = glm::radians(angle_deg);

    Point2D straight_vec = { -GameConstants::BALL_SPEED_REFLECT_PLAYER_2, 0.0f };

    GLfloat r_x = cos(-angle) * straight_vec.x - sin(-angle) * straight_vec.y;
    GLfloat r_y = sin(-angle) * straight_vec.x + cos(-angle) * straight_vec.y;

    Point2D rotated_vec = { r_x, r_y };

    ctx.ball.speed = rotated_vec;
}

void checkCollision_player_2(Rectangle2D& p, Rectangle2D& ball, GameContext &ctx)
{
    CollisionInfo info;

    info.collision_x = p.position.x - p.width * 0.5 - ball.width * 0.5;
    info.lower_bound = p.position.y - p.height * 0.5;
    info.upper_bound = p.position.y + p.height * 0.5;

    if (hasCollided_player_2(info, ctx)) {
        updateBallDirection_player_2(ctx, info);
    }
}

void checkCollision_up(Rectangle2D& ball, GameContext& ctx)
{
    GLfloat up_collision_y = ctx.proj_up - ctx.ball.height * 0.5;

    if (ctx.ball.position.y >= up_collision_y) {
        ctx.ball.position.y = up_collision_y - FLT_MIN;
        ctx.ball.speed.y = -ctx.ball.speed.y;
    }
}

void checkCollision_down(Rectangle2D& ball, GameContext& ctx)
{
    GLfloat down_collision_y = ctx.proj_down + ctx.ball.height * 0.5;

    if (ctx.ball.position.y <= down_collision_y) {
        ctx.ball.position.y = down_collision_y + FLT_MIN;
        ctx.ball.speed.y = -ctx.ball.speed.y;
    }
}

static void drawRectangle(Rectangle2D &rect, GameContext &ctx)
{
    GLint uniform_offset = ctx.shader_handler.getUniformVariableId("offset");

    glm::vec2 offset = glm::vec2(rect.position.x, rect.position.y);
    glUniform2fv(uniform_offset, 1, glm::value_ptr(offset));
    glDrawElements(GL_TRIANGLES, rect.getNumIndices(), GL_UNSIGNED_INT, (void *)(sizeof(GLuint) * ctx.index_offset));
    ctx.index_offset += rect.getNumIndices();
}

static void drawGameObjects(GameContext &ctx)
{
    ctx.buffer_handler.bindIndexBuffer();

    GLint uniform_view_projection = ctx.shader_handler.getUniformVariableId("view_projection");

    glUniformMatrix4fv(uniform_view_projection, 1, GL_FALSE, glm::value_ptr(ctx.view_projection));

    ctx.index_offset = 0;

    drawRectangle(ctx.p1, ctx);
    drawRectangle(ctx.p2, ctx);
    drawRectangle(ctx.ball, ctx);

    for (Rectangle2D& line : ctx.lines) {
        drawRectangle(line, ctx);
    }

    ctx.buffer_handler.bindIndexBuffer();
}

static bool isBallOutOfBoundsLeft(Rectangle2D& ball, GameContext& ctx)
{
    if (ball.position.x + ball.width * 0.5 <= ctx.proj_left) {
        return true;
    }
    return false;
}

static bool isBallOutOfBoundsRight(Rectangle2D& ball, GameContext& ctx)
{
    if (ball.position.x - ball.width * 0.5 >= ctx.proj_right) {
        return true;
    }
    return false;
}

static void resetGame(GameContext &ctx)
{
    using namespace GameConstants;

    ctx.p1_scored = false;
    ctx.p2_scored = false;

    ctx.should_reset_ball = false;
    ctx.ball.position = { BALL_POS_INITIAL[0], BALL_POS_INITIAL[1]};
    ctx.ball.speed = { BALL_SPEED_INITIAL, 0.0f };

    std::this_thread::sleep_for(std::chrono::seconds(WAIT_SECONDS_BEFORE_BALL_SPAWNS));
    ctx.last_frame_time += WAIT_SECONDS_BEFORE_BALL_SPAWNS;
}

static void checkCollisionsAndBallOutOfBounds(GameContext &ctx)
{
    if (!ctx.p1_scored && !ctx.p2_scored) {

        checkCollision_player_1(ctx.p1, ctx.ball, ctx);
        checkCollision_player_2(ctx.p2, ctx.ball, ctx);

        if (playerScored_player_1(ctx.p1, ctx)) ctx.p1_scored = true;
        else if (playerScored_player_2(ctx.p2, ctx)) ctx.p2_scored = true;

    } else if (ctx.p1_scored && !ctx.ball_out_of_bounds) {

        if (isBallOutOfBoundsLeft(ctx.ball, ctx)) {
            ctx.should_reset_ball = true;
            ctx.ball_out_of_bounds = false;
        }
    } else if (ctx.p2_scored && !ctx.ball_out_of_bounds) {

        if (isBallOutOfBoundsRight(ctx.ball, ctx)) {
            ctx.should_reset_ball = true;
            ctx.ball_out_of_bounds = false;
        }
    }
}

static void runGameLoop(GameContext &ctx)
{
    using namespace GameConstants;

    float curr_frame_time = glfwGetTime();
    ctx.delta_time = curr_frame_time - ctx.last_frame_time;
    ctx.last_frame_time = curr_frame_time;

    glfwPollEvents();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ctx.shader_handler.enableShaders();

    updatePosition(ctx.p1, ctx);
    updatePosition(ctx.p2, ctx);
    updatePosition(ctx.ball, ctx);

    checkCollisionsAndBallOutOfBounds(ctx);

    if (ctx.should_reset_ball) {
        resetGame(ctx);
    }

    checkCollision_up(ctx.ball, ctx);
    checkCollision_down(ctx.ball, ctx);

    drawGameObjects(ctx);

    ctx.shader_handler.disableShaders();

    glfwSwapBuffers(ctx.main_window);
}

void handleKeys(GLFWwindow* window, int key, int code, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    GameContext &ctx = *static_cast<GameContext *>(glfwGetWindowUserPointer(window));
    static bool pressed[GLFW_KEY_LAST + 1] = {};

    if (key >= 0 && key < 1024) {

        if (action == GLFW_PRESS) {
            pressed[key] = true;
        } else if (action == GLFW_RELEASE) {
            pressed[key] = false;
        }
    }

    if (!pressed[GLFW_KEY_W] && !pressed[GLFW_KEY_S]) {
        ctx.p1.speed.y = 0.0f;
    } else if (pressed[GLFW_KEY_W] && !pressed[GLFW_KEY_S]) {
        ctx.p1.speed.y = GameConstants::PLAYER_1_SPEED;
    }  else if (!pressed[GLFW_KEY_W] && pressed[GLFW_KEY_S]) {
        ctx.p1.speed.y = -GameConstants::PLAYER_1_SPEED;
    }
    
    if (!pressed[GLFW_KEY_UP] && !pressed[GLFW_KEY_DOWN]) {
        ctx.p2.speed.y = 0.0f;
    } else if (pressed[GLFW_KEY_UP] && !pressed[GLFW_KEY_DOWN]) {
        ctx.p2.speed.y = GameConstants::PLAYER_2_SPEED;
    }  else if (!pressed[GLFW_KEY_UP] && pressed[GLFW_KEY_DOWN]) {
        ctx.p2.speed.y = -GameConstants::PLAYER_2_SPEED;
    }
}


void initGameShaders(GameContext &ctx)
{
    Shader v_shader{ 0, GL_VERTEX_SHADER, v_shader_code };
    Shader f_shader{ 0, GL_FRAGMENT_SHADER, f_shader_code };

    ctx.shader_handler.add(v_shader);
    ctx.shader_handler.add(f_shader);
    ctx.shader_handler.compileShaders();
    ctx.shader_handler.linkShaders();
    ctx.shader_handler.validateShaders();
}

void initGameObjects(GameContext &ctx)
{
    using namespace GameConstants;

    ctx.p1 = Rectangle2D(PLAYER_1_WIDTH, PLAYER_1_HEIGHT, { PLAYER_1_POS_INITIAL[0], PLAYER_1_POS_INITIAL[1] } );
    ctx.p2 = Rectangle2D(PLAYER_2_WIDTH, PLAYER_2_HEIGHT, { PLAYER_2_POS_INITIAL[0], PLAYER_2_POS_INITIAL[1] } );
    ctx.ball = Rectangle2D(BALL_WIDTH, BALL_HEIGHT, { BALL_POS_INITIAL[0], BALL_POS_INITIAL[0] } );
    ctx.lines = createLines(LINES_WIDTH, LINES_HEIGHT, NUM_LINES);
    ctx.ball.speed = { BALL_SPEED_INITIAL, 0.0f };
}

glm::mat4 initViewProjectionMatrix(GameContext &ctx) 
{
    glm::vec3 camera_pos = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 camera_target = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 up_vector = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::mat4 view = glm::lookAt(camera_pos, camera_target, up_vector);

    glm::mat4 projection = glm::ortho(ctx.proj_down, ctx.proj_up, ctx.proj_left, ctx.proj_right, ctx.proj_near, ctx.proj_far);

    glm::mat4 view_projection = projection * view;

    return view_projection;
}

int main()
{
    glfwInit();
    initWindowArgs();

    GameContext ctx;

    ctx.main_window = glfwCreateWindow(ctx.win_width, ctx.win_height, "Simple Pong", nullptr, nullptr);
    initMainWindow(ctx.main_window, ctx);

    ctx.view_projection = initViewProjectionMatrix(ctx);

    glewExperimental = GL_TRUE;
    glewInit();

    initGameObjects(ctx);

    ctx.buffer_handler.generateBuffers();
    loadRectanglesToBuffers(ctx);

    initGameShaders(ctx);
    glfwSetKeyCallback(ctx.main_window, handleKeys);

    ctx.last_frame_time = glfwGetTime();
    while (!glfwWindowShouldClose(ctx.main_window)) {
        runGameLoop(ctx);
    }

    return 0;
}