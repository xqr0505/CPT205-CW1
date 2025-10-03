#include <iostream>
#include <GL/freeglut.h>
#include <cmath>

// --- Global settings ---
int windowWidth = 600;
int windowHeight = 800;

// State flags
bool is_day = true;
bool show_greeting = false;
bool isCoverVisible = true;

// Cover ring animation
float ringRotationAngle = 0.0f;
bool isAnimating = false;
bool isOpening = false; // true = opening, false = closing

// Sun / moon
float sunPosX = 500.0f;
float sunPosY = 700.0f;
float sunRadius = 50.0f;

// Cloud animation
float cloud1_posX = 100.0f;
float cloud2_posX = 450.0f;

// Greeting fade
float greeting_alpha = 0.0f;

// Building transform
float buildingPosX = 165.0f;
float buildingPosY = 300.0f;
float buildingScaleX = 0.3f;
float buildingScaleY = 0.3f;

const float SHADOW_OFFSET = 7.0f;
const float SHADOW_ALPHA_DAY = 0.2f;
const float SHADOW_ALPHA_NIGHT = 0.3f;

/* Draw a filled circle at (cx,cy) with radius r */
void drawCircle(float cx, float cy, float r) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 360; i++) {
        float angle = i * 3.14159f / 180.0f;
        glVertex2f(cx + r * cos(angle), cy + r * sin(angle));
    }
    glEnd();
}

/* Draw a feathered (blurred-edge) filled circle using layered circles */
void drawFeatheredCircle(float cx, float cy, float radius, float feather,
    float r, float g, float b, float a, int layers = 10)
{
    for (int i = 0; i < layers; ++i)
    {
        float currentRadius = radius - ((float)i / layers) * feather;
        float currentAlpha = a * ((float)(i + 1) / layers);
        glColor4f(r, g, b, currentAlpha);
        drawCircle(cx, cy, currentRadius);
    }
}

/* Draw a cubic Bezier curve using 4 control points */
void drawCubicBezierCurve(float p0x, float p0y, float p1x, float p1y,
    float p2x, float p2y, float p3x, float p3y,
    int segments = 50)
{
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= segments; ++i)
    {
        float t = (float)i / (float)segments;
        float t_inv = 1.0f - t;

        float b0 = t_inv * t_inv * t_inv;
        float b1 = 3.0f * t * t_inv * t_inv;
        float b2 = 3.0f * t * t * t_inv;
        float b3 = t * t * t;

        float x = b0 * p0x + b1 * p1x + b2 * p2x + b3 * p3x;
        float y = b0 * p0y + b1 * p1y + b2 * p2y + b3 * p3y;

        glVertex2f(x, y);
    }
    glEnd();
}

/* Building lower layer (two main faces) */
void drawBuilding_LowerLayer()
{
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

/* Lighter upper building parts */
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

/* Darker upper building parts */
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

/* Draw a slanted stripe (quad) along a segment */
void drawSlantedStripe(float x1, float y1, float x2, float y2, float height)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    float length = sqrt(dx * dx + dy * dy);
    if (length == 0) return;

    float perp_dx = -dy / length * (height / 2.0f);
    float perp_dy = dx / length * (height / 2.0f);

    float v1x = x1 + perp_dx;
    float v1y = y1 + perp_dy;

    float v2x = x2 + perp_dx;
    float v2y = y2 + perp_dy;

    float v3x = x2 - perp_dx;
    float v3y = y2 - perp_dy;

    float v4x = x1 - perp_dx;
    float v4y = y1 - perp_dy;

    glBegin(GL_QUADS);
    glVertex2f(v1x, v1y);
    glVertex2f(v2x, v2y);
    glVertex2f(v3x, v3y);
    glVertex2f(v4x, v4y);
    glEnd();
}

/* Building decorative stripes */
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

/* Draw sky (different gradient for day/night) */
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
    }
    else {
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

/* Simple ground */
void drawGrass() {
    glBegin(GL_QUADS);
    glColor3f(0.5f, 0.78f, 0.37f);
    glVertex2f(0.0f, 300.0f);
    glVertex2f(600.0f, 300.0f);
    glVertex2f(600.0f, 0.0f);
    glVertex2f(0.0f, 0.0f);
    glEnd();
}

/* Sun or moon */
void drawSunOrMoon() {
    if (is_day) {
        glColor3f(1.0f, 0.84f, 0.0f);
    }
    else {
        glColor3f(0.9f, 0.9f, 0.85f);
    }

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(sunPosX, sunPosY);
    for (int i = 0; i <= 360; i++) {
        float angle = i * 3.14159f / 180.0f;
        glVertex2f(sunPosX + sunRadius * cos(angle),
            sunPosY + sunRadius * sin(angle));
    }
    glEnd();
}

/* Back cloud layer with shadows */
void drawCloudLayer_Back()
{
    float shadow_alpha = is_day ? SHADOW_ALPHA_DAY : SHADOW_ALPHA_NIGHT;
    float feather_width = 10.0f;
    drawFeatheredCircle(50.0f, 420.0f + SHADOW_OFFSET, 90.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(150.0f, 390.0f + SHADOW_OFFSET, 80.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(250.0f, 340.0f + SHADOW_OFFSET, 70.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(350.0f, 340.0f + SHADOW_OFFSET, 70.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(450.0f, 390.0f + SHADOW_OFFSET, 80.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(550.0f, 420.0f + SHADOW_OFFSET, 90.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);

    if (is_day) {
        glColor3f(0.44f, 0.70f, 0.96f);
    }
    else {
        glColor3f(0.15f, 0.35f, 0.55f);
    }

    drawCircle(50.0f, 420.0f, 90.0f);
    drawCircle(150.0f, 390.0f, 80.0f);
    drawCircle(250.0f, 340.0f, 70.0f);
    drawCircle(350.0f, 340.0f, 70.0f);
    drawCircle(450.0f, 390.0f, 80.0f);
    drawCircle(550.0f, 420.0f, 90.0f);
}

/* Middle cloud layer */
void drawCloudLayer_Middle()
{
    float shadow_alpha = is_day ? SHADOW_ALPHA_DAY : SHADOW_ALPHA_NIGHT;
    float feather_width = 10.0f;

    drawFeatheredCircle(50.0f, 370.0f + SHADOW_OFFSET, 90.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(150.0f, 330.0f + SHADOW_OFFSET, 80.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(250.0f, 300.0f + SHADOW_OFFSET, 70.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(350.0f, 300.0f + SHADOW_OFFSET, 70.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(450.0f, 330.0f + SHADOW_OFFSET, 80.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(550.0f, 370.0f + SHADOW_OFFSET, 90.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);

    if (is_day) {
        glColor3f(0.8f, 0.9f, 1.0f);
    }
    else {
        glColor3f(0.4f, 0.6f, 0.75f);
    }

    drawCircle(50.0f, 370.0f, 90.0f);
    drawCircle(150.0f, 330.0f, 80.0f);
    drawCircle(250.0f, 300.0f, 70.0f);
    drawCircle(350.0f, 300.0f, 70.0f);
    drawCircle(450.0f, 330.0f, 80.0f);
    drawCircle(550.0f, 370.0f, 90.0f);
}

/* Front cloud layer */
void drawCloudLayer_Front()
{
    float shadow_alpha = is_day ? SHADOW_ALPHA_DAY : SHADOW_ALPHA_NIGHT;
    float feather_width = 10.0f;

    drawFeatheredCircle(50.0f, 320.0f + SHADOW_OFFSET, 90.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(150.0f, 290.0f + SHADOW_OFFSET, 80.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(250.0f, 260.0f + SHADOW_OFFSET, 70.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(350.0f, 260.0f + SHADOW_OFFSET, 70.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(450.0f, 290.0f + SHADOW_OFFSET, 80.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(550.0f, 320.0f + SHADOW_OFFSET, 90.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);

    if (is_day) {
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    else {
        glColor3f(0.85f, 0.9f, 0.95f);
    }

    drawCircle(50.0f, 320.0f, 90.0f);
    drawCircle(150.0f, 290.0f, 80.0f);
    drawCircle(250.0f, 260.0f, 70.0f);
    drawCircle(350.0f, 260.0f, 70.0f);
    drawCircle(450.0f, 290.0f, 80.0f);
    drawCircle(550.0f, 320.0f, 90.0f);
}

/* Draw three layered background clouds */
void drawLayeredBackgroundClouds()
{
    drawCloudLayer_Back();
    drawCloudLayer_Middle();
    drawCloudLayer_Front();
}

/* Draw a stylized cloud from multiple circles */
void drawCloud(float x_offset, float y_offset, float scale) {
    glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
    drawCircle(x_offset, y_offset, 25 * scale);
    drawCircle(x_offset + 30 * scale, y_offset + 5 * scale, 30 * scale);
    drawCircle(x_offset - 30 * scale, y_offset + 2 * scale, 20 * scale);
    drawCircle(x_offset + 15 * scale, y_offset + 20 * scale, 20 * scale);
    drawCircle(x_offset - 10 * scale, y_offset + 15 * scale, 22 * scale);
}

/* Back bushes with shadow */
void drawBushes_BackLayer()
{
    float shadow_alpha = is_day ? SHADOW_ALPHA_DAY : SHADOW_ALPHA_NIGHT;
    float feather_width = 10.0f;

    drawFeatheredCircle(20.0f, 100.0f + SHADOW_OFFSET, 100.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(110.0f, 80.0f + SHADOW_OFFSET, 70.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(190.0f, 30.0f + SHADOW_OFFSET, 55.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);

    glColor3f(0.15f, 0.45f, 0.2f);
    drawCircle(20.0f, 100.0f, 100.0f);
    drawCircle(110.0f, 80.0f, 70.0f);
    drawCircle(190.0f, 30.0f, 55.0f);

    drawFeatheredCircle(580.0f, 100.0f + SHADOW_OFFSET, 100.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(490.0f, 80.0f + SHADOW_OFFSET, 70.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(410.0f, 30.0f + SHADOW_OFFSET, 55.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);

    glColor3f(0.15f, 0.45f, 0.2f);
    drawCircle(580.0f, 100.0f, 100.0f);
    drawCircle(490.0f, 80.0f, 70.0f);
    drawCircle(410.0f, 30.0f, 55.0f);
}

/* Front bushes with shadow */
void drawBushes_FrontLayer()
{
    float shadow_alpha = is_day ? SHADOW_ALPHA_DAY : SHADOW_ALPHA_NIGHT;
    float feather_width = 10.0f;

    drawFeatheredCircle(20.0f, 70.0f + SHADOW_OFFSET, 90.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(100.0f, 50.0f + SHADOW_OFFSET, 60.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(160.0f, 20.0f + SHADOW_OFFSET, 40.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);

    glColor3f(0.3f, 0.65f, 0.35f);
    drawCircle(20.0f, 70.0f, 90.0f);
    drawCircle(100.0f, 50.0f, 60.0f);
    drawCircle(160.0f, 20.0f, 40.0f);

    drawFeatheredCircle(580.0f, 70.0f + SHADOW_OFFSET, 90.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(500.0f, 50.0f + SHADOW_OFFSET, 60.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(440.0f, 20.0f + SHADOW_OFFSET, 40.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);

    glColor3f(0.3f, 0.65f, 0.35f);
    drawCircle(580.0f, 70.0f, 90.0f);
    drawCircle(500.0f, 50.0f, 60.0f);
    drawCircle(440.0f, 20.0f, 40.0f);
}

/* Draw all bushes */
void drawAllBushes()
{
    drawBushes_BackLayer();
    drawBushes_FrontLayer();
}

/* Draw greeting text when enabled */
void drawGreetingText() {
    if (show_greeting) {
        glColor4f(1.0f, 0.84f, 0.0f, greeting_alpha);
        glRasterPos2f(50.0f, 750.0f);
        const char* text = "Happy 20th Anniversary, XJTLU!";
        for (const char* c = text; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
        }
    }
}

/* Simple cover polygons that mask the building */
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
}

/* Two-tone rotating ring on the cover */
void drawCoverRing()
{
    const float centerX = 300.0f;
    const float centerY = 400.0f;
    const float outerRadius = 220.0f;
    const float ringWidth = 25.0f;
    const float innerRadius = outerRadius - ringWidth;
    const int numSegments = 100;

    glPushMatrix();
    glTranslatef(centerX, centerY, 0.0f);
    glRotatef(ringRotationAngle, 0.0f, 0.0f, 1.0f);
    glTranslatef(-centerX, -centerY, 0.0f);

    glColor3f(0.67f, 0.61f, 0.70f);
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = -90; i <= 90; ++i)
    {
        float angle = i * 3.14159f / 180.0f;
        glVertex2f(centerX + outerRadius * cos(angle), centerY + outerRadius * sin(angle));
        glVertex2f(centerX + innerRadius * cos(angle), centerY + innerRadius * sin(angle));
    }
    glEnd();

    glColor3f(0.33f, 0.23f, 0.40f);
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 90; i <= 270; ++i)
    {
        float angle = i * 3.14159f / 180.0f;
        glVertex2f(centerX + outerRadius * cos(angle), centerY + outerRadius * sin(angle));
        glVertex2f(centerX + innerRadius * cos(angle), centerY + innerRadius * sin(angle));
    }
    glEnd();

    glPopMatrix();
}

/* Draw the greeting card cover */
void drawGreetingCardCover()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

    glPushMatrix();
    glTranslatef(buildingPosX, buildingPosY, 0.0f);
    glScalef(buildingScaleX, buildingScaleY, 1.0f);
    drawBuilding_LowerLayer();
    drawBuilding_UpperLayer_Light();
    drawBuilding_UpperLayer_Dark();
    drawBuilding_Stripes();

    drawCoverPolygons();
    glPopMatrix();

    drawCoverRing();

    glColor3f(1.0f, 1.0f, 1.0f);
    float textX = 180.0f;
    float textY = buildingPosY - 30.0f;
    glRasterPos2f(textX, textY);
    const char* text = "XJTLU 20TH ANNIVERSARY";
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}

void display() {
    if (isCoverVisible)
    {
        drawGreetingCardCover();
    }
    else
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();

        drawSky();
        drawSunOrMoon();
        drawLayeredBackgroundClouds();
        drawCloud(cloud1_posX, 650.0f, 1.0f);
        drawCloud(cloud2_posX, 550.0f, 0.8f);
        drawGrass();

        glPushMatrix();
        drawXJTLUCenterBuilding();
        glPopMatrix();
        drawAllBushes();

        drawGreetingText();
    }

    glutSwapBuffers();
}

/* Animation update called by timer */
void update(int value) {

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

    cloud1_posX += 0.2f;
    if (cloud1_posX > 700.0f) {
        cloud1_posX = -100.0f;
    }

    cloud2_posX += 0.15f;
    if (cloud2_posX > 700.0f) {
        cloud2_posX = -100.0f;
    }

    if (show_greeting && greeting_alpha < 1.0f) {
        greeting_alpha += 0.02f;
    }
    else if (!show_greeting && greeting_alpha > 0.0f) {
        greeting_alpha -= 0.02f;
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 'o': case 'O':
        if (!isAnimating) {
            isAnimating = true;
            ringRotationAngle = 0.0f;

            if (isCoverVisible) {
                isOpening = true;
            }
            else {
                isCoverVisible = true;
                isAnimating = false;
                ringRotationAngle = 0.0f;
            }
        }
        break;
    case 'n': case 'N':
        is_day = !is_day;
        break;
    case 'q': case 'Q': case 27:
        exit(0);
        break;
    }
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        show_greeting = !show_greeting;
    }
}

void reshape(int w, int h)
{
    glViewport(0, 0, 600, 800);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 600.0, 0.0, 800.0);

    if (w != 600 || h != 800) {
        glutReshapeWindow(600, 800);
    }

    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
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
    glutMouseFunc(mouse);
    glutTimerFunc(16, update, 0);

    std::cout << "--- Controls ---" << std::endl;
    std::cout << "Press 'o': Open/close the card" << std::endl;
    std::cout << "Press 'n': Toggle day/night" << std::endl;
    std::cout << "Left click: Show/hide greeting" << std::endl;
    std::cout << "Press 'q' or 'ESC': Quit" << std::endl;

    glutMainLoop();
    return 0;
}