#include <iostream>
#include <GL/freeglut.h>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>

// ===================================
// Global constants and configuration
// ===================================

// Math constants
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float DEG2RAD = PI / 180.0f;

// Window settings
int windowWidth = 600;
int windowHeight = 800;

// =======================
// Global state variables
// =======================
// Mode/state flags
bool is_day = true;
bool show_greeting = false;
bool isCoverVisible = true;
bool areLightsOn = false;

// UI
bool showHints = true;

// Postcard mode / flash
bool isPostcardMode = false;
bool isFlashing = false;
float flashAlpha = 0.0f;
int flashFrameCount = 0;
const int FLASH_DURATION = 15; // frames

// Cover ring animation
float ringRotationAngle = 0.0f;
bool isAnimating = false;
bool isOpening = false; // true = opening, false = closing

// Sun / Moon
float sunPosX = 500.0f;
float sunPosY = 700.0f;
float sunRadius = 50.0f;

// Clouds animation
float cloud1_posX = -10.0f;
float cloud2_posX = 150.0f;
float cloud3_posX = 500.0f;

// Greeting text fade
float greeting_alpha = 0.0f;

// Building transform
float buildingPosX = 152.0f;
float buildingPosY = 300.0f;
float buildingScaleX = 0.33f;
float buildingScaleY = 0.33f;

// Shadow appearance
const float SHADOW_OFFSET = 7.0f;
const float SHADOW_ALPHA_DAY = 0.15f;
const float SHADOW_ALPHA_NIGHT = 0.2f;

// View (zoom and pan)
const float MIN_LEFT = 0.0f;
const float MIN_RIGHT = 600.0f;
const float MIN_BOTTOM = 0.0f;
const float MIN_TOP = 800.0f;

float viewLeft = MIN_LEFT;
float viewRight = MIN_RIGHT;
float viewBottom = MIN_BOTTOM;
float viewTop = MIN_TOP;

float zoomFactor = 1.0f;
const float ZOOM_STEP = 0.1f;
const float MIN_ZOOM = 1.0f;
const float MAX_ZOOM = 3.0f;

const float PAN_STEP = 20.0f;

// =================
// Data structures
// =================

// Balloon structure
struct Balloon {
    float x, y;                 // Position
    float size;                 // Size multiplier
    float colorR, colorG, colorB;  // Color
    float speed;                // Rising speed
    bool active;                // Active state

    Balloon(float _x, float _y, float _size, float r, float g, float b, float spd)
        : x(_x), y(_y), size(_size), colorR(r), colorG(g), colorB(b), speed(spd), active(true) {}
};

std::vector<Balloon> balloons;

// Star structure
struct Star {
    float x, y;
    float brightness;
    float twinkleSpeed;
};

std::vector<Star> stars;
float starTwinklePhase = 0.0f;

// Balloon color presets
struct BalloonColor { float r, g, b; };

const BalloonColor balloonColors[] = {
    {1.0f, 0.3f, 0.3f},    // Red
    {1.0f, 0.6f, 0.2f},    // Orange
    {1.0f, 0.9f, 0.3f},    // Yellow
    {0.4f, 0.9f, 0.5f},    // Green
    {0.3f, 0.7f, 1.0f},    // Blue
    {1.0f, 0.4f, 0.7f},    // Pink
    {0.5f, 0.3f, 0.9f},    // Purple
};
const int numBalloonColors = sizeof(balloonColors) / sizeof(balloonColors[0]);

// ===========================
//  Primitive drawing helpers
// ===========================
// Draws a filled circle at (cx, cy) with radius r
void drawCircle(float cx, float cy, float r) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 360; i++) {
        float angle = i * DEG2RAD;
        glVertex2f(cx + r * cos(angle), cy + r * sin(angle));
    }
    glEnd();
}

// Draws a feathered (blurred edge) filled circle using layered circles
void drawFeatheredCircle(float cx, float cy, float radius, float feather,
    float r, float g, float b, float a, int layers = 10)
{
    for (int i = 0; i < layers; ++i) {
        float currentRadius = radius - (static_cast<float>(i) / layers) * feather;
        float currentAlpha = a * (static_cast<float>(i + 1) / layers);
        glColor4f(r, g, b, currentAlpha);
        drawCircle(cx, cy, currentRadius);
    }
}

// Draws a slanted stripe (quad) along a segment
void drawSlantedStripe(float x1, float y1, float x2, float y2, float height)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    float length = sqrt(dx * dx + dy * dy);
    if (length == 0) return;

    float perp_dx = -dy / length * (height / 2.0f);
    float perp_dy =  dx / length * (height / 2.0f);

    float v1x = x1 + perp_dx; float v1y = y1 + perp_dy;
    float v2x = x2 + perp_dx; float v2y = y2 + perp_dy;
    float v3x = x2 - perp_dx; float v3y = y2 - perp_dy;
    float v4x = x1 - perp_dx; float v4y = y1 - perp_dy;

    glBegin(GL_QUADS);
    glVertex2f(v1x, v1y);
    glVertex2f(v2x, v2y);
    glVertex2f(v3x, v3y);
    glVertex2f(v4x, v4y);
    glEnd();
}

// Draws a quadratic Bezier curve passing through points p0, p1, p2
void drawQuadraticBezier(float p0x, float p0y, float p1x, float p1y, float p2x, float p2y)
{
    float ctrl_x = 2.0f * p1x - 0.5f * p0x - 0.5f * p2x;
    float ctrl_y = 2.0f * p1y - 0.5f * p0y - 0.5f * p2y;

    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= 20; ++i) {
        float t = static_cast<float>(i) / 20.0f;
        float u = 1.0f - t;
        float x = u * u * p0x + 2.0f * u * t * ctrl_x + t * t * p2x;
        float y = u * u * p0y + 2.0f * u * t * ctrl_y + t * t * p2y;
        glVertex2f(x, y);
    }
    glEnd();
}

// Rasterizes a small text string at (x, y)
void drawText(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
}

// Draws a semi-transparent tape corner quad at (x, y) rotated by rotation degrees
void drawTapeCorner(float x, float y, float width, float height, float rotation) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glRotatef(rotation, 0.0f, 0.0f, 1.0f);

    glColor4f(1.0f, 1.0f, 1.0f, 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(-width / 2.0f, -height / 2.0f);
    glVertex2f( width / 2.0f, -height / 2.0f);
    glVertex2f( width / 2.0f,  height / 2.0f);
    glVertex2f(-width / 2.0f,  height / 2.0f);
    glEnd();

    glPopMatrix();
}

// =====================
//   Center building
// =====================

// Draws the lower layer of the building
void drawBuilding_LowerLayer()
{
    if (!is_day && areLightsOn) {
        glColor3f(0.98f, 0.82f, 0.49f);
        glBegin(GL_POLYGON);
        glVertex2f(50.0f, 0.0f);
        glVertex2f(50.0f, 550.0f);
        glVertex2f(450.0f, 750.0f);
        glVertex2f(450.0f, 0.0f);
        glEnd();

        glColor3f(0.9f, 0.69f, 0.37f);
        glBegin(GL_POLYGON);
        glVertex2f(450.0f, 0.0f);
        glVertex2f(450.0f, 750.0f);
        glVertex2f(850.0f, 550.0f);
        glVertex2f(850.0f, 0.0f);
        glEnd();
    } else {
        glColor3f(0.68f, 0.73f, 0.79f);
        glBegin(GL_POLYGON);
        glVertex2f(50.0f, 0.0f);
        glVertex2f(50.0f, 550.0f);
        glVertex2f(450.0f, 750.0f);
        glVertex2f(450.0f, 0.0f);
        glEnd();

        glColor3f(0.25f, 0.46f, 0.62f);
        glBegin(GL_POLYGON);
        glVertex2f(450.0f, 0.0f);
        glVertex2f(450.0f, 750.0f);
        glVertex2f(850.0f, 550.0f);
        glVertex2f(850.0f, 0.0f);
        glEnd();
    }
}

// Draws lighter upper building parts
void drawBuilding_UpperLayer_Light()
{
    glColor3f(0.91f, 0.88f, 0.84f);

    glBegin(GL_POLYGON);
    glVertex2f(0.0f, 300.0f);
    glVertex2f(0.0f, 550.0f);
    glVertex2f(200.0f, 650.0f);
    glVertex2f(200.0f, 350.0f);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(0.0f, 250.0f);
    glVertex2f(150.0f, 285.0f);
    glVertex2f(450.0f, 100.0f);
    glVertex2f(450.0f, 0.0f);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(250.0f, 250.0f);
    glVertex2f(250.0f, 675.0f);
    glVertex2f(450.0f, 775.0f);
    glVertex2f(450.0f, 300.0f);
    glEnd();
}

// Draws darker upper building parts
void drawBuilding_UpperLayer_Dark()
{
    glColor3f(0.56f, 0.53f, 0.51f);

    glBegin(GL_POLYGON);
    glVertex2f(450.0f, 300.0f);
    glVertex2f(450.0f, 775.0f);
    glVertex2f(600.0f, 700.0f);
    glVertex2f(650.0f, 500.0f);
    glVertex2f(700.0f, 490.0f);
    glVertex2f(700.0f, 400.0f);
    glVertex2f(650.0f, 250.0f);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(700.0f, 650.0f);
    glVertex2f(900.0f, 550.0f);
    glVertex2f(900.0f, 250.0f);
    glVertex2f(800.0f, 260.0f);
    glVertex2f(750.0f, 450.0f);
    glVertex2f(700.0f, 400.0f);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(450.0f, 0.0f);
    glVertex2f(450.0f, 100.0f);
    glVertex2f(800.0f, 200.0f);
    glVertex2f(900.0f, 200.0f);
    glVertex2f(900.0f, 0.0f);
    glEnd();
}

// Draws decorative slanted stripes on the building
void drawBuilding_Stripes()
{
    const float stripeHeight = 15.0f;

    glColor3f(0.73f, 0.65f, 0.53f);
    drawSlantedStripe(0.0f, 50.0f, 400.0f, 50.0f, stripeHeight);
    drawSlantedStripe(0.0f, 100.0f, 300.0f, 100.0f, stripeHeight);
    drawSlantedStripe(300.0f, 650.0f, 450.0f, 725.0f, stripeHeight);
    drawSlantedStripe(300.0f, 600.0f, 450.0f, 675.0f, stripeHeight);
    drawSlantedStripe(300.0f, 550.0f, 450.0f, 625.0f, stripeHeight);
    drawSlantedStripe(0.0f, 500.0f, 150.0f, 575.0f, stripeHeight);
    drawSlantedStripe(0.0f, 450.0f, 100.0f, 500.0f, stripeHeight);

    glColor3f(0.37f, 0.33f, 0.28f);
    drawSlantedStripe(650.0f, 50.0f, 900.0f, 50.0f, stripeHeight);
    drawSlantedStripe(500.0f, 100.0f, 700.0f, 100.0f, stripeHeight);
}

// Draws the complete CB
void drawXJTLUCenterBuilding()
{
    glPushMatrix();
    glTranslatef(buildingPosX, buildingPosY, 0.0f);
    glScalef(buildingScaleX, buildingScaleY, 1.0f);

    drawBuilding_LowerLayer();
    drawBuilding_UpperLayer_Light();
    drawBuilding_UpperLayer_Dark();
    drawBuilding_Stripes();

    glPopMatrix();
}

// ===============
//   Environment  
// ===============

// Draws the sky gradient (day or night)
void drawSky() {
    if (is_day) {
        glBegin(GL_QUADS);
        glColor3f(0.26f, 0.61f, 0.97f);
        glVertex2f(0.0f, 800.0f);
        glVertex2f(600.0f, 800.0f);
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex2f(600.0f, 0.0f);
        glVertex2f(0.0f, 0.0f);
        glEnd();
    } else {
        glBegin(GL_QUADS);
        glColor3f(0.05f, 0.05f, 0.2f);
        glVertex2f(0.0f, 800.0f);
        glVertex2f(600.0f, 800.0f);
        glColor3f(0.1f, 0.1f, 0.35f);
        glVertex2f(600.0f, 0.0f);
        glVertex2f(0.0f, 0.0f);
        glEnd();
    }
}

// Draws the ground
void drawGrass() {
    glBegin(GL_QUADS);
    glColor3f(0.5f, 0.78f, 0.37f);
    glVertex2f(0.0f, 300.0f);
    glVertex2f(600.0f, 300.0f);
    glVertex2f(600.0f, 0.0f);
    glVertex2f(0.0f, 0.0f);
    glEnd();
}

// Draws the sun (day) or moon (night)
void drawSunOrMoon() {
    if (is_day) {
        glColor3f(1.0f, 0.84f, 0.0f);
    } else {
        glColor3f(0.9f, 0.9f, 0.85f);
    }

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(sunPosX, sunPosY);
    for (int i = 0; i <= 360; i++) {
        float angle = i * DEG2RAD;
        glVertex2f(sunPosX + sunRadius * cos(angle),
                   sunPosY + sunRadius * sin(angle));
    }
    glEnd();
}

// Initializes star field for the night sky
void initializeStars() {
    stars.clear();
    srand(12345); // Fixed seed for consistent star positions

    for (int i = 0; i < 100; ++i) {
        Star star;
        star.x = (rand() % 600);
        star.y = 400.0f + (rand() % 400); // Stars in upper half of sky
        star.brightness = 0.5f + (rand() % 50) / 100.0f;
        star.twinkleSpeed = 0.5f + (rand() % 150) / 100.0f;
        stars.push_back(star);
    }

    srand(static_cast<unsigned int>(time(NULL))); // Reset to random seed
}

// Draws twinkling stars during night
void drawStars() {
    if (!is_day) {
        glPointSize(2.0f);
        glBegin(GL_POINTS);

        for (size_t i = 0; i < stars.size(); ++i) {
            float twinkle = 0.5f + 0.5f * sin(starTwinklePhase * stars[i].twinkleSpeed + static_cast<float>(i));
            float alpha = stars[i].brightness * twinkle;
            glColor4f(1.0f, 1.0f, 1.0f, alpha);
            glVertex2f(stars[i].x, stars[i].y);
        }

        glEnd();
        glPointSize(1.0f);
    }
}

// Cloud
// Draws back cloud layer with shadow
void drawCloudLayer_Back()
{
    float shadow_alpha = is_day ? SHADOW_ALPHA_DAY : SHADOW_ALPHA_NIGHT;
    float feather_width = 10.0f;
	// Shadow circles
    drawFeatheredCircle(50.0f, 420.0f + SHADOW_OFFSET, 90.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(150.0f, 390.0f + SHADOW_OFFSET, 80.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(250.0f, 340.0f + SHADOW_OFFSET, 70.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(350.0f, 340.0f + SHADOW_OFFSET, 70.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(450.0f, 390.0f + SHADOW_OFFSET, 80.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(550.0f, 420.0f + SHADOW_OFFSET, 90.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);

    if (is_day) {
        glColor3f(0.44f, 0.70f, 0.96f);
    } else {
        glColor3f(0.15f, 0.35f, 0.55f);
    }
    
    drawCircle(50.0f, 420.0f, 90.0f);
    drawCircle(150.0f, 390.0f, 80.0f);
    drawCircle(250.0f, 340.0f, 70.0f);
    drawCircle(350.0f, 340.0f, 70.0f);
    drawCircle(450.0f, 390.0f, 80.0f);
    drawCircle(550.0f, 420.0f, 90.0f);
}

// Draws middle cloud layer
void drawCloudLayer_Middle()
{
    float shadow_alpha = is_day ? SHADOW_ALPHA_DAY : SHADOW_ALPHA_NIGHT;
    float feather_width = 10.0f;
    // Shadow 
    drawFeatheredCircle(50.0f, 370.0f + SHADOW_OFFSET, 90.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(150.0f, 330.0f + SHADOW_OFFSET, 80.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(250.0f, 300.0f + SHADOW_OFFSET, 70.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(350.0f, 300.0f + SHADOW_OFFSET, 70.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(450.0f, 330.0f + SHADOW_OFFSET, 80.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(550.0f, 370.0f + SHADOW_OFFSET, 90.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);

    if (is_day) {
        glColor3f(0.8f, 0.9f, 1.0f);
    } else {
        glColor3f(0.4f, 0.6f, 0.75f);
    }

    drawCircle(50.0f, 370.0f, 90.0f);
    drawCircle(150.0f, 330.0f, 80.0f);
    drawCircle(250.0f, 300.0f, 70.0f);
    drawCircle(350.0f, 300.0f, 70.0f);
    drawCircle(450.0f, 330.0f, 80.0f);
    drawCircle(550.0f, 370.0f, 90.0f);
}

// Draws front cloud layer
void drawCloudLayer_Front()
{
    float shadow_alpha = is_day ? SHADOW_ALPHA_DAY : SHADOW_ALPHA_NIGHT;
    float feather_width = 10.0f;
    // Shadow 
    drawFeatheredCircle(50.0f, 320.0f + SHADOW_OFFSET, 90.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(150.0f, 290.0f + SHADOW_OFFSET, 80.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(250.0f, 260.0f + SHADOW_OFFSET, 70.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(350.0f, 260.0f + SHADOW_OFFSET, 70.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(450.0f, 290.0f + SHADOW_OFFSET, 80.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(550.0f, 320.0f + SHADOW_OFFSET, 90.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);

    if (is_day) {
        glColor3f(1.0f, 1.0f, 1.0f);
    } else {
        glColor3f(0.85f, 0.9f, 0.95f);
    }

    drawCircle(50.0f, 320.0f, 90.0f);
    drawCircle(150.0f, 290.0f, 80.0f);
    drawCircle(250.0f, 260.0f, 70.0f);
    drawCircle(350.0f, 260.0f, 70.0f);
    drawCircle(450.0f, 290.0f, 80.0f);
    drawCircle(550.0f, 320.0f, 90.0f);
}

// Draws all three layered background clouds
void drawLayeredBackgroundClouds() {
    drawCloudLayer_Back();
    drawCloudLayer_Middle();
    drawCloudLayer_Front();
}

// Draws a cloud from multiple circles
void drawCloud(float x_offset, float y_offset, float scale) {
    glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
    drawCircle(x_offset, y_offset, 25 * scale);
    drawCircle(x_offset + 30 * scale, y_offset + 5 * scale, 30 * scale);
    drawCircle(x_offset - 30 * scale, y_offset + 2 * scale, 20 * scale);
    drawCircle(x_offset + 15 * scale, y_offset + 20 * scale, 20 * scale);
    drawCircle(x_offset - 10 * scale, y_offset + 15 * scale, 22 * scale);
}

// Bushes

// Draws the back layer of bushes with shadows
void drawBushes_BackLayer()
{
    float shadow_alpha = is_day ? SHADOW_ALPHA_DAY : SHADOW_ALPHA_NIGHT;
    float feather_width = 10.0f;
	// left side shadows
    drawFeatheredCircle(20.0f, 100.0f + SHADOW_OFFSET, 100.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(110.0f, 80.0f + SHADOW_OFFSET, 70.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(190.0f, 30.0f + SHADOW_OFFSET, 55.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
	// left side bushes
    glColor3f(0.15f, 0.45f, 0.2f);
    drawCircle(20.0f, 100.0f, 100.0f);
    drawCircle(110.0f, 80.0f, 70.0f);
    drawCircle(190.0f, 30.0f, 55.0f);

	// right side shadows
    drawFeatheredCircle(580.0f, 100.0f + SHADOW_OFFSET, 100.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(490.0f, 80.0f + SHADOW_OFFSET, 70.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(410.0f, 30.0f + SHADOW_OFFSET, 55.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
	// right side bushes
    glColor3f(0.15f, 0.45f, 0.2f);
    drawCircle(580.0f, 100.0f, 100.0f);
    drawCircle(490.0f, 80.0f, 70.0f);
    drawCircle(410.0f, 30.0f, 55.0f);
}

// Draws the front layer of bushes with shadows
void drawBushes_FrontLayer()
{
    float shadow_alpha = is_day ? SHADOW_ALPHA_DAY : SHADOW_ALPHA_NIGHT;
    float feather_width = 10.0f;
	// left side shadows
    drawFeatheredCircle(20.0f, 70.0f + SHADOW_OFFSET, 90.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(100.0f, 50.0f + SHADOW_OFFSET, 60.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(160.0f, 20.0f + SHADOW_OFFSET, 40.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
	// left side bushes
    glColor3f(0.3f, 0.65f, 0.35f);
    drawCircle(20.0f, 70.0f, 90.0f);
    drawCircle(100.0f, 50.0f, 60.0f);
    drawCircle(160.0f, 20.0f, 40.0f);
	// right side shadows
    drawFeatheredCircle(580.0f, 70.0f + SHADOW_OFFSET, 90.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(500.0f, 50.0f + SHADOW_OFFSET, 60.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(440.0f, 20.0f + SHADOW_OFFSET, 40.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
	// right side bushes
    glColor3f(0.3f, 0.65f, 0.35f);
    drawCircle(580.0f, 70.0f, 90.0f);
    drawCircle(500.0f, 50.0f, 60.0f);
    drawCircle(440.0f, 20.0f, 40.0f);
}

// Draws all bushes
void drawAllBushes() {
    drawBushes_BackLayer();
    drawBushes_FrontLayer();
}

// ============
// Balloons 
// ============

// Draws a feathered balloon shadow
void drawBalloonShadow(float x, float y, float size, float shadow_alpha) {
    const float balloonWidth = 40.0f * size;
    const float balloonHeight = 60.0f * size;
    const int segments = 60;
    const int layers = 5;

    for (int layer = 0; layer < layers; ++layer) {
        float layerFactor = 1.0f - static_cast<float>(layer) / layers;
        float currentAlpha = shadow_alpha * (static_cast<float>(layer + 1) / layers);
        float shrinkFactor = 0.9f + 0.1f * layerFactor;

        glColor4f(0.0f, 0.0f, 0.0f, currentAlpha);

        // Full balloon shadow
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(x, y);

        for (int i = 0; i <= segments; ++i) {
            float angle = TWO_PI * static_cast<float>(i) / segments;
            float verticalPos = sin(angle);

            float radiusX, radiusY;
            if (verticalPos >= 0) {
                radiusX = balloonWidth * (0.9f + 0.1f * verticalPos) * shrinkFactor;
                radiusY = balloonHeight * 0.55f * shrinkFactor;
            } else {
                radiusX = balloonWidth * (0.9f + 0.5f * verticalPos) * shrinkFactor;
                radiusY = balloonHeight * 0.55f * shrinkFactor;
            }

            glVertex2f(x + radiusX * cos(angle), y + radiusY * sin(angle));
        }
        glEnd();
    }
}

// Draws a balloon with left/right tone and a string
void drawBalloon(float x, float y, float size, float r, float g, float b) {
    const float balloonWidth = 40.0f * size;
    const float balloonHeight = 60.0f * size;
    const float stringLength = 80.0f * size;

    // Shadow parameters
    float shadow_alpha = is_day ? SHADOW_ALPHA_DAY : SHADOW_ALPHA_NIGHT;
    const float shadowOffsetX = 5.0f * size;
    const float shadowOffsetY = 5.0f * size;

    // Feathered shadow
    drawBalloonShadow(x + shadowOffsetX, y + shadowOffsetY, size, shadow_alpha);

    // String (sine curve)
    glLineWidth(2.0f * size);
    glColor3f(r * 0.6f, g * 0.6f, b * 0.6f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= 50; ++i) {
        float t = static_cast<float>(i) / 50.0f;
        float stringX = x + sin(t * PI * 4.0f) * 3.0f * size;
        float stringY = y - t * stringLength;
        glVertex2f(stringX, stringY);
    }
    glEnd();
    glLineWidth(1.0f);

    // Left half (lighter color)
    const int segments = 60;
    float lightR = std::min(1.0f, r + 0.2f);
    float lightG = std::min(1.0f, g + 0.2f);
    float lightB = std::min(1.0f, b + 0.2f);

    glColor3f(lightR, lightG, lightB);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);

    for (int i = 0; i <= segments / 2; ++i) {
        float angle = PI * 1.5f - static_cast<float>(i) / (segments / 2) * PI;
        float verticalPos = sin(angle);

        float radiusX, radiusY;
        if (verticalPos >= 0) {
            radiusX = balloonWidth * (0.9f + 0.1f * verticalPos);
            radiusY = balloonHeight * 0.55f;
        } else {
            radiusX = balloonWidth * (0.9f + 0.5f * verticalPos);
            radiusY = balloonHeight * 0.55f;
        }

        float cosA = cos(angle);
        float sinA = sin(angle);
        glVertex2f(x + radiusX * cosA, y + radiusY * sinA);
    }
    glEnd();

    // Right half (darker color)
    float darkR = std::max(0.0f, r - 0.15f);
    float darkG = std::max(0.0f, g - 0.15f);
    float darkB = std::max(0.0f, b - 0.15f);

    glColor3f(darkR, darkG, darkB);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);

    for (int i = 0; i <= segments / 2; ++i) {
        float angle = PI * 0.5f - static_cast<float>(i) / (segments / 2) * PI;
        float verticalPos = sin(angle);

        float radiusX, radiusY;
        if (verticalPos >= 0) {
            radiusX = balloonWidth * (0.9f + 0.1f * verticalPos);
            radiusY = balloonHeight * 0.55f;
        } else {
            radiusX = balloonWidth * (0.9f + 0.5f * verticalPos);
            radiusY = balloonHeight * 0.55f;
        }

        float cosA = cos(angle);
        float sinA = sin(angle);
        glVertex2f(x + radiusX * cosA, y + radiusY * sinA);
    }
    glEnd();
}


// =======================
// Greeting card cover
// =======================

// Draws polygons to mask the building on the cover
void drawCoverPolygons()
{
    glColor3f(0.67f, 0.61f, 0.70f);

    glBegin(GL_POLYGON);
    glVertex2f(0.0f, 450.0f);
    glVertex2f(350.0f, 625.0f);
    glVertex2f(350.0f, 450.0f);
    glVertex2f(0.0f, 100.0f);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(150.0f, 100.0f);
    glVertex2f(450.0f, 400.0f);
    glVertex2f(450.0f, 100.0f);
    glEnd();

    glColor3f(0.33f, 0.23f, 0.40f);

    glBegin(GL_POLYGON);
    glVertex2f(550.0f, 620.0f);
    glVertex2f(800.0f, 490.0f);
    glVertex2f(800.0f, 100.0f);
    glVertex2f(550.0f, 100.0f);
    glEnd();

    // Liverpool White Pavilion
    const float xs[] = { 585.0f, 615.0f, 645.0f, 675.0f, 705.0f, 735.0f, 765.0f };
    const int count = sizeof(xs) / sizeof(xs[0]);

    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(1.0f);

    glBegin(GL_LINES);
    for (int i = 0; i < count; ++i) {
        float x = xs[i];
        glVertex2f(x, 100.0f);
        glVertex2f(x, 250.0f);

        glVertex2f(x + 10.0f, 100.0f);
        glVertex2f(x + 10.0f, 250.0f);
    }
    glEnd();

    glLineWidth(1.0f);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINES);
    // Horizontal lines
    glVertex2f(560.0f, 250.0f); glVertex2f(790.0f, 250.0f);
    glVertex2f(560.0f, 260.0f); glVertex2f(790.0f, 260.0f);
    glVertex2f(600.0f, 280.0f); glVertex2f(750.0f, 280.0f);
    glVertex2f(600.0f, 290.0f); glVertex2f(750.0f, 290.0f);

    // Vertical lines
    glVertex2f(560.0f, 250.0f); glVertex2f(560.0f, 260.0f);
    glVertex2f(790.0f, 250.0f); glVertex2f(790.0f, 260.0f);
    glVertex2f(600.0f, 280.0f); glVertex2f(600.0f, 290.0f);
    glVertex2f(750.0f, 280.0f); glVertex2f(750.0f, 290.0f);
    glEnd();

    // Curves
    glLineWidth(1.0f);
    glColor3f(1.0f, 1.0f, 1.0f);

    // Curve 1
    drawQuadraticBezier(560.0f, 260.0f, 575.0f, 265.0f, 600.0f, 280.0f);

    // Curve 2
    drawQuadraticBezier(790.0f, 260.0f, 775.0f, 265.0f, 750.0f, 280.0f);

    // Curve 3 (cosine-like)
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= 100; ++i) {
        float t = static_cast<float>(i) / 100.0f;
        float x = 600.0f + t * (750.0f - 600.0f);
        float angle = (x - 675.0f) / 75.0f * PI;
        float y = 20.0f * cos(angle) + 310.0f;
        glVertex2f(x, y);
    }
    glEnd();

    glLineWidth(1.0f);
}

// Draws a two-tone rotating ring on the cover
void drawCoverRing()
{
    const float centerX = 300.0f;
    const float centerY = 400.0f;
    const float outerRadius = 220.0f;
    const float ringWidth = 25.0f;
    const float innerRadius = outerRadius - ringWidth;

    glPushMatrix();
    glTranslatef(centerX, centerY, 0.0f);
    glRotatef(ringRotationAngle, 0.0f, 0.0f, 1.0f);
    glTranslatef(-centerX, -centerY, 0.0f);

    glColor3f(0.67f, 0.61f, 0.70f);
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = -90; i <= 90; ++i) {
        float angle = i * DEG2RAD;
        glVertex2f(centerX + outerRadius * cos(angle), centerY + outerRadius * sin(angle));
        glVertex2f(centerX + innerRadius * cos(angle), centerY + innerRadius * sin(angle));
    }
    glEnd();

    glColor3f(0.33f, 0.23f, 0.40f);
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 90; i <= 270; ++i) {
        float angle = i * DEG2RAD;
        glVertex2f(centerX + outerRadius * cos(angle), centerY + outerRadius * sin(angle));
        glVertex2f(centerX + innerRadius * cos(angle), centerY + innerRadius * sin(angle));
    }
    glEnd();

    glPopMatrix();
}

// Draws the greeting card cover view
void drawGreetingCardCover()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Background halves
    glColor3f(0.67f, 0.61f, 0.70f);
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(300.0f, 0.0f);
    glVertex2f(300.0f, 800.0f);
    glVertex2f(0.0f, 800.0f);
    glEnd();

    glColor3f(0.33f, 0.23f, 0.40f);
    glBegin(GL_QUADS);
    glVertex2f(300.0f, 0.0f);
    glVertex2f(600.0f, 0.0f);
    glVertex2f(600.0f, 800.0f);
    glVertex2f(300.0f, 800.0f);
    glEnd();

    // CB Building (masked by cover polygons)
    glPushMatrix();
    glTranslatef(buildingPosX, buildingPosY, 0.0f);
    glScalef(buildingScaleX, buildingScaleY, 1.0f);
    drawBuilding_LowerLayer();
    drawBuilding_UpperLayer_Light();
    drawBuilding_UpperLayer_Dark();
    drawBuilding_Stripes();
    drawCoverPolygons();
    glPopMatrix();

    // Rotating ring
    drawCoverRing();

    // Title text
    glColor3f(1.0f, 1.0f, 1.0f);
    float textX = 180.0f;
    float textY = buildingPosY - 30.0f;
    glRasterPos2f(textX, textY);
    const char* text = "XJTLU 20TH ANNIVERSARY";
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}

// ============================================
// Postcard snapshot
// ============================================
// Draws a white flash overlay for transitions
void drawFlashEffect() {
    if (isFlashing && flashAlpha > 0.0f) {
        glColor4f(1.0f, 1.0f, 1.0f, flashAlpha);
        glBegin(GL_QUADS);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(600.0f, 0.0f);
        glVertex2f(600.0f, 800.0f);
        glVertex2f(0.0f, 800.0f);
        glEnd();
    }
}

// Draws the postcard snapshot with taped photo and message
void drawPostcard() {
    // Background
    glColor3f(0.85f, 0.8f, 0.95f);
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(600.0f, 0.0f);
    glVertex2f(600.0f, 800.0f);
    glVertex2f(0.0f, 800.0f);
    glEnd();

    // Photo dimensions and position
    const float photoWidth = 480.0f;
    const float photoHeight = 640.0f;
    const float photoX = (600.0f - photoWidth) / 2.0f; // 60
    const float photoY = 120.0f; // From bottom

    // Photo border (frame)
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glVertex2f(photoX - 5.0f, photoY - 5.0f);
    glVertex2f(photoX + photoWidth + 5.0f, photoY - 5.0f);
    glVertex2f(photoX + photoWidth + 5.0f, photoY + photoHeight + 5.0f);
    glVertex2f(photoX - 5.0f, photoY + photoHeight + 5.0f);
    glEnd();

    // Viewport/projection for the photo content
    glViewport((int)photoX, (int)photoY, (int)photoWidth, (int)photoHeight);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(viewLeft, viewRight, viewBottom, viewTop);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Frozen scene content
    drawSky();
    drawStars();
    drawSunOrMoon();
    drawLayeredBackgroundClouds();
    drawCloud(cloud1_posX, 650.0f, 0.8f);
    drawCloud(cloud2_posX, 600.0f, 1.2f);
    drawCloud(cloud3_posX, 550.0f, 0.7f);
    drawGrass();

    glPushMatrix();
    drawXJTLUCenterBuilding();
    glPopMatrix();
    drawAllBushes();

    // Active balloons
    for (size_t i = 0; i < balloons.size(); ++i) {
        if (balloons[i].active) {
            drawBalloon(balloons[i].x, balloons[i].y, balloons[i].size,
                        balloons[i].colorR, balloons[i].colorG, balloons[i].colorB);
        }
    }

    // Restore viewport and projection
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glViewport(0, 0, 600, 800);

    // Reset projection to screen coordinates
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 600, 0, 800);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Tape corners
    const float tapeWidth = 40.0f;
    const float tapeHeight = 15.0f;
    const float tapeOffset = 5.0f;

    drawTapeCorner(photoX + tapeOffset, photoY + photoHeight - tapeOffset, tapeWidth, tapeHeight, -45.0f);
    drawTapeCorner(photoX + photoWidth - tapeOffset, photoY + photoHeight - tapeOffset, tapeWidth, tapeHeight, 45.0f);
    drawTapeCorner(photoX + tapeOffset, photoY + tapeOffset, tapeWidth, tapeHeight, 45.0f);
    drawTapeCorner(photoX + photoWidth - tapeOffset, photoY + tapeOffset, tapeWidth, tapeHeight, -45.0f);

    // Message box
    const float msgBoxX = photoX;
    const float msgBoxY = 20.0f;
    const float msgBoxWidth = photoWidth;
    const float msgBoxHeight = 80.0f;

    // Message background
    glColor3f(1.0f, 0.98f, 0.85f);
    glBegin(GL_QUADS);
    glVertex2f(msgBoxX, msgBoxY);
    glVertex2f(msgBoxX + msgBoxWidth, msgBoxY);
    glVertex2f(msgBoxX + msgBoxWidth, msgBoxY + msgBoxHeight);
    glVertex2f(msgBoxX, msgBoxY + msgBoxHeight);
    glEnd();

    // Message border
    glColor3f(0.8f, 0.75f, 0.65f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(msgBoxX, msgBoxY);
    glVertex2f(msgBoxX + msgBoxWidth, msgBoxY);
    glVertex2f(msgBoxX + msgBoxWidth, msgBoxY + msgBoxHeight);
    glVertex2f(msgBoxX, msgBoxY + msgBoxHeight);
    glEnd();
    glLineWidth(1.0f);

    // Message text
    glColor3f(0.3f, 0.25f, 0.2f);
    glRasterPos2f(msgBoxX + 40.0f, msgBoxY + 50.0f);
    const char* line1 = "Happy 20th Anniversary, XJTLU!";
    for (const char* c = line1; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
    }

    glRasterPos2f(msgBoxX + 80.0f, msgBoxY + 25.0f);
    const char* line2 = "Wishing you a bright future ahead!";
    for (const char* c = line2; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    // Restore matrices
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

// Draws on-screen hint bar at the bottom
void drawOnScreenHints()
{
    if (!showHints) return;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, 0, windowHeight);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    const char* hintText = "";

    if (isPostcardMode) {
        hintText = "Press 'p' to Exit Postcard Mode";
    } else if (isCoverVisible) {
        hintText = "Press 'o' to Open/Close Card | Press 'i' to toggle hints on/off | Press 'q' or 'esc': Quit ";
    } else {
        if (zoomFactor > 1.0f) {
            hintText = "+/-: Zoom | Arrow Keys: Pan View | 'r': Reset View | 'p': Postcard";
        } else {
            if (is_day) {
                hintText = "Click: Add Balloon | 'n': Night | '+': Zoom In | 'p': Postcard";
            } else {
                hintText = "Click: Add Balloon | 'n': Day | '+': Zoom In | 'l': Lights | 'p': Postcard";
            }
        }
    }

    // Background for hint text
    glColor4f(0.0f, 0.0f, 0.0f, 0.4f);
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(windowWidth, 0.0f);
    glVertex2f(windowWidth, 25.0f);
    glVertex2f(0.0f, 25.0f);
    glEnd();

    // Text
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(10.0f, 8.0f, hintText);

    // Restore matrices
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

// ===============================
// View and projection helpers
// ===============================

// Updates the projection matrix based on current view bounds
void updateProjection()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(viewLeft, viewRight, viewBottom, viewTop);
    glMatrixMode(GL_MODELVIEW);
}

// Resets the view to default (no zoom/pan)
void resetViewToDefault()
{
    zoomFactor = MIN_ZOOM;
    viewLeft = MIN_LEFT;
    viewRight = MIN_RIGHT;
    viewBottom = MIN_BOTTOM;
    viewTop = MIN_TOP;
    updateProjection();
}

// =================
// Callbacks 
// =================

// Main display callback
void display() {
    if (isCoverVisible) {
        drawGreetingCardCover();
    } else if (isPostcardMode) {
        drawPostcard();
    } else {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();

        drawSky();
        drawStars();
        drawSunOrMoon();
        drawLayeredBackgroundClouds();
        drawCloud(cloud1_posX, 650.0f, 0.8f);
        drawCloud(cloud2_posX, 600.0f, 1.2f);
        drawCloud(cloud3_posX, 550.0f, 0.7f);
        drawGrass();

        glPushMatrix();
        drawXJTLUCenterBuilding();
        glPopMatrix();
        drawAllBushes();

        // Active balloons
        for (size_t i = 0; i < balloons.size(); ++i) {
            if (balloons[i].active) {
                drawBalloon(balloons[i].x, balloons[i].y, balloons[i].size,
                            balloons[i].colorR, balloons[i].colorG, balloons[i].colorB);
            }
        }
    }

    // Flash overlay on top
    if (isFlashing) {
        drawFlashEffect();
    }

    drawOnScreenHints();
    glutSwapBuffers();
}

// Timer callback to update animation and state
void update(int value) {
    // Flash animation
    if (isFlashing) {
        flashFrameCount++;

        if (flashFrameCount < FLASH_DURATION / 3) {
            flashAlpha = (static_cast<float>(flashFrameCount) / (FLASH_DURATION / 3.0f)) * 0.85f;
        } else {
            flashAlpha = 0.85f * (1.0f - (static_cast<float>(flashFrameCount) - FLASH_DURATION / 3) / (FLASH_DURATION * 2.0f / 3.0f));
        }

        if (flashFrameCount >= FLASH_DURATION) {
            isFlashing = false;
            flashAlpha = 0.0f;
            flashFrameCount = 0;
        }

        glutPostRedisplay();
        glutTimerFunc(16, update, 0);
        return;
    }

    // Pause animations in postcard mode
    if (isPostcardMode) {
        glutPostRedisplay();
        glutTimerFunc(16, update, 0);
        return;
    }

    // Cover ring animation
    if (isAnimating) {
        const float rotationSpeed = 3.0f;
        ringRotationAngle += rotationSpeed;

        if (ringRotationAngle >= 180.0f) {
            ringRotationAngle = 0.0f;
            isAnimating = false;
            if (isOpening) {
                isCoverVisible = false;
            }
        }
    }

    // Cloud drift
    cloud1_posX += 0.2f;
    if (cloud1_posX > 700.0f) cloud1_posX = -100.0f;

    cloud2_posX += 0.15f;
    if (cloud2_posX > 700.0f) cloud2_posX = -100.0f;

    cloud3_posX += 0.18f;
    if (cloud3_posX > 700.0f) cloud3_posX = -100.0f;

    // Star twinkling phase
    starTwinklePhase += 0.05f;
    if (starTwinklePhase > TWO_PI) {
        starTwinklePhase = 0.0f;
    }

    // Update balloons
    for (size_t i = 0; i < balloons.size(); ++i) {
        if (balloons[i].active) {
            balloons[i].y += balloons[i].speed;
            if (balloons[i].y > viewTop + 100.0f) {
                balloons[i].active = false;
            }
        }
    }

    // Periodically clean inactive balloons
    static int frameCount = 0;
    frameCount++;
    if (frameCount > 300) {
        frameCount = 0;
        std::vector<Balloon> activeBalloons;
        for (size_t i = 0; i < balloons.size(); ++i) {
            if (balloons[i].active) {
                activeBalloons.push_back(balloons[i]);
            }
        }
        balloons = activeBalloons;
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

// Special keys (arrow keys) for panning when zoomed
void specialKeys(int key, int x, int y) {
    if (isCoverVisible || isPostcardMode) {
        return;
    }

    float currentWidth = viewRight - viewLeft;
    float currentHeight = viewTop - viewBottom;

    bool viewChanged = false;

    switch (key) {
    case GLUT_KEY_UP: {
        float newTop = viewTop + PAN_STEP;
        float newBottom = viewBottom + PAN_STEP;
        if (newTop <= MIN_TOP) {
            viewTop = newTop;
            viewBottom = newBottom;
            viewChanged = true;
        }
        break;
    }
    case GLUT_KEY_DOWN: {
        float newBottom = viewBottom - PAN_STEP;
        if (newBottom >= MIN_BOTTOM) {
            viewBottom = newBottom;
            viewTop = viewBottom + currentHeight;
            viewChanged = true;
        }
        break;
    }
    case GLUT_KEY_LEFT: {
        float newLeft = viewLeft - PAN_STEP;
        if (newLeft >= MIN_LEFT) {
            viewLeft = newLeft;
            viewRight = viewLeft + currentWidth;
            viewChanged = true;
        }
        break;
    }
    case GLUT_KEY_RIGHT: {
        float newRight = viewRight + PAN_STEP;
        if (newRight <= MIN_RIGHT) {
            viewRight = newRight;
            viewLeft = viewRight - currentWidth;
            viewChanged = true;
        }
        break;
    }
    }

    if (viewChanged) {
        updateProjection();
        glutPostRedisplay();
    }
}

// Keyboard input handler
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 'i': case 'I':
        showHints = !showHints;
        break;
    case 'o': case 'O':
        if (!isAnimating && !isPostcardMode) {
            isAnimating = true;
            ringRotationAngle = 0.0f;

            if (isCoverVisible) {
                isOpening = true;
            } else {
                resetViewToDefault();
                isCoverVisible = true;
                isAnimating = false;
                ringRotationAngle = 0.0f;
            }
        }
        break;
    case 'n': case 'N':
        if (!isPostcardMode) {
            is_day = !is_day;
        }
        break;
    case 'p': case 'P':
        if (!isCoverVisible) {
            if (!isPostcardMode) {
                isFlashing = true;
                flashFrameCount = 0;
                flashAlpha = 0.0f;
            }
            isPostcardMode = !isPostcardMode;
        }
        break;
    case 'l': case 'L':
        if (!is_day && !isPostcardMode) {
            areLightsOn = !areLightsOn;
        }
        break;
    case '+': case '=': {
        if (isCoverVisible || isPostcardMode) break;

        if (zoomFactor < MAX_ZOOM) {
            zoomFactor += ZOOM_STEP;

            float centerX = (viewLeft + viewRight) / 2.0f;
            float centerY = (viewBottom + viewTop) / 2.0f;

            float newWidth = (MIN_RIGHT - MIN_LEFT) / zoomFactor;
            float newHeight = (MIN_TOP - MIN_BOTTOM) / zoomFactor;

            viewLeft = centerX - newWidth / 2.0f;
            viewRight = centerX + newWidth / 2.0f;
            viewBottom = centerY - newHeight / 2.0f;
            viewTop = centerY + newHeight / 2.0f;

            if (viewLeft < MIN_LEFT) {
                float offset = MIN_LEFT - viewLeft;
                viewLeft += offset; viewRight += offset;
            }
            if (viewRight > MIN_RIGHT) {
                float offset = viewRight - MIN_RIGHT;
                viewLeft -= offset; viewRight -= offset;
            }
            if (viewBottom < MIN_BOTTOM) {
                float offset = MIN_BOTTOM - viewBottom;
                viewBottom += offset; viewTop += offset;
            }
            if (viewTop > MIN_TOP) {
                float offset = viewTop - MIN_TOP;
                viewBottom -= offset; viewTop -= offset;
            }

            updateProjection();
            glutPostRedisplay();
        }
        break;
    }
    case '-': case '_': {
        if (isCoverVisible || isPostcardMode) break;

        if (zoomFactor > MIN_ZOOM) {
            zoomFactor -= ZOOM_STEP;
            if (zoomFactor < MIN_ZOOM) zoomFactor = MIN_ZOOM;

            float centerX = (viewLeft + viewRight) / 2.0f;
            float centerY = (viewBottom + viewTop) / 2.0f;

            float newWidth = (MIN_RIGHT - MIN_LEFT) / zoomFactor;
            float newHeight = (MIN_TOP - MIN_BOTTOM) / zoomFactor;

            viewLeft = centerX - newWidth / 2.0f;
            viewRight = centerX + newWidth / 2.0f;
            viewBottom = centerY - newHeight / 2.0f;
            viewTop = centerY + newHeight / 2.0f;

            if (viewLeft < MIN_LEFT) {
                float offset = MIN_LEFT - viewLeft;
                viewLeft += offset; viewRight += offset;
            }
            if (viewRight > MIN_RIGHT) {
                float offset = viewRight - MIN_RIGHT;
                viewLeft -= offset; viewRight -= offset;
            }
            if (viewBottom < MIN_BOTTOM) {
                float offset = MIN_BOTTOM - viewBottom;
                viewBottom += offset; viewTop += offset;
            }
            if (viewTop > MIN_TOP) {
                float offset = viewTop - MIN_TOP;
                viewBottom -= offset; viewTop -= offset;
            }

            updateProjection();
            glutPostRedisplay();
        }
        break;
    }
    case 'r': case 'R':
        if (!isCoverVisible && !isPostcardMode) {
            resetViewToDefault();
            glutPostRedisplay();
        }
        break;
    case 'q': case 'Q': case 27:
        exit(0);
        break;
    }
}

// Mouse input handler (left click to spawn balloons)
void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        // Only create balloons when not in cover or postcard mode
        if (!isCoverVisible && !isPostcardMode) {
            float worldX = viewLeft + static_cast<float>(x) / windowWidth * (viewRight - viewLeft);
            float worldY = viewTop - static_cast<float>(y) / windowHeight * (viewTop - viewBottom);

            float randomSize = 0.6f + (rand() % 70) / 100.0f;

            int colorIndex = rand() % numBalloonColors;
            BalloonColor color = balloonColors[colorIndex];

            float randomSpeed = 0.5f + (rand() % 100) / 100.0f;

            balloons.push_back(Balloon(worldX, worldY, randomSize, color.r, color.g, color.b, randomSpeed));
        }
    }
}

// Reshape callback (keeps a fixed 600x800 viewport)
void reshape(int w, int h)
{
    glViewport(0, 0, 600, 800);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(viewLeft, viewRight, viewBottom, viewTop);

    if (w != 600 || h != 800) {
        glutReshapeWindow(600, 800);
    }

    glMatrixMode(GL_MODELVIEW);
}

// Program entry point
int main(int argc, char** argv) {
    // Initialize random seed
    srand(static_cast<unsigned int>(time(NULL)));

    // Initialize stars
    initializeStars();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(100, 100);

    glutCreateWindow("XJTLU 20th Anniversary Greeting Card");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouse);
    glutTimerFunc(16, update, 0);

    std::cout << "--- Controls ---" << std::endl;
    std::cout << "Press 'i': Toggle on-screen hints" << std::endl;
    std::cout << "Press 'o': Open/close the card" << std::endl;
    std::cout << "Press 'n': Toggle day/night" << std::endl;
    std::cout << "Press 'l': Turn on/off building lights (at night)" << std::endl;
    std::cout << "Press 'p': Enter/exit Postcard Mode (snapshot)" << std::endl;
    std::cout << "Press '+': Zoom in" << std::endl;
    std::cout << "Press '-': Zoom out" << std::endl;
    std::cout << "Press 'r': Reset view" << std::endl;
    std::cout << "Arrow keys: Pan view (when zoomed)" << std::endl;
    std::cout << "Left click: Create floating balloon" << std::endl;
    std::cout << "Press 'q' or 'ESC': Quit" << std::endl;

    glutMainLoop();
    return 0;
}