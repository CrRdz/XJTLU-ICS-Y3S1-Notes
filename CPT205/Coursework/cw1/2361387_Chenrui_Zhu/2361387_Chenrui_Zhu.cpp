#define FREEGLUT_STATIC 
#include <GL/freeglut.h>
#include <string>
#include <vector>
#include <utility>
#include <math.h>

// ------------- Global variable ------------------
enum ArrowState { NONE, LEFT, RIGHT };
ArrowState hoveredArrow = NONE;

// Boolean flag indicating whether the mouse is hovering over the left arrow
bool hoverLeft = false; 
// Boolean flag indicating whether the mouse is hovering over the right arrow
bool hoverRight = false; 

// Current window width, initialized to 800 pixels
int windowWidth = 800; 
// Current window height, initialized to 600 pixels
int windowHeight = 600;

int page = 0; // Current page: 0 cover, 1 inner page

// Represents a single balloon in the scene
struct Balloon {
    float x, y;      // location
    float r, g, b;   // color
    float speed;     // Rate of ascent
    bool active;     // Whether it rises
};
// Container for all dynamically released balloons
std::vector<Balloon> balloons;
bool balloonsActive = false;  // whether it being released
float globalTime = 0.0f;  // For tail animation

// Represents a single particle of a firework
struct FireworkParticle {
    float x, y;
    float vx, vy;
    float r, g, b;
    float life;    // Remaining lifespan
    bool active;
};
// Container for all firework particles
std::vector<FireworkParticle> fireworks;
bool fireworksActive = false;  // whether the ceremonial cannon firing
bool autoFireOnPageChange = false; // Automatic Trigger upon Page Turn

// Represents a single ribbon particle launched from a launcher
struct RibbonParticle {
    float x, y;       // location
    float vx, vy;     // speed
    float r, g, b;    // color
    float life;       // remaining lifespan
    bool active;
};

// Represents a single ribbon launcher
struct Launcher {
    float x, y;       // launcher location
    float width, height;
    bool hover;
    bool active;      // whether ribbon being launched
    std::vector<RibbonParticle> particles;

    float r1, g1, b1; // top left color 
    float r2, g2, b2; // botoom color
    float r3, g3, b3; // top right color 
};
// Container holding all ribbon launchers in the scene
std::vector<Launcher> launchers;

// Represents a star in the background for decoration
struct Star {
    float x, y;   // coordinates
    float size;   // Size of the stars
};
// Container holding all stars in the background
std::vector<Star> stars;

// ------------- Initialize ------------------
// Initialize the launcher, determining its position, shape, and color.
void initLaunchers() {
    Launcher l1 = { 0.3f, -0.7f, 0.1f, 0.4f, false, false, 
        {}, 1.0f,0.6f,0.0f, 1.0f,0.3f,0.5f, 0.7f,0.0f,0.8f };
    Launcher l2 = { 0.5f, -0.7f, 0.1f, 0.4f, false, false, 
        {}, 0.2f,0.4f,1.0f, 0.0f,0.7f,0.6f, 0.0f,0.8f,0.5f };
    Launcher l3 = { 0.7f, -0.7f, 0.1f, 0.4f, false, false, 
        {}, 0.6f,0.4f,0.8f, 0.5f,0.6f,1.0f, 0.9f,0.9f,1.0f };
    Launcher l4 = { 0.9f, -0.7f, 0.1f, 0.4f, false, false, 
        {}, 1.0f,0.2f,0.2f, 1.0f,0.6f,0.0f, 1.0f,0.9f,0.0f };
    launchers.push_back(l1);
    launchers.push_back(l2);
    launchers.push_back(l3);
    launchers.push_back(l4);
}

// Initialize stars and determine their positions and sizes
void initStars(int numStars = 50) {
    stars.clear();
    for (int i = 0; i < numStars; ++i) {
        Star s;
        s.x = static_cast<float>(rand() % 2000 - 1000) / 1000.0f; // [-1,1] Normalized space
        s.y = static_cast<float>(rand() % 2000 - 1000) / 1000.0f; // [-1,1]
        s.size = static_cast<float>(rand() % 3 + 1) / 300.0f; // star size
        stars.push_back(s);
    }
}

// ------------- helpful tools ------------------
void setColor(float r, float g, float b) {
    glColor3f(r, g, b);
}

void drawQuad(float x1, float y1, float x2, float y2,
    float x3, float y3, float x4, float y4) {
    glBegin(GL_QUADS);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glVertex2f(x3, y3);
    glVertex2f(x4, y4);
    glEnd();
}

void drawTitleText(const std::string& text, float x, float y, float scale) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1.0f);
    for (char c : text)
        glutStrokeCharacter(GLUT_STROKE_ROMAN, c);
    glPopMatrix();
}

void drawInnerText(const std::string& text, float x, float y,
    void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (char c : text)
        glutBitmapCharacter(font, c);
}

void drawText(const std::string& text, float x, float y,
    void* font = GLUT_BITMAP_8_BY_13) {
    glRasterPos2f(x, y);
    for (char c : text)
        glutBitmapCharacter(font, c);
}

// draw backgrounds
void drawBackground(float r, float g, float b) {
    setColor(r, g, b);
    drawQuad(-1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f);
}

// Draw a gradient color background
void drawGradientBackground() {
    glBegin(GL_QUADS);
	// Top left corner - purple
    glColor3f(2.06f, 0.87f, 1.93f);
    glVertex2f(-1.0f, 1.0f);

	// Top right corner - purple 
    glColor3f(2.06f, 0.87f, 1.93f);
    glVertex2f(1.0f, 1.0f);

	// Bottom right corner - blue
    glColor3f(0.0f, 0.0f, 0.66f);
    glVertex2f(1.0f, -1.0f);

	// Bottom left corner - Blue
    glColor3f(0.0f, 0.0f, 0.66f);
    glVertex2f(-1.0f, -1.0f);
    glEnd();
}

// ----------------- Drawing part -----------------
// Vertices and lines needed for drawing cb building
std::vector<std::pair<float, float>> vertices = {
    {0,0},{2.3,0},{2.3,3},{2.8,3},{2.8,4.5},{2.3,4.5},
    {2.3,8.5},{4.3,10},{4.6,8.5},{5.2,9},{5.4,10.5},
    {10,14.5},{10,8.5},{7.7,6.5},{3.3,4.3},{3.3,2.8},
    {4,3},{5.8,4.5},{10,6},{10,0},{16,0},{16,0.5},
    {18.5,0.5},{18.5,1},{14,5},{10,6}
};

std::vector<std::pair<float, float>> smallPoly1 = {{ 10,8.5 },{ 15,6.5 }, { 15,11 }, { 10,14.5 }};

std::vector<std::pair<float, float>> smallPoly2 = {{15.6,10.2},{18.5,8},{18.5,3},{17,3}};

std::vector<std::pair<float, float>> smallLine1 = {{18.5,0.5},{19.4,0.5},{19.4,0},{0,0}};

std::vector<std::pair<float, float>> smallLine2 = {{21,0},{0,0}};

std::vector<std::pair<float, float>> smallLine3 = {{18,3},{18,1.5}};

std::vector<std::pair<float, float>> smallLine4 = {{10,0},{10,6}};

std::vector<std::pair<float, float>> smallLine5 = {{10,8.5},{10,14.5}};

// draw cb building
void drawComplexShape(float tx = 0, float ty = 0, float scale = 1.0f) {
    glPushMatrix();

    glTranslatef(tx, ty, 0.0f);
    glScalef(scale, scale, 1.0f);

    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(1.5f);

    glBegin(GL_LINE_STRIP);
    for (auto& p : vertices) glVertex2f(p.first, p.second);
    glEnd();

    glBegin(GL_LINE_LOOP);
    for (auto& p : smallPoly1) glVertex2f(p.first, p.second);
    glEnd();

    glBegin(GL_LINE_LOOP);
    for (auto& p : smallPoly2) glVertex2f(p.first, p.second);
    glEnd();

    auto drawLine = [](std::vector<std::pair<float, float>>& pts) {
        glBegin(GL_LINE_STRIP);
        for (auto& p : pts) glVertex2f(p.first, p.second);
        glEnd();
    };
    drawLine(smallLine1);
    drawLine(smallLine2);
    drawLine(smallLine3);
    drawLine(smallLine4);
    drawLine(smallLine5);

    glPopMatrix();
}

// draw firework particles
void spawnFirework(float baseX = 0.0f, float baseY = -0.9f) {
    fireworks.clear();
	int numParticles = 50 + rand() % 50;  // 50~100 particles
    for (int i = 0; i < numParticles; ++i) {
        FireworkParticle p;
        p.x = baseX;
        p.y = baseY;
        float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * 3.14159f;
        float speed = 0.03f + static_cast<float>(rand()) / RAND_MAX * 0.07f;
        p.vx = cos(angle) * speed;
        p.vy = sin(angle) * speed;
        p.r = static_cast<float>(rand()) / RAND_MAX;
        p.g = static_cast<float>(rand()) / RAND_MAX;
        p.b = static_cast<float>(rand()) / RAND_MAX;
		p.life = 2.0f;  // initial lifespan
        p.active = true;
        fireworks.push_back(p);
    }
    fireworksActive = true;
}

// update firework particles
void updateFireworks() {
    if (!fireworksActive) return;
    for (auto& p : fireworks) {
        if (!p.active) continue;
        p.x += p.vx;
        p.y += p.vy;
        p.vy -= 0.001f;  // gravity
        p.life -= 0.02f;
        if (p.life <= 0.0f) p.active = false;
    }
}

// draw firework 
void drawFireworks() {
    // Exit early if no fireworks are active
    if (!fireworksActive) return;

    // Loop through all firework particles
    for (auto& p : fireworks) {
        if (!p.active) continue;
        glColor3f(p.r, p.g, p.b);

        // Draw the particle as a small square (quad)
        glBegin(GL_QUADS);
        float size = 0.01f;
        glVertex2f(p.x - size, p.y - size);
        glVertex2f(p.x + size, p.y - size);
        glVertex2f(p.x + size, p.y + size);
        glVertex2f(p.x - size, p.y + size);
        glEnd();
    }
}

// draw launchers
void drawLaunchers() {
    for (auto& l : launchers) {
        glPushMatrix();
        glTranslatef(l.x, l.y, 0.0f);

        if (l.hover) glScalef(1.2f, 1.2f, 1.0f);

		// draw triangle body
        glBegin(GL_TRIANGLES);
        glColor3f(l.r1, l.g1, l.b1);
        glVertex2f(-l.width / 2, l.height);
        glColor3f(l.r2, l.g2, l.b2);
        glVertex2f(0.0f, 0.0f);
        glColor3f(l.r3, l.g3, l.b3);
        glVertex2f(l.width / 2, l.height);
        glEnd();

		// draw golden rim
        glColor3f(0.95f, 0.85f, 0.2f);
        float goldHeight = 0.015f;
        glBegin(GL_QUADS);
        glVertex2f(-l.width / 2, l.height);
        glVertex2f(l.width / 2, l.height);
        glVertex2f(l.width / 2, l.height - goldHeight);
        glVertex2f(-l.width / 2, l.height - goldHeight);
        glEnd();

        glPopMatrix();

		// Particle spawn position
        l.x; 
        l.y + l.height;
    }
}

// spawn launcher ribbons
void spawnLauncherRibbons(Launcher& l) {
    l.particles.clear();
    int num = 40 + rand() % 20;

	// Generate particles
    for (int i = 0; i < num; ++i) {
        RibbonParticle p;
        p.x = l.x;
        p.y = l.y + l.height;

        float angle = (rand() % 40 - 20) * 3.14159f / 180.0f;
        float speed = 0.02f + static_cast<float>(rand()) / RAND_MAX * 0.03f;

        p.vx = sin(angle) * speed;
        p.vy = cos(angle) * speed;
        p.r = static_cast<float>(rand()) / RAND_MAX;
        p.g = static_cast<float>(rand()) / RAND_MAX;
        p.b = static_cast<float>(rand()) / RAND_MAX;
        p.life = 1.5f;
        p.active = true;

        l.particles.push_back(p);
    }
    l.active = true;
}

// draw launcher ribbons
void drawLaunchersRibbons() {
    for (auto& l : launchers) {
        if (!l.active) continue;
        for (auto& p : l.particles) {
            if (!p.active) continue;
            glColor3f(p.r, p.g, p.b);
            glBegin(GL_LINES);
            glVertex2f(p.x, p.y);
            glVertex2f(p.x - p.vx * 5, p.y - p.vy * 5);
            glEnd();
        }
    }
}

// update launcher ribbons
void updateLaunchersRibbons() {
    for (auto& l : launchers) {
        if (!l.active) continue;
        for (auto& p : l.particles) {
            if (!p.active) continue;
            p.x += p.vx;
            p.y += p.vy;
            p.vy -= 0.001f;
            p.life -= 0.02f;
            if (p.life <= 0) p.active = false;
        }
    }
}

// draw flowers at bottom
// draw single petal
void drawPetal(float centerX, float centerY, float radiusX, float radiusY, float rotationAngle, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(centerX, centerY);

	// draw petal shape
    for (float angle = 0; angle <= 2 * 3.14159f; angle += 0.1f) {
        float x = radiusX * cos(angle);
        float y = radiusY * sin(angle);
        float rotatedX = centerX + x * cos(rotationAngle) - y * sin(rotationAngle);
        float rotatedY = centerY + x * sin(rotationAngle) + y * cos(rotationAngle);
        glVertex2f(rotatedX, rotatedY);
    }
    glEnd();
}

// draw whole flower
void drawFlower(float centerX, float centerY, int numPetals, float petalRadiusX, float petalRadiusY, float r, float g, float b) {
    for (int i = 0; i < numPetals; ++i) {
        float angle = i * (2 * 3.14159f / numPetals);
        drawPetal(centerX, centerY, petalRadiusX, petalRadiusY, angle, r, g, b);
    }

	// center of flower
	glColor3f(1.0f, 0.9f, 0.0f); // yellow center
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(centerX, centerY);
    for (float a = 0; a <= 2 * 3.1415f; a += 0.1f) {
        glVertex2f(centerX + 0.01f * cos(a), centerY + 0.01f * sin(a));
    }
    glEnd();
}

// draw multiple flowers at bottom
void drawFlowersAtBottom() {
	// the position of flowers 
    float y = -0.9f;
    
    drawFlower(-0.8f, y, 6, 0.04f, 0.02f, 1.0f, 0.5f, 0.6f);
    drawFlower(-0.5f, y, 5, 0.045f, 0.02f, 0.8f, 0.7f, 1.0f);
    drawFlower(-0.2f, y, 7, 0.04f, 0.018f, 0.6f, 1.0f, 0.6f);
    drawFlower(0.1f, y, 6, 0.045f, 0.02f, 1.0f, 0.8f, 0.5f);
    drawFlower(0.4f, y, 5, 0.04f, 0.018f, 0.7f, 0.9f, 1.0f);
    drawFlower(0.7f, y, 6, 0.045f, 0.02f, 1.0f, 0.6f, 0.8f);
}

// draw stars in background
void drawStars() {
	setColor(1.0f, 1.0f, 1.0f); // white stars
    for (auto& s : stars) {
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(s.x, s.y);
        for (float angle = 0; angle <= 2 * 3.14159f; angle += 0.1f) {
            float dx = s.size * cos(angle);
            float dy = s.size * sin(angle);
            glVertex2f(s.x + dx, s.y + dy);
        }
        glEnd();
    }
}

// draw balloon
// draw single balloon with dynamic tail
void drawBalloon(const Balloon& b)
{
	// balloon body
    glPushMatrix();
    glTranslatef(b.x, b.y, 0.0f);

	// draw oval shape
    glColor3f(b.r, b.g, b.b);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.0f, 0.0f);
    for (int i = 0; i <= 360; i += 10)
    {
        float theta = i * 3.1415926f / 180.0f;
        glVertex2f(0.05f * cos(theta), 0.07f * sin(theta));
    }
    glEnd();

    glPopMatrix();

	// draw balloon tail
    glColor3f(0.2f, 0.2f, 0.2f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_STRIP);

    // Starting from the bottom of the balloon (b.x, b.y - 0.05)
    float amplitude = 0.01f;
    float waveLength = 3.5f;
    float length = 0.20f;
    int segments = 50;

    for (int i = 0; i <= segments; i++)
    {
        float t = (float)i / segments;  // 0~1
        float fade = 1.0f - t;          // Gradually tapering at the end (swinging smaller and smaller)

        float y = b.y - 0.07f - t * length;
        float x = b.x + 0.002f + amplitude * fade * sin(waveLength * t + globalTime + b.y * 10.0f);
        glVertex2f(x, y);
    }
    glEnd();
}

// draw static balloons 
void drawStaticBalloons() {
    std::vector<Balloon> staticBalloons = {
		{ -0.9f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, false },  // purple balloon
		{ -0.6f, 0.4f, 1.0f, 0.4f, 0.4f, 0.0f, false },  // red balloon
        { -0.3f, 0.6f, 0.4f, 0.8f, 1.0f, 0.0f, false },  // blue balloon
        {  0.0f, 0.3f, 0.9f, 0.8f, 0.3f, 0.0f, false },  // yellow balloon
		{  0.13f, 0.7f, 1.0f, 0.6f, 0.2f, 0.0f, false },  // Orange balloon
        {  0.3f, 0.5f, 0.7f, 0.6f, 1.0f, 0.0f, false },  // Light purple balloon
        {  0.7f, 0.5f, 0.5f, 1.0f, 0.6f, 0.0f, false }   // Green balloon
    };

    for (auto& b : staticBalloons) {
        drawBalloon(b);
    }
}

// spawn a new balloon
void spawnBalloon() {
    Balloon b;
    b.x = -0.8f + static_cast<float>(rand() % 160) / 100.0f; // [-0.8, 0.8]
	b.y = -1.1f; // from bottom
    b.r = static_cast<float>(rand() % 100) / 100.0f;
    b.g = static_cast<float>(rand() % 100) / 100.0f;
    b.b = static_cast<float>(rand() % 100) / 100.0f;
	b.speed = 0.002f + static_cast<float>(rand() % 10) / 5000.0f; // 0.002 ~ 0.004
    b.active = true;
    balloons.push_back(b);
}

// update balloons position
void updateBalloons() {
    if (!balloonsActive) return;

    for (auto& b : balloons) {
        if (b.active) {
            b.y += b.speed;
            if (b.y > 1.2f) b.active = false; // Disappear when exceeding the top
        }
    }
}

// draw arrows for page turning
void drawArrows(bool isLeft) {
    glPushMatrix();

	float arrowX = isLeft ? -0.875f : 0.875f;  // center of arrow X
	float arrowY = -0.8f;                      // center of arrow Y
    bool isHover = (isLeft && hoverLeft) || (!isLeft && hoverRight);

    // Pan to the center of the arrow and then zoom in
    glTranslatef(arrowX, arrowY, 0.0f);
    if (isHover)
        glScalef(1.3f, 1.3f, 1.0f); // Zoom in 30%

    // Move back to the local origin to draw
    glTranslatef(-arrowX, -arrowY, 0.0f);

    // Brightens on hover
    if (isHover)
        setColor(0.6f, 0.6f, 0.6f);
    else
        setColor(0.2f, 0.2f, 0.2f);

    if (isLeft) {
        glBegin(GL_TRIANGLES);
        glVertex2f(-0.9f, -0.8f);
        glVertex2f(-0.85f, -0.75f);
        glVertex2f(-0.85f, -0.85f);
        glEnd();
    }
    else {
        glBegin(GL_TRIANGLES);
        glVertex2f(0.9f, -0.8f);
        glVertex2f(0.85f, -0.75f);
        glVertex2f(0.85f, -0.85f);
        glEnd();
    }

    glPopMatrix();
}

void drawSymmetricFivePetal(float centerX, float centerY, float radius = 0.05f) {
    const int segments = 200; 
    const int petalCount = 5;

    glColor3f(1.0f, 1.0f, 1.0f); 

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(centerX, centerY);

    for (int i = 0; i <= segments; ++i) {
        float theta = (2.0f * 3.1415926f * i) / segments;
        // r = radius * (0.5 + 0.5 * cos(petalCount * theta))
        float r = radius * (0.5f + 0.5f * cos(petalCount * theta));
        float x = centerX + r * cos(theta);
        float y = centerY + r * sin(theta);
        glVertex2f(x, y);
    }

    glEnd();
}


// draw title page
void drawTitlePage() {
	drawGradientBackground(); // use gradient background
    drawStars();
    drawFlowersAtBottom();
    
    setColor(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    drawTitleText("HAPPY 20th Anniversary Of XJTLU", -0.8f, 0.0f, 0.0007f);
    
    setColor(0.0f, 0.0f, 0.0f);
    drawInnerText("2006 - 2025 | Xian-Jiaotong Liverpool University", -0.5f, 0.8f);
}

// draw inner page
void drawInnerPage() {
    drawBackground(0.97f, 0.90f, 0.80f);
    setColor(0.0f, 0.0f, 0.0f);
    
    glLineWidth(4.0f);
    drawInnerText("Wishing XJTLU a More Fantastic Future!", 0.1f, -0.1f);
    drawInnerText("| Light and Wings | 2006-2025", -0.9f, 0.85f); 
    drawText("Press b/B to release the balloon | Press f/F to release the ribbons | Press Esc to exit", -0.92f, -0.95f);
    drawText("CLICK ? BOOM!", 0.47f, -0.75f);

    glPushMatrix();
    // Translate and scale to adjust position
    glTranslatef(-0.8f, -0.8f, 0.0f);   // Move down to the left
    glScalef(0.05f, 0.07f, 1.0f);       // Scale to appropriate size
    drawComplexShape();                 // Call the building drawing function
    glPopMatrix();

    // Draw a balloon
    if (balloonsActive) {
        for (auto& b : balloons)
            if (b.active) drawBalloon(b);
    }
    drawStaticBalloons();

    drawSymmetricFivePetal(-0.7f, 0.5f, 0.04f);  
    drawSymmetricFivePetal(-0.2f, 0.3f, 0.03f);  
    drawSymmetricFivePetal(0.2f, 0.6f, 0.035f);  
    drawSymmetricFivePetal(0.5f, 0.2f, 0.04f);   
    drawSymmetricFivePetal(0.7f, 0.7f, 0.06f);  

    drawLaunchers();
    drawLaunchersRibbons();
    updateLaunchersRibbons();
}

// ----------------- render -----------------
void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    switch (page) {
    case 0:
        drawTitlePage();
        drawArrows(false); // Only show the right arrow for turning pages
        break;
    case 1:
        drawInnerPage();
        drawArrows(true);  // Only show the left arrow for turning pages
        break;
    }
    updateBalloons();

    drawFireworks();  // Added fireworks drawing
    updateFireworks(); // Update fireworks particle

    glutSwapBuffers();
}

// ----------------- mouse -----------------
void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        int win_width = glutGet(GLUT_WINDOW_WIDTH);
        int win_height = glutGet(GLUT_WINDOW_HEIGHT);

        float normalizedX = static_cast<float>(x - win_width / 2) / (win_width / 2);
        float normalizedY = static_cast<float>(win_height / 2 - y) / (win_height / 2);

        float nx = (2.0f * x / win_width) - 1.0f;
        float ny = 1.0f - (2.0f * y / win_height);

        // Check the firework tube click
        for (auto& l : launchers) {
            if (l.hover) {
                spawnLauncherRibbons(l);  // Pass in the corresponding fireworks tube
            }
        }

        // Left arrow to turn the page
        if (page == 1 && normalizedX >= -0.92f && normalizedX <= -0.83f &&
            normalizedY >= -0.87f && normalizedY <= -0.73f) {
            page = 0;
            autoFireOnPageChange = true; // Flipping the page triggers a firworks
        }
        // right arrow to turn the page
        else if (page == 0 && normalizedX >= 0.83f && normalizedX <= 0.92f &&
            normalizedY >= -0.87f && normalizedY <= -0.73f) {
            page = 1;
            autoFireOnPageChange = true; // Flipping the page triggers a firworks
        }

        glutPostRedisplay();
    }
}

void checkLaunchersHover(float nx, float ny) {
    for (auto& l : launchers) {
        float halfWidth = l.width / 2.0f;
        float topY = l.y + l.height;
        l.hover = (nx > l.x - halfWidth && nx < l.x + halfWidth &&
            ny > l.y && ny < topY);
    }
}

void MouseMove(int x, int y) {
    windowWidth = glutGet(GLUT_WINDOW_WIDTH);
    windowHeight = glutGet(GLUT_WINDOW_HEIGHT);

    float nx = (2.0f * x / windowWidth) - 1.0f;   // normalized
    float ny = 1.0f - (2.0f * y / windowHeight);  // Y Flip

    hoverLeft = (nx > -0.95f && nx < -0.8f && ny > -0.85f && ny < -0.75f);
    hoverRight = (nx > 0.8f && nx < 0.95f && ny > -0.85f && ny < -0.75f);
    checkLaunchersHover(nx, ny);
    glutPostRedisplay();

}

void OnTimer(int value) {
    // Turning the page triggers automatic cannon firing
    if (autoFireOnPageChange) {
        spawnFirework(0.0f, -0.9f); // Central bottom launch
        autoFireOnPageChange = false; // Reset
    }
    
    updateBalloons();       // Update balloon position
    updateFireworks();      // Update fireworks particle
    glutPostRedisplay();    // Request to redraw
    glutTimerFunc(16, OnTimer, 1); // Called every 16ms (~60 frames per second)
}

// ----------------- Keyboard processing -----------------
void handleKeypress(unsigned char key, int x, int y) {
    // Press 'Esc' to exit.
    if (key == 27) exit(0);

    if (page == 1 && (key == 'b' || key == 'B')) {
        balloonsActive = true;
        spawnBalloon(); // Press 'b'/'B' each time to generate a new balloon.
    }
    if (key == 'f' || key == 'F') { // Press the 'f'/'F' key to launch the salute cannon
        spawnFirework(0.0f, -0.9f);
    }
}

// ----------------- init window -----------------
void initWindow() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
}

// ----------------- main -----------------
int main(int argc, char** argv) {
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(800, 500);
    glutCreateWindow("Greeting Card for 20th anniversary of XJTLU");
    
    initWindow();
    initStars();
    initLaunchers();

    glutDisplayFunc(renderScene);
    glutMouseFunc(mouse);
    glutPassiveMotionFunc(MouseMove);
    glutKeyboardFunc(handleKeypress);

    glutTimerFunc(16, OnTimer, 1);

    glutMainLoop();
    return 0;
}
