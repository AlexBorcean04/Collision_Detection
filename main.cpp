#include <GL/glut.h>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>
#include <string>

const int WINDOW_WIDTH  = 800;
const int WINDOW_HEIGHT = 600;

// Paddle
float paddleX      = 350.0f;
float paddleY      = 580.0f;
float paddleWidth  = 100.0f;
float paddleHeight = 20.0f;
float originalPaddleWidth = 100.0f;

//Ball
struct Ball {
    float x, y;
    float speedX, speedY;
    float size;
    bool  active;
};

std::vector<Ball> balls;
int   activeBallsCount  = 0;
float defaultBallSpeed  = 3.0f;
float defaultBallSize   = 15.0f;

// an enum to handle whether we're at the menu or playing
enum GameState {
    STATE_MENU,
    STATE_PLAY
};

GameState currentState = STATE_MENU; // start at the menu

int score = 0;
int level = 1;
int lives = 3;
bool gameOver = false;

int hitsSinceLastSpeedUp = 0;

// Power-ups
enum PowerUpType {
    PU_WIDEN_PADDLE,  // Double arrow shape
    PU_EXTRA_LIFE,    // Heart shape
    PU_MULTI_BALL,    // Cluster of spheres
    PU_SLOW_MOTION,   // Hourglass shape
    PU_SPEED_BOOST    // Lightning shape
};

struct PowerUp {
    bool  active;
    float x, y;
    float size;
    PowerUpType type;
    float rotationAngle; // for spinning
};

const int MAX_POWERUPS = 5;
PowerUp powerUps[MAX_POWERUPS];
bool slowMotionActive = false;
int slowMotionDuration = 0;
bool paddleWidened = false;
int paddleWidenedTimer = 0;

// Forward Declarations
void initGame();
void spawnInitialBall();
void loseLifeAndRespawnBall();
bool checkCollisionAABB(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2);
void trySpawnPowerUp();
void applyPowerUpEffect(PowerUpType type);
void updateBalls();
void updatePowerUps();
void drawDoubleArrow3D(float size);
void drawHeart3D(float size);
void drawCluster3D(float size);
void drawHourglass3D(float size);
void drawLightning3D(float size);

void displayText(float x, float y, const std::string &text);

// Game Initialization
void initGame() {
    balls.clear();
    // Mark all power-ups inactive
    for (int i = 0; i < MAX_POWERUPS; i++) {
        powerUps[i].active = false;
    }
    score = 0;
    level = 1;
    lives = 3;
    gameOver = false;

    paddleWidth = originalPaddleWidth;
    paddleWidened = false;
    slowMotionActive = false;
    slowMotionDuration = 0;
    spawnInitialBall();
    std::cout << "Game initialized!\n";
}

// Spawns a single ball in the middle with random X direction
void spawnInitialBall() {
    Ball b;
    b.x = WINDOW_WIDTH / 2.0f;
    b.y = WINDOW_HEIGHT / 2.0f;
    b.size = defaultBallSize;
    b.active = true;
    b.speedX = (rand() % 2 == 0) ? defaultBallSpeed : -defaultBallSpeed;
    b.speedY = -defaultBallSpeed;
    balls.push_back(b);
    activeBallsCount = 1;
}

// When a life is lost,the ball is respawned
// on the opposite side of the paddle
void loseLifeAndRespawnBall() {
    lives--;
    std::cout << "Lost a life! Lives left: " << lives << std::endl;
    if (lives <= 0) {
        gameOver = true;
        return;
    }

    for (auto &b : balls) {
        if (b.active) {
            // We place it near top, e.g. y=60, x ~ paddle center
            b.x = paddleX + paddleWidth/2.0f;
            b.y = 60.0f;
            // If speedY is currently positive (going up), invert it so it goes down.
            if (b.speedY > 0) b.speedY = -b.speedY;
            return;
        }
    }
    spawnInitialBall();
}

// Collision (AABB)
bool checkCollisionAABB(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2) {
    if (x1 + w1 < x2 || x2 + w2 < x1) return false;
    if (y1 + h1 < y2 || y2 + h2 < y1) return false;
    return true;
}

// Power-up logic
void trySpawnPowerUp() {
    // Small chance each frame
    if (rand() % 300 == 0) {
        for (int i = 0; i < MAX_POWERUPS; i++) {
            if (!powerUps[i].active) {
                powerUps[i].active = true;
                powerUps[i].x = (float)(rand() % (WINDOW_WIDTH - 50) + 25);
                powerUps[i].y = (float)(rand() % (WINDOW_HEIGHT - 100) + 25);
                powerUps[i].size = 20.0f;
                powerUps[i].rotationAngle = 0.0f;
                int t = rand() % 5;
                powerUps[i].type = (PowerUpType) t;
                break;
            }
        }
    }
}

void applyPowerUpEffect(PowerUpType type) {
    switch(type) {
        case PU_WIDEN_PADDLE:
            if (!paddleWidened) {
                paddleWidth *= 2.5f;
                paddleWidened = true;
                paddleWidenedTimer = 600;
                std::cout << "Paddle widened!\n";
            }
            break;

        case PU_EXTRA_LIFE:
            lives++;
            std::cout << "Extra life! Lives: " << lives << std::endl;
            break;

        case PU_MULTI_BALL: {
            // If there's at least one active ball, spawn two more from it
            for (auto &b : balls) {
                if (b.active) {
                    // spawn additional balls with slightly varied speeds
                    Ball b1 = b;
                    b1.speedX *= 1.1f;
                    b1.speedY *= -1.2f;
                    b1.active  = true;
                    balls.push_back(b1);
                    Ball b2 = b;
                    b2.speedX *= -1.2f;
                    b2.speedY *= 1.1f;
                    b2.active  = true;
                    balls.push_back(b2);
                    activeBallsCount += 2;
                    std::cout << "Multi-ball!\n";
                    break;
                }
            }
        }
        break;

        case PU_SLOW_MOTION:
            slowMotionActive   = true;
            slowMotionDuration = 300;
            std::cout << "Slow motion!\n";
            break;

        case PU_SPEED_BOOST:
            // Increase speed of all active balls
            for (auto &b : balls) {
                if (!b.active) continue;
                if (b.speedX > 0) b.speedX += 1.0f;
                else b.speedX -= 1.0f;
                if (b.speedY > 0) b.speedY += 1.0f;
                else b.speedY -= 1.0f;
            }
            std::cout << "Speed Boost!\n";
            break;
    }
}

void updatePowerUps() {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!powerUps[i].active) continue;
        // spin
        powerUps[i].rotationAngle += 2.0f;

        // collision with any active ball
        for (auto &b : balls) {
            if (!b.active) continue;
            float ballLeft   = b.x - b.size;
            float ballRight  = b.x + b.size;
            float ballTop    = b.y - b.size;
            float ballBottom = b.y + b.size;
            float puLeft   = powerUps[i].x - powerUps[i].size;
            float puRight  = powerUps[i].x + powerUps[i].size;
            float puTop    = powerUps[i].y - powerUps[i].size;
            float puBottom = powerUps[i].y + powerUps[i].size;

            if (checkCollisionAABB(puLeft, puTop, puRight - puLeft, puBottom - puTop, ballLeft, ballTop, ballRight - ballLeft, ballBottom - ballTop))
            {
                applyPowerUpEffect(powerUps[i].type);
                powerUps[i].active = false;
                break;
            }
        }
    }

    // handle durations
    if (paddleWidened) {
        paddleWidenedTimer--;
        if (paddleWidenedTimer <= 0) {
            paddleWidth    = originalPaddleWidth;
            paddleWidened  = false;
        }
    }

    if (slowMotionActive) {
        slowMotionDuration--;
        if (slowMotionDuration <= 0) {
            slowMotionActive = false;
        }
    }
}

void updateBalls() {
    float speedFactor = (slowMotionActive) ? 0.5f : 1.0f;

    for (auto &b : balls) {
        if (!b.active) continue;

        b.x += b.speedX * speedFactor;
        b.y += b.speedY * speedFactor;

        // check side walls
        if (b.x - b.size < 0) {
            b.x = b.size;
            b.speedX *= -1;
        }
        else if (b.x + b.size > WINDOW_WIDTH) {
            b.x = WINDOW_WIDTH - b.size;
            b.speedX *= -1;
        }

        // top wall
        if (b.y - b.size < 0) {
            b.y = b.size;
            b.speedY *= -1;
        }

        // paddle collision
        float ballLeft = b.x - b.size;
        float ballRight = b.x + b.size;
        float ballTop = b.y - b.size;
        float ballBottom = b.y + b.size;

        if (checkCollisionAABB(
                paddleX, paddleY, paddleWidth, paddleHeight, ballLeft, ballTop, (ballRight - ballLeft), (ballBottom - ballTop)))
        {
            b.y = paddleY - b.size;  // place above paddle
            b.speedY *= -1;
            score++;
            hitsSinceLastSpeedUp++;
            if (hitsSinceLastSpeedUp >= 5) {
                hitsSinceLastSpeedUp = 0;
                level++;
                // speed up all active balls slightly
                for (auto &bb : balls) {
                    if (!bb.active) continue;
                    if (bb.speedX > 0) bb.speedX += 0.5f; else bb.speedX -= 0.5f;
                    if (bb.speedY > 0) bb.speedY += 0.5f; else bb.speedY -= 0.5f;
                }
                std::cout << "Level up! " << level << std::endl;
            }
            std::cout << "Score: " << score << std::endl;
        }
        if (b.y - b.size > WINDOW_HEIGHT) {
            // ball is lost
            b.active = false;
            activeBallsCount--;
            if (activeBallsCount <= 0) {
                // lose a life => spawn from bottom side
                loseLifeAndRespawnBall();
                // If lives > 0, we still have 1 active ball now
                if (!gameOver) {
                    activeBallsCount = 1;
                }
            }
        }
    }
}

// Timer Callback (Game Loop)
void update(int value) {
    // Only update if we're in STATE_PLAY
    if (currentState == STATE_PLAY && !gameOver) {
        updateBalls();
        updatePowerUps();
        trySpawnPowerUp();
    }
    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void handleKeyboard(unsigned char key, int x, int y) {
    float moveSpeed = 20.0f;
    switch(key) {
        case 13: // Enter key
            // If we're on the menu, go to play
            if (currentState == STATE_MENU) {
                currentState = STATE_PLAY;
            }
            break;

        case 'a':
        case 'A':
            if (currentState == STATE_PLAY && !gameOver) {
                paddleX -= moveSpeed;
                if (paddleX < 0) paddleX = 0;
            }
            break;
        case 'd':
        case 'D':
            if (currentState == STATE_PLAY && !gameOver) {
                paddleX += moveSpeed;
                if (paddleX + paddleWidth > WINDOW_WIDTH) {
                    paddleX = WINDOW_WIDTH - paddleWidth;
                }
            }
            break;
        case 27: // ESC
            exit(0);
            break;
        default:
            break;
    }
}

// 3D Shapes for Power-Ups
// 1) "Double Arrow" shape for Widen Paddle
void drawDoubleArrow3D(float size) {
    static GLfloat arrowVerts[][2] = {
        // Left arrow
        { -2.f,  0.f }, { -1.f,  1.f }, { -1.f,  0.3f },
        {  1.f,  0.3f }, {  1.f,  1.f }, {  2.f,  0.f },
        {  1.f, -1.f }, {  1.f, -0.3f }, { -1.f, -0.3f },
        { -1.f, -1.f }
    };
    int numVerts = 10;
    float thickness = 0.3f * size;
    float scale     = 0.3f * size;

    // FRONT FACE
    glBegin(GL_TRIANGLE_FAN);
    for(int i = 0; i < numVerts; i++) {
        glVertex3f(arrowVerts[i][0]*scale, arrowVerts[i][1]*scale, thickness);
    }
    glEnd();

    // BACK FACE
    glBegin(GL_TRIANGLE_FAN);
    for(int i = numVerts-1; i >= 0; i--) {
        glVertex3f(arrowVerts[i][0]*scale, arrowVerts[i][1]*scale, -thickness);
    }
    glEnd();

    // SIDE QUADS
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i < numVerts; i++) {
        glVertex3f(arrowVerts[i][0]*scale, arrowVerts[i][1]*scale,  thickness);
        glVertex3f(arrowVerts[i][0]*scale, arrowVerts[i][1]*scale, -thickness);
    }
    // close loop
    glVertex3f(arrowVerts[0][0]*scale, arrowVerts[0][1]*scale, thickness);
    glVertex3f(arrowVerts[0][0]*scale, arrowVerts[0][1]*scale, -thickness);
    glEnd();
}

// 2) Heart shape for Extra Life
void drawHeart3D(float size) {
    static GLfloat heartVerts[][2] = {
        {  0.0f,  1.0f },
        {  1.0f,  2.0f },
        {  2.0f,  1.5f},
        {  2.0f,  0.5f},
        {  0.0f, -1.5f},
        { -2.0f,  0.5f},
        { -2.0f,  1.5f},
        { -1.0f,  2.0f}
    };
    int numVerts = 8;
    float thickness = 0.4f * size;
    float scale    = 0.4f * size;

    // FRONT FACE
    glBegin(GL_TRIANGLE_FAN);
    for(int i = 0; i < numVerts; i++) {
        glVertex3f(heartVerts[i][0]*scale, heartVerts[i][1]*scale, thickness);
    }
    glEnd();

    // BACK FACE
    glBegin(GL_TRIANGLE_FAN);
    for(int i = numVerts-1; i >= 0; i--) {
        glVertex3f(heartVerts[i][0]*scale, heartVerts[i][1]*scale, -thickness);
    }
    glEnd();

    // SIDE QUADS
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i < numVerts; i++) {
        glVertex3f(heartVerts[i][0]*scale, heartVerts[i][1]*scale,  thickness);
        glVertex3f(heartVerts[i][0]*scale, heartVerts[i][1]*scale, -thickness);
    }
    glVertex3f(heartVerts[0][0]*scale, heartVerts[0][1]*scale, thickness);
    glVertex3f(heartVerts[0][0]*scale, heartVerts[0][1]*scale, -thickness);
    glEnd();
}

// 3) Cluster of small spheres for Multi-Ball
void drawCluster3D(float size) {
    float r = size * 0.4f;
    glPushMatrix();
    // Middle
    glutSolidSphere(r, 12, 12);
    glPopMatrix();

    glPushMatrix();
    // Upper-left
    glTranslatef(-r*1.2f, r*0.8f, 0.f);
    glutSolidSphere(r, 12, 12);
    glPopMatrix();

    glPushMatrix();
    // Upper-right
    glTranslatef(r*1.2f, r*0.8f, 0.f);
    glutSolidSphere(r, 12, 12);
    glPopMatrix();
}

// 4) Hourglass shape for Slow Motion
void drawHourglass3D(float size) {
    // Approximate hourglass with two cones base-to-base
    float radius = 0.7f * size;
    float height = 1.4f * size;

    // Top cone
    glPushMatrix();
    glTranslatef(0.0f, height/2.0f, 0.0f);
    glutSolidCone(radius, -height/2.0f, 12, 2);
    glPopMatrix();

    // Bottom cone
    glPushMatrix();
    glTranslatef(0.0f, -height/2.0f, 0.0f);
    glutSolidCone(radius, height/2.0f, 12, 2);
    glPopMatrix();
}

// 5) Lightning shape for Speed Boost
void drawLightning3D(float size) {
    static GLfloat boltVerts[][2] = {
        { -0.5f,  1.0f },
        {  0.3f,  0.3f },
        { -0.2f,  0.2f },
        {  0.4f, -0.8f },
        { -0.5f, -1.0f }
    };
    int numVerts = 5;
    float thickness = 0.4f * size;
    float scale    = 0.6f * size;

    // FRONT FACE
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i < numVerts; i++) {
        glVertex3f(boltVerts[i][0]*scale, boltVerts[i][1]*scale,  thickness);
        if (i < numVerts-1) {
            glVertex3f(boltVerts[i+1][0]*scale, boltVerts[i+1][1]*scale, thickness);
        }
    }
    glEnd();

    // BACK FACE
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = numVerts-1; i >= 0; i--) {
        glVertex3f(boltVerts[i][0]*scale, boltVerts[i][1]*scale, -thickness);
        if (i > 0) {
            glVertex3f(boltVerts[i-1][0]*scale, boltVerts[i-1][1]*scale, -thickness);
        }
    }
    glEnd();

    // SIDE QUADS
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i < numVerts; i++) {
        glVertex3f(boltVerts[i][0]*scale, boltVerts[i][1]*scale,  thickness);
        glVertex3f(boltVerts[i][0]*scale, boltVerts[i][1]*scale, -thickness);
    }
    glEnd();
}

// 3D Drawing
void initLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    GLfloat ambient[]  = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat diffuse[]  = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat position[] = {0.0f, 500.0f, 1000.0f, 1.0f};

    glLightfv(GL_LIGHT0, GL_AMBIENT,  ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
}

void drawPaddle3D() {
    glPushMatrix();
    glColor3f(0.0f, 1.0f, 0.0f);
    glTranslatef(paddleX + paddleWidth/2.0f, paddleY + paddleHeight/2.0f, 0.0f);
    glScalef(paddleWidth, paddleHeight, 10.0f);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void drawBall3D(const Ball &b) {
    glPushMatrix();
    glColor3f(1.0f, 0.0f, 0.0f);
    glTranslatef(b.x, b.y, 0.0f);
    glutSolidSphere(b.size, 16, 16);
    glPopMatrix();
}

// Decide which shape to draw based on the power-up type
void drawPowerUp3D(const PowerUp &p) {
    glPushMatrix();
    glTranslatef(p.x, p.y, 0.0f);
    glRotatef(p.rotationAngle, 0.0f, 1.0f, 0.0f);
    switch(p.type) {
        case PU_WIDEN_PADDLE:
            glColor3f(0.6f, 0.9f, 0.2f);  // greenish
            drawDoubleArrow3D(p.size);
            break;
        case PU_EXTRA_LIFE:
            glColor3f(1.0f, 0.2f, 0.2f);  // red heart
            drawHeart3D(p.size);
            break;
        case PU_MULTI_BALL:
            glColor3f(1.0f, 1.0f, 0.2f);  // yellowish
            drawCluster3D(p.size);
            break;
        case PU_SLOW_MOTION:
            glColor3f(0.7f, 0.7f, 1.0f);  // light blue
            drawHourglass3D(p.size);
            break;
        case PU_SPEED_BOOST:
            glColor3f(1.0f, 1.0f, 0.2f);  // lightning
            drawLightning3D(p.size);
            break;
    }
    glPopMatrix();
}

void drawFloor() {
    // Simple plane behind everything
    glPushMatrix();
    glColor3f(0.3f, 0.3f, 0.3f);
    glTranslatef(WINDOW_WIDTH/2.0f, WINDOW_HEIGHT/2.0f, 60.0f);
    glRotatef(90, 1, 0, 0);
    glScalef(WINDOW_WIDTH*2, WINDOW_HEIGHT*2, 1);
    glutSolidCube(1.0f);
    glPopMatrix();
}

// Text Overlay
void displayText(float x, float y, const std::string &text) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }
    glEnable(GL_LIGHTING);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawBoundary() {
    // Draw a rectangle matching the playable area in XY, z=0
    glPushMatrix();
    // Set boundary color (white) and line width
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);

    glBegin(GL_LINE_LOOP);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(WINDOW_WIDTH, 0.0f, 0.0f);
    glVertex3f(WINDOW_WIDTH, WINDOW_HEIGHT, 0.0f);
    glVertex3f(0.0f, WINDOW_HEIGHT, 0.0f);
    glEnd();
    glPopMatrix();
}

// Rendering
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (currentState == STATE_MENU) {
        // render a minimalistic menu
        // we'll use an orthographic overlay to draw text
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 1.0f, 1.0f);

        // Center the menu text somewhat
        displayText(WINDOW_WIDTH * 0.5f - 100, WINDOW_HEIGHT * 0.5f - 10, "3D Pong - Press ENTER to Start");
        glEnable(GL_LIGHTING);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }
    else if (currentState == STATE_PLAY) {
        // Normal 3D Pong rendering
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        // "camera" transform
        glTranslatef(-WINDOW_WIDTH/2.0f, -WINDOW_HEIGHT/2.0f, -1200.0f);

        // floor behind it all
        drawFloor();

        drawBoundary();

        // paddle
        drawPaddle3D();

        // balls
        for (auto &b : balls) {
            if (b.active) {
                drawBall3D(b);
            }
        }

        // power-ups
        for (int i = 0; i < MAX_POWERUPS; i++) {
            if (powerUps[i].active) {
                drawPowerUp3D(powerUps[i]);
            }
        }

        // Text overlay for score/lives/level
        std::string statusText = "Score: " + std::to_string(score) +
                                 "  Lives: " + std::to_string(lives) +
                                 "  Level: " + std::to_string(level);

        if (gameOver) {
            statusText += "  [ GAME OVER ]";
        }
        displayText(20.0f, 40.0f, statusText);
    }
    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (GLfloat)w/(GLfloat)h, 1.0, 5000.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int main(int argc, char** argv) {
    srand(static_cast<unsigned int>(time(nullptr)));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("3D Pong");

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    initLighting();

    initGame();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(handleKeyboard);
    glutTimerFunc(0, update, 0);

    glutMainLoop();
    return 0;
}
