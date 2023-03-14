#include "display_FRM_OpenGL.h"

mesh load_giant_triangle()
{
    float vertices[] = {
        //giant triangle     uv coordinates?
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         3.0f, -1.0f, 0.0f,  2.0f, 0.0f,
        -1.0f,  3.0f, 0.0f,  0.0f, 2.0f
    };

    mesh triangle;
    triangle.vertexCount = 3;

    glGenVertexArrays(1, &triangle.VAO);
    glBindVertexArray(triangle.VAO);

    glGenBuffers(1, &triangle.VBO);
    glBindBuffer(GL_ARRAY_BUFFER, triangle.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return triangle;
}

void draw_to_window(struct image_data* img_data, Shader* shader, mesh* triangle)
{
    shader->use();
    glUniform1f(glGetUniformLocation(shader->ID, "new_zoom"), img_data->img_pos.new_zoom);
    glUniform2fv(glGetUniformLocation(shader->ID, "bottom_left_pos"), 1, img_data->img_pos.bottom_left);

    glViewport(0, 0, img_data->width * 8, img_data->height * 8);
    glBindTexture(GL_TEXTURE_2D, img_data->render_texture);
    glDrawArrays(GL_TRIANGLES, 0, triangle->vertexCount);
}

void draw_FRM_to_framebuffer(float* palette,
    Shader* shader, mesh* triangle,
    struct image_data* img_data)
{
    glViewport(0, 0, img_data->width, img_data->height);

    glBindFramebuffer(GL_FRAMEBUFFER, img_data->framebuffer);
    glBindVertexArray(triangle->VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, img_data->FRM);

    //shader
    shader->use();
    //glUniform1f(glGetUniformLocation(shader->ID, "new_zoom"), img_data->img_pos.new_zoom);
    //glUniform2fv(glGetUniformLocation(shader->ID, "bottom_left_pos"), 1, img_data->img_pos.bottom_left);
    glUniform3fv(glGetUniformLocation(shader->ID, "ColorPalette"), 256, palette);
    shader->setInt("Indexed_FRM", 0);

    glDrawArrays(GL_TRIANGLES, 0, triangle->vertexCount);

    //bind framebuffer back to default
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void draw_PAL_to_framebuffer(float* palette, Shader* shader,
                            mesh* triangle, struct image_data* img_data, SDL_Surface* surface)
{
    glViewport(0, 0, img_data->width, img_data->height);

    glBindTexture(GL_TEXTURE_2D, img_data->PAL_data);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, surface->w, surface->h, 0, GL_RED, GL_UNSIGNED_BYTE, surface->pixels);


    glBindFramebuffer(GL_FRAMEBUFFER, img_data->framebuffer);
    glBindVertexArray(triangle->VAO);
    glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, img_data->FRM);
    //glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, img_data->PAL_data);

    //shader
    shader->use();
    //glUniform1f(glGetUniformLocation(shader->ID, "new_zoom"), img_data->img_pos.new_zoom);
    //glUniform2fv(glGetUniformLocation(shader->ID, "bottom_left_pos"), 1, img_data->img_pos.bottom_left);
    glUniform3fv(glGetUniformLocation(shader->ID, "ColorPalette"), 256, palette);
    shader->setInt("Indexed_FRM", 0);
    //shader->setInt("Indexed_PAL", 1);

    glDrawArrays(GL_TRIANGLES, 0, triangle->vertexCount);

    //bind framebuffer back to default
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}