#define _USE_MATH_DEFINES

#include <iostream> 
#include <cmath>
#include <GL/freeglut.h>
#include <sstream>
#include "Utilidades.h"

/********************************* CONSTANTS *********************************/
// Window
#define PROJECT_NAME "Videojuego de motochoque"
#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 600

// General game behaviour
#define FPS 60
#define SPEED_INCREMENT 0.2
#define MAX_SPEED 50
#define ANGLE_INCREMENT 0.5
#define MAX_ANGLE 45

// Road
#define ROAD_AMPLITUDE 10
#define ROAD_PERIOD 300
#define ROAD_LENGTH 150
#define ROAD_WIDTH 8
#define ROAD_BORDER_HEIGHT 0.4
#define ROAD_TUNNEL_HEIGHT 4
#define DISTANCE_BETWEEN_TUNNELS 1000
#define TUNNEL_LENGTH 400
#define QUAD_DENSITY 15 

// Lighting
#define LAMP_HEIGHT ROAD_TUNNEL_HEIGHT
#define DISTANCE_BETWEEN_LAMPS ROAD_LENGTH / 4
#define VEHICLE_PASSING_LAMP_DISTANCE DISTANCE_BETWEEN_LAMPS / 2

// Camera
#define FOV_Y 45
#define Z_NEAR 1
#define Z_FAR 200

// Others
#define SECOND_IN_MILLIS 1000.0f
#define X 0
#define Y 1
#define Z 2


/***************************** GLOBAL VARIABLES ******************************/
// Modes
static int draw_mode; // GL_LINE or GL_FILL
static enum {PLAYER_VIEW, BIRDS_EYE_VIEW} camera_mode;

// Vehicle physics
static float speed = 0.0;
static float velocity[3] = { 0.0, 1.0, 1.0 };
static float position[3] = { 0.0, 1.0, 0.0 };
static float turn_angle = 0;

/***************************** HELPER FUNCTIONS ******************************/
bool tunnelExit(int z) {
    return z != 0 && (z % (TUNNEL_LENGTH) == 0);
}

bool tunnelEntry(int z) {
    return z > DISTANCE_BETWEEN_TUNNELS && (z % DISTANCE_BETWEEN_TUNNELS == 0);
}

float road_tracing(float u) {
    return ROAD_AMPLITUDE + ROAD_AMPLITUDE * sin(2 * M_PI * (u - ROAD_PERIOD / 4) / ROAD_PERIOD);
}

void configureStreetlamps() {
    // TODO: fix streetlamp flicker
    GLfloat SL_direction[] = { 0.0, -1.0, 0.0 }; 
    
    glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, SL_direction);
    glLightfv(GL_LIGHT3, GL_SPOT_DIRECTION, SL_direction);
    glLightfv(GL_LIGHT4, GL_SPOT_DIRECTION, SL_direction);
    glLightfv(GL_LIGHT5, GL_SPOT_DIRECTION, SL_direction);
    
    static float i1 = DISTANCE_BETWEEN_LAMPS; 
    static float i2 = i1 + DISTANCE_BETWEEN_LAMPS; 
    static float i3 = i2 + DISTANCE_BETWEEN_LAMPS; 
    static float i4 = i3 + DISTANCE_BETWEEN_LAMPS; 

    if (position[Z] > (i1 + VEHICLE_PASSING_LAMP_DISTANCE)) {
        i1 += DISTANCE_BETWEEN_LAMPS;
        i2 += DISTANCE_BETWEEN_LAMPS;
        i3 += DISTANCE_BETWEEN_LAMPS;
        i4 += DISTANCE_BETWEEN_LAMPS;
    }

    GLfloat position_SL1[] = { road_tracing(i1), LAMP_HEIGHT, i1, 1.0 };
    GLfloat position_SL2[] = { road_tracing(i2), LAMP_HEIGHT, i2, 1.0 };
    GLfloat position_SL3[] = { road_tracing(i3), LAMP_HEIGHT, i3, 1.0 };
    GLfloat position_SL4[] = { road_tracing(i4), LAMP_HEIGHT, i4, 1.0 };
    
    // Geometric structures holding lamps
    // TODO: draw lamp structures, and 1 sign every 20 to 50 lamps, only if not in tunnel
     

	glLightfv(GL_LIGHT2, GL_POSITION, position_SL1);
	glLightfv(GL_LIGHT3, GL_POSITION, position_SL2);
	glLightfv(GL_LIGHT4, GL_POSITION, position_SL3);
	glLightfv(GL_LIGHT5, GL_POSITION, position_SL4);
}

void configureMoonlight() {
    GLfloat ML_position[] = { 0.0, 10.0, 0.0, 0.0 };
	glLightfv(GL_LIGHT0, GL_POSITION, ML_position);
}

void configureHeadlight() {
    GLfloat HL_position[] = { 0.0, 0.7, 0.0, 1.0 };
    GLfloat HL_direction[] = { 0.0, -0.3, -0.6 };
	glLightfv(GL_LIGHT1, GL_POSITION, HL_position);
    glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, HL_direction);
}

void showControls() {
    std::cout << "Game controls:" << endl;
    std::cout << "\tArrrows: control vehicle movement." << endl;
    std::cout << "\t'A' or 'a': toggle between solid and wire drawing modes." << endl;
    std::cout << "\t'C' or 'c': toggle between player view and birds-eye view." << endl;
    std::cout << "\tESC: exit." << endl;
}

void drawBorder(GLfloat* v0, GLfloat* v1, GLfloat* v2, GLfloat* v3) {
    glBegin(GL_QUADS);
       glVertex3f(v0[0], v0[1], v0[2]); 
       glVertex3f(v1[0], v1[1], v1[2]); 
       glVertex3f(v2[0], v2[1], v2[2]); 
       glVertex3f(v3[0], v3[1], v3[2]); 
    glEnd();
}

void displayRoad(int length) {
    glPolygonMode(GL_FRONT_AND_BACK, draw_mode);

    // Material configuration
    GLfloat D[] = { 0.8, 0.8, 0.8 };
    GLfloat S[] = { 0.3, 0.3, 0.3 };
    float brightness_exponent = 2;

    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, D);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, S);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, brightness_exponent);

    bool in_tunnel = false;
    for (int i = position[Z] - TUNNEL_LENGTH; i < position[Z] + length; i++) {
        // Quad vertexes
		GLfloat v3[3] = { road_tracing(i + 1) + ROAD_WIDTH, 0, (float)i + 1 }; // next left
        GLfloat v0[3] = { road_tracing(i + 1) - ROAD_WIDTH, 0, (float)i + 1 }; // next right
        GLfloat v1[3] = { road_tracing(i    ) - ROAD_WIDTH, 0, (float)i }; // last right
        GLfloat v2[3] = { road_tracing(i    ) + ROAD_WIDTH, 0, (float)i }; // last left
	    
        // Road quad
        quad(v0, v3, v2, v1, QUAD_DENSITY, QUAD_DENSITY);
        
        // Left road border
        GLfloat v0_y[3] = { road_tracing(i + 1) - ROAD_WIDTH, ROAD_BORDER_HEIGHT, (float)i + 1 };
        GLfloat v1_y[3] = { road_tracing(i    ) - ROAD_WIDTH, ROAD_BORDER_HEIGHT, (float)i };
        drawBorder(v0, v1, v1_y, v0_y);

        // Right road border
        GLfloat v3_y[3] = { road_tracing(i + 1) + ROAD_WIDTH, ROAD_BORDER_HEIGHT, (float)i + 1 };
        GLfloat v2_y[3] = { road_tracing(i    ) + ROAD_WIDTH, ROAD_BORDER_HEIGHT, (float)i };
        drawBorder(v3, v2, v2_y, v3_y);
        
        // Checking if tunnel should be displayed
        if (tunnelExit(i) && in_tunnel) {
            in_tunnel = false;
        }
        else if (tunnelEntry(i) && !in_tunnel){
            in_tunnel = true;
        }

        // Tunnel display
        if (in_tunnel) {
            // Right wall
            GLfloat v0_y[3] = { road_tracing(i + 1) - ROAD_WIDTH, ROAD_TUNNEL_HEIGHT, (float)i + 1 };
            GLfloat v1_y[3] = { road_tracing(i    ) - ROAD_WIDTH, ROAD_TUNNEL_HEIGHT, (float)i };

            // Left wall
            GLfloat v3_y[3] = { road_tracing(i + 1) + ROAD_WIDTH, ROAD_TUNNEL_HEIGHT, (float)i + 1 };
            GLfloat v2_y[3] = { road_tracing(i    ) + ROAD_WIDTH, ROAD_TUNNEL_HEIGHT, (float)i };

            // Only draw walls half the time
            if (i % 8 < 4) {
                drawBorder(v0, v1, v1_y, v0_y);
                drawBorder(v3, v2, v2_y, v3_y);
            } 
            // Other half is semitransparent to show weather (is this even possible)
            else {
                // TODO  
                in_tunnel = !!in_tunnel;
            }

            // Always draw ceiling
            drawBorder(v0_y, v1_y, v2_y, v3_y);
        }
	} 

    configureStreetlamps();
}


void setupLighting() {
    // Moonlight
    GLfloat A0[] = { 0.1, 0.1, 0.1, 1.0 };
    GLfloat D0[] = { 0.1, 0.1, 0.1, 1.0 };
    GLfloat S0[] = { 0.0, 0.0, 0.0, 1.0 };

    glLightfv(GL_LIGHT0, GL_AMBIENT, A0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, D0);
    glLightfv(GL_LIGHT0, GL_SPECULAR, S0);
   

    // Vehicle headlight
    GLfloat A1[] = { 0.5, 0.5, 0.5, 1.0 };
    GLfloat D1[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat S1[] = { 1.0, 1.0, 1.0, 1.0 };
    float cutoff = 25.0; // degrees
    float exponent = 20.0;
    
    glLightfv(GL_LIGHT1, GL_AMBIENT, A1);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, D1);
    glLightfv(GL_LIGHT1, GL_SPECULAR, S1);
    glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, cutoff);
    glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, exponent);

    // Streetlamps
    GLfloat A[] = {0.3, 0.3, 0.3, 1.0};
    GLfloat D[] = {0.5, 0.5, 0.2, 1.0};
    GLfloat S[] = {0.3, 0.3, 0.3, 1.0};
    cutoff = 90.0; // degrees
    exponent = 3.0;

    glLightfv(GL_LIGHT2, GL_AMBIENT, A);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, D);
    glLightfv(GL_LIGHT2, GL_SPECULAR, S);
    glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, cutoff);
    glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, exponent);

    glLightfv(GL_LIGHT3, GL_AMBIENT, A);
    glLightfv(GL_LIGHT3, GL_DIFFUSE, D);
    glLightfv(GL_LIGHT3, GL_SPECULAR, S);
    glLightf(GL_LIGHT3, GL_SPOT_CUTOFF, cutoff);
    glLightf(GL_LIGHT3, GL_SPOT_EXPONENT, exponent);
   
    glLightfv(GL_LIGHT4, GL_AMBIENT, A);
    glLightfv(GL_LIGHT4, GL_DIFFUSE, D);
    glLightfv(GL_LIGHT4, GL_SPECULAR, S);
    glLightf(GL_LIGHT4, GL_SPOT_CUTOFF, cutoff);
    glLightf(GL_LIGHT4, GL_SPOT_EXPONENT, exponent);

    glLightfv(GL_LIGHT5, GL_AMBIENT, A);
    glLightfv(GL_LIGHT5, GL_DIFFUSE, D);
    glLightfv(GL_LIGHT5, GL_SPECULAR, S);
    glLightf(GL_LIGHT5, GL_SPOT_CUTOFF, cutoff);
    glLightf(GL_LIGHT5, GL_SPOT_EXPONENT, exponent);
}

/********************************* CALLBACKS *********************************/
void init() {
    showControls();

    // Enabling / disabling of elements / settings
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT2);
    glEnable(GL_LIGHT3);
    glEnable(GL_LIGHT4);
    glEnable(GL_LIGHT5);
    
    // Set-up of elements
    setupLighting();

	glClearColor(0, 0, 0, 1);

    // Mode initialization
    draw_mode = GL_FILL;
    camera_mode = PLAYER_VIEW;

}

void display() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

    // Camera-dependent elements
    configureMoonlight();
    configureHeadlight();
    
    gluLookAt(
           position[X], position[Y], position[Z], 
           position[X] + velocity[X], velocity[Y], position[Z] + velocity[Z], 
           0, 1, 0
        );
   
    // Camera-independent elements
    displayRoad(ROAD_LENGTH);


	glutSwapBuffers();
}

void reshape(GLint w, GLint h) {
	glViewport(0, 0, w, h);
	float aspect_ratio = (float)w / h;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(FOV_Y, aspect_ratio, Z_NEAR, Z_FAR);
}

void onTimer(int interval) {
	static int previous = glutGet(GLUT_ELAPSED_TIME);
	int current = glutGet(GLUT_ELAPSED_TIME);
	float elapsed = (current - previous) / SECOND_IN_MILLIS;

	float displacement = elapsed * speed;
	previous = current;

	position[X] += displacement * velocity[X];
	position[Z] += displacement * velocity[Z];

	glutPostRedisplay();

    std::stringstream title;
	title << PROJECT_NAME << " " << speed << "m/s" ;
	glutSetWindowTitle(title.str().c_str());

	glutTimerFunc(interval, onTimer, interval);
}

void onSpecialKey(int key, int x, int y) {
	switch (key) {
        case GLUT_KEY_UP:
            if (speed + SPEED_INCREMENT <= MAX_SPEED)
                speed += SPEED_INCREMENT;
            else
                speed = MAX_SPEED;
            break;
        case GLUT_KEY_DOWN:
            if (speed >= SPEED_INCREMENT) 
                speed -= SPEED_INCREMENT;
            else 
                speed = 0;
            break;
        case GLUT_KEY_LEFT:
            if (turn_angle + ANGLE_INCREMENT <= MAX_ANGLE)
                turn_angle += ANGLE_INCREMENT;
            break;
        case GLUT_KEY_RIGHT:
            if (turn_angle - ANGLE_INCREMENT >= -MAX_ANGLE)
                turn_angle -= ANGLE_INCREMENT;
            break;
	}

	velocity[X] = sin(turn_angle * M_PI / 180);
	velocity[Z] = cos(turn_angle * M_PI / 180);

	glutPostRedisplay();
}

void onKey(unsigned char key, int x, int y) {
	switch (key) {
        case 'a':
        case 'A':
            draw_mode = (draw_mode == GL_LINE) ? GL_FILL : GL_LINE;
            break;

        case 'c':
        case 'C':
            // TODO: change to first-person|third-person|birds-eye-view
            camera_mode = (camera_mode == PLAYER_VIEW) ? BIRDS_EYE_VIEW : PLAYER_VIEW; 
            position[Y] = (position[Y] == 1.0) ? 150 : 1.0;
            if (glIsEnabled(GL_LIGHT1))
                glDisable(GL_LIGHT1);
            else
                glEnable(GL_LIGHT1);
            break;

        case 27: // esc
            exit(0);
	}

	glutPostRedisplay();
}

int main(int argc, char** argv) {
	glutInit(&argc, argv); 
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutCreateWindow(PROJECT_NAME);
	init(); 

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutTimerFunc(SECOND_IN_MILLIS / FPS, onTimer, SECOND_IN_MILLIS / FPS);
	glutSpecialFunc(onSpecialKey);
	glutKeyboardFunc(onKey);

	glutMainLoop();
}
