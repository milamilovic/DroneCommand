#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <sstream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

unsigned int compileShader(GLenum type, const char* source);
unsigned int createShader(const char* vsSource, const char* fsSource);
static unsigned loadImageToTexture(const char* filePath);

struct Drone {
    float x, y;
    float radius;
    float batteryLevel;
    bool active;
};
Drone drone1 = { -0.5f, -0.5f, 0.05f, 100.0f, false };
Drone drone2 = { 0.5f, -0.5f, 0.05f, 100.0f, false };

void initializeDroneVertices(const Drone& drone, float vertices[], float aspectRatio) {
    const float centerX = drone.x;
    const float centerY = drone.y;
    const float radius = drone.radius;
    const int numPoints = 30;

    vertices[0] = drone.x;      // X
    vertices[1] = drone.y;      // Y
    vertices[2] = 0.102;  // R
    vertices[3] = 0.278;  // G
    vertices[4] = 0.149;  // B
    vertices[5] = 1.0;  // A

    for (int i = 0; i < numPoints; i++) {
        float theta = (2.07f * 3.1415f / numPoints) * i;
        float x = centerX + radius * cos(theta) * aspectRatio;
        float y = centerY + radius * sin(theta);

        int index = (i + 1) * 6;
        vertices[index] = x;
        vertices[index + 1] = y;
        vertices[index + 2] = 0.102; // Red
        vertices[index + 3] = 0.278; // Green
        vertices[index + 4] = 0.149; // Blue
        vertices[index + 5] = 1.0; // Alpha
    }
}

void initializeProgressVertices(float x, float y, float width, float height, float percentage, float r, float g, float b, float progressBarVertices[]) {
    float rg = 0.184, gg = 0.22, bg = 0.196;

    float borderXOffset = 0.007f;
    float borderYOffset = 0.008f;

    float adjustedWidth = width - 2 * borderXOffset;

    float vertices[] = {
        // Background bar (gray)
        x, y, rg, gg, bg, 1.0f,
        x + width, y, rg, gg, bg, 1.0f,
        x + width, y - height, rg, gg, bg, 1.0f,
        x, y - height, rg, gg, bg, 1.0f,

        // Filled portion (based on percentage)
        x + borderXOffset, y - borderYOffset, r, g, b, 1.0f,
        x + borderXOffset + adjustedWidth * percentage, y - borderYOffset, r, g, b, 1.0f,
        x + borderXOffset + adjustedWidth * percentage, y - height + borderYOffset, r, g, b, 1.0f,
        x + borderXOffset, y - height + borderYOffset, r, g, b, 1.0f,
    };

    memcpy(progressBarVertices, vertices, sizeof(vertices));
}

struct NoFlyZone {
    float x, y, radius;
    bool dragging;
    bool resizing;
};
NoFlyZone noFlyZone = { 0.05f, -0.07f, 0.20f, false, false };

void initializeNoFlyZoneVertices(float noFlyZoneVertices[], float aspectRatio) {
    const float centerX = noFlyZone.x;
    const float centerY = noFlyZone.y;
    const float radius = noFlyZone.radius;
    const int numPoints = 30;

    float R = noFlyZone.resizing ? 0.431f : 0.8f;
    float G = noFlyZone.resizing ? 0.031f : 0.0f;
    float B = noFlyZone.resizing ? 0.031f : 0.05f;

    // Set center point
    noFlyZoneVertices[0] = centerX;      // X
    noFlyZoneVertices[1] = centerY;      // Y
    noFlyZoneVertices[2] = R;  // R
    noFlyZoneVertices[3] = G;  // G
    noFlyZoneVertices[4] = B;  // B
    noFlyZoneVertices[5] = 0.5;  // A

    // Circle points
    for (int i = 0; i < numPoints; i++) {
        float theta = (2.07f * 3.1415f / numPoints) * i;
        float x = centerX + radius * cos(theta) * aspectRatio;
        float y = centerY + radius * sin(theta);

        int index = (i + 1) * 6;
        noFlyZoneVertices[index] = x;
        noFlyZoneVertices[index + 1] = y;
        noFlyZoneVertices[index + 2] = R; // Red
        noFlyZoneVertices[index + 3] = G; // Green
        noFlyZoneVertices[index + 4] = B; // Blue
        noFlyZoneVertices[index + 5] = 0.5; // Alpha
    }
}

void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
    if (noFlyZone.dragging) {
        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);

        float normalizedX = (2.0f * xpos / windowWidth - 1.0f);
        float normalizedY = -1.0f * (2.0f * ypos / windowHeight - 1.0f);

        float initialRadius = 0.20f;
        const float minX = -0.87f + (noFlyZone.radius - initialRadius) * 0.75;
        const float maxX = 0.87f - (noFlyZone.radius - initialRadius) * 0.75;
        const float minY = -0.51f + noFlyZone.radius - initialRadius;
        const float maxY = 0.83f - noFlyZone.radius + initialRadius;

        if (normalizedX < minX) normalizedX = minX;
        if (normalizedX > maxX) normalizedX = maxX;
        if (normalizedY < minY) normalizedY = minY;
        if (normalizedY > maxY) normalizedY = maxY;

        noFlyZone.x = normalizedX;
        noFlyZone.y = normalizedY;
    }
}

bool isPointInCircle(float x, float y, float cx, float cy, float radius) {
    float dx = x - cx;
    float dy = y - cy;
    return (dx * dx + dy * dy) <= (radius * radius);
}


void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            int windowWidth, windowHeight;
            glfwGetWindowSize(window, &windowWidth, &windowHeight);

            float aspectRatio = static_cast<float>(windowWidth) / windowHeight;
            float normalizedX = (2.0f * xpos / windowWidth - 1.0f) * aspectRatio;
            float normalizedY = -1.0f * (2.0f * ypos / windowHeight - 1.0f);

            if (isPointInCircle(normalizedX, normalizedY, noFlyZone.x, noFlyZone.y, noFlyZone.radius)) {
                noFlyZone.dragging = true;
            }
        } else if (action == GLFW_RELEASE) {
            noFlyZone.dragging = false;
        }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            int windowWidth, windowHeight;
            glfwGetWindowSize(window, &windowWidth, &windowHeight);

            float aspectRatio = static_cast<float>(windowWidth) / windowHeight;
            float normalizedX = (2.0f * xpos / windowWidth - 1.0f) * aspectRatio;
            float normalizedY = -1.0f * (2.0f * ypos / windowHeight - 1.0f);

            if (isPointInCircle(normalizedX, normalizedY, noFlyZone.x, noFlyZone.y, noFlyZone.radius)) {
                noFlyZone.resizing = true;
            }
        } else if (action == GLFW_RELEASE) {
            noFlyZone.resizing = false;
        }
    }
}

int main(void)
{

    if (!glfwInit())
    {
        std::cout<<"GLFW Biblioteka se nije ucitala! :(\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window;
    unsigned int wWidth = 1044;
    unsigned int wHeight = 800;
    const char wTitle[] = "Drone command";
    window = glfwCreateWindow(wWidth, wHeight, wTitle, NULL, NULL);
    
    if (window == NULL)
    {
        std::cout << "Prozor nije napravljen! :(\n";
        glfwTerminate();
        return 2;
    }

    glfwMakeContextCurrent(window);


    if (glewInit() != GLEW_OK)
    {
        std::cout << "GLEW nije mogao da se ucita! :'(\n";
        return 3;
    }

    unsigned int mapShader = createShader("map.vert", "map.frag");
    unsigned int textureShader = createShader("texture.vert", "texture.frag");
    unsigned int basicShader = createShader("basic.vert", "basic.frag");

    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    float noFlyZoneVertices[186];
    initializeNoFlyZoneVertices(noFlyZoneVertices, 0.75);
    float droneVertices1[186];
    initializeDroneVertices(drone1, droneVertices1, 0.75);
    float droneVertices2[186];
    initializeDroneVertices(drone2, droneVertices2, 0.75);
    float progressVertices1[48];
    float progressVertices2[48];
    initializeProgressVertices(-0.9f, -0.74f, 0.4f, 0.08f, drone1.batteryLevel / 100.0f, 0.329f, 0.612f, 0.404f, progressVertices1);
    initializeProgressVertices(-0.9f, -0.86f, 0.4f, 0.08f, drone2.batteryLevel / 100.0f, 0.329f, 0.612f, 0.404f, progressVertices2);


    float map[] = {
        // X    Y      S    T
        1.0f,  1.0f,  1.0f, 1.0f, // Top-right
        1.0f, -0.7f,  1.0f, 0.0f, // Bottom-right
       -1.0f, -0.7f,  0.0f, 0.0f, // Bottom-left
       -1.0f,  1.0f,  0.0f, 1.0f  // Top-left
    };

    unsigned int indices[] = {
        0, 1, 3, // First triangle
        1, 2, 3  // Second triangle
    };

    unsigned int stride = (2 + 2) * sizeof(float);

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(map), map, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    //no fly zone
    unsigned int noFlyZoneVAO, noFlyZoneVBO;
    glGenVertexArrays(1, &noFlyZoneVAO);
    glGenBuffers(1, &noFlyZoneVBO);
    glBindVertexArray(noFlyZoneVAO);
    glBindBuffer(GL_ARRAY_BUFFER, noFlyZoneVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(noFlyZoneVertices), noFlyZoneVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    //drone 1
    unsigned int droneVAO1, droneVBO1;
    glGenVertexArrays(1, &droneVAO1);
    glGenBuffers(1, &droneVBO1);
    glBindVertexArray(droneVAO1);
    glBindBuffer(GL_ARRAY_BUFFER, droneVBO1);
    glBufferData(GL_ARRAY_BUFFER, sizeof(droneVertices1), droneVertices1, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    //drone 2
    unsigned int droneVAO2, droneVBO2;
    glGenVertexArrays(1, &droneVAO2);
    glGenBuffers(1, &droneVBO2);
    glBindVertexArray(droneVAO2);
    glBindBuffer(GL_ARRAY_BUFFER, droneVBO2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(droneVertices2), droneVertices2, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    unsigned int progressIndices1[] = {
        0, 1, 2, 2, 3, 0, // Background
        4, 5, 6, 6, 7, 4  // Filled portion
    };

    unsigned int progressIndices2[] = {
        0, 1, 2, 2, 3, 0, // Background
        4, 5, 6, 6, 7, 4  // Filled portion
    };

    //progress bar 1
    unsigned int progressVAO1, progressVBO1, progressEBO1;
    glGenVertexArrays(1, &progressVAO1);
    glGenBuffers(1, &progressVBO1);
    glGenBuffers(1, &progressEBO1);
    glBindVertexArray(progressVAO1);
    glBindBuffer(GL_ARRAY_BUFFER, progressVBO1);
    glBufferData(GL_ARRAY_BUFFER, sizeof(progressVertices1), progressVertices1, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, progressEBO1);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(progressIndices1), progressIndices1, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    //progress bar 2
    unsigned int progressVAO2, progressVBO2, progressEBO2;
    glGenVertexArrays(1, &progressVAO2);
    glGenBuffers(1, &progressVBO2);
    glGenBuffers(1, &progressEBO2);
    glBindVertexArray(progressVAO2);
    glBindBuffer(GL_ARRAY_BUFFER, progressVBO2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(progressVertices2), progressVertices2, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, progressEBO2);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(progressIndices2), progressIndices2, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    //battery level 1
    float rectangleVertices11[] = {
        // Positions        // Texture Coords
        -0.73f, -0.76f,       0.0f, 1.0f,
        -0.73f, -0.80f,       0.0f, 0.0f,
        -0.71f, -0.80f,       1.0f, 0.0f,
        -0.71f, -0.76f,       1.0f, 1.0f
    };
    float rectangleVertices12[] = {
        // Positions        // Texture Coords
        -0.71f, -0.76f,       0.0f, 1.0f,
        -0.71f, -0.80f,       0.0f, 0.0f,
        -0.69f, -0.80f,       1.0f, 0.0f,
        -0.69f, -0.76f,       1.0f, 1.0f
    };
    float rectangleVertices13[] = {
        // Positions        // Texture Coords
        -0.69f, -0.76f,       0.0f, 1.0f,
        -0.69f, -0.80f,       0.0f, 0.0f,
        -0.67f, -0.80f,       1.0f, 0.0f,
        -0.67f, -0.76f,       1.0f, 1.0f
    };
    unsigned int batteryIndices1[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int batteryIndices2[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int batteryIndices3[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int batteryVao1[3], batteryVbo1[3], batteryEbo1[3];
    glGenVertexArrays(3, batteryVao1);
    glGenBuffers(3, batteryVbo1);
    glGenBuffers(3, batteryEbo1);
    //first number
    glBindVertexArray(batteryVao1[0]);
    glBindBuffer(GL_ARRAY_BUFFER, batteryVbo1[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectangleVertices11), rectangleVertices11, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batteryEbo1[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(batteryIndices1), batteryIndices1, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //second number
    glBindVertexArray(batteryVao1[1]);
    glBindBuffer(GL_ARRAY_BUFFER, batteryVbo1[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectangleVertices12), rectangleVertices12, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batteryEbo1[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(batteryIndices2), batteryIndices2, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //third number
    glBindVertexArray(batteryVao1[2]);
    glBindBuffer(GL_ARRAY_BUFFER, batteryVbo1[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectangleVertices13), rectangleVertices13, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batteryEbo1[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(batteryIndices3), batteryIndices3, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    //battery level 2
    float rectangleVertices21[] = {
        // Positions        // Texture Coords
        -0.73f, -0.88f,       0.0f, 1.0f,
        -0.73f, -0.92f,       0.0f, 0.0f,
        -0.71f, -0.92f,       1.0f, 0.0f,
        -0.71f, -0.88f,       1.0f, 1.0f
    };
    float rectangleVertices22[] = {
        // Positions        // Texture Coords
        -0.71f, -0.88f,       0.0f, 1.0f,
        -0.71f, -0.92f,       0.0f, 0.0f,
        -0.69f, -0.92f,       1.0f, 0.0f,
        -0.69f, -0.88f,       1.0f, 1.0f
    };
    float rectangleVertices23[] = {
        // Positions        // Texture Coords
        -0.69f, -0.88f,       0.0f, 1.0f,
        -0.69f, -0.92f,       0.0f, 0.0f,
        -0.67f, -0.92f,       1.0f, 0.0f,
        -0.67f, -0.88f,       1.0f, 1.0f
    };
    unsigned int batteryIndices21[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int batteryIndices22[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int batteryIndices23[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int batteryVao2[3], batteryVbo2[3], batteryEbo2[3];
    glGenVertexArrays(3, batteryVao2);
    glGenBuffers(3, batteryVbo2);
    glGenBuffers(3, batteryEbo2);
    //first number
    glBindVertexArray(batteryVao2[0]);
    glBindBuffer(GL_ARRAY_BUFFER, batteryVbo2[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectangleVertices21), rectangleVertices21, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batteryEbo2[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(batteryIndices21), batteryIndices21, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //second number
    glBindVertexArray(batteryVao2[1]);
    glBindBuffer(GL_ARRAY_BUFFER, batteryVbo2[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectangleVertices22), rectangleVertices22, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batteryEbo2[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(batteryIndices22), batteryIndices22, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //third number
    glBindVertexArray(batteryVao2[2]);
    glBindBuffer(GL_ARRAY_BUFFER, batteryVbo2[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectangleVertices23), rectangleVertices23, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batteryEbo2[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(batteryIndices23), batteryIndices23, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // 1 texture
    unsigned texture1 = loadImageToTexture("res/1.png");
    glBindTexture(GL_TEXTURE_2D, texture1);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    unsigned uTexLoc = glGetUniformLocation(textureShader, "uTex");
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    // 2 texture
    unsigned texture2 = loadImageToTexture("res/2.png");
    glBindTexture(GL_TEXTURE_2D, texture2);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    // 3 texture
    unsigned texture3 = loadImageToTexture("res/3.png");
    glBindTexture(GL_TEXTURE_2D, texture3);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    // 4 texture
    unsigned texture4 = loadImageToTexture("res/4.png");
    glBindTexture(GL_TEXTURE_2D, texture4);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    // 5 texture
    unsigned texture5 = loadImageToTexture("res/5.png");
    glBindTexture(GL_TEXTURE_2D, texture5);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    // 6 texture
    unsigned texture6 = loadImageToTexture("res/6.png");
    glBindTexture(GL_TEXTURE_2D, texture6);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    // 7 texture
    unsigned texture7 = loadImageToTexture("res/7.png");
    glBindTexture(GL_TEXTURE_2D, texture7);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    // 8 texture
    unsigned texture8 = loadImageToTexture("res/8.png");
    glBindTexture(GL_TEXTURE_2D, texture8);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    // 9 texture
    unsigned texture9 = loadImageToTexture("res/9.png");
    glBindTexture(GL_TEXTURE_2D, texture9);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    // 0 texture
    unsigned texture0 = loadImageToTexture("res/0.png");
    glBindTexture(GL_TEXTURE_2D, texture0);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    
    unsigned mapTexture = loadImageToTexture("res/majevica.png");
    glBindTexture(GL_TEXTURE_2D, mapTexture);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(mapShader);
    unsigned uTexLoc2 = glGetUniformLocation(mapShader, "uTex");
    glUniform1i(uTexLoc2, 0);
    glUseProgram(0);

    float timeSinceLastUpdate = 0.0f;


    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            noFlyZone = { 0.05f, -0.07f, 0.20f, false, false };
        }
        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {
            if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
                drone1.active = true;
            }
        }
        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {
            if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
                drone2.active = true;
            }
        }
        if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
            if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
                drone1.active = false;
            }
        }
        if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
            if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
                drone2.active = false;
            }
        }
        if (drone1.active) {
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                if(drone1.y < 0.95) drone1.y += 0.0005;
            }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                if (drone1.y > -0.65) drone1.y -= 0.0005;
            }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                if (drone1.x > -0.96) drone1.x -= 0.0005;
            }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                if (drone1.x < 0.96) drone1.x += 0.0005;
            }
        }
        if (drone2.active) {
            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
                if (drone2.y < 0.95) drone2.y += 0.0005;
            }
            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
                if (drone2.y > -0.65) drone2.y -= 0.0005;
            }
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
                if (drone2.x > -0.96) drone2.x -= 0.0005;
            }
            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
                if (drone2.x < 0.96) drone2.x += 0.0005;
            }
        }

        float currentTime = glfwGetTime();
        if (currentTime - timeSinceLastUpdate > 0.016f) {  // 60 FPS
            timeSinceLastUpdate = currentTime;

            if (drone1.active && drone1.batteryLevel > 0) {
                drone1.batteryLevel -= 0.1f;
            }
            if (drone2.active && drone2.batteryLevel > 0) {
                drone2.batteryLevel -= 0.1f;
            }
        }

        glClearColor(0.184, 0.341, 0.227, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        //map texture
        glUseProgram(mapShader);
        glBindVertexArray(VAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mapTexture);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
        glUseProgram(0);

        //no fly zone
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(basicShader);
        if (noFlyZone.resizing) {
            float aspectRatio = 0.75f;
            float maxXEdge = noFlyZone.x + noFlyZone.radius * aspectRatio;
            float minXEdge = noFlyZone.x - noFlyZone.radius * aspectRatio;
            float maxYEdge = noFlyZone.y + noFlyZone.radius;
            float minYEdge = noFlyZone.y - noFlyZone.radius;

            const float mapMaxX = 1.0f;
            const float mapMinX = -1.0f;
            const float mapMaxY = 1.0f;
            const float mapMinY = -0.7f;

            if (maxXEdge < mapMaxX && minXEdge > mapMinX && maxYEdge < mapMaxY && minYEdge > mapMinY) {
                noFlyZone.radius += 0.0001f;
            }
        }
        initializeNoFlyZoneVertices(noFlyZoneVertices, 0.75f);
        glBindBuffer(GL_ARRAY_BUFFER, noFlyZoneVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(noFlyZoneVertices), noFlyZoneVertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(noFlyZoneVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 31);

        //drone 1
        initializeDroneVertices(drone1, droneVertices1, 0.75f);
        glBindBuffer(GL_ARRAY_BUFFER, droneVBO1);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(droneVertices1), droneVertices1);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(droneVAO1);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 31);

        //drone 2
        initializeDroneVertices(drone2, droneVertices2, 0.75f);
        glBindBuffer(GL_ARRAY_BUFFER, droneVBO2);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(droneVertices2), droneVertices2);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(droneVAO2);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 31);

        //progress bar 1
        initializeProgressVertices(-0.9f, -0.74f, 0.4f, 0.08f, drone1.batteryLevel / 100.0f, 0.329f, 0.612f, 0.404f, progressVertices1);
        glBindVertexArray(progressVAO1);
        glBindBuffer(GL_ARRAY_BUFFER, progressVBO1);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(progressVertices1), progressVertices1);  // Update buffer data
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glUseProgram(basicShader);
        glBindVertexArray(progressVAO1);
        glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glUseProgram(0);

        //progress bar 2
        initializeProgressVertices(-0.9f, -0.86f, 0.4f, 0.08f, drone2.batteryLevel / 100.0f, 0.329f, 0.612f, 0.404f, progressVertices2);
        glBindVertexArray(progressVAO2);
        glBindBuffer(GL_ARRAY_BUFFER, progressVBO2);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(progressVertices2), progressVertices2);  // Update buffer data
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glUseProgram(basicShader);
        glBindVertexArray(progressVAO2);
        glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);


        int hundreds, tens, ones;
        int battery = static_cast<int>(round(drone1.batteryLevel));
        hundreds = battery / 100;
        tens = (battery / 10) % 10;
        ones = battery % 10;

        // Draw hundreds digit for Drone 1
        glUseProgram(textureShader);
        glActiveTexture(GL_TEXTURE0);
        switch (hundreds) {
            case 0: {
                break;
            }
            case 1: {
                glBindTexture(GL_TEXTURE_2D, texture1);
                break;
            }
            case 2: {
                glBindTexture(GL_TEXTURE_2D, texture2);
                break;
            }
            case 3: {
                glBindTexture(GL_TEXTURE_2D, texture3);
                break;
            }
            case 4: {
                glBindTexture(GL_TEXTURE_2D, texture4);
                break;
            }
            case 5: {
                glBindTexture(GL_TEXTURE_2D, texture5);
                break;
            }
            case 6: {
                glBindTexture(GL_TEXTURE_2D, texture6);
                break;
            }
            case 7: {
                glBindTexture(GL_TEXTURE_2D, texture7);
                break;
            }
            case 8: {
                glBindTexture(GL_TEXTURE_2D, texture8);
                break;
            }
            case 9: {
                glBindTexture(GL_TEXTURE_2D, texture9);
                break;
            }
            default: glBindTexture(GL_TEXTURE_2D, texture0); // Fallback, should not occur
        }
        if (hundreds != 0) {
            glBindVertexArray(batteryVao1[0]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        // Draw tens digit for Drone 1
        switch (tens) {
        case 0: {
            glBindTexture(GL_TEXTURE_2D, texture0);
            break;
        }
        case 1: {
            glBindTexture(GL_TEXTURE_2D, texture1);
            break;
        }
        case 2: {
            glBindTexture(GL_TEXTURE_2D, texture2);
            break;
        }
        case 3: {
            glBindTexture(GL_TEXTURE_2D, texture3);
            break;
        }
        case 4: {
            glBindTexture(GL_TEXTURE_2D, texture4);
            break;
        }
        case 5: {
            glBindTexture(GL_TEXTURE_2D, texture5);
            break;
        }
        case 6: {
            glBindTexture(GL_TEXTURE_2D, texture6);
            break;
        }
        case 7: {
            glBindTexture(GL_TEXTURE_2D, texture7);
            break;
        }
        case 8: {
            glBindTexture(GL_TEXTURE_2D, texture8);
            break;
        }
        case 9: {
            glBindTexture(GL_TEXTURE_2D, texture9);
            break;
        }
        default: glBindTexture(GL_TEXTURE_2D, texture0); // Fallback, should not occur
        }
        glBindVertexArray(batteryVao1[1]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Draw ones digit for Drone 1
        switch (ones) {
        case 0: {
            glBindTexture(GL_TEXTURE_2D, texture0);
            break;
        }
        case 1: {
            glBindTexture(GL_TEXTURE_2D, texture1);
            break;
        }
        case 2: {
            glBindTexture(GL_TEXTURE_2D, texture2);
            break;
        }
        case 3: {
            glBindTexture(GL_TEXTURE_2D, texture3);
            break;
        }
        case 4: {
            glBindTexture(GL_TEXTURE_2D, texture4);
            break;
        }
        case 5: {
            glBindTexture(GL_TEXTURE_2D, texture5);
            break;
        }
        case 6: {
            glBindTexture(GL_TEXTURE_2D, texture6);
            break;
        }
        case 7: {
            glBindTexture(GL_TEXTURE_2D, texture7);
            break;
        }
        case 8: {
            glBindTexture(GL_TEXTURE_2D, texture8);
            break;
        }
        case 9: {
            glBindTexture(GL_TEXTURE_2D, texture9);
            break;
        }
        default: glBindTexture(GL_TEXTURE_2D, texture0); // Fallback, should not occur
        }
        glBindVertexArray(batteryVao1[2]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Process battery level for Drone 2
        battery = static_cast<int>(round(drone2.batteryLevel));
        hundreds = battery / 100;
        tens = (battery / 10) % 10;
        ones = battery % 10;

        // Draw hundreds digit for Drone 2
        switch (hundreds) {
        case 0: {
            //glBindTexture(GL_TEXTURE_2D, texture0);
            break;
        }
        case 1: {
            glBindTexture(GL_TEXTURE_2D, texture1);
            break;
        }
        case 2: {
            glBindTexture(GL_TEXTURE_2D, texture2);
            break;
        }
        case 3: {
            glBindTexture(GL_TEXTURE_2D, texture3);
            break;
        }
        case 4: {
            glBindTexture(GL_TEXTURE_2D, texture4);
            break;
        }
        case 5: {
            glBindTexture(GL_TEXTURE_2D, texture5);
            break;
        }
        case 6: {
            glBindTexture(GL_TEXTURE_2D, texture6);
            break;
        }
        case 7: {
            glBindTexture(GL_TEXTURE_2D, texture7);
            break;
        }
        case 8: {
            glBindTexture(GL_TEXTURE_2D, texture8);
            break;
        }
        case 9: {
            glBindTexture(GL_TEXTURE_2D, texture9);
            break;
        }
        default: glBindTexture(GL_TEXTURE_2D, texture0); // Fallback, should not occur
        }

        if (hundreds != 0) {
            glBindVertexArray(batteryVao2[0]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        // Draw tens digit for Drone 2
        switch (tens) {
        case 0: {
            glBindTexture(GL_TEXTURE_2D, texture0);
            break;
        }
        case 1: {
            glBindTexture(GL_TEXTURE_2D, texture1);
            break;
        }
        case 2: {
            glBindTexture(GL_TEXTURE_2D, texture2);
            break;
        }
        case 3: {
            glBindTexture(GL_TEXTURE_2D, texture3);
            break;
        }
        case 4: {
            glBindTexture(GL_TEXTURE_2D, texture4);
            break;
        }
        case 5: {
            glBindTexture(GL_TEXTURE_2D, texture5);
            break;
        }
        case 6: {
            glBindTexture(GL_TEXTURE_2D, texture6);
            break;
        }
        case 7: {
            glBindTexture(GL_TEXTURE_2D, texture7);
            break;
        }
        case 8: {
            glBindTexture(GL_TEXTURE_2D, texture8);
            break;
        }
        case 9: {
            glBindTexture(GL_TEXTURE_2D, texture9);
            break;
        }
        default: glBindTexture(GL_TEXTURE_2D, texture0); // Fallback, should not occur
        }
        glBindVertexArray(batteryVao2[1]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Draw ones digit for Drone 2
        switch (ones) {
        case 0: {
            glBindTexture(GL_TEXTURE_2D, texture0);
            break;
        }
        case 1: {
            glBindTexture(GL_TEXTURE_2D, texture1);
            break;
        }
        case 2: {
            glBindTexture(GL_TEXTURE_2D, texture2);
            break;
        }
        case 3: {
            glBindTexture(GL_TEXTURE_2D, texture3);
            break;
        }
        case 4: {
            glBindTexture(GL_TEXTURE_2D, texture4);
            break;
        }
        case 5: {
            glBindTexture(GL_TEXTURE_2D, texture5);
            break;
        }
        case 6: {
            glBindTexture(GL_TEXTURE_2D, texture6);
            break;
        }
        case 7: {
            glBindTexture(GL_TEXTURE_2D, texture7);
            break;
        }
        case 8: {
            glBindTexture(GL_TEXTURE_2D, texture8);
            break;
        }
        case 9: {
            glBindTexture(GL_TEXTURE_2D, texture9);
            break;
        }
        default: glBindTexture(GL_TEXTURE_2D, texture0); // Fallback, should not occur
        }
        glBindVertexArray(batteryVao2[2]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glBindVertexArray(0);
        glUseProgram(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    glDeleteTextures(1, &mapTexture);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &noFlyZoneVBO);
    glDeleteVertexArrays(1, &noFlyZoneVAO);
    glDeleteBuffers(1, &droneVBO1);
    glDeleteVertexArrays(1, &droneVAO1);
    glDeleteBuffers(1, &droneVBO2);
    glDeleteVertexArrays(1, &droneVAO2);
    glDeleteVertexArrays(1, &progressVAO1);
    glDeleteBuffers(1, &progressVBO1);
    glDeleteBuffers(1, &progressEBO1);
    glDeleteVertexArrays(1, &progressVAO2);
    glDeleteBuffers(1, &progressVBO2);
    glDeleteBuffers(1, &progressEBO2);
    glDeleteVertexArrays(3, batteryVao1);
    glDeleteBuffers(3, batteryVbo1);
    glDeleteBuffers(3, batteryEbo1);
    glDeleteVertexArrays(3, batteryVao2);
    glDeleteBuffers(3, batteryVbo2);
    glDeleteBuffers(3, batteryEbo2);
    glDeleteProgram(mapShader);
    glDeleteProgram(textureShader);

    glfwTerminate();
    return 0;
}

unsigned int compileShader(GLenum type, const char* source)
{
    std::string content = "";
    std::ifstream file(source);
    std::stringstream ss;
    if (file.is_open())
    {
        ss << file.rdbuf();
        file.close();
        std::cout << "Uspjesno procitao fajl sa putanje \"" << source << "\"!" << std::endl;
    }
    else {
        ss << "";
        std::cout << "Greska pri citanju fajla sa putanje \"" << source << "\"!" << std::endl;
    }
     std::string temp = ss.str();
     const char* sourceCode = temp.c_str();

    int shader = glCreateShader(type);
    
    int success;
    char infoLog[512];
    glShaderSource(shader, 1, &sourceCode, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        if (type == GL_VERTEX_SHADER)
            printf("VERTEX");
        else if (type == GL_FRAGMENT_SHADER)
            printf("FRAGMENT");
        printf(" sejder ima gresku! Greska: \n");
        printf(infoLog);
    }
    return shader;
}
unsigned int createShader(const char* vsSource, const char* fsSource)
{
    
    unsigned int program;
    unsigned int vertexShader;
    unsigned int fragmentShader;

    program = glCreateProgram();

    vertexShader = compileShader(GL_VERTEX_SHADER, vsSource);
    fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource);

    
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);
    glValidateProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success);
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(program, 512, NULL, infoLog);
        std::cout << "Objedinjeni sejder ima gresku! Greska: \n";
        std::cout << infoLog << std::endl;
    }

    glDetachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    return program;
}
static unsigned loadImageToTexture(const char* filePath) {
    int TextureWidth;
    int TextureHeight;
    int TextureChannels;
    unsigned char* ImageData = stbi_load(filePath, &TextureWidth, &TextureHeight, &TextureChannels, 0);
    if (ImageData != NULL)
    {
        stbi__vertical_flip(ImageData, TextureWidth, TextureHeight, TextureChannels);

        GLint InternalFormat = -1;
        switch (TextureChannels) {
        case 1: InternalFormat = GL_RED; break;
        case 2: InternalFormat = GL_RG; break;
        case 3: InternalFormat = GL_RGB; break;
        case 4: InternalFormat = GL_RGBA; break;
        default: InternalFormat = GL_RGB; break;
        }

        unsigned int Texture;
        glGenTextures(1, &Texture);
        glBindTexture(GL_TEXTURE_2D, Texture);
        glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, TextureWidth, TextureHeight, 0, InternalFormat, GL_UNSIGNED_BYTE, ImageData);
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(ImageData);
        return Texture;
    }
    else
    {
        std::cout << "Textura nije ucitana! Putanja texture: " << filePath << std::endl;
        stbi_image_free(ImageData);
        return 0;
    }
}