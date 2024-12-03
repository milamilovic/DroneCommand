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
    bool destroyed;
};
Drone drone1 = { -0.5f, -0.5f, 0.05f, 100.0f, false, false };
Drone drone2 = { 0.5f, -0.5f, 0.05f, 100.0f, false, false };

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

bool areClashing(float x1, float y1, float r1, float x2, float y2, float r2) {
    float dx = (x2 - x1) * 4 / 3;
    float dy = y2 - y1;
    float distance = std::sqrt(dx * dx + dy * dy);
    return distance <= (r1 + r2);
}

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
    initializeProgressVertices(-0.9f, -0.86f, 0.4f, 0.08f, drone1.batteryLevel / 100.0f, 0.329f, 0.612f, 0.404f, progressVertices1);
    initializeProgressVertices(-0.9f, -0.86f, 0.6f, 0.08f, drone2.batteryLevel / 100.0f, 0.329f, 0.612f, 0.404f, progressVertices2);


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
        -0.23f, -0.76f,       0.0f, 1.0f,
        -0.23f, -0.80f,       0.0f, 0.0f,
        -0.21f, -0.80f,       1.0f, 0.0f,
        -0.21f, -0.76f,       1.0f, 1.0f
    };
    float rectangleVertices22[] = {
        // Positions        // Texture Coords
        -0.21f, -0.76f,       0.0f, 1.0f,
        -0.21f, -0.80f,       0.0f, 0.0f,
        -0.19f, -0.80f,       1.0f, 0.0f,
        -0.19f, -0.76f,       1.0f, 1.0f
    };
    float rectangleVertices23[] = {
        // Positions        // Texture Coords
        -0.19f, -0.76f,       0.0f, 1.0f,
        -0.19f, -0.80f,       0.0f, 0.0f,
        -0.17f, -0.80f,       1.0f, 0.0f,
        -0.17f, -0.76f,       1.0f, 1.0f
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

    //coordinates 1 : X
    float coordinates1x[] = {
        // Positions        // Texture Coords
        -0.90f, -0.86f,       0.0f, 1.0f,
        -0.90f, -0.90f,       0.0f, 0.0f,
        -0.875f, -0.90f,       1.0f, 0.0f,
        -0.875f, -0.86f,       1.0f, 1.0f
    };
    float coordinates1dot1[] = {
        // Positions        // Texture Coords
        -0.87f, -0.87f,       0.0f, 1.0f,
        -0.87f, -0.875f,       0.0f, 0.0f,
        -0.875f, -0.875f,       1.0f, 0.0f,
        -0.875f, -0.87f,       1.0f, 1.0f
    };
    float coordinates1dot2[] = {
        // Positions        // Texture Coords
        -0.87f, -0.88f,       0.0f, 1.0f,
        -0.87f, -0.885f,       0.0f, 0.0f,
        -0.875f, -0.885f,       1.0f, 0.0f,
        -0.875f, -0.88f,       1.0f, 1.0f
    };
    float coordinates1minus[] = {
        // Positions        // Texture Coords
        -0.85f, -0.875f,       0.0f, 1.0f,
        -0.85f, -0.88f,       0.0f, 0.0f,
        -0.84f, -0.88f,       1.0f, 0.0f,
        -0.84f, -0.875f,       1.0f, 1.0f
    };
    float coordinates1num1[] = {
        // Positions        // Texture Coords
        -0.83f, -0.86f,       0.0f, 1.0f,
        -0.83f, -0.90f,       0.0f, 0.0f,
        -0.81f, -0.90f,       1.0f, 0.0f,
        -0.81f, -0.86f,       1.0f, 1.0f
    };
    float coordinates1dot[] = {
        // Positions        // Texture Coords
        -0.80f, -0.89f,       0.0f, 1.0f,
        -0.80f, -0.895f,       0.0f, 0.0f,
        -0.805f, -0.895f,       1.0f, 0.0f,
        -0.805f, -0.89f,       1.0f, 1.0f
    };
    float coordinates1num2[] = {
        // Positions        // Texture Coords
        -0.79f, -0.86f,       0.0f, 1.0f,
        -0.79f, -0.90f,       0.0f, 0.0f,
        -0.77f, -0.90f,       1.0f, 0.0f,
        -0.77f, -0.86f,       1.0f, 1.0f
    };
    float coordinates1num3[] = {
        // Positions        // Texture Coords
        -0.77f, -0.86f,       0.0f, 1.0f,
        -0.77f, -0.90f,       0.0f, 0.0f,
        -0.75f, -0.90f,       1.0f, 0.0f,
        -0.75f, -0.86f,       1.0f, 1.0f
    };
    unsigned int indices1x[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices1dot1[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices1dot2[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices1minus[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices1num1[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices1dot[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices1num2[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices1num3[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int coordinatesVao1[8], coordinatesVbo1[8], coordinatesEbo1[8];
    glGenVertexArrays(8, coordinatesVao1);
    glGenBuffers(8, coordinatesVbo1);
    glGenBuffers(8, coordinatesEbo1);
    //drone 1 x
    glBindVertexArray(coordinatesVao1[0]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo1[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates1x), coordinates1x, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo1[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices1x), indices1x, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 1 x dot 1
    glBindVertexArray(coordinatesVao1[1]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo1[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates1dot1), coordinates1dot1, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo1[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices1dot1), indices1dot1, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 1 x dot 2
    glBindVertexArray(coordinatesVao1[2]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo1[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates1dot2), coordinates1dot2, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo1[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices1dot2), indices1dot2, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 1 x minus
    glBindVertexArray(coordinatesVao1[3]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo1[3]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates1minus), coordinates1minus, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo1[3]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices1minus), indices1minus, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 1 x number 1
    glBindVertexArray(coordinatesVao1[4]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo1[4]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates1num1), coordinates1num1, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo1[4]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices1num1), indices1num1, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 1 x dot
    glBindVertexArray(coordinatesVao1[5]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo1[5]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates1dot), coordinates1dot, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo1[5]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices1dot), indices1dot, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 1 x number 2
    glBindVertexArray(coordinatesVao1[6]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo1[6]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates1num2), coordinates1num2, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo1[6]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices1num1), indices1num1, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 1 x number 3
    glBindVertexArray(coordinatesVao1[7]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo1[7]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates1num3), coordinates1num3, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo1[7]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices1num1), indices1num1, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


    //coordinates 1 : Y
    float coordinates1y[] = {
        // Positions        // Texture Coords
        -0.70f, -0.86f,       0.0f, 1.0f,
        -0.70f, -0.90f,       0.0f, 0.0f,
        -0.675f, -0.90f,       1.0f, 0.0f,
        -0.675f, -0.86f,       1.0f, 1.0f
    };
    float coordinates1dot1y[] = {
        // Positions        // Texture Coords
        -0.67f, -0.87f,       0.0f, 1.0f,
        -0.67f, -0.875f,       0.0f, 0.0f,
        -0.675f, -0.875f,       1.0f, 0.0f,
        -0.675f, -0.87f,       1.0f, 1.0f
    };
    float coordinates1dot2y[] = {
        // Positions        // Texture Coords
        -0.67f, -0.88f,       0.0f, 1.0f,
        -0.67f, -0.885f,       0.0f, 0.0f,
        -0.675f, -0.885f,       1.0f, 0.0f,
        -0.675f, -0.88f,       1.0f, 1.0f
    };
    float coordinates1minusy[] = {
        // Positions        // Texture Coords
        -0.65f, -0.875f,       0.0f, 1.0f,
        -0.65f, -0.88f,       0.0f, 0.0f,
        -0.64f, -0.88f,       1.0f, 0.0f,
        -0.64f, -0.875f,       1.0f, 1.0f
    };
    float coordinates1num1y[] = {
        // Positions        // Texture Coords
        -0.63f, -0.86f,       0.0f, 1.0f,
        -0.63f, -0.90f,       0.0f, 0.0f,
        -0.61f, -0.90f,       1.0f, 0.0f,
        -0.61f, -0.86f,       1.0f, 1.0f
    };
    float coordinates1doty[] = {
        // Positions        // Texture Coords
        -0.60f, -0.89f,       0.0f, 1.0f,
        -0.60f, -0.895f,       0.0f, 0.0f,
        -0.605f, -0.895f,       1.0f, 0.0f,
        -0.605f, -0.89f,       1.0f, 1.0f
    };
    float coordinates1num2y[] = {
        // Positions        // Texture Coords
        -0.59f, -0.86f,       0.0f, 1.0f,
        -0.59f, -0.90f,       0.0f, 0.0f,
        -0.57f, -0.90f,       1.0f, 0.0f,
        -0.57f, -0.86f,       1.0f, 1.0f
    };
    float coordinates1num3y[] = {
        // Positions        // Texture Coords
        -0.57f, -0.86f,       0.0f, 1.0f,
        -0.57f, -0.90f,       0.0f, 0.0f,
        -0.55f, -0.90f,       1.0f, 0.0f,
        -0.55f, -0.86f,       1.0f, 1.0f
    };
    unsigned int indices1y[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices1dot1y[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices1dot2y[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices1minusy[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices1num1y[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices1doty[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices1num2y[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices1num3y[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int coordinatesVao1y[8], coordinatesVbo1y[8], coordinatesEbo1y[8];
    glGenVertexArrays(8, coordinatesVao1y);
    glGenBuffers(8, coordinatesVbo1y);
    glGenBuffers(8, coordinatesEbo1y);
    //drone 1 y
    glBindVertexArray(coordinatesVao1y[0]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo1y[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates1y), coordinates1y, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo1y[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices1y), indices1y, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 1 y dot 1
    glBindVertexArray(coordinatesVao1y[1]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo1y[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates1dot1y), coordinates1dot1y, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo1y[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices1dot1y), indices1dot1y, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 1 y dot 2
    glBindVertexArray(coordinatesVao1y[2]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo1y[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates1dot2y), coordinates1dot2y, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo1y[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices1dot2y), indices1dot2y, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 1 y minus
    glBindVertexArray(coordinatesVao1y[3]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo1y[3]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates1minusy), coordinates1minusy, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo1y[3]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices1minusy), indices1minusy, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 1 y number 1
    glBindVertexArray(coordinatesVao1y[4]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo1y[4]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates1num1y), coordinates1num1y, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo1y[4]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices1num1y), indices1num1y, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 1 y dot
    glBindVertexArray(coordinatesVao1y[5]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo1y[5]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates1doty), coordinates1doty, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo1y[5]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices1doty), indices1doty, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 1 y number 2
    glBindVertexArray(coordinatesVao1y[6]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo1y[6]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates1num2y), coordinates1num2y, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo1y[6]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices1num1y), indices1num1y, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 1 y number 3
    glBindVertexArray(coordinatesVao1y[7]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo1y[7]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates1num3y), coordinates1num3y, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo1y[7]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices1num1y), indices1num1y, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    //coordinates 2 : X
    float coordinates2x[] = {
        // Positions        // Texture Coords
        -0.40f, -0.86f,       0.0f, 1.0f,
        -0.40f, -0.90f,       0.0f, 0.0f,
        -0.375f, -0.90f,       1.0f, 0.0f,
        -0.375f, -0.86f,       1.0f, 1.0f
    };
    float coordinates2dot1[] = {
        // Positions        // Texture Coords
        -0.37f, -0.87f,       0.0f, 1.0f,
        -0.37f, -0.875f,       0.0f, 0.0f,
        -0.375f, -0.875f,       1.0f, 0.0f,
        -0.375f, -0.87f,       1.0f, 1.0f
    };
    float coordinates2dot2[] = {
        // Positions        // Texture Coords
        -0.37f, -0.88f,       0.0f, 1.0f,
        -0.37f, -0.885f,       0.0f, 0.0f,
        -0.375f, -0.885f,       1.0f, 0.0f,
        -0.375f, -0.88f,       1.0f, 1.0f
    };
    float coordinates2minus[] = {
        // Positions        // Texture Coords
        -0.35f, -0.875f,       0.0f, 1.0f,
        -0.35f, -0.88f,       0.0f, 0.0f,
        -0.34f, -0.88f,       1.0f, 0.0f,
        -0.34f, -0.875f,       1.0f, 1.0f
    };
    float coordinates2num1[] = {
        // Positions        // Texture Coords
        -0.33f, -0.86f,       0.0f, 1.0f,
        -0.33f, -0.90f,       0.0f, 0.0f,
        -0.31f, -0.90f,       1.0f, 0.0f,
        -0.31f, -0.86f,       1.0f, 1.0f
    };
    float coordinates2dot[] = {
        // Positions        // Texture Coords
        -0.30f, -0.89f,       0.0f, 1.0f,
        -0.30f, -0.895f,       0.0f, 0.0f,
        -0.305f, -0.895f,       1.0f, 0.0f,
        -0.305f, -0.89f,       1.0f, 1.0f
    };
    float coordinates2num2[] = {
        // Positions        // Texture Coords
        -0.29f, -0.86f,       0.0f, 1.0f,
        -0.29f, -0.90f,       0.0f, 0.0f,
        -0.27f, -0.90f,       1.0f, 0.0f,
        -0.27f, -0.86f,       1.0f, 1.0f
    };
    float coordinates2num3[] = {
        // Positions        // Texture Coords
        -0.27f, -0.86f,       0.0f, 1.0f,
        -0.27f, -0.90f,       0.0f, 0.0f,
        -0.25f, -0.90f,       1.0f, 0.0f,
        -0.25f, -0.86f,       1.0f, 1.0f
    };
    unsigned int indices2x[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices2dot1[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices2dot2[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices2minus[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices2num1[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices2dot[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices2num2[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices2num3[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int coordinatesVao2[8], coordinatesVbo2[8], coordinatesEbo2[8];
    glGenVertexArrays(8, coordinatesVao2);
    glGenBuffers(8, coordinatesVbo2);
    glGenBuffers(8, coordinatesEbo2);
    //drone 2 x
    glBindVertexArray(coordinatesVao2[0]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo2[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates2x), coordinates2x, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo2[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices2x), indices2x, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 2 x dot 1
    glBindVertexArray(coordinatesVao2[1]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo2[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates2dot1), coordinates2dot1, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo2[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices2dot1), indices2dot1, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 2 x dot 2
    glBindVertexArray(coordinatesVao2[2]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo2[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates2dot2), coordinates2dot2, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo2[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices2dot2), indices2dot2, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 2 x minus
    glBindVertexArray(coordinatesVao2[3]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo2[3]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates2minus), coordinates2minus, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo2[3]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices2minus), indices2minus, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 2 x number 1
    glBindVertexArray(coordinatesVao2[4]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo2[4]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates2num1), coordinates2num1, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo2[4]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices2num1), indices2num1, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 2 x dot
    glBindVertexArray(coordinatesVao2[5]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo2[5]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates2dot), coordinates2dot, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo2[5]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices2dot), indices2dot, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 2 x number 2
    glBindVertexArray(coordinatesVao2[6]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo2[6]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates2num2), coordinates2num2, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo2[6]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices2num1), indices2num1, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 2 x number 3
    glBindVertexArray(coordinatesVao2[7]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo2[7]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates2num3), coordinates2num3, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo2[7]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices2num1), indices2num1, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


    //coordinates 2 : Y
    float coordinates2y[] = {
        // Positions        // Texture Coords
        -0.20f, -0.86f,       0.0f, 1.0f,
        -0.20f, -0.90f,       0.0f, 0.0f,
        -0.175f, -0.90f,       1.0f, 0.0f,
        -0.175f, -0.86f,       1.0f, 1.0f
    };
    float coordinates2dot1y[] = {
        // Positions        // Texture Coords
        -0.17f, -0.87f,       0.0f, 1.0f,
        -0.17f, -0.875f,       0.0f, 0.0f,
        -0.175f, -0.875f,       1.0f, 0.0f,
        -0.175f, -0.87f,       1.0f, 1.0f
    };
    float coordinates2dot2y[] = {
        // Positions        // Texture Coords
        -0.17f, -0.88f,       0.0f, 1.0f,
        -0.17f, -0.885f,       0.0f, 0.0f,
        -0.175f, -0.885f,       1.0f, 0.0f,
        -0.175f, -0.88f,       1.0f, 1.0f
    };
    float coordinates2minusy[] = {
        // Positions        // Texture Coords
        -0.15f, -0.875f,       0.0f, 1.0f,
        -0.15f, -0.88f,       0.0f, 0.0f,
        -0.14f, -0.88f,       1.0f, 0.0f,
        -0.14f, -0.875f,       1.0f, 1.0f
    };
    float coordinates2num1y[] = {
        // Positions        // Texture Coords
        -0.13f, -0.86f,       0.0f, 1.0f,
        -0.13f, -0.90f,       0.0f, 0.0f,
        -0.11f, -0.90f,       1.0f, 0.0f,
        -0.11f, -0.86f,       1.0f, 1.0f
    };
    float coordinates2doty[] = {
        // Positions        // Texture Coords
        -0.10f, -0.89f,       0.0f, 1.0f,
        -0.10f, -0.895f,       0.0f, 0.0f,
        -0.105f, -0.895f,       1.0f, 0.0f,
        -0.105f, -0.89f,       1.0f, 1.0f
    };
    float coordinates2num2y[] = {
        // Positions        // Texture Coords
        -0.09f, -0.86f,       0.0f, 1.0f,
        -0.09f, -0.90f,       0.0f, 0.0f,
        -0.07f, -0.90f,       1.0f, 0.0f,
        -0.07f, -0.86f,       1.0f, 1.0f
    };
    float coordinates2num3y[] = {
        // Positions        // Texture Coords
        -0.07f, -0.86f,       0.0f, 1.0f,
        -0.07f, -0.90f,       0.0f, 0.0f,
        -0.05f, -0.90f,       1.0f, 0.0f,
        -0.05f, -0.86f,       1.0f, 1.0f
    };
    unsigned int indices2y[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices2dot1y[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices2dot2y[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices2minusy[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices2num1y[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices2doty[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices2num2y[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indices2num3y[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int coordinatesVao2y[8], coordinatesVbo2y[8], coordinatesEbo2y[8];
    glGenVertexArrays(8, coordinatesVao2y);
    glGenBuffers(8, coordinatesVbo2y);
    glGenBuffers(8, coordinatesEbo2y);
    //drone 2 y
    glBindVertexArray(coordinatesVao2y[0]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo2y[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates2y), coordinates2y, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo2y[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices2y), indices2y, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 2 y dot 1
    glBindVertexArray(coordinatesVao2y[1]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo2y[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates2dot1y), coordinates2dot1y, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo2y[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices2dot1y), indices2dot1y, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 2 y dot 2
    glBindVertexArray(coordinatesVao2y[2]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo2y[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates2dot2y), coordinates2dot2y, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo2y[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices2dot2y), indices2dot2y, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 2 y minus
    glBindVertexArray(coordinatesVao2y[3]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo2y[3]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates2minusy), coordinates2minusy, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo2y[3]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices2minusy), indices2minusy, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 2 y number 1
    glBindVertexArray(coordinatesVao2y[4]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo2y[4]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates2num1y), coordinates2num1y, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo2y[4]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices2num1y), indices2num1y, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 2 y dot
    glBindVertexArray(coordinatesVao2y[5]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo2y[5]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates2doty), coordinates2doty, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo2y[5]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices2doty), indices2doty, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 2 y number 2
    glBindVertexArray(coordinatesVao2y[6]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo2y[6]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates2num2y), coordinates2num2y, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo2y[6]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices2num1y), indices2num1y, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //drone 2 y number 3
    glBindVertexArray(coordinatesVao2y[7]);
    glBindBuffer(GL_ARRAY_BUFFER, coordinatesVbo2y[7]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates2num3y), coordinates2num3y, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinatesEbo2y[7]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices2num1y), indices2num1y, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // IME PREZIME I INDEKS
    float name1[] = {
        // Positions        // Texture Coords
        0.60f, -0.92f,       0.0f, 1.0f,
        0.60f, -0.96f,       0.0f, 0.0f,
        0.62f, -0.96f,       1.0f, 0.0f,
        0.62f, -0.92f,       1.0f, 1.0f
    };
    float name2[] = {
        // Positions        // Texture Coords
        0.62f, -0.92f,       0.0f, 1.0f,
        0.62f, -0.96f,       0.0f, 0.0f,
        0.63f, -0.96f,       1.0f, 0.0f,
        0.63f, -0.92f,       1.0f, 1.0f
    };
    float name3[] = {
        // Positions        // Texture Coords
        0.63f, -0.92f,       0.0f, 1.0f,
        0.63f, -0.96f,       0.0f, 0.0f,
        0.65f, -0.96f,       1.0f, 0.0f,
        0.65f, -0.92f,       1.0f, 1.0f
    };
    float name4[] = {
        // Positions        // Texture Coords
        0.65f, -0.92f,       0.0f, 1.0f,
        0.65f, -0.96f,       0.0f, 0.0f,
        0.67f, -0.96f,       1.0f, 0.0f,
        0.67f, -0.92f,       1.0f, 1.0f
    };
    float surname1[] = {
        // Positions        // Texture Coords
        0.69f, -0.92f,       0.0f, 1.0f,
        0.69f, -0.96f,       0.0f, 0.0f,
        0.71f, -0.96f,       1.0f, 0.0f,
        0.71f, -0.92f,       1.0f, 1.0f
    };
    float surname2[] = {
        // Positions        // Texture Coords
        0.71f, -0.92f,       0.0f, 1.0f,
        0.71f, -0.96f,       0.0f, 0.0f,
        0.72f, -0.96f,       1.0f, 0.0f,
        0.72f, -0.92f,       1.0f, 1.0f
    };
    float surname3[] = {
        // Positions        // Texture Coords
        0.72f, -0.92f,       0.0f, 1.0f,
        0.72f, -0.96f,       0.0f, 0.0f,
        0.74f, -0.96f,       1.0f, 0.0f,
        0.74f, -0.92f,       1.0f, 1.0f
    };
    float surname4[] = {
        // Positions        // Texture Coords
        0.74f, -0.92f,       0.0f, 1.0f,
        0.74f, -0.96f,       0.0f, 0.0f,
        0.76f, -0.96f,       1.0f, 0.0f,
        0.76f, -0.92f,       1.0f, 1.0f
    };
    float surname5[] = {
        // Positions        // Texture Coords
        0.76f, -0.92f,       0.0f, 1.0f,
        0.76f, -0.96f,       0.0f, 0.0f,
        0.78f, -0.96f,       1.0f, 0.0f,
        0.78f, -0.92f,       1.0f, 1.0f
    };
    float surname6[] = {
        // Positions        // Texture Coords
        0.78f, -0.92f,       0.0f, 1.0f,
        0.78f, -0.96f,       0.0f, 0.0f,
        0.79f, -0.96f,       1.0f, 0.0f,
        0.79f, -0.92f,       1.0f, 1.0f
    };
    float surname7[] = {
        // Positions        // Texture Coords
        0.79f, -0.91f,       0.0f, 1.0f,
        0.79f, -0.96f,       0.0f, 0.0f,
        0.81f, -0.96f,       1.0f, 0.0f,
        0.81f, -0.91f,       1.0f, 1.0f
    };
    float indeks1[] = {
        // Positions        // Texture Coords
        0.83f, -0.92f,       0.0f, 1.0f,
        0.83f, -0.96f,       0.0f, 0.0f,
        0.85f, -0.96f,       1.0f, 0.0f,
        0.85f, -0.92f,       1.0f, 1.0f
    };
    float indeks2[] = {
        // Positions        // Texture Coords
        0.85f, -0.92f,       0.0f, 1.0f,
        0.85f, -0.96f,       0.0f, 0.0f,
        0.87f, -0.96f,       1.0f, 0.0f,
        0.87f, -0.92f,       1.0f, 1.0f
    };
    float indeks3[] = {
        // Positions        // Texture Coords
        0.87f, -0.92f,       0.0f, 1.0f,
        0.87f, -0.96f,       0.0f, 0.0f,
        0.89f, -0.96f,       1.0f, 0.0f,
        0.89f, -0.92f,       1.0f, 1.0f
    };
    float indeks4[] = {
        // Positions        // Texture Coords
        0.89f, -0.92f,       0.0f, 1.0f,
        0.89f, -0.96f,       0.0f, 0.0f,
        0.91f, -0.96f,       1.0f, 0.0f,
        0.91f, -0.92f,       1.0f, 1.0f
    };
    float indeks5[] = {
        // Positions        // Texture Coords
        0.91f, -0.94f,       0.0f, 1.0f,
        0.91f, -0.945f,       0.0f, 0.0f,
        0.92f, -0.945f,       1.0f, 0.0f,
        0.92f, -0.94f,       1.0f, 1.0f
    };
    float indeks6[] = {
        // Positions        // Texture Coords
        0.92f, -0.92f,       0.0f, 1.0f,
        0.92f, -0.96f,       0.0f, 0.0f,
        0.94f, -0.96f,       1.0f, 0.0f,
        0.94f, -0.92f,       1.0f, 1.0f
    };
    float indeks7[] = {
        // Positions        // Texture Coords
        0.94f, -0.92f,       0.0f, 1.0f,
        0.94f, -0.96f,       0.0f, 0.0f,
        0.96f, -0.96f,       1.0f, 0.0f,
        0.96f, -0.92f,       1.0f, 1.0f
    };
    float indeks8[] = {
        // Positions        // Texture Coords
        0.96f, -0.92f,       0.0f, 1.0f,
        0.96f, -0.96f,       0.0f, 0.0f,
        0.98f, -0.96f,       1.0f, 0.0f,
        0.98f, -0.92f,       1.0f, 1.0f
    };
    float indeks9[] = {
        // Positions        // Texture Coords
        0.98f, -0.92f,       0.0f, 1.0f,
        0.98f, -0.96f,       0.0f, 0.0f,
        1.00f, -0.96f,       1.0f, 0.0f,
        1.00f, -0.92f,       1.0f, 1.0f
    };
    unsigned int nameIndices1[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int nameIndices2[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int nameIndices3[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int nameIndices4[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int surnameIndices1[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int surnameIndices2[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int surnameIndices3[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int surnameIndices4[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int surnameIndices5[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int surnameIndices6[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int surnameIndices7[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indexIndices1[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indexIndices2[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indexIndices3[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indexIndices4[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indexIndices5[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indexIndices6[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indexIndices7[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indexIndices8[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int indexIndices9[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int nameVao[11], nameVbo[11], nameEbo[11];
    glGenVertexArrays(11, nameVao);
    glGenBuffers(11, nameVbo);
    glGenBuffers(11, nameEbo);
    unsigned int indexVao[9], indexVbo[9], indexEbo[9];
    glGenVertexArrays(9, indexVao);
    glGenBuffers(9, indexVbo);
    glGenBuffers(9, indexEbo);
    //M
    glBindVertexArray(nameVao[0]);
    glBindBuffer(GL_ARRAY_BUFFER, nameVbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(name1), name1, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, nameEbo[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(nameIndices1), nameIndices1, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //I
    glBindVertexArray(nameVao[1]);
    glBindBuffer(GL_ARRAY_BUFFER, nameVbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(name2), name2, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, nameEbo[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(nameIndices2), nameIndices2, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //L
    glBindVertexArray(nameVao[2]);
    glBindBuffer(GL_ARRAY_BUFFER, nameVbo[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(name3), name3, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, nameEbo[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(nameIndices3), nameIndices3, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //A
    glBindVertexArray(nameVao[3]);
    glBindBuffer(GL_ARRAY_BUFFER, nameVbo[3]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(name4), name4, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, nameEbo[3]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(nameIndices4), nameIndices4, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //M
    glBindVertexArray(nameVao[4]);
    glBindBuffer(GL_ARRAY_BUFFER, nameVbo[4]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(surname1), surname1, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, nameEbo[4]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(surnameIndices1), surnameIndices1, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //I
    glBindVertexArray(nameVao[5]);
    glBindBuffer(GL_ARRAY_BUFFER, nameVbo[5]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(surname2), surname2, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, nameEbo[5]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(surnameIndices2), surnameIndices2, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //L
    glBindVertexArray(nameVao[6]);
    glBindBuffer(GL_ARRAY_BUFFER, nameVbo[6]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(surname3), surname3, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, nameEbo[6]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(surnameIndices3), surnameIndices3, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //O
    glBindVertexArray(nameVao[7]);
    glBindBuffer(GL_ARRAY_BUFFER, nameVbo[7]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(surname4), surname4, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, nameEbo[7]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(surnameIndices4), surnameIndices4, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //V
    glBindVertexArray(nameVao[8]);
    glBindBuffer(GL_ARRAY_BUFFER, nameVbo[8]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(surname5), surname5, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, nameEbo[8]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(surnameIndices5), surnameIndices5, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //I
    glBindVertexArray(nameVao[9]);
    glBindBuffer(GL_ARRAY_BUFFER, nameVbo[9]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(surname6), surname6, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, nameEbo[9]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(surnameIndices6), surnameIndices6, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //C
    glBindVertexArray(nameVao[10]);
    glBindBuffer(GL_ARRAY_BUFFER, nameVbo[10]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(surname7), surname7, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, nameEbo[10]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(surnameIndices7), surnameIndices7, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //S
    glBindVertexArray(indexVao[0]);
    glBindBuffer(GL_ARRAY_BUFFER, indexVbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(indeks1), indeks1, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexEbo[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexIndices1), indexIndices1, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //V
    glBindVertexArray(indexVao[1]);
    glBindBuffer(GL_ARRAY_BUFFER, indexVbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(indeks2), indeks2, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexEbo[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexIndices2), indexIndices2, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //2
    glBindVertexArray(indexVao[2]);
    glBindBuffer(GL_ARRAY_BUFFER, indexVbo[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(indeks3), indeks3, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexEbo[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexIndices3), indexIndices3, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //2
    glBindVertexArray(indexVao[3]);
    glBindBuffer(GL_ARRAY_BUFFER, indexVbo[3]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(indeks4), indeks4, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexEbo[3]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexIndices4), indexIndices4, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //-
    glBindVertexArray(indexVao[4]);
    glBindBuffer(GL_ARRAY_BUFFER, indexVbo[4]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(indeks5), indeks5, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexEbo[4]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexIndices5), indexIndices5, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //2
    glBindVertexArray(indexVao[5]);
    glBindBuffer(GL_ARRAY_BUFFER, indexVbo[5]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(indeks6), indeks6, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexEbo[5]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexIndices6), indexIndices6, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //0
    glBindVertexArray(indexVao[6]);
    glBindBuffer(GL_ARRAY_BUFFER, indexVbo[6]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(indeks7), indeks7, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexEbo[6]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexIndices7), indexIndices7, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //2
    glBindVertexArray(indexVao[7]);
    glBindBuffer(GL_ARRAY_BUFFER, indexVbo[7]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(indeks8), indeks8, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexEbo[7]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexIndices8), indexIndices8, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //1
    glBindVertexArray(indexVao[8]);
    glBindBuffer(GL_ARRAY_BUFFER, indexVbo[8]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(indeks9), indeks9, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexEbo[8]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexIndices9), indexIndices9, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    //destroyed drone 1
    float destroyed11[] = {
        // Positions        // Texture Coords
        -0.90f, -0.94f,       0.0f, 1.0f,
        -0.90f, -0.98f,       0.0f, 0.0f,
        -0.88f, -0.98f,       1.0f, 0.0f,
        -0.88f, -0.94f,       1.0f, 1.0f
    };
    float destroyed12[] = {
        // Positions        // Texture Coords
        -0.88f, -0.94f,       0.0f, 1.0f,
        -0.88f, -0.98f,       0.0f, 0.0f,
        -0.86f, -0.98f,       1.0f, 0.0f,
        -0.86f, -0.94f,       1.0f, 1.0f
    };
    float destroyed13[] = {
        // Positions        // Texture Coords
        -0.86f, -0.94f,       0.0f, 1.0f,
        -0.86f, -0.98f,       0.0f, 0.0f,
        -0.84f, -0.98f,       1.0f, 0.0f,
        -0.84f, -0.94f,       1.0f, 1.0f
    };
    float destroyed14[] = {
        // Positions        // Texture Coords
        -0.84f, -0.94f,       0.0f, 1.0f,
        -0.84f, -0.98f,       0.0f, 0.0f,
        -0.82f, -0.98f,       1.0f, 0.0f,
        -0.82f, -0.94f,       1.0f, 1.0f
    };
    float destroyed15[] = {
        // Positions        // Texture Coords
        -0.82f, -0.94f,       0.0f, 1.0f,
        -0.82f, -0.98f,       0.0f, 0.0f,
        -0.80f, -0.98f,       1.0f, 0.0f,
        -0.80f, -0.94f,       1.0f, 1.0f
    };
    float destroyed16[] = {
        // Positions        // Texture Coords
        -0.80f, -0.94f,       0.0f, 1.0f,
        -0.80f, -0.98f,       0.0f, 0.0f,
        -0.78f, -0.98f,       1.0f, 0.0f,
        -0.78f, -0.94f,       1.0f, 1.0f
    };
    float destroyed17[] = {
        // Positions        // Texture Coords
        -0.78f, -0.94f,       0.0f, 1.0f,
        -0.78f, -0.98f,       0.0f, 0.0f,
        -0.76f, -0.98f,       1.0f, 0.0f,
        -0.76f, -0.94f,       1.0f, 1.0f
    };
    float destroyed18[] = {
        // Positions        // Texture Coords
        -0.76f, -0.94f,       0.0f, 1.0f,
        -0.76f, -0.98f,       0.0f, 0.0f,
        -0.74f, -0.98f,       1.0f, 0.0f,
        -0.74f, -0.94f,       1.0f, 1.0f
    };
    float destroyed19[] = {
        // Positions        // Texture Coords
        -0.74f, -0.94f,       0.0f, 1.0f,
        -0.74f, -0.98f,       0.0f, 0.0f,
        -0.72f, -0.98f,       1.0f, 0.0f,
        -0.72f, -0.94f,       1.0f, 1.0f
    };
    unsigned int destroyedindices11[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int destroyedindices12[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int destroyedindices13[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int destroyedindices14[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int destroyedindices15[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int destroyedindices16[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int destroyedindices17[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int destroyedindices18[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int destroyedindices19[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int destroyedVao1[9], destroyedVbo1[9], destroyedEbo1[9];
    glGenVertexArrays(9, destroyedVao1);
    glGenBuffers(9, destroyedVbo1);
    glGenBuffers(9, destroyedEbo1);
    //D
    glBindVertexArray(destroyedVao1[0]);
    glBindBuffer(GL_ARRAY_BUFFER, destroyedVbo1[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(destroyed11), destroyed11, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, destroyedEbo1[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(destroyedindices11), destroyedindices11, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //E
    glBindVertexArray(destroyedVao1[1]);
    glBindBuffer(GL_ARRAY_BUFFER, destroyedVbo1[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(destroyed12), destroyed12, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, destroyedEbo1[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(destroyedindices12), destroyedindices12, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //S
    glBindVertexArray(destroyedVao1[2]);
    glBindBuffer(GL_ARRAY_BUFFER, destroyedVbo1[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(destroyed13), destroyed13, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, destroyedEbo1[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(destroyedindices13), destroyedindices13, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //T
    glBindVertexArray(destroyedVao1[3]);
    glBindBuffer(GL_ARRAY_BUFFER, destroyedVbo1[3]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(destroyed14), destroyed14, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, destroyedEbo1[3]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(destroyedindices14), destroyedindices14, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //R
    glBindVertexArray(destroyedVao1[4]);
    glBindBuffer(GL_ARRAY_BUFFER, destroyedVbo1[4]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(destroyed15), destroyed15, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, destroyedEbo1[4]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(destroyedindices15), destroyedindices15, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //O
    glBindVertexArray(destroyedVao1[5]);
    glBindBuffer(GL_ARRAY_BUFFER, destroyedVbo1[5]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(destroyed16), destroyed16, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, destroyedEbo1[5]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(destroyedindices16), destroyedindices16, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //Y
    glBindVertexArray(destroyedVao1[6]);
    glBindBuffer(GL_ARRAY_BUFFER, destroyedVbo1[6]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(destroyed17), destroyed17, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, destroyedEbo1[6]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(destroyedindices17), destroyedindices17, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //E
    glBindVertexArray(destroyedVao1[7]);
    glBindBuffer(GL_ARRAY_BUFFER, destroyedVbo1[7]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(destroyed18), destroyed18, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, destroyedEbo1[7]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(destroyedindices18), destroyedindices18, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //D
    glBindVertexArray(destroyedVao1[8]);
    glBindBuffer(GL_ARRAY_BUFFER, destroyedVbo1[8]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(destroyed11), destroyed19, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, destroyedEbo1[8]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(destroyedindices19), destroyedindices19, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


    //destroyed drone 1
    float destroyed21[] = {
        // Positions        // Texture Coords
        -0.40f, -0.94f,       0.0f, 1.0f,
        -0.40f, -0.98f,       0.0f, 0.0f,
        -0.38f, -0.98f,       1.0f, 0.0f,
        -0.38f, -0.94f,       1.0f, 1.0f
    };
    float destroyed22[] = {
        // Positions        // Texture Coords
        -0.38f, -0.94f,       0.0f, 1.0f,
        -0.38f, -0.98f,       0.0f, 0.0f,
        -0.36f, -0.98f,       1.0f, 0.0f,
        -0.36f, -0.94f,       1.0f, 1.0f
    };
    float destroyed23[] = {
        // Positions        // Texture Coords
        -0.36f, -0.94f,       0.0f, 1.0f,
        -0.36f, -0.98f,       0.0f, 0.0f,
        -0.34f, -0.98f,       1.0f, 0.0f,
        -0.34f, -0.94f,       1.0f, 1.0f
    };
    float destroyed24[] = {
        // Positions        // Texture Coords
        -0.34f, -0.94f,       0.0f, 1.0f,
        -0.34f, -0.98f,       0.0f, 0.0f,
        -0.32f, -0.98f,       1.0f, 0.0f,
        -0.32f, -0.94f,       1.0f, 1.0f
    };
    float destroyed25[] = {
        // Positions        // Texture Coords
        -0.32f, -0.94f,       0.0f, 1.0f,
        -0.32f, -0.98f,       0.0f, 0.0f,
        -0.30f, -0.98f,       1.0f, 0.0f,
        -0.30f, -0.94f,       1.0f, 1.0f
    };
    float destroyed26[] = {
        // Positions        // Texture Coords
        -0.30f, -0.94f,       0.0f, 1.0f,
        -0.30f, -0.98f,       0.0f, 0.0f,
        -0.28f, -0.98f,       1.0f, 0.0f,
        -0.28f, -0.94f,       1.0f, 1.0f
    };
    float destroyed27[] = {
        // Positions        // Texture Coords
        -0.28f, -0.94f,       0.0f, 1.0f,
        -0.28f, -0.98f,       0.0f, 0.0f,
        -0.26f, -0.98f,       1.0f, 0.0f,
        -0.26f, -0.94f,       1.0f, 1.0f
    };
    float destroyed28[] = {
        // Positions        // Texture Coords
        -0.26f, -0.94f,       0.0f, 1.0f,
        -0.26f, -0.98f,       0.0f, 0.0f,
        -0.24f, -0.98f,       1.0f, 0.0f,
        -0.24f, -0.94f,       1.0f, 1.0f
    };
    float destroyed29[] = {
        // Positions        // Texture Coords
        -0.24f, -0.94f,       0.0f, 1.0f,
        -0.24f, -0.98f,       0.0f, 0.0f,
        -0.22f, -0.98f,       1.0f, 0.0f,
        -0.22f, -0.94f,       1.0f, 1.0f
    };
    unsigned int destroyedindices21[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int destroyedindices22[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int destroyedindices23[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int destroyedindices24[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int destroyedindices25[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int destroyedindices26[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int destroyedindices27[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int destroyedindices28[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int destroyedindices29[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int destroyedVao2[9], destroyedVbo2[9], destroyedEbo2[9];
    glGenVertexArrays(9, destroyedVao2);
    glGenBuffers(9, destroyedVbo2);
    glGenBuffers(9, destroyedEbo2);
    //D
    glBindVertexArray(destroyedVao2[0]);
    glBindBuffer(GL_ARRAY_BUFFER, destroyedVbo2[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(destroyed21), destroyed21, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, destroyedEbo2[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(destroyedindices21), destroyedindices21, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //E
    glBindVertexArray(destroyedVao2[1]);
    glBindBuffer(GL_ARRAY_BUFFER, destroyedVbo2[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(destroyed22), destroyed22, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, destroyedEbo2[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(destroyedindices22), destroyedindices22, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //S
    glBindVertexArray(destroyedVao2[2]);
    glBindBuffer(GL_ARRAY_BUFFER, destroyedVbo2[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(destroyed23), destroyed23, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, destroyedEbo2[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(destroyedindices23), destroyedindices23, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //T
    glBindVertexArray(destroyedVao2[3]);
    glBindBuffer(GL_ARRAY_BUFFER, destroyedVbo2[3]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(destroyed24), destroyed24, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, destroyedEbo2[3]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(destroyedindices24), destroyedindices24, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //R
    glBindVertexArray(destroyedVao2[4]);
    glBindBuffer(GL_ARRAY_BUFFER, destroyedVbo2[4]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(destroyed25), destroyed25, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, destroyedEbo2[4]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(destroyedindices25), destroyedindices25, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //O
    glBindVertexArray(destroyedVao2[5]);
    glBindBuffer(GL_ARRAY_BUFFER, destroyedVbo2[5]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(destroyed26), destroyed26, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, destroyedEbo2[5]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(destroyedindices26), destroyedindices26, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //Y
    glBindVertexArray(destroyedVao2[6]);
    glBindBuffer(GL_ARRAY_BUFFER, destroyedVbo2[6]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(destroyed27), destroyed27, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, destroyedEbo2[6]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(destroyedindices27), destroyedindices27, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //E
    glBindVertexArray(destroyedVao2[7]);
    glBindBuffer(GL_ARRAY_BUFFER, destroyedVbo2[7]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(destroyed28), destroyed28, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, destroyedEbo2[7]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(destroyedindices28), destroyedindices28, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //D
    glBindVertexArray(destroyedVao2[8]);
    glBindBuffer(GL_ARRAY_BUFFER, destroyedVbo2[8]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(destroyed21), destroyed29, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, destroyedEbo2[8]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(destroyedindices29), destroyedindices29, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    //led
    float ledVertices[] =
    {
        -0.93, -0.78,    0.184f, 0.22f, 0.196f, 1.0,
        -0.43, -0.78,    0.184f, 0.22f, 0.196f, 1.0
    };
    stride = 6 * sizeof(float);
    unsigned int ledVAO;
    glGenVertexArrays(1, &ledVAO);
    glBindVertexArray(ledVAO);
    unsigned int ledVBO;
    glGenBuffers(1, &ledVBO);
    glBindBuffer(GL_ARRAY_BUFFER, ledVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ledVertices), ledVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);




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
    // x texture
    unsigned texturex = loadImageToTexture("res/x.png");
    glBindTexture(GL_TEXTURE_2D, texturex);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    // y texture
    unsigned texturey = loadImageToTexture("res/y.png");
    glBindTexture(GL_TEXTURE_2D, texturey);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    // dot texture
    unsigned textureDot = loadImageToTexture("res/dot.png");
    glBindTexture(GL_TEXTURE_2D, textureDot);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    // minus texture
    unsigned textureMinus = loadImageToTexture("res/minus.png");
    glBindTexture(GL_TEXTURE_2D, textureMinus);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    // m texture
    unsigned texturem = loadImageToTexture("res/m.png");
    glBindTexture(GL_TEXTURE_2D, texturem);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    // i texture
    unsigned texturei = loadImageToTexture("res/i.png");
    glBindTexture(GL_TEXTURE_2D, texturei);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    // l texture
    unsigned texturel = loadImageToTexture("res/l.png");
    glBindTexture(GL_TEXTURE_2D, texturel);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    // a texture
    unsigned texturea = loadImageToTexture("res/a.png");
    glBindTexture(GL_TEXTURE_2D, texturea);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    // o texture
    unsigned textureo = loadImageToTexture("res/o.png");
    glBindTexture(GL_TEXTURE_2D, textureo);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    // v texture
    unsigned texturev = loadImageToTexture("res/v.png");
    glBindTexture(GL_TEXTURE_2D, texturev);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    // c texture
    unsigned texturec = loadImageToTexture("res/c.png");
    glBindTexture(GL_TEXTURE_2D, texturec);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    // s texture
    unsigned textures = loadImageToTexture("res/s.png");
    glBindTexture(GL_TEXTURE_2D, textures);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    // d texture
    unsigned textured = loadImageToTexture("res/d.png");
    glBindTexture(GL_TEXTURE_2D, textured);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    // e texture
    unsigned texturee = loadImageToTexture("res/e.png");
    glBindTexture(GL_TEXTURE_2D, texturee);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    // t texture
    unsigned texturet = loadImageToTexture("res/t.png");
    glBindTexture(GL_TEXTURE_2D, texturet);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(textureShader);
    glUniform1i(uTexLoc, 0);
    glUseProgram(0);
    // r texture
    unsigned texturer = loadImageToTexture("res/r.png");
    glBindTexture(GL_TEXTURE_2D, texturer);
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
    const float fixedTimeStep = 0.016f;
    float accumulator = 0.0f;


    while (!glfwWindowShouldClose(window)) {

        float currentTime = glfwGetTime();
        float deltaTime = currentTime - timeSinceLastUpdate;
        timeSinceLastUpdate = currentTime;

        accumulator += deltaTime;

        while (accumulator >= fixedTimeStep) {

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
            if (drone1.active && !drone1.destroyed) {
                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                    drone1.y += 0.005;
                    if (drone1.y > 1) drone1.destroyed = true;
                }
                if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                    drone1.y -= 0.005;
                    if (drone1.y < -0.65) drone1.destroyed = true;
                }
                if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                    drone1.x -= 0.005;
                    if (drone1.x < -1) drone1.destroyed = true;
                }
                if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                    drone1.x += 0.005;
                    if (drone1.x > 1) drone1.destroyed = true;
                }
            }
            if (drone2.active && !drone2.destroyed) {
                if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
                    drone2.y += 0.005;
                    if (drone2.y > 1) drone2.destroyed = true;
                }
                if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
                    drone2.y -= 0.005;
                    if (drone2.y < -0.65) drone2.destroyed = true;
                }
                if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
                    drone2.x -= 0.005;
                    if (drone2.x < -1) drone2.destroyed = true;
                }
                if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
                    drone2.x += 0.005;
                    if (drone2.x > 1) drone2.destroyed = true;
                }
            }

            timeSinceLastUpdate = currentTime;

            if (drone1.active && drone1.batteryLevel > 0) {
                drone1.batteryLevel -= 0.1f;
            }
            if (drone2.active && drone2.batteryLevel > 0) {
                drone2.batteryLevel -= 0.1f;
            }

            if (drone1.destroyed) {
                drone1.batteryLevel = 0.0f;
            }
            if (drone2.destroyed) {
                drone2.batteryLevel = 0.0f;
            }

            if (drone1.batteryLevel <= 0.0f) {
                drone1.destroyed = true;
            }
            if (drone2.batteryLevel <= 0.0f) {
                drone2.destroyed = true;
            }

            //2 drones
            if (areClashing(drone1.x, drone1.y, drone1.radius, drone2.x, drone2.y, drone2.radius)) {
                drone1.destroyed = true;
                drone2.destroyed = true;
            }

            // drone 1 and no fly zone
            if (areClashing(drone1.x, drone1.y, drone1.radius, noFlyZone.x, noFlyZone.y, noFlyZone.radius)) {
                drone1.destroyed = true;
            }

            // drone 2 and no fly zone
            if (areClashing(drone2.x, drone2.y, drone2.radius, noFlyZone.x, noFlyZone.y, noFlyZone.radius)) {
                drone2.destroyed = true;
            }


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
                    noFlyZone.radius += 0.001f;
                }
            }

            if ((!drone1.active || drone1.destroyed) && (!drone2.active || drone2.destroyed)) {

                float newLedVertices[] =
                {
                    -0.93, -0.78,    0.184f, 0.22f, 0.196f, 1.0,
                    -0.43, -0.78,    0.184f, 0.22f, 0.196f, 1.0
                };
                memcpy(ledVertices, newLedVertices, sizeof(newLedVertices));
            }
            else if ((!drone1.active || drone1.destroyed) && drone2.active) {

                float newLedVertices[] =
                {
                    -0.93, -0.78,    0.184f, 0.22f, 0.196f, 1.0,
                    -0.43, -0.78,    0.329f, 0.612f, 0.404f, 1.0
                };
                memcpy(ledVertices, newLedVertices, sizeof(newLedVertices));
            }
            else if (drone1.active && (!drone2.active || drone2.destroyed)) {

                float newLedVertices[] =
                {
                    -0.93, -0.78,    0.329f, 0.612f, 0.404f, 1.0,
                    -0.43, -0.78,    0.184f, 0.22f, 0.196f, 1.0
                };
                memcpy(ledVertices, newLedVertices, sizeof(newLedVertices));
            }
            else {

                float newLedVertices[] =
                {
                    -0.93, -0.78,    0.329f, 0.612f, 0.404f, 1.0,
                    -0.43, -0.78,    0.329f, 0.612f, 0.404f, 1.0
                };
                memcpy(ledVertices, newLedVertices, sizeof(newLedVertices));
            }

            if (!drone1.destroyed) {
                initializeDroneVertices(drone1, droneVertices1, 0.75f);
            }

            //drone 2
            if (!drone2.destroyed) {
                initializeDroneVertices(drone2, droneVertices2, 0.75f);
            }


            initializeProgressVertices(-0.9f, -0.74f, 0.4f, 0.08f, drone1.batteryLevel / 100.0f, 0.329f, 0.612f, 0.404f, progressVertices1);
            initializeProgressVertices(-0.4f, -0.74f, 0.4f, 0.08f, drone2.batteryLevel / 100.0f, 0.329f, 0.612f, 0.404f, progressVertices2);


            accumulator -= fixedTimeStep;
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
        initializeNoFlyZoneVertices(noFlyZoneVertices, 0.75f);
        glBindBuffer(GL_ARRAY_BUFFER, noFlyZoneVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(noFlyZoneVertices), noFlyZoneVertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(noFlyZoneVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 31);

        //drone 1
        if (!drone1.destroyed) {
            glBindBuffer(GL_ARRAY_BUFFER, droneVBO1);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(droneVertices1), droneVertices1);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(droneVAO1);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 31);
        }

        //drone 2
        if (!drone2.destroyed) {
            glBindBuffer(GL_ARRAY_BUFFER, droneVBO2);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(droneVertices2), droneVertices2);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(droneVAO2);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 31);
        }
        glUseProgram(basicShader);
        glBindVertexArray(ledVAO);
        glBindBuffer(GL_ARRAY_BUFFER, ledVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(ledVertices), ledVertices);  // Update buffer data
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(ledVAO);
        glPointSize(8);
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        glDrawArrays(GL_POINTS, 0, 2);
        glBindVertexArray(0);
        glUseProgram(0);


        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        //progress bar 1
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

        //coordinates
        // x for drone 1
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texturex);
        glBindVertexArray(coordinatesVao1[0]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        // : for x for drone 1
        glBindTexture(GL_TEXTURE_2D, textureDot);
        glBindVertexArray(coordinatesVao1[1]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, textureDot);
        glBindVertexArray(coordinatesVao1[2]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        if (drone1.x < 0 || drone1.destroyed) {
            // - for x for drone 1
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureMinus);
            glBindVertexArray(coordinatesVao1[3]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        if (!drone1.destroyed) {
            // num1 for x for drone 1
            int num1 = 0;
            if (drone1.x <= -1 || drone1.x >= 1) num1 = 1;
            switch (num1) {
            case 0: {
                glBindTexture(GL_TEXTURE_2D, texture0);
                break;
            }
            case 1: {
                glBindTexture(GL_TEXTURE_2D, texture1);
                break;
            }
            default: glBindTexture(GL_TEXTURE_2D, texture0); // Fallback, should not occur
            }
            glBindVertexArray(coordinatesVao1[4]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            // . for x for drone 1
            glBindTexture(GL_TEXTURE_2D, textureDot);
            glBindVertexArray(coordinatesVao1[5]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            // num2 for x for drone 1
            float scaledX = abs(drone1.x) * 100.0f;
            int precise = static_cast<int>(round(scaledX));
            int first = (precise / 10) % 10;
            int second = precise % 10;
            switch (first) {
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
            glBindVertexArray(coordinatesVao1[6]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            // num3 for x for drone 1
            switch (second) {
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
            glBindVertexArray(coordinatesVao1[7]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        // y for drone 1
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texturey);
        glBindVertexArray(coordinatesVao1y[0]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        // : for y for drone 1
        glBindTexture(GL_TEXTURE_2D, textureDot);
        glBindVertexArray(coordinatesVao1y[1]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, textureDot);
        glBindVertexArray(coordinatesVao1y[2]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        if (drone1.y < 0 || drone1.destroyed) {
            // - for y for drone 1
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureMinus);
            glBindVertexArray(coordinatesVao1y[3]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        if (!drone1.destroyed) {
            // num1 for y for drone 1
            int num1 = 0;
            if (drone1.y <= -1 || drone1.y >= 1) num1 = 1;
            switch (num1) {
            case 0: {
                glBindTexture(GL_TEXTURE_2D, texture0);
                break;
            }
            case 1: {
                glBindTexture(GL_TEXTURE_2D, texture1);
                break;
            }
            default: glBindTexture(GL_TEXTURE_2D, texture0); // Fallback, should not occur
            }
            glBindVertexArray(coordinatesVao1y[4]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            // . for x for drone 1
            glBindTexture(GL_TEXTURE_2D, textureDot);
            glBindVertexArray(coordinatesVao1y[5]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            // num2 for x for drone 1
            float scaledY = abs(drone1.y) * 100.0f;
            int precise = static_cast<int>(round(scaledY));
            int first = (precise / 10) % 10;
            int second = precise % 10;
            switch (first) {
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
            glBindVertexArray(coordinatesVao1y[6]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            // num3 for x for drone 1
            switch (second) {
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
            glBindVertexArray(coordinatesVao1y[7]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        // x for drone 2
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texturex);
        glBindVertexArray(coordinatesVao2[0]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        // : for x for drone 2
        glBindTexture(GL_TEXTURE_2D, textureDot);
        glBindVertexArray(coordinatesVao2[1]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, textureDot);
        glBindVertexArray(coordinatesVao2[2]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        if (drone2.x < 0 || drone2.destroyed) {
            // - for x for drone 1
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureMinus);
            glBindVertexArray(coordinatesVao2[3]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        // num1 for x for drone 2
        if (!drone2.destroyed) {
            int num1 = 0;
            if (drone2.x <= -1 || drone2.x >= 1) num1 = 1;
            switch (num1) {
            case 0: {
                glBindTexture(GL_TEXTURE_2D, texture0);
                break;
            }
            case 1: {
                glBindTexture(GL_TEXTURE_2D, texture1);
                break;
            }
            default: glBindTexture(GL_TEXTURE_2D, texture0); // Fallback, should not occur
            }
            glBindVertexArray(coordinatesVao2[4]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            // . for x for drone 2
            glBindTexture(GL_TEXTURE_2D, textureDot);
            glBindVertexArray(coordinatesVao2[5]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            // num2 for x for drone 2
            int scaledX = abs(drone2.x) * 100.0f;
            int precise = static_cast<int>(round(scaledX));
            int first = (precise / 10) % 10;
            int second = precise % 10;
            switch (first) {
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
            glBindVertexArray(coordinatesVao2[6]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            // num3 for x for drone 2
            switch (second) {
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
            glBindVertexArray(coordinatesVao2[7]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        // y for drone 2
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texturey);
        glBindVertexArray(coordinatesVao2y[0]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        // : for y for drone 2
        glBindTexture(GL_TEXTURE_2D, textureDot);
        glBindVertexArray(coordinatesVao2y[1]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, textureDot);
        glBindVertexArray(coordinatesVao2y[2]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        if (drone2.y < 0 || drone2.destroyed) {
            // - for y for drone 2
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureMinus);
            glBindVertexArray(coordinatesVao2y[3]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        // num1 for y for drone 2
        if (!drone2.destroyed) {
            int num1 = 0;
            if (drone2.y <= -1 || drone2.y >= 1) num1 = 1;
            switch (num1) {
            case 0: {
                glBindTexture(GL_TEXTURE_2D, texture0);
                break;
            }
            case 1: {
                glBindTexture(GL_TEXTURE_2D, texture1);
                break;
            }
            default: glBindTexture(GL_TEXTURE_2D, texture0); // Fallback, should not occur
            }
            glBindVertexArray(coordinatesVao2y[4]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            // . for x for drone 2
            glBindTexture(GL_TEXTURE_2D, textureDot);
            glBindVertexArray(coordinatesVao2y[5]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            // num2 for x for drone 1
            int scaledY = abs(drone2.y) * 100.0f;
            int precise = static_cast<int>(round(scaledY));
            int first = (precise / 10) % 10;
            int second = precise % 10;
            switch (first) {
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
            glBindVertexArray(coordinatesVao2y[6]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            // num3 for x for drone 1
            switch (second) {
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
            glBindVertexArray(coordinatesVao2y[7]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        //IME I PREZIME
        //m
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texturem);
        glBindVertexArray(nameVao[0]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        //i
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texturei);
        glBindVertexArray(nameVao[1]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        //l
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texturel);
        glBindVertexArray(nameVao[2]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        //a
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texturea);
        glBindVertexArray(nameVao[3]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        //m
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texturem);
        glBindVertexArray(nameVao[4]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        //i
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texturei);
        glBindVertexArray(nameVao[5]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        //l
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texturel);
        glBindVertexArray(nameVao[6]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        //o
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureo);
        glBindVertexArray(nameVao[7]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        //v
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texturev);
        glBindVertexArray(nameVao[8]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        //i
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texturei);
        glBindVertexArray(nameVao[9]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        //c
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texturec);
        glBindVertexArray(nameVao[10]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        //s
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures);
        glBindVertexArray(indexVao[0]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        //v
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texturev);
        glBindVertexArray(indexVao[1]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        //2
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture2);
        glBindVertexArray(indexVao[2]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        //2
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture2);
        glBindVertexArray(indexVao[3]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        //-
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureMinus);
        glBindVertexArray(indexVao[4]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        //2
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture2);
        glBindVertexArray(indexVao[5]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        //0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture0);
        glBindVertexArray(indexVao[6]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        //2
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture2);
        glBindVertexArray(indexVao[7]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        //1
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        glBindVertexArray(indexVao[8]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);


        //DESTROYED 1
        if (drone1.destroyed) {
            //d
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textured);
            glBindVertexArray(destroyedVao1[0]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            //e
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texturee);
            glBindVertexArray(destroyedVao1[1]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            //s
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures);
            glBindVertexArray(destroyedVao1[2]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            //t
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texturet);
            glBindVertexArray(destroyedVao1[3]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            //r
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texturer);
            glBindVertexArray(destroyedVao1[4]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            //o
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureo);
            glBindVertexArray(destroyedVao1[5]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            //y
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texturey);
            glBindVertexArray(destroyedVao1[6]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            //e
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texturee);
            glBindVertexArray(destroyedVao1[7]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            //d
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textured);
            glBindVertexArray(destroyedVao1[8]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        //DESTROYED 2
        if (drone2.destroyed) {
            //d
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textured);
            glBindVertexArray(destroyedVao2[0]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            //e
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texturee);
            glBindVertexArray(destroyedVao2[1]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            //s
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures);
            glBindVertexArray(destroyedVao2[2]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            //t
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texturet);
            glBindVertexArray(destroyedVao2[3]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            //r
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texturer);
            glBindVertexArray(destroyedVao2[4]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            //o
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureo);
            glBindVertexArray(destroyedVao2[5]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            //y
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texturey);
            glBindVertexArray(destroyedVao2[6]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            //e
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texturee);
            glBindVertexArray(destroyedVao2[7]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            //d
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textured);
            glBindVertexArray(destroyedVao2[8]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

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
    glDeleteVertexArrays(8, coordinatesVao1);
    glDeleteBuffers(8, coordinatesVbo1);
    glDeleteBuffers(8, coordinatesEbo1);
    glDeleteVertexArrays(8, coordinatesVao1y);
    glDeleteBuffers(8, coordinatesVbo1y);
    glDeleteBuffers(8, coordinatesEbo1y);
    glDeleteVertexArrays(8, coordinatesVao2);
    glDeleteBuffers(8, coordinatesVbo2);
    glDeleteBuffers(8, coordinatesEbo2);
    glDeleteVertexArrays(8, coordinatesVao2y);
    glDeleteBuffers(8, coordinatesVbo2y);
    glDeleteBuffers(8, coordinatesEbo2y);
    glDeleteVertexArrays(11, nameVao);
    glDeleteBuffers(11, nameVbo);
    glDeleteBuffers(11, nameEbo);
    glDeleteVertexArrays(9, indexVao);
    glDeleteBuffers(9, indexVbo);
    glDeleteBuffers(9, indexEbo);
    glDeleteVertexArrays(9, destroyedVao1);
    glDeleteBuffers(9, destroyedVbo1);
    glDeleteBuffers(9, destroyedEbo1);
    glDeleteVertexArrays(9, destroyedVao2);
    glDeleteBuffers(9, destroyedVbo2);
    glDeleteBuffers(9, destroyedEbo2);
    glDeleteBuffers(1, &ledVBO);
    glDeleteVertexArrays(1, &ledVAO);
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