#include <iostream>
#include <GL/freeglut.h>
#include <cmath>

// --- 全局变量定义 ---
int windowWidth = 600;
int windowHeight = 800;

bool is_day = true;
bool show_greeting = false;
bool isCoverVisible = true;
// 太阳/月亮参数
float sunPosX = 500.0f;    // 太阳的X轴中心
float sunPosY = 700.0f;    // 太阳的Y轴中心 
float sunRadius = 50.0f;   // 太阳的半径

// 云朵动画参数
float cloud1_posX = 100.0f;
float cloud2_posX = 450.0f;

// 祝福语动画参数
float greeting_alpha = 0.0f; // 祝福语透明度

//cb
float buildingPosX = 150.0f; // X轴中心位置
float buildingPosY = 300.0f; // Y轴基线位置
float buildingScaleX = 0.3f;
float buildingScaleY = 0.3f;

const float SHADOW_OFFSET = 7.0f; // 阴影偏移的像素距离
const float SHADOW_ALPHA_DAY = 0.2f;  // 白天阴影的透明度 
const float SHADOW_ALPHA_NIGHT = 0.3f; // 夜晚阴影
/**
 * @brief 辅助函数：在指定位置绘制一个实心圆
 * @param cx 圆心x坐标
 * @param cy 圆心y坐标
 * @param r  半径
 */
void drawCircle(float cx, float cy, float r) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 360; i++) {
        float angle = i * 3.14159f / 180.0f;
        glVertex2f(cx + r * cos(angle), cy + r * sin(angle));
    }
    glEnd();
}

/**
 * @brief 辅助函数：绘制一个带有羽化/模糊边缘的实心圆
 * @param cx         圆心x坐标
 * @param cy         圆心y坐标
 * @param radius     圆的最大半径
 * @param feather    羽化的宽度（像素）
 * @param r, g, b, a 圆心的颜色和最大不透明度
 * @param layers     用于模拟的层数，越多越平滑但性能开销越大
 */
void drawFeatheredCircle(float cx, float cy, float radius, float feather,
    float r, float g, float b, float a, int layers = 10)
{
    // 循环绘制每一层，从最外层（最大、最透明）开始
    for (int i = 0; i < layers; ++i)
    {
        // 计算当前层的半径
        // 从 radius 线性减小到 radius - feather
        float currentRadius = radius - ((float)i / layers) * feather;

        // 计算当前层的透明度
        // 从 0.0 线性增加到 a
        float currentAlpha = a * ((float)(i + 1) / layers);

        // 设置颜色并绘制
        glColor4f(r, g, b, currentAlpha);
        drawCircle(cx, cy, currentRadius);
    }
}
/**
 * @brief 使用4个控制点绘制一条三次贝塞尔曲线
 * @param p0x, p0y 起点 (P0) 的坐标
 * @param p1x, p1y 起点控制柄 (P1) 的坐标
 * @param p2x, p2y 终点控制柄 (P2) 的坐标
 * @param p3x, p3y 终点 (P3) 的坐标
 * @param segments 曲线的平滑度，段数越多越平滑
 */
void drawCubicBezierCurve(float p0x, float p0y, float p1x, float p1y,
    float p2x, float p2y, float p3x, float p3y,
    int segments = 50)
{
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= segments; ++i)
    {
        // 计算参数 t (从 0.0 到 1.0)
        float t = (float)i / (float)segments;
        float t_inv = 1.0f - t;

        // 三次贝塞尔曲线 
        float b0 = t_inv * t_inv * t_inv;
        float b1 = 3.0f * t * t_inv * t_inv;
        float b2 = 3.0f * t * t * t_inv;
        float b3 = t * t * t;

        // 根据混合权重和控制点坐标，计算出曲线上当前 t 位置的 (x, y) 坐标
        float x = b0 * p0x + b1 * p1x + b2 * p2x + b3 * p3x;
        float y = b0 * p0y + b1 * p1y + b2 * p2y + b3 * p3y;

        // 将计算出的点添加到线带中
        glVertex2f(x, y);
    }
    glEnd();
}

/**
 * 建筑的下层
 * 包含蓝色和灰蓝色的两个主要面
 */
void drawBuilding_LowerLayer()
{
    // 绘制左侧多边形
    glColor3f(0.68f, 0.73f, 0.79f);
    glBegin(GL_POLYGON);
    glVertex2f(50.0f, 0.0f);
    glVertex2f(50.0f, 550.0f);
    glVertex2f(450.0f, 750.0f);
    glVertex2f(450.0f, 0.0f);
    glEnd();

    // 绘制右侧多边形
    glColor3f(0.25f, 0.46f, 0.62f);
    glBegin(GL_POLYGON);
    glVertex2f(450.0f, 0.0f);
    glVertex2f(450.0f, 750.0f);
    glVertex2f(850.0f, 550.0f);
    glVertex2f(850.0f, 0.0f);
    glEnd();
}

/**
 * 建筑的上层浅色部分
 * 包含三个独立的浅米色多边形
 */
void drawBuilding_UpperLayer_Light()
{
    // 设置统一颜色
    glColor3f(0.91f, 0.88f, 0.84f);

    // 多边形 1
    glBegin(GL_POLYGON);
    glVertex2f(0.0f, 300.0f);
    glVertex2f(0.0f, 550.0f);
    glVertex2f(200.0f, 650.0f);
    glVertex2f(200.0f, 350.0f);
    glEnd();

    // 多边形 2
    glBegin(GL_POLYGON);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(0.0f, 250.0f);
    glVertex2f(150.0f, 285.0f);
    glVertex2f(450.0f, 100.0f);
    glVertex2f(450.0f, 0.0f);
    glEnd();

    // 多边形 3
    glBegin(GL_POLYGON);
    glVertex2f(250.0f, 250.0f);
    glVertex2f(250.0f, 675.0f);
    glVertex2f(450.0f, 775.0f);
    glVertex2f(450.0f, 300.0f);
    glEnd();
}

/**
 * 建筑的上层深色部分
 * 包含两个独立的深灰色多边形
 */
void drawBuilding_UpperLayer_Dark()
{

    glColor3f(0.56f, 0.53f, 0.51f);

    // --- 多边形1 拆分后的多边形 A ---
    glBegin(GL_POLYGON);
    glVertex2f(450.0f, 300.0f);
    glVertex2f(450.0f, 775.0f);
    glVertex2f(600.0f, 700.0f);
    glVertex2f(650.0f, 500.0f);
    glVertex2f(700.0f, 490.0f);
    glVertex2f(700.0f, 400.0f);
    glVertex2f(650.0f, 250.0f);
    glEnd();

    // --- 多边形1 拆分后的多边形 B ---
    glBegin(GL_POLYGON);
    glVertex2f(700.0f, 650.0f);
    glVertex2f(900.0f, 550.0f);
    glVertex2f(900.0f, 250.0f);
    glVertex2f(800.0f, 260.0f);
    glVertex2f(750.0f, 450.0f);
    glVertex2f(700.0f, 400.0f);
    glEnd();

    // --- 多边形2  ---
    glBegin(GL_POLYGON);
    glVertex2f(450.0f, 0.0f);
    glVertex2f(450.0f, 100.0f);
    glVertex2f(800.0f, 200.0f);
    glVertex2f(900.0f, 200.0f);
    glVertex2f(900.0f, 0.0f);
    glEnd();
}



void drawSlantedStripe(float x1, float y1, float x2, float y2, float height)
{
    // 计算线段的向量
    float dx = x2 - x1;
    float dy = y2 - y1;

    // 计算线段的长度，用于标准化
    float length = sqrt(dx * dx + dy * dy);
    if (length == 0) return; // 避免除以零

    // 计算垂直于线段的单位向量，并乘以厚度的一半
    // 这是用来将线段的两个端点向两侧扩展，形成四边形的四个顶点
    float perp_dx = -dy / length * (height / 2.0f);
    float perp_dy = dx / length * (height / 2.0f);

    // 计算四边形的四个顶点
    float v1x = x1 + perp_dx;
    float v1y = y1 + perp_dy;

    float v2x = x2 + perp_dx;
    float v2y = y2 + perp_dy;

    float v3x = x2 - perp_dx;
    float v3y = y2 - perp_dy;

    float v4x = x1 - perp_dx;
    float v4y = y1 - perp_dy;

    // 使用GL_QUADS绘制这个细长的四边形
    glBegin(GL_QUADS);
    glVertex2f(v1x, v1y);
    glVertex2f(v2x, v2y);
    glVertex2f(v3x, v3y);
    glVertex2f(v4x, v4y);
    glEnd();
}
/**
 * @brief 绘制建筑的条纹
 */
void drawBuilding_Stripes()
{
    // --- 参数定义 ---
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

// 绘制天空
void drawSky() {
    if (is_day) {
        // 白天
        glBegin(GL_QUADS);
        glColor3f(0.26f, 0.61f, 0.97f); // 顶部
        glVertex2f(0.0f, 800.0f);
        glVertex2f(600.0f, 800.0f);
        glColor3f(1.0f, 1.0f, 1.0f);// 底部
        glVertex2f(600.0f, 0.0f);
        glVertex2f(0.0f, 0.0f);
        glEnd();
    }
    else {
        // 夜晚
        glBegin(GL_QUADS);
        glColor3f(0.05f, 0.05f, 0.2f); // 顶部
        glVertex2f(0.0f, 800.0f);
        glVertex2f(600.0f, 800.0f);
        glColor3f(0.1f, 0.1f, 0.35f); // 底部
        glVertex2f(600.0f, 0.0f);
        glVertex2f(0.0f, 0.0f);
        glEnd();
    }
}

// 绘制草地
void drawGrass() {
    glBegin(GL_QUADS);
    glColor3f(0.5f, 0.78f, 0.37f);
    glVertex2f(0.0f, 300.0f); 
    glVertex2f(600.0f, 300.0f);
    glVertex2f(600.0f, 0.0f);
    glVertex2f(0.0f, 0.0f);
    glEnd();
}

void drawSunOrMoon() {
    if (is_day) {
        glColor3f(1.0f, 0.84f, 0.0f); // 太阳
    }
    else {
        glColor3f(0.9f, 0.9f, 0.85f); // 月亮
    }

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(sunPosX, sunPosY); // 圆心
    for (int i = 0; i <= 360; i++) {
        float angle = i * 3.14159f / 180.0f;
        glVertex2f(sunPosX + sunRadius * cos(angle),
            sunPosY + sunRadius * sin(angle));
    }
    glEnd();
}

/**
 * @brief 绘制最远处的云层 (第三层)
 */
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

    // 根据白天/夜晚状态设置颜色
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

/**
 * @brief 绘制中间的云层 (第二层)
 */
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

/**
 * @brief 绘制最前景的云层 (第一层)
 */
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
        glColor3f(1.0f, 1.0f, 1.0f); // 白天: 纯白色
    }
    else {
        glColor3f(0.85f, 0.9f, 0.95f); // 夜晚: 极浅的蓝白色
    }

    drawCircle(50.0f, 320.0f, 90.0f);
    drawCircle(150.0f, 290.0f, 80.0f);
    drawCircle(250.0f, 260.0f, 70.0f);
    drawCircle(350.0f, 260.0f, 70.0f);
    drawCircle(450.0f, 290.0f, 80.0f);
    drawCircle(550.0f, 320.0f, 90.0f);
}


/**
 * @brief 绘制三层背景云
 */
void drawLayeredBackgroundClouds()
{
    drawCloudLayer_Back();
    drawCloudLayer_Middle();
    drawCloudLayer_Front();
}


/**
 * @brief 绘制云朵 
 * @param x_offset 云朵的基准x坐标
 * @param y_offset 云朵的基准y坐标
 * @param scale    云朵的整体缩放
 */
void drawCloud(float x_offset, float y_offset, float scale) {
    glColor4f(1.0f, 1.0f, 1.0f, 0.9f); // 白色半透明

    // 使用 drawCircle 辅助函数，通过组合多个圆形来创建云朵
    drawCircle(x_offset, y_offset, 25 * scale);
    drawCircle(x_offset + 30 * scale, y_offset + 5 * scale, 30 * scale);
    drawCircle(x_offset - 30 * scale, y_offset + 2 * scale, 20 * scale);
    drawCircle(x_offset + 15 * scale, y_offset + 20 * scale, 20 * scale);
    drawCircle(x_offset - 10 * scale, y_offset + 15 * scale, 22 * scale);
}

/**
 * @brief 绘制背景层的灌木 (后景)
 */
void drawBushes_BackLayer()

{
    float shadow_alpha = is_day ? SHADOW_ALPHA_DAY : SHADOW_ALPHA_NIGHT;
    float feather_width = 10.0f;

   
    //左侧后景灌木
    drawFeatheredCircle(20.0f, 100.0f + SHADOW_OFFSET, 100.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(110.0f, 80.0f + SHADOW_OFFSET, 70.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(190.0f, 30.0f + SHADOW_OFFSET, 55.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);

    glColor3f(0.15f, 0.45f, 0.2f);
    drawCircle(20.0f, 100.0f, 100.0f);
    drawCircle(110.0f, 80.0f, 70.0f);
    drawCircle(190.0f, 30.0f, 55.0f);
    
    // 右侧后景灌木
    drawFeatheredCircle(580.0f, 100.0f + SHADOW_OFFSET, 100.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(490.0f, 80.0f + SHADOW_OFFSET, 70.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(410.0f, 30.0f + SHADOW_OFFSET, 55.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);

    glColor3f(0.15f, 0.45f, 0.2f);
    drawCircle(580.0f, 100.0f, 100.0f);
    drawCircle(490.0f, 80.0f, 70.0f);
    drawCircle(410.0f, 30.0f, 55.0f);
}

/**
 * @brief 绘制前景层的灌木
 */
void drawBushes_FrontLayer()
{
    float shadow_alpha = is_day ? SHADOW_ALPHA_DAY : SHADOW_ALPHA_NIGHT;
    float feather_width = 10.0f; 

    // 左侧前景灌木 
    // 阴影
    drawFeatheredCircle(20.0f, 70.0f + SHADOW_OFFSET, 90.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(100.0f, 50.0f + SHADOW_OFFSET, 60.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(160.0f, 20.0f + SHADOW_OFFSET, 40.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);

    glColor3f(0.3f, 0.65f, 0.35f); 
    drawCircle(20.0f, 70.0f, 90.0f);
    drawCircle(100.0f, 50.0f, 60.0f);
    drawCircle(160.0f, 20.0f, 40.0f);
    // 右侧前景灌木 
    // 阴影
    drawFeatheredCircle(580.0f, 70.0f + SHADOW_OFFSET, 90.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(500.0f, 50.0f + SHADOW_OFFSET, 60.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);
    drawFeatheredCircle(440.0f, 20.0f + SHADOW_OFFSET, 40.0f, feather_width, 0.0f, 0.0f, 0.0f, shadow_alpha);

    glColor3f(0.3f, 0.65f, 0.35f);
    drawCircle(580.0f, 70.0f, 90.0f);
    drawCircle(500.0f, 50.0f, 60.0f);
    drawCircle(440.0f, 20.0f, 40.0f);
}

/**
 * @brief 主函数：按正确顺序绘制所有灌木
 */
void drawAllBushes()
{
    drawBushes_BackLayer();
    drawBushes_FrontLayer();
}
/**
 * @brief 绘制祝福语
 */
void drawGreetingText() {
    if (show_greeting) {
        glColor4f(1.0f, 0.84f, 0.0f, greeting_alpha);

        // 将文本放置在窗口左上角 (50, 750) 的位置
        glRasterPos2f(50.0f, 750.0f);

        const char* text = "Happy 20th Anniversary, XJTLU!";
        for (const char* c = text; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
        }
    }
}

/**
 * @brief 绘制用于遮挡建筑的白色多边形 (封面专用)
 */
void drawCoverPolygons()
{
    // 设置为纯白色
    glColor3f(1.0f, 1.0f, 1.0f);

    // 根据您提供的坐标绘制三个多边形
    // 多边形 1
    glBegin(GL_POLYGON);
    glVertex2f(0.0f, 450.0f);
    glVertex2f(350.0f, 625.0f);
    glVertex2f(350.0f, 450.0f);
    glVertex2f(0.0f, 100.0f);
    glEnd();

    // 多边形 2
    glBegin(GL_POLYGON);
    glVertex2f(150.0f, 100.0f);
    glVertex2f(450.0f, 400.0f);
    glVertex2f(450.0f, 100.0f);
    glEnd();

    // 多边形 3
    glBegin(GL_POLYGON);
    glVertex2f(550.0f, 620.0f);
    glVertex2f(800.0f, 490.0f);
    glVertex2f(800.0f, 100.0f);
    glVertex2f(550.0f, 100.0f);
    glEnd();
}

/**
 * @brief 主函数：绘制完整的贺卡封面
 */
void drawGreetingCardCover()
{
    // 1. 设置纯白色背景
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 2. 绘制居中的建筑
    // 注意：这里的变换与内部场景的建筑变换是独立的
    // 但我们使用相同的全局变量，以确保位置和大小一致
    glPushMatrix();
    glTranslatef(buildingPosX, buildingPosY, 0.0f);
    glScalef(buildingScaleX, buildingScaleY, 1.0f);

    // 先绘制建筑本身
    drawBuilding_LowerLayer();
    drawBuilding_UpperLayer_Light();
    drawBuilding_UpperLayer_Dark();
    drawBuilding_Stripes();

    // 然后在建筑上层绘制用于遮挡的白色多边形
    drawCoverPolygons();

    glPopMatrix();
}
//void display() {
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//    glLoadIdentity();
//
//    // 绘制背景
//    drawSky();
//    drawSunOrMoon();
//    drawLayeredBackgroundClouds();
//    drawCloud(cloud1_posX, 650.0f, 1.0f);
//    drawCloud(cloud2_posX, 550.0f, 0.8f);
//
//    // 绘制前景
//    drawGrass();
//    glPushMatrix();
//    drawXJTLUCenterBuilding();
//    drawAllBushes();
//    glPopMatrix();
//
//    drawGreetingText();
//
//    glutSwapBuffers();
//
//}
void display() {
    // 根据状态执行不同的绘制逻辑
    if (isCoverVisible)
    {
        // --- 状态一：绘制封面 ---
        drawGreetingCardCover();
    }
    else
    {
        // --- 状态二：绘制贺卡内部场景 ---
        // (这里的代码就是您之前 display 函数的全部内容)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();

        drawSky();
        drawSunOrMoon();
        drawLayeredBackgroundClouds();
        drawCloud(cloud1_posX, 650.0f, 1.0f);
        drawCloud(cloud2_posX, 550.0f, 0.8f);
        drawGrass();

        // 注意：这里的PushMatrix/PopMatrix是为了隔离建筑和灌木的变换
        // 但您的代码中将它们放在了一起，我会遵循您的版本
        glPushMatrix();
        drawXJTLUCenterBuilding();
        drawAllBushes();
        glPopMatrix();

        drawGreetingText();
    }

    // 无论哪个状态，最后都要交换缓冲区以显示画面
    glutSwapBuffers();
}

/**
 * @brief 动画更新函数
 */
void update(int value) {
    // 云朵动画
    cloud1_posX += 0.2f; // 调整速度
    if (cloud1_posX > 700.0f) { // 飘出右边界
        cloud1_posX = -100.0f; // 从左边界外重生
    }

    cloud2_posX += 0.15f; // 调整速度
    if (cloud2_posX > 700.0f) { // 飘出右边界
        cloud2_posX = -100.0f; // 从左边界外重生
    }

    // 祝福语
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
        isCoverVisible = !isCoverVisible;
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
    // 设置视口
    glViewport(0, 0, 600, 800);

    // 设置投影矩阵
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 600.0, 0.0, 800.0); 

    // 强制窗口尺寸恢复
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

    glutCreateWindow("XJTLU 20th Anniversary Greeting Card - Detailed Building");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutTimerFunc(16, update, 0);

    std::cout << "--- 操作指南 ---" << std::endl;
    std::cout << "按 'o' 键: 打开/合上贺卡" << std::endl; // <-- 新增
    std::cout << "按 'n' 键: 切换白天和夜晚模式" << std::endl;
    std::cout << "点击鼠标左键: 显示/隐藏祝福语" << std::endl;
    std::cout << "按 'q' 或 'ESC' 键: 退出程序" << std::endl;

    glutMainLoop();
    return 0;
}