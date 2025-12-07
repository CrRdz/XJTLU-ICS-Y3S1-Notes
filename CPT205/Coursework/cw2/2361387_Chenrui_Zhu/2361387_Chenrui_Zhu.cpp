#define FREEGLUT_STATIC
#include <GL/freeglut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>

// Constants
#define PI 3.14159265358979323846


// STRUCTURES
struct Image {
    int width = 0;
    int height = 0;
    std::vector<unsigned char> rgba; // 4 bytes per pixel (R, G, B, A)
};

// BMP File Header packing to ensure correct byte alignment
#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};

struct BMPInfoHeader {
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};
#pragma pack(pop)

// Environment objects
struct Tree {
    float x, z;
    float scale;
};

struct Building {
    float x, z;
    float width, depth;
    float height;
};

struct FloatingBlock {
    float x, y, z;
    float width, height, depth;
    float floatOffset;
    float floatSpeed;
    std::vector<Building> buildings;
};

struct Connection {
    int fromIdx;
    int toIdx;
    float maxDist;
};

struct Billboard {
    float x, y, z;
    float width, height;
    float angle;
    float floatOffset;
    int texIndex;
};

// Traffic system objects
struct CargoTrain {
    float angle;
    float speed;
    int length;
    float r, g, b;
};

struct TransportRing {
    float radius;
    float height;
    float rotateSpeed;
    std::vector<CargoTrain> trains;
    float currentRot = 0.0f;
};


// GLOBAL VARIABLES
// Time and Environment
float gTime = 0.0f;
bool isNight = false;
bool enableFog = true;

// Camera Control
float gYaw = 0.0f;
float gPitch = -10.0f;
float gDistance = 2.0f;
float camSmoothYaw = 0.0f;
int lastMouseX = 0;
int lastMouseY = 0;
bool dragging = false;

// Sky Dome Configuration
int domeSegmentsLat = 32;
int domeSegmentsLon = 64;
float domeRadius = 100.0f;
float horizonExtend = 0.06f;
float seamYawOffset = 0.0f;

// Textures
GLuint gTexSky = 0;
GLuint gTexGround = 0;
GLuint gTexTree = 0;
std::vector<GLuint> gAdTextures;

// Scene Objects
std::vector<Tree>  gForest;
std::vector<FloatingBlock> gCityBlocks;
std::vector<Connection> gConnections;
std::vector<Billboard> gBillboards;
std::vector<TransportRing> gTransportRings;

// Player/Car State
float carX = -52.0f;
float carY = 20.0f;
float carZ = 45.0f;
float carYaw = -50.0f;
float carSpeed = 0.06f;

// key State
bool keyState[256] = { false };


// HELPER FUNCTIONS
// Convert degrees to radians
float toRad(float angle) {
    return angle * PI / 180.0f;
}

// Linear interpolation
float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

/**
 * Loads a BMP image from disk.
 * Supports 24-bit and 32-bit BMP files.
 * Handles color keying for transparency (pure black becomes transparent).
 */
bool loadBMP(const std::string& path, Image& img, bool useColorKey = true) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    BMPFileHeader fh;
    BMPInfoHeader ih;

    f.read((char*)&fh, sizeof(fh));
    if (!f || fh.bfType != 0x4D42) return false; // 'BM' signature

    f.read((char*)&ih, sizeof(ih));
    if (ih.biBitCount != 24 && ih.biBitCount != 32) return false;

    const int width = ih.biWidth;
    const int height = std::abs(ih.biHeight);
    const int bytesPerPixel = ih.biBitCount / 8;

    // Row size must be a multiple of 4 bytes
    const int rowSize = ((width * bytesPerPixel + 3) / 4) * 4;

    f.seekg(fh.bfOffBits, std::ios::beg);
    std::vector<unsigned char> fileData(rowSize * height);

    f.read((char*)fileData.data(), fileData.size());

    img.width = width;
    img.height = height;
    img.rgba.resize(width * height * 4);

    bool topDown = (ih.biHeight < 0);
    for (int y = 0; y < height; ++y) {
        int srcY = topDown ? y : (height - 1 - y);
        const unsigned char* srcRow = &fileData[srcY * rowSize];

        for (int x = 0; x < width; ++x) {
            size_t dstIdx = (y * width + x) * 4;
            size_t srcIdx = x * bytesPerPixel;

            unsigned char b = srcRow[srcIdx];
            unsigned char g = srcRow[srcIdx + 1];
            unsigned char r = srcRow[srcIdx + 2];
            unsigned char a = 255;

            if (ih.biBitCount == 32) {
                a = srcRow[srcIdx + 3];
            }
            else if (useColorKey && r == 0 && g == 0 && b == 0) {
                a = 0; // Transparent if black
            }

            img.rgba[dstIdx] = r;
            img.rgba[dstIdx + 1] = g;
            img.rgba[dstIdx + 2] = b;
            img.rgba[dstIdx + 3] = a;
        }
    }
    return true;
}

/**
 * Uploads image data to an OpenGL texture.
 */
GLuint createTexture(const Image& img, bool isGround, bool isTree = false) {
    if (img.width <= 0 || img.height <= 0) return 0;

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    GLint wrapMode = isGround ? GL_REPEAT : GL_CLAMP;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width, img.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.rgba.data());

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    return texID;
}

// Helper to draw a scaled cube
void drawBox(float w, float h, float d) {
    glPushMatrix();
    glScalef(w, h, d);
    glutSolidCube(1.0f);
    glPopMatrix();
}

// Helper to draw 2D UI elements with rounded corners
void drawRoundedRect(float x, float y, float w, float h, float r, GLenum mode) {
    int segments = 16;
    float theta;

    glBegin(mode);
    // Top-right corner
    for (int i = 0; i <= segments; i++) {
        theta = i * PI / 2.0f / segments;
        glVertex2f(x + w - r + cos(theta) * r, y + h - r + sin(theta) * r);
    }

    // Top-left corner
    for (int i = 0; i <= segments; i++) {
        theta = PI / 2.0f + i * PI / 2.0f / segments;
        glVertex2f(x + r + cos(theta) * r, y + h - r + sin(theta) * r);
    }

    // Bottom-left corner
    for (int i = 0; i <= segments; i++) {
        theta = PI + i * PI / 2.0f / segments;
        glVertex2f(x + r + cos(theta) * r, y + r + sin(theta) * r);
    }

    // Bottom-right corner
    for (int i = 0; i <= segments; i++) {
        theta = 3.0f * PI / 2.0f + i * PI / 2.0f / segments;
        glVertex2f(x + w - r + cos(theta) * r, y + r + sin(theta) * r);
    }
    glEnd();
}


// SCENE DRAWING
// Calculates the rim coordinates for the skydome/ground intersection
std::pair<float, float> computeDomeRim() {
    float maxTheta = PI * std::min(0.95f, 0.5f + std::max(0.0f, horizonExtend));
    return std::make_pair(domeRadius * std::sin(maxTheta), domeRadius * std::cos(maxTheta));
}

void drawSkyDome() {
    if (!gTexSky) return;

    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, gTexSky);

    if (isNight) {
        glColor3f(0.5f, 0.2f, 0.6f);
    }
    else {
        glColor3f(1.0f, 1.0f, 1.0f);
    }

    glDepthMask(GL_FALSE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT); // Render inside of sphere

    float maxTheta = PI * std::min(0.95f, 0.5f + std::max(0.0f, horizonExtend));

    // Generate dome geometry
    for (int i = 0; i < domeSegmentsLat; ++i) {
        float t0 = (float)i / domeSegmentsLat * maxTheta;
        float t1 = (float)(i + 1) / domeSegmentsLat * maxTheta;
        float y0 = domeRadius * cos(t0), y1 = domeRadius * cos(t1);
        float r0 = domeRadius * sin(t0), r1 = domeRadius * sin(t1);
        float v0 = t0 / maxTheta, v1 = t1 / maxTheta;

        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j <= domeSegmentsLon; ++j) {
            float u = (float)j / domeSegmentsLon;
            float phi = (float)j / domeSegmentsLon * 2.0f * PI;
            float cp = cos(phi), sp = sin(phi);

            glTexCoord2f(u, v1);
            glVertex3f(r1 * cp, y1, r1 * sp);
            glTexCoord2f(u, v0);
            glVertex3f(r0 * cp, y0, r0 * sp);
        }
        glEnd();
    }

    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glPopAttrib();
}

void drawGroundDiskPerfectlyAttached() {
    std::pair<float, float> rim = computeDomeRim();
    float y = rim.second - 0.01f;
    float r = rim.first;
    int segs = std::max(32, domeSegmentsLon);

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LIGHTING_BIT);
    glDisable(GL_LIGHTING);

    if (gTexGround) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, gTexGround);
    }
    else {
        glDisable(GL_TEXTURE_2D);
    }

    if (isNight) {
        glColor3f(0.3f, 0.1f, 0.4f);
    }
    else {
        glColor3f(1.0f, 1.0f, 1.0f);
    }

    glPushMatrix();
    if (seamYawOffset != 0.0f) {
        glRotatef(seamYawOffset, 0.0f, 1.0f, 0.0f);
    }

    glBegin(GL_TRIANGLE_FAN);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(0.0f, y, 0.0f); // Center

    for (int j = 0; j <= segs; ++j) {
        float phi = (float)j / segs * 2.0f * PI;
        float x = r * cos(phi), z = r * sin(phi);

        glTexCoord2f(x * 0.1f, z * 0.1f);
        glVertex3f(x, y, z);
    }
    glEnd();

    glPopMatrix();
    glPopAttrib();
}

void drawForest() {
    if (!gTexTree) return;

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LIGHTING_BIT);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, gTexTree);

    // Use Alpha Test for cutout effect
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);

    if (isNight) {
        glColor3f(0.1f, 0.5f, 0.6f);
    }
    else {
        glColor3f(1.0f, 1.0f, 1.0f);
    }

    std::pair<float, float> rim = computeDomeRim();
    float plantY = rim.second - 0.1f;

    // Get camera info for billboarding (facing the camera)
    float mv[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    float rightX = mv[0], rightZ = mv[8];

    for (const auto& t : gForest) {
        glPushMatrix();
        glTranslatef(t.x, plantY, t.z);

        // Billboard calculation
        float len = sqrt(rightX * rightX + rightZ * rightZ);
        if (len > 0.0001f) {
            float rX = rightX / len, rZ = rightZ / len;
            // Construct rotation matrix to face camera
            float mat[16] = { rX, 0, rZ, 0,  0, 1, 0, 0,  -rZ, 0, rX, 0,  0, 0, 0, 1 };
            glMultMatrixf(mat);
        }

        glScalef(t.scale, t.scale, t.scale);
        float w = 7.0f, h = 16.0f;

        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-w, 0.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(w, 0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(w, h, 0.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-w, h, 0.0f);
        glEnd();

        glPopMatrix();
    }
    glDisable(GL_ALPHA_TEST);
    glPopAttrib();
}

void drawFloatingCity() {
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LIGHTING_BIT | GL_LINE_BIT);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat pos[] = { 50.0f, 100.0f, 50.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, pos);

    if (isNight) {
        GLfloat a[] = { 0.1f, 0, 0.2f, 1.f };
        GLfloat d[] = { 0.5f, 0, 0.8f, 1.f };
        GLfloat s[] = { 0.8f, 0, 1.f, 1.f };
        glLightfv(GL_LIGHT0, GL_AMBIENT, a);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, d);
        glLightfv(GL_LIGHT0, GL_SPECULAR, s);
    }
    else {
        GLfloat a[] = { 0.2f, 0.2f, 0.2f, 1.f };
        GLfloat d[] = { 1.f, 1.f, 1.f, 1.f };
        GLfloat s[] = { 1.f, 1.f, 1.f, 1.f };
        glLightfv(GL_LIGHT0, GL_AMBIENT, a);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, d);
        glLightfv(GL_LIGHT0, GL_SPECULAR, s);
    }

    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Draw City Blocks
    for (const auto& b : gCityBlocks) {
        glPushMatrix();

        float floatY = sin(gTime * b.floatSpeed + b.floatOffset) * 1.0f;
        glTranslatef(b.x, b.y + floatY, b.z);

        // Base platform
        glColor4f(0.1f, 0.1f, 0.15f, 1.0f);
        drawBox(b.width, b.height, b.depth);

        glDisable(GL_LIGHTING);

        // Wireframe edge for platform
        if (isNight) {
            glColor3f(1.0f, 0.0f, 0.8f);
        }
        else {
            glColor3f(0.0f, 0.6f, 0.8f);
        }

        glPushMatrix();
        glScalef(b.width * 1.01f, b.height * 1.01f, b.depth * 1.01f);
        glutWireCube(1.0f);
        glPopMatrix();

        glEnable(GL_LIGHTING);

        // Draw Buildings on the block
        for (const auto& bld : b.buildings) {
            glPushMatrix();
            float cy = (b.height * 0.5f) + (bld.height * 0.5f);
            glTranslatef(bld.x, cy, bld.z);

            // Building Body
            if (isNight) {
                glColor4f(0.1f, 0.05f, 0.2f, 0.85f);
            }
            else {
                glColor4f(0.5f, 0.6f, 0.8f, 0.6f);
            }
            drawBox(bld.width, bld.height, bld.depth);

            glDisable(GL_LIGHTING);

            // Building Edges / Neon Lines
            if (isNight) {
                if (((int)bld.height % 2) == 0) {
                    glColor3f(0.0f, 1.0f, 1.0f);
                }
                else {
                    glColor3f(1.0f, 0.08f, 0.58f);
                }
                glLineWidth(2.0f);
            }
            else {
                glColor3f(0.8f, 0.9f, 1.0f);
                glLineWidth(1.0f);
            }

            glPushMatrix();
            glScalef(bld.width * 1.01f, bld.height * 1.01f, bld.depth * 1.01f);
            glutWireCube(1.0f);
            glPopMatrix();

            // Horizontal "floor" lines
            if (isNight) {
                glColor3f(0.5f, 0.0f, 1.0f);
            }
            else {
                glColor3f(0.4f, 0.5f, 0.6f);
            }

            int floors = (int)(bld.height / 1.5f);
            for (int f = 0; f < floors; ++f) {
                glPushMatrix();
                glTranslatef(0.0f, -bld.height * 0.5f + f * 1.5f, 0.0f);
                glScalef(bld.width * 1.01f, 0.05f, bld.depth * 1.01f);
                glutWireCube(1.0f);
                glPopMatrix();
            }
            glEnable(GL_LIGHTING);
            glPopMatrix();
        }
        glPopMatrix();
    }
    glDisable(GL_BLEND);
    glPopAttrib();
}

void drawConnections() {
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LINE_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(3.0f);

    int dashOffset = (int)(gTime * 0.2f);

    for (const auto& c : gConnections) {
        const auto& b1 = gCityBlocks[c.fromIdx];
        const auto& b2 = gCityBlocks[c.toIdx];

        float y1 = b1.y + sin(gTime * b1.floatSpeed + b1.floatOffset) * 2.0f;
        float y2 = b2.y + sin(gTime * b2.floatSpeed + b2.floatOffset) * 2.0f;

        float x1 = b1.x, z1 = b1.z, sy = y1 + b1.height;
        float x2 = b2.x, z2 = b2.z, ey = y2 + b2.height;
        float dx = x2 - x1, dy = ey - sy, dz = z2 - z1;

        if (isNight) {
            glColor4f(0.2f, 1.0f, 0.2f, 0.9f);
        }
        else {
            glColor4f(0.3f, 1.0f, 0.0f, 0.6f);
        }

        // Draw dashed lines representing data flow or bridges
        glBegin(GL_LINES);
        for (int i = 0; i < 40; ++i) {
            if ((i + dashOffset) % 5 < 3) {
                float t0 = (float)i / 40;
                float t1 = (float)(i + 1) / 40;
                // Add a slight curve (sine wave) to the line
                glVertex3f(x1 + dx * t0, sy + dy * t0 - 2.0f * sin(t0 * PI), z1 + dz * t0);
                glVertex3f(x1 + dx * t1, sy + dy * t1 - 2.0f * sin(t1 * PI), z1 + dz * t1);
            }
        }
        glEnd();
    }
    glPopAttrib();
}

void drawBillboards() {
    if (gBillboards.empty()) return;

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_LINE_BIT);

    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    for (const auto& bb : gBillboards) {
        if (bb.texIndex < gAdTextures.size()) {
            glBindTexture(GL_TEXTURE_2D, gAdTextures[bb.texIndex]);
        }

        glPushMatrix();
        float floatY = sin(gTime * 0.03f + bb.floatOffset) * 1.5f;
        glTranslatef(bb.x, bb.y + floatY, bb.z);
        glRotatef(bb.angle, 0.0f, 1.0f, 0.0f);

        float w = bb.width / 2.0f, h = bb.height / 2.0f, d = 0.05f;

        // Draw Texture Image
        glEnable(GL_TEXTURE_2D);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-w, -h, d); glTexCoord2f(1.0f, 1.0f); glVertex3f(w, -h, d);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(w, h, d); glTexCoord2f(0.0f, 0.0f); glVertex3f(-w, h, d);
        // Back side
        glTexCoord2f(0.0f, 1.0f); glVertex3f(w, -h, -d); glTexCoord2f(1.0f, 1.0f); glVertex3f(-w, -h, -d);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-w, h, -d); glTexCoord2f(0.0f, 0.0f); glVertex3f(w, h, -d);
        glEnd();

        // Draw Neon Frame (Night Mode)
        if (isNight) {
            glDisable(GL_TEXTURE_2D);
            glLineWidth(3.0f);
            float glow = 0.5f + 0.5f * sin(gTime * 0.1f);
            glColor4f(1.0f, 0.08f, 0.58f, glow);

            glBegin(GL_LINE_LOOP);
            glVertex3f(-w, -h, d + 0.02f); glVertex3f(w, -h, d + 0.02f); glVertex3f(w, h, d + 0.02f); glVertex3f(-w, h, d + 0.02f);
            glEnd();

            glBegin(GL_LINE_LOOP);
            glVertex3f(w, -h, -d - 0.02f); glVertex3f(-w, -h, -d - 0.02f); glVertex3f(-w, h, -d - 0.02f); glVertex3f(w, h, -d - 0.02f);
            glEnd();

            glEnable(GL_TEXTURE_2D);
        }

        // Draw Projector/Stand
        glDisable(GL_TEXTURE_2D);
        float projY = -h - 2.0f;
        glPushMatrix();
        glTranslatef(0.0f, projY, 0.0f);
        glColor3f(0.2f, 0.2f, 0.25f);
        glScalef(2.0f, 1.0f, 1.0f);
        glutSolidCube(1.0f);

        glEnable(GL_LIGHTING);
        glTranslatef(0.0f, 0.5f, 0.0f);
        glColor3f(0.0f, 1.0f, 1.0f);
        glutSolidSphere(0.3f, 10, 10);
        glDisable(GL_LIGHTING);

        glPopMatrix();

        // Draw Holographic Beam
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        glColor4f(0.0f, 0.8f, 1.0f, 0.15f);
        float lensY = projY + 0.5f;

        glBegin(GL_TRIANGLES);
        glVertex3f(0.0f, lensY, 0.0f); glVertex3f(-w, -h, 0.0f); glVertex3f(w, -h, 0.0f);

        glColor4f(0.0f, 0.8f, 1.0f, 0.05f);
        glVertex3f(0.0f, lensY, 0.0f); glVertex3f(-w, -h, 0.0f); glVertex3f(-w, h, 0.0f);
        glVertex3f(0.0f, lensY, 0.0f); glVertex3f(w, -h, 0.0f); glVertex3f(w, h, 0.0f);
        glEnd();

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_TRUE);
        glPopMatrix();
    }
    glPopAttrib();
}

void drawCarGeometry() {
    glPushMatrix();
    glScalef(1.0f, 1.0f, 1.8f);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void drawMyCar() {
    glPushMatrix();
    glTranslatef(carX, carY, carZ);
    glRotatef(carYaw, 0.0f, 1.0f, 0.0f);
    glRotatef(45.0f, 0.0f, 0.0f, 1.0f);
    glRotatef(10.0f, 1.0f, 0.0f, 0.0f);

    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_CULL_FACE);

    GLfloat s[] = { 1.f, 1.f, 1.f, 1.f };
    GLfloat sh[] = { 128.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, s);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, sh);

    // Car Body (Core)
    glPushMatrix();
    glScalef(0.4f, 0.4f, 0.6f);
    glColor4f(1.0f, 0.85f, 0.6f, 0.9f);
    glutSolidCube(1.0f);

    glDisable(GL_LIGHTING);
    glLineWidth(2.0f);
    glColor4f(1.0f, 0.95f, 0.8f, 1.0f);
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-1.0f, -1.0f);
    glutWireCube(1.0f);
    glDisable(GL_POLYGON_OFFSET_LINE);
    glEnable(GL_LIGHTING);
    glPopMatrix();

    // Outer Shell
    glColor4f(0.95f, 0.98f, 1.0f, 0.1f);
    glCullFace(GL_FRONT);
    drawCarGeometry();
    glCullFace(GL_BACK);
    drawCarGeometry();

    // Wireframe details
    glDisable(GL_LIGHTING);
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(1.5f);
    glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-1.0f, -1.0f);

    glPushMatrix();
    glScalef(1.0f, 1.0f, 1.8f);
    glutWireCube(1.0f);
    glPopMatrix();
    glDisable(GL_POLYGON_OFFSET_LINE);
    glDisable(GL_LINE_SMOOTH);

    // Engine Trails
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glLineWidth(2.0f);

    float tailZ = 0.9f;
    float corners[4][2] = { {-0.5f, -0.5f}, { 0.5f, -0.5f}, { 0.5f, 0.5f}, {-0.5f, 0.5f} };

    glBegin(GL_LINES);
    for (int k = 0; k < 4; ++k) {
        float trailLen = 2.0f + sin(gTime * 0.1f + k) * 0.5f;
        glColor4f(1.0f, 1.0f, 1.0f, 0.6f); glVertex3f(corners[k][0], corners[k][1], tailZ);
        glColor4f(1.0f, 1.0f, 1.0f, 0.0f); glVertex3f(corners[k][0], corners[k][1], tailZ + trailLen);
        glColor4f(1.0f, 1.0f, 1.0f, 0.8f); glVertex3f(corners[k][0], corners[k][1], tailZ);
        glColor4f(1.0f, 1.0f, 1.0f, 0.0f); glVertex3f(corners[k][0], corners[k][1], tailZ + 1.5f);
    }
    glEnd();
    glPopAttrib();
    glPopMatrix();
}

void drawTrafficLight(float x, float y, float z, float facingAngle, int state) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(facingAngle, 0.0f, 1.0f, 0.0f);

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // Structure
    glPushMatrix();
    glTranslatef(0.0f, -1.0f, 0.0f);
    glColor3f(0.15f, 0.15f, 0.2f);
    glScalef(0.8f, 0.4f, 0.8f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Joint
    glPushMatrix();
    glTranslatef(0.0f, -0.8f, 0.0f);
    glColor3f(0.1f, 0.1f, 0.1f);
    glutSolidSphere(0.3f, 10, 10);
    glTranslatef(0.0f, 0.25f, 0.0f);
    glDisable(GL_LIGHTING);
    glColor3f(0.6f, 0.9f, 1.0f);
    glutSolidSphere(0.15f, 8, 8);
    glEnable(GL_LIGHTING);
    glPopMatrix();

    // Holographic Base beams
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glColor4f(0.4f, 0.8f, 1.0f, 0.3f);
    for (int i = 0; i < 4; ++i) {
        glVertex3f(0.0f, -0.6f, 0.0f);
        float ang = i * PI / 2.0f;
        glVertex3f(cos(ang) * 0.2f, 1.5f, sin(ang) * 0.2f);
    }
    glColor4f(0.6f, 0.9f, 1.0f, 0.5f);
    glVertex3f(0.0f, -0.6f, 0.0f);
    glVertex3f(0.0f, 1.5f, 0.0f);
    glEnd();

    // Light Halo
    glPushMatrix();
    glTranslatef(0.0f, 1.5f, 0.0f);
    static const float colors[3][3] = {
        {0.0f, 1.0f, 0.0f}, // Green
        {1.0f, 1.0f, 0.0f}, // Yellow
        {1.0f, 0.0f, 0.0f}  // Red
    };
    float r = colors[state][0], g = colors[state][1], b = colors[state][2];

    glColor4f(r, g, b, 0.6f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.0f, 0.0f, 0.0f);
    for (int i = 0; i <= 32; ++i) {
        float a = (float)i / 32.0f * 2.0f * PI;
        glVertex3f(cos(a) * 0.9f, sin(a) * 0.9f, 0.0f);
    }
    glEnd();

    glLineWidth(3.0f);
    glColor4f(r, g, b, 0.9f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i <= 32; ++i) {
        float a = (float)i / 32.0f * 2.0f * PI;
        glVertex3f(cos(a) * 0.9f, sin(a) * 0.9f, 0.0f);
    }
    glEnd();
    glPopMatrix();
    glPopAttrib();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
    glPopMatrix();
}

void drawOrbitalSystem() {
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LINE_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int ringIndex = 0;
    for (auto& ring : gTransportRings) {
        // Calculate traffic light timing
        int cycleLen = 1000;
        int currentTick = ((int)gTime + ringIndex * 333) % cycleLen;
        int lightState = (currentTick < 500) ? 0 : (currentTick < 650 ? 1 : 2);

        float fixedAngle = 45.0f + ringIndex * 50.0f;
        float rad = fixedAngle * PI / 180.0f;

        drawTrafficLight(cos(rad) * (ring.radius + 3.5f), ring.height, sin(rad) * (ring.radius + 3.5f), -fixedAngle - 90.0f, lightState);

        ringIndex++;

        // Update train positions if light is not RED (2)
        if (lightState != 2) {
            ring.currentRot += ring.rotateSpeed;
            for (auto& train : ring.trains) {
                train.angle += train.speed * 0.01f;
            }
        }

        // Draw Ring Tracks
        glPushMatrix();
        glRotatef(ring.currentRot, 0.0f, 1.0f, 0.0f);
        glColor4f(0.2f, 0.3f, 0.4f, 0.2f);

        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= 120; ++i) {
            float a = (float)i / 120.0f * 2.0f * PI;
            float c = cos(a), s = sin(a);
            glNormal3f(0.0f, 1.0f, 0.0f);
            glVertex3f(c * (ring.radius - 1.0f), ring.height, s * (ring.radius - 1.0f));
            glVertex3f(c * (ring.radius + 1.0f), ring.height, s * (ring.radius + 1.0f));
        }
        glEnd();

        // Draw Ring Edges (Glowing lines)
        glDisable(GL_LIGHTING);
        glColor4f(0.4f, 0.9f, 1.0f, 0.4f);
        glLineWidth(1.5f);

        auto drawRingLine = [&](float r_offset) {
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i <= 120; ++i) {
                float a = (float)i / 120.0f * 2.0f * PI;
                glVertex3f(cos(a) * r_offset, ring.height + 0.1f, sin(a) * r_offset);
            }
            glEnd();
            };

        drawRingLine(ring.radius - 1.0f);
        drawRingLine(ring.radius + 1.0f);
        glEnable(GL_LIGHTING);

        // Draw Trains
        for (auto& train : ring.trains) {
            for (int k = 0; k < train.length; ++k) {
                float spacing = 0.12f;
                float currentA = train.angle - (k * spacing * (train.speed > 0 ? 1 : -1));
                float cx = cos(currentA) * ring.radius;
                float cz = sin(currentA) * ring.radius;
                float cy = ring.height + 0.5f;
                float yaw = -currentA * 180.0f / PI + (train.speed > 0 ? 90 : -90);

                glPushMatrix();
                glTranslatef(cx, cy, cz);
                glRotatef(yaw, 0.0f, 1.0f, 0.0f);

                // Base of train car
                glColor4f(0.3f, 0.3f, 0.3f, 0.3f);
                glPushMatrix();
                glScalef(1.5f, 0.3f, 0.8f);
                glutSolidCube(1.0f);
                glPopMatrix();

                // Cargo Container
                float cargoHeight = (k % 2 == 0) ? 0.8f : 0.6f;
                glColor4f(train.r, train.g, train.b, 0.4f);
                glPushMatrix();
                glTranslatef(0.0f, 0.3f + cargoHeight / 2.0f, 0.0f);
                glScalef(1.2f, cargoHeight, 0.6f);
                glutSolidCube(1.0f);

                glDisable(GL_LIGHTING);
                glColor4f(1.0f, 1.0f, 1.0f, 0.3f);
                glutWireCube(1.001f);
                glEnable(GL_LIGHTING);
                glPopMatrix();

                // Tail lights
                glDisable(GL_LIGHTING);
                glPointSize(3.0f);
                glBegin(GL_POINTS);
                if (lightState == 2) glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
                else if (lightState == 1) glColor4f(0.8f, 0.2f, 0.0f, 0.9f);
                else glColor4f(0.6f, 0.0f, 0.0f, 0.8f);

                if (k == train.length - 1) glVertex3f(-0.8f, 0.5f, 0.0f);
                glEnd();
                glEnable(GL_LIGHTING);
                glPopMatrix();
            }
        }
        glPopMatrix();
    }
    glPopAttrib();
}

void drawTextHUD() {
    // Switch to 2D Orthographic projection for HUD
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    gluOrtho2D(0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int boxW = 160, boxH = 130, margin = 25, radius = 12;
    int lineHeight = 18, titleGap = 6;
    float x = w - boxW - margin, y = margin;

    // HUD Background
    if (isNight) {
        glColor4f(0.1f, 0.0f, 0.2f, 0.6f);
    }
    else {
        glColor4f(0.0f, 0.0f, 0.0f, 0.2f);
    }
    drawRoundedRect(x, y, boxW, boxH, radius, GL_POLYGON);

    // HUD Border
    if (isNight) {
        glColor4f(1.0f, 0.0f, 0.8f, 0.9f);
    }
    drawRoundedRect(x, y, boxW, boxH, radius, GL_LINE_LOOP);

    std::vector<std::string> lines = {
        "CONTROLS",
        "W / S : Move",
        "A / D : Rotate",
        "Space : Up",
        "N : Toggle Night",
        "Mouse : Look"
    };

    float textBlockSpan = (lines.size() - 1) * lineHeight + titleGap;
    float currentY = y + boxH / 2.0f + (textBlockSpan / 2.0f) - 4.0f;
    float centerX = x + boxW / 2.0f;

    for (size_t i = 0; i < lines.size(); ++i) {
        int stringWidth = 0;
        for (char c : lines[i]) {
            stringWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_12, c);
        }

        int textX = (int)(centerX - stringWidth / 2.0f);

        if (i == 0) { // Title
            if (isNight) glColor4f(0.0f, 1.0f, 1.0f, 1.0f);
            else glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

            // Shadow effect
            glRasterPos2i(textX, (int)currentY);
            for (char c : lines[i]) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
            glRasterPos2i(textX + 1, (int)currentY);
            for (char c : lines[i]) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);

            currentY -= (lineHeight + titleGap);
        }
        else { // Instructions
            glColor4f(0.9f, 0.9f, 0.9f, 1.0f);
            glRasterPos2i(textX, (int)currentY);
            for (char c : lines[i]) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
            currentY -= lineHeight;
        }
    }

    glPopAttrib();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}


// INITIALIZATION
void initOrbitalSystem() {
    gTransportRings.clear();
    TransportRing r1; r1.radius = 35.0f; r1.height = 6.0f; r1.rotateSpeed = 0.1f;
    r1.trains = {
        {0.0f,0.6f,3,1.0f,0.2f,0.0f},
        {1.2f,0.7f,2,0.0f,1.0f,0.8f},
        {2.5f,0.5f,3,1.0f,0.0f,0.8f},
        {4.0f,0.6f,3,1.0f,0.9f,0.0f},
        {5.5f,0.55f,2,0.0f,0.4f,1.0f}
    };
    gTransportRings.push_back(r1);

    TransportRing r2; r2.radius = 50.0f; r2.height = 10.0f; r2.rotateSpeed = -0.05f;
    r2.trains = {
        {0.0f,-0.2f,10,0.9f,0.1f,0.1f},
        {2.1f,-0.25f,8,0.1f,0.8f,0.1f},
        {4.2f,-0.2f,12,0.6f,0.0f,1.0f}
    };
    gTransportRings.push_back(r2);

    TransportRing r3; r3.radius = 65.0f; r3.height = 13.0f; r3.rotateSpeed = 0.02f;
    r3.trains = {
        {0.0f,0.3f,4,1.0f,0.0f,0.5f},
        {1.05f,0.3f,4,0.0f,1.0f,0.0f},
        {2.1f,0.3f,4,0.0f,0.5f,1.0f},
        {3.15f,0.3f,4,1.0f,0.5f,0.0f},
        {4.2f,0.3f,4,0.5f,0.0f,1.0f},
        {5.25f,0.3f,4,0.0f,1.0f,1.0f}
    };
    gTransportRings.push_back(r3);
}

void initForest(int count, float minR, float maxR) {
    gForest.clear();
    for (int i = 0; i < count; ++i) {
        Tree t;
        float angle = (rand() % 3600) / 10.0f * (PI / 180.0f);
        float randomRatio = (rand() % 10000) / 10000.0f;
        // Distribute trees using square root to avoid clustering at center
        float r = std::sqrt(randomRatio * (maxR * maxR - minR * minR) + (minR * minR));

        t.x = r * cos(angle);
        t.z = r * sin(angle);
        t.scale = 0.8f + (rand() % 70) / 100.0f;
        gForest.push_back(t);
    }
}

void initFloatingCity(int count) {
    gCityBlocks.clear();
    for (int i = 0; i < count; ++i) {
        FloatingBlock b;
        float angle = (rand() % 3600) / 10.0f * (PI / 180.0f);
        float r = 40.0f + (rand() % 60);

        b.x = r * cos(angle);
        b.z = r * sin(angle);
        b.y = 10.0f + (rand() % 60);
        b.width = 10.0f + (rand() % 10);
        b.height = 2.0f;
        b.depth = 10.0f + (rand() % 10);
        b.floatOffset = (rand() % 360) * (PI / 180.0f);
        b.floatSpeed = 0.01f + (rand() % 5) / 200.0f;

        int numBuildings = 2 + rand() % 4;
        for (int j = 0; j < numBuildings; ++j) {
            Building bld;
            bld.width = 2.0f + (rand() % 30) / 10.0f;
            bld.depth = 2.0f + (rand() % 30) / 10.0f;
            bld.height = 8.0f + (rand() % 100) / 10.0f;

            float maxX = std::max(0.0f, (b.width * 0.5f) - (bld.width * 0.5f));
            float maxZ = std::max(0.0f, (b.depth * 0.5f) - (bld.depth * 0.5f));

            bld.x = -maxX + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (2.0f * maxX + 0.01f)));
            bld.z = -maxZ + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (2.0f * maxZ + 0.01f)));

            b.buildings.push_back(bld);
        }
        gCityBlocks.push_back(b);
    }
}

void initConnections(float maxConnectionDistance) {
    gConnections.clear();
    for (size_t i = 0; i < gCityBlocks.size(); ++i) {
        int bestNeighbor = -1;
        float minDistance = 999999.0f;

        for (size_t j = 0; j < gCityBlocks.size(); ++j) {
            if (i == j) continue;
            float dx = gCityBlocks[i].x - gCityBlocks[j].x;
            float dz = gCityBlocks[i].z - gCityBlocks[j].z;
            float distSq = dx * dx + dz * dz;

            if (distSq < maxConnectionDistance * maxConnectionDistance && distSq < minDistance) {
                minDistance = distSq;
                bestNeighbor = (int)j;
            }
        }

        if (bestNeighbor != -1) {
            Connection c;
            c.fromIdx = (int)i;
            c.toIdx = bestNeighbor;
            c.maxDist = maxConnectionDistance;
            gConnections.push_back(c);
        }
    }
}

bool isTooCloseToCity(float x, float z, float minDistance) {
    for (const auto& block : gCityBlocks) {
        float dx = x - block.x;
        float dz = z - block.z;
        if (dx * dx + dz * dz < minDistance * minDistance) return true;
    }
    return false;
}

void initBillboards(int) {
    gBillboards.clear();
    if (gAdTextures.empty()) return;

    float minDist = 60.0f;
    for (size_t i = 0; i < gAdTextures.size(); ++i) {
        Billboard bb;
        bb.texIndex = (int)i;
        bb.width = 30.0f;
        bb.height = 20.0f;
        bb.angle = (rand() % 360);
        bb.floatOffset = (rand() % 360) * (PI / 180.0f);

        bool found = false;
        int attempts = 0;
        while (!found && attempts < 200) {
            attempts++;
            float angle = (rand() % 3600) / 10.0f * (PI / 180.0f);
            float r = (rand() % 110);

            bb.x = r * cos(angle);
            bb.z = r * sin(angle);
            bb.y = 40.0f + (rand() % 20);

            if (isTooCloseToCity(bb.x, bb.z, 25.0f)) continue;

            bool tooClose = false;
            for (const auto& ex : gBillboards) {
                if ((bb.x - ex.x) * (bb.x - ex.x) + (bb.z - ex.z) * (bb.z - ex.z) < minDist * minDist) {
                    tooClose = true;
                    break;
                }
            }
            if (!tooClose) found = true;
        }
        if (found) gBillboards.push_back(bb);
    }
}


// INPUT LOGIC
void updateCar() {
    float rad = carYaw * (PI / 180.0f);
    float nextX = carX;
    float nextY = carY;
    float nextZ = carZ;

    if (keyState['w'] || keyState['W']) {
        nextX -= sin(rad) * carSpeed;
        nextZ -= cos(rad) * carSpeed;
    }
    if (keyState['s'] || keyState['S']) {
        nextX += sin(rad) * carSpeed;
        nextZ += cos(rad) * carSpeed;
    }
    if (keyState[' ']) nextY += carSpeed;
    if (keyState['q'] || keyState['Q']) nextY -= carSpeed;
    if (keyState['a'] || keyState['A']) carYaw += 0.5f;
    if (keyState['d'] || keyState['D']) carYaw -= 0.5f;

    // Boundary Check
    float distSq = nextX * nextX + nextY * nextY + nextZ * nextZ;
    if (distSq < (domeRadius - 5.0f) * (domeRadius - 5.0f) && nextY > 2.0f) {
        carX = nextX;
        carY = nextY;
        carZ = nextZ;
    }
}

void display() {
    updateCar();
    gTime += 1.0f;

    // Setup environment colors (Fog and Clear)
    if (isNight) {
        glClearColor(0.05f, 0.0f, 0.1f, 1.0f);
        if (enableFog) {
            GLfloat c[] = { 0.1f, 0.0f, 0.2f, 1.0f };
            glFogfv(GL_FOG_COLOR, c);
        }
    }
    else {
        glClearColor(0.6f, 0.7f, 0.9f, 1.0f);
        if (enableFog) {
            GLfloat c[] = { 0.6f, 0.7f, 0.9f, 1.0f };
            glFogfv(GL_FOG_COLOR, c);
        }
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Setup Projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, glutGet(GLUT_WINDOW_WIDTH) / (double)glutGet(GLUT_WINDOW_HEIGHT), 0.1, 1000.0);

    // Setup Camera
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Camera Smoothing
    camSmoothYaw = lerp(camSmoothYaw, carYaw, 0.1f);

    // Calculate Camera Position based on Car Position
    float tDist = gDistance * 6.0f;
    float rYaw = toRad(camSmoothYaw);
    float rPitch = toRad(20.0f + gPitch);

    float camX = carX + sin(rYaw) * cos(rPitch) * tDist;
    float camY = carY + sin(rPitch) * tDist + 2.0f;
    float camZ = carZ + cos(rYaw) * cos(rPitch) * tDist;

    // Prevent Camera from clipping through the Sky Dome
    float distCenter = sqrt(camX * camX + camY * camY + camZ * camZ);
    if (distCenter > 95.0f) {
        float r = 95.0f / distCenter;
        camX *= r;
        camY *= r;
        camZ *= r;

        // Camera collision pushback against car
        float dCar = sqrt(pow(camX - carX, 2) + pow(camY - carY, 2) + pow(camZ - carZ, 2));
        if (dCar < 5.0f) {
            float slide = (5.0f - dCar) / 5.0f;
            camY += slide * 20.0f;
        }
    }

    gluLookAt(camX, camY, camZ, carX, carY + 2.0f, carZ, 0.0f, 1.0f, 0.0f);

    // Draw 3D Scene
    if (enableFog) glEnable(GL_FOG);

    drawSkyDome();
    drawGroundDiskPerfectlyAttached();
    drawForest();
    drawFloatingCity();

    if (enableFog) glDisable(GL_FOG); // Disable fog for glowing elements

    drawConnections();
    drawBillboards();
    drawOrbitalSystem();
    drawMyCar();

    // Draw 2D HUD
    drawTextHUD();

    glutSwapBuffers();
    glutPostRedisplay();
}

void reshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
}

void mouse(int button, int state, int x, int y) {
    if (button == 0) {
        dragging = (state == 0);
        lastMouseX = x;
        lastMouseY = y;
    }
    else if (button == 3 && state == 0) { // Wheel Up
        gDistance = std::max(0.5f, gDistance - 0.1f);
    }
    else if (button == 4 && state == 0) { // Wheel Down
        gDistance += 0.1f;
    }
}

void motion(int x, int y) {
    if (dragging) {
        gPitch += (y - lastMouseY) * 0.3f;
        if (gPitch > 17.0f) gPitch = 17.0f;
        if (gPitch < -10.0f) gPitch = -10.0f;
        lastMouseX = x;
        lastMouseY = y;
        glutPostRedisplay();
    }
}

void keyboard(unsigned char key, int, int) {
    if (key == 'n' || key == 'N') {
        isNight = !isNight;
    }
    else if (key == 27) { // ESC
        exit(0);
    }
    else {
        keyState[key] = true;
    }
}

void keyboardUp(unsigned char key, int, int) {
    keyState[key] = false;
}

int main(int argc, char** argv) {
    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(1224, 768);
    glutCreateWindow("Future City");

    // Initialize GL State
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);

    // Load Resources
    Image img;
    if (loadBMP("sky.bmp", img)) 
        gTexSky = createTexture(img, false);
    if (loadBMP("grass.bmp", img)) 
        gTexGround = createTexture(img, true);
    if (loadBMP("tree.bmp", img)) 
        gTexTree = createTexture(img, false, true);
    std::vector<std::string> adFiles = { "ad5.bmp", "ad2.bmp", "ad1.bmp", "ad4.bmp","ad3.bmp" };
    for (const auto& f : adFiles) {
        if (loadBMP(f, img)) gAdTextures.push_back(createTexture(img, false));
    }

    // Generate Procedural Content
    initForest(200, 20.0f, 90.0f);
    initFloatingCity(40);
    initConnections(25.0f);
    initBillboards(5);
    initOrbitalSystem();

    // Configure Fog
    if (enableFog) {
        glEnable(GL_FOG);
        glFogi(GL_FOG_MODE, GL_EXP2);
        GLfloat c[] = { 0.6f, 0.7f, 0.9f, 1.0f };
        glFogfv(GL_FOG_COLOR, c);
        glFogf(GL_FOG_DENSITY, 0.006f);
        glHint(GL_FOG_HINT, GL_NICEST);
    }

    // Register Callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);

    // Start Loop
    glutMainLoop();
    return 0;
}