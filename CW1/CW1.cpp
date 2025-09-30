#include <iostream>
#include <GL/freeglut.h>
#include <cmath>

// --- 全局变量定义 ---
int windowWidth = 600;
int windowHeight = 800;

bool is_day = true;
bool show_greeting = false;

// 太阳/月亮参数
float sunPosX = 500.0f;    // 太阳的X轴中心
float sunPosY = 700.0f;    // 太阳的Y轴中心 (可动画)
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

/**
 * 建筑的下层
 * 包含蓝色和灰蓝色的两个主要面
 */
void drawBuilding_LowerLayer()
{
    // 1. 绘制左侧的灰蓝色多边形
    glColor3f(0.68f, 0.73f, 0.79f);
    glBegin(GL_POLYGON);
    glVertex2f(50.0f, 0.0f);
    glVertex2f(50.0f, 550.0f);
    glVertex2f(450.0f, 750.0f);
    glVertex2f(450.0f, 0.0f);
    glEnd();

    // 2. 绘制右侧的蓝色多边形
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
    // RGB(233, 226, 214) -> (0.91, 0.88, 0.84)
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
 * @brief 根据精确的坐标数据绘制建筑的水平纹理条纹
 *
 * 该函数根据预设的y坐标和x范围，绘制一系列有厚度的矩形条纹。
 * 左侧条纹颜色稍浅，右侧稍深，以营造光照和层次感。
 */
void drawBuilding_Stripes()
{
    // --- 参数定义 ---
    const float stripeHeight = 10.0f; 
    const float leftColorR = 0.39f, leftColorG = 0.35f, leftColorB = 0.33f;
    const float rightColorR = 0.31f, rightColorG = 0.27f, rightColorB = 0.25f; 


    glColor3f(leftColorR, leftColorG, leftColorB);
    drawSlantedStripe(0.0f, 50.0f, 400.0f, 50.0f, stripeHeight);
    drawSlantedStripe(0.0f, 100.0f, 300.0f, 100.0f, stripeHeight);
    drawSlantedStripe(300.0f, 650.0f, 450.0f, 725.0f, stripeHeight); 
    drawSlantedStripe(300.0f, 600.0f, 450.0f, 675.0f, stripeHeight); 
    drawSlantedStripe(300.0f, 550.0f, 450.0f, 625.0f, stripeHeight); 
    drawSlantedStripe(0.0f, 500.0f, 150.0f, 575.0f, stripeHeight); 
    drawSlantedStripe(0.0f, 450.0f, 100.0f, 500.0f, stripeHeight);

    glColor3f(rightColorR, rightColorG, rightColorB);
    drawSlantedStripe(650.0f, 50.0f, 900.0f, 50.0f, stripeHeight);
    drawSlantedStripe(500.0f, 100.0f, 700.0f, 100.0f, stripeHeight);
}

void drawXJTLUCenterBuilding()
{
    glPushMatrix();

    glTranslatef(buildingPosX, buildingPosY, 0.0f);
    glScalef(buildingScaleX, buildingScaleY, 1.0f);

    // 按照从后往前的顺序绘制
    drawBuilding_LowerLayer();
    drawBuilding_UpperLayer_Light();
    drawBuilding_UpperLayer_Dark();

    drawBuilding_Stripes();
    glPopMatrix();
}

// 绘制天空
void drawSky() {
    // 根据新坐标系 (0,0) 到 (600,800) 来绘制
    if (is_day) {
        // 白天：渐变蓝天
        glBegin(GL_QUADS);
        glColor3f(0.53f, 0.81f, 0.92f); // 顶部浅蓝色
        glVertex2f(0.0f, 800.0f);
        glVertex2f(600.0f, 800.0f);
        glColor3f(0.82f, 0.94f, 0.98f); // 底部更浅的蓝色
        glVertex2f(600.0f, 0.0f);
        glVertex2f(0.0f, 0.0f);
        glEnd();
    }
    else {
        // 夜晚：深蓝色夜空
        glBegin(GL_QUADS);
        glColor3f(0.05f, 0.05f, 0.2f); // 顶部深蓝
        glVertex2f(0.0f, 800.0f);
        glVertex2f(600.0f, 800.0f);
        glColor3f(0.1f, 0.1f, 0.35f); // 底部更深的蓝
        glVertex2f(600.0f, 0.0f);
        glVertex2f(0.0f, 0.0f);
        glEnd();
    }
}

// 绘制草地
void drawGrass() {
    glBegin(GL_QUADS);
    glColor3f(0.5f, 0.78f, 0.37f);
    glVertex2f(0.0f, 300.0f); // 高度可以自定义
    glVertex2f(600.0f, 300.0f);
    glVertex2f(600.0f, 0.0f);
    glVertex2f(0.0f, 0.0f);
    glEnd();
}

void drawSunOrMoon() {
    if (is_day) {
        glColor3f(1.0f, 0.84f, 0.0f); // 黄色太阳
    }
    else {
        glColor3f(0.9f, 0.9f, 0.85f); // 浅黄色月亮
    }

    // 使用新的全局变量来绘制
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
 * @brief 绘制云朵 (适配 600x800 坐标系)
 * @param x_offset 云朵的基准x坐标
 * @param y_offset 云朵的基准y坐标
 * @param scale    云朵的整体缩放
 */
void drawCloud(float x_offset, float y_offset, float scale) {
    glColor4f(1.0f, 1.0f, 1.0f, 0.9f); // 白色半透明

    // 使用 drawCircle 辅助函数，通过组合多个圆形来创建云朵
    // 所有坐标和半径都是基于 (x_offset, y_offset) 的相对值
    drawCircle(x_offset, y_offset, 25 * scale);
    drawCircle(x_offset + 30 * scale, y_offset + 5 * scale, 30 * scale);
    drawCircle(x_offset - 30 * scale, y_offset + 2 * scale, 20 * scale);
    drawCircle(x_offset + 15 * scale, y_offset + 20 * scale, 20 * scale);
    drawCircle(x_offset - 10 * scale, y_offset + 15 * scale, 22 * scale);
}



/**
 * @brief 绘制祝福语 (适配 600x800 坐标系)
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

// --- 主回调函数 ---

// 核心绘制函数
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // 绘制背景
    drawSky();
    drawSunOrMoon();
    drawCloud(cloud1_posX, 650.0f, 1.0f);
    drawCloud(cloud2_posX, 550.0f, 0.8f);

    // 绘制前景
    drawGrass();


    glPushMatrix();
    drawXJTLUCenterBuilding();
    glPopMatrix();

    drawGreetingText();

    glutSwapBuffers();
}


/**
 * @brief 动画更新函数
 */
void update(int value) {
    // 云朵动画：在窗口 (0, 600) 范围内来回移动
    cloud1_posX += 0.2f; // 调整速度
    if (cloud1_posX > 700.0f) { // 飘出右边界
        cloud1_posX = -100.0f; // 从左边界外重生
    }

    cloud2_posX += 0.15f; // 调整速度
    if (cloud2_posX > 700.0f) { // 飘出右边界
        cloud2_posX = -100.0f; // 从左边界外重生
    }

    // 祝福语淡入淡出动画 (逻辑不变)
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
    // 1. 设置视口
    glViewport(0, 0, 600, 800);

    // 2. 设置投影矩阵
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 600.0, 0.0, 800.0); // 坐标系左下角(0,0), 右上角(600,800)

    // 3. 强制窗口尺寸恢复到 600x800
    if (w != 600 || h != 800) {
        glutReshapeWindow(600, 800);
    }

    glMatrixMode(GL_MODELVIEW);
}

// --- 主函数 ---
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
    std::cout << "按 'n' 键: 切换白天和夜晚模式" << std::endl;
    std::cout << "点击鼠标左键: 显示/隐藏祝福语" << std::endl;
    std::cout << "按 'q' 或 'ESC' 键: 退出程序" << std::endl;

    glutMainLoop();
    return 0;
}