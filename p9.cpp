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
#define DISTANCE_BETWEEN_TUNNELS 800
#define TUNNEL_LENGTH 400 
#define NUM_STREETLAMPS 4
#define LAMP_CYLINDER_RADIUS 0.3f
#define NUM_LAMPS_BETWEEN_SIGNS 5
#define SIGN_HEIGHT 3
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
bool outsideTunnel(int z) {
    return z != 0 && z % (DISTANCE_BETWEEN_TUNNELS + TUNNEL_LENGTH) < DISTANCE_BETWEEN_TUNNELS;
}

float road_tracing(float u) {
    return ROAD_AMPLITUDE + ROAD_AMPLITUDE * sin(2 * M_PI * (u - ROAD_PERIOD / 4) / ROAD_PERIOD);
}

// Draw a rectangle assuming parameters in counterclockwise order
void drawRectangle(GLfloat* v0, GLfloat* v1, GLfloat* v2, GLfloat* v3) {
    glBegin(GL_TRIANGLE_STRIP);
       glVertex3f(v0[0], v0[1], v0[2]); 
       glVertex3f(v1[0], v1[1], v1[2]); 
       glVertex3f(v3[0], v3[1], v3[2]); 
       glVertex3f(v2[0], v2[1], v2[2]); 
    glEnd();
}

void drawCylinder(GLfloat* pos, GLfloat radius, GLfloat height, GLfloat slices) {
    GLfloat h0, h1, x, z;
    
    for (int i = 0; i < slices; i++) {
        h0 = ((float)i)     * 2.0 * M_PI / slices;
        h1 = ((float)i + 1) * 2.0 * M_PI / slices;
        
        GLfloat v0[3] = { pos[0] + radius * cos(h0), pos[1], pos[2] + radius * sin(h0) }; // bottom left
        GLfloat v1[3] = { pos[0] + radius * cos(h1), pos[1], pos[2] + radius * sin(h1) }; // bottom right
        GLfloat v2[3] = { pos[0] + radius * cos(h0), pos[1] + height, pos[2] + radius * sin(h0) }; // top left
        GLfloat v3[3] = { pos[0] + radius * cos(h1), pos[1] + height, pos[2] + radius * sin(h1) }; // top right
        drawRectangle(v3, v2, v0, v1);
    }
}

// Configures lighting, adds geometry to lighting and controls where signs appear 
void configureRoad() {
    // TODO: sign lamp on mid
    // TODO: fix streetlamp flicker on furthest lamps
    static float SL_z[NUM_STREETLAMPS] = { DISTANCE_BETWEEN_LAMPS, 
                                           2 * DISTANCE_BETWEEN_LAMPS, 
                                           3 * DISTANCE_BETWEEN_LAMPS,
                                           4 * DISTANCE_BETWEEN_LAMPS
                                    }; 
    static int first_SL = 0;
    static int count_SL_since_last_sign = 0;
    static int sign_index = -1; // index in SL_z of z position of the sign 

    // Rotate lamp positions to render next lamp in correct position
    if (position[Z] > (SL_z[first_SL] + VEHICLE_PASSING_LAMP_DISTANCE)) {
        SL_z[first_SL] += NUM_STREETLAMPS * DISTANCE_BETWEEN_LAMPS;
        first_SL = (first_SL + 1) % 4;
        count_SL_since_last_sign++;
        
        // The sign must not be rendered after we passed enough lamps
        if (sign_index != -1 && count_SL_since_last_sign > NUM_STREETLAMPS - 1) {
            sign_index = -1;
            count_SL_since_last_sign = 0;
        }
    }
    
    // Check if sign should be rendered 
    if (count_SL_since_last_sign == NUM_LAMPS_BETWEEN_SIGNS) {
        sign_index = (NUM_STREETLAMPS + first_SL - 1) % NUM_STREETLAMPS; // % is not mod op
        count_SL_since_last_sign = 0;
    }
   
    // By default we assume the lamps are not in a tunnel 
    GLfloat positions_SL[NUM_STREETLAMPS][4] = {
        { road_tracing(SL_z[0]) + ROAD_WIDTH, LAMP_HEIGHT, SL_z[0], 1.0 },
        { road_tracing(SL_z[1]) - ROAD_WIDTH, LAMP_HEIGHT, SL_z[1], 1.0 },
        { road_tracing(SL_z[2]) + ROAD_WIDTH, LAMP_HEIGHT, SL_z[2], 1.0 },
        { road_tracing(SL_z[3]) - ROAD_WIDTH, LAMP_HEIGHT, SL_z[3], 1.0 }
    };
    
    GLfloat directions_SL[NUM_STREETLAMPS][3] = {
        { -1.0, -1.0, 0.0 }, // left one directs light to the right
        {  1.0, -1.0, 0.0 }, // right one directs light to the left
        { -1.0, -1.0, 0.0 },  
        {  1.0, -1.0, 0.0 }
    };

    // Geometric structures holding lamps
    for (int i = 0; i < NUM_STREETLAMPS; i++) {
        // Render sign
        if (i == sign_index && outsideTunnel(SL_z[i])) {
            drawCylinder(
                    new GLfloat[] { road_tracing(SL_z[sign_index]) + ROAD_WIDTH, 0, positions_SL[i][Z] }, 
                    LAMP_CYLINDER_RADIUS,
                    LAMP_HEIGHT, 
                    20
                 );
            drawCylinder(
                    new GLfloat[] { road_tracing(SL_z[sign_index]) - ROAD_WIDTH, 0, positions_SL[i][Z] }, 
                    LAMP_CYLINDER_RADIUS,
                    LAMP_HEIGHT, 
                    20
                 );
            
            // Rectangle that will contain the texture
            drawRectangle(
                    new GLfloat[] { road_tracing(SL_z[sign_index]) + ROAD_WIDTH + LAMP_CYLINDER_RADIUS, 
                                    LAMP_HEIGHT + SIGN_HEIGHT, 
                                    positions_SL[i][Z]
                                  }, // top right
                    new GLfloat[] { road_tracing(SL_z[sign_index]) - ROAD_WIDTH - LAMP_CYLINDER_RADIUS, 
                                    LAMP_HEIGHT + SIGN_HEIGHT, 
                                    positions_SL[i][Z]
                                  }, // top left
                    new GLfloat[] { road_tracing(SL_z[sign_index]) - ROAD_WIDTH - LAMP_CYLINDER_RADIUS, 
                                    LAMP_HEIGHT - 0.1, 
                                    positions_SL[i][Z]
                                  }, // bottom left
                    new GLfloat[] { road_tracing(SL_z[sign_index]) + ROAD_WIDTH + LAMP_CYLINDER_RADIUS, 
                                    LAMP_HEIGHT - 0.1, 
                                    positions_SL[i][Z]
                                  }  // bottom right
                 );

            positions_SL[i][X] = road_tracing(SL_z[i]); // sign lamp on middle
            directions_SL[i][X] = 0.0; // pointing down
        }
        // Render lamp supports for outside tunnel
        else if (outsideTunnel(SL_z[i])) {
            drawCylinder(
                    new GLfloat[] { positions_SL[i][X], 0, positions_SL[i][Z] }, 
                    LAMP_CYLINDER_RADIUS,
                    LAMP_HEIGHT, 
                    20
                );
        }
        // Do not render anything else, tunnel geometry supports lamps
        else {
            positions_SL[i][X] = road_tracing(SL_z[i]); // in tunnel -> lamps on middle
            directions_SL[i][X] = 0.0; // so they point straight down
        }
    }

    // Lamps themselves
    glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, directions_SL[0]);
    glLightfv(GL_LIGHT3, GL_SPOT_DIRECTION, directions_SL[1]);
    glLightfv(GL_LIGHT4, GL_SPOT_DIRECTION, directions_SL[2]);
    glLightfv(GL_LIGHT5, GL_SPOT_DIRECTION, directions_SL[3]);

	glLightfv(GL_LIGHT2, GL_POSITION, positions_SL[0]);
	glLightfv(GL_LIGHT3, GL_POSITION, positions_SL[1]);
	glLightfv(GL_LIGHT4, GL_POSITION, positions_SL[2]);
	glLightfv(GL_LIGHT5, GL_POSITION, positions_SL[3]);
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


void displayRoad(int length) {
    glPolygonMode(GL_FRONT_AND_BACK, draw_mode);

    // Material configuration
    GLfloat D_road[] = { 0.8, 0.8, 0.8 };
    GLfloat S_road[] = { 0.3, 0.3, 0.3 };
    float BE_road = 2;

    float BE_border = 5;

    GLfloat D_tunnel[] = { 0.6, 0.6, 0.6 };
    GLfloat S_tunnel[] = { 0.5, 0.5, 0.5 };
    float BE_tunnel_wall = 4;
    float BE_tunnel_ceil = 2;

    for (int i = position[Z] - TUNNEL_LENGTH; i < position[Z] + length; i++) {
    
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, D_road);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, S_road);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, BE_road);

        // Road
		GLfloat v3[3] = { road_tracing(i + 1) + ROAD_WIDTH, 0, (float)i + 1 }; // next left
        GLfloat v0[3] = { road_tracing(i + 1) - ROAD_WIDTH, 0, (float)i + 1 }; // next right
        GLfloat v1[3] = { road_tracing(i    ) - ROAD_WIDTH, 0, (float)i }; // last right
        GLfloat v2[3] = { road_tracing(i    ) + ROAD_WIDTH, 0, (float)i }; // last left
        quad(v0, v3, v2, v1, QUAD_DENSITY, QUAD_DENSITY);
        
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, BE_border);

        // Left road border
        GLfloat v0_y[3] = { road_tracing(i + 1) - ROAD_WIDTH, ROAD_BORDER_HEIGHT, (float)i + 1 };
        GLfloat v1_y[3] = { road_tracing(i    ) - ROAD_WIDTH, ROAD_BORDER_HEIGHT, (float)i };
        drawRectangle(v0, v1, v1_y, v0_y);

        // Right road border
        GLfloat v3_y[3] = { road_tracing(i + 1) + ROAD_WIDTH, ROAD_BORDER_HEIGHT, (float)i + 1 };
        GLfloat v2_y[3] = { road_tracing(i    ) + ROAD_WIDTH, ROAD_BORDER_HEIGHT, (float)i };
        drawRectangle(v3, v2, v2_y, v3_y);

        // Tunnel
        if (!outsideTunnel(i)) {
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, D_tunnel);
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, S_tunnel);
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, BE_tunnel_wall);

            // Right wall
            GLfloat v0_y[3] = { road_tracing(i + 1) - ROAD_WIDTH, ROAD_TUNNEL_HEIGHT, (float)i + 1 };
            GLfloat v1_y[3] = { road_tracing(i    ) - ROAD_WIDTH, ROAD_TUNNEL_HEIGHT, (float)i };

            // Left wall
            GLfloat v3_y[3] = { road_tracing(i + 1) + ROAD_WIDTH, ROAD_TUNNEL_HEIGHT, (float)i + 1 };
            GLfloat v2_y[3] = { road_tracing(i    ) + ROAD_WIDTH, ROAD_TUNNEL_HEIGHT, (float)i };

            // Only draw walls half the time
            if (i % 8 < 4) {
                drawRectangle(v0, v1, v1_y, v0_y);
                drawRectangle(v3, v2, v2_y, v3_y);
            } 
            // Other half is semitransparent to show weather (is this even possible)
            /*
            else {
                // TODO  
            }
            */

            // Always draw ceiling
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, BE_tunnel_ceil);
            drawRectangle(v0_y, v1_y, v2_y, v3_y);
        }
	}

    configureRoad();
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
    GLfloat A[] = {0.7, 0.7, 0.7, 1.0};
    GLfloat D[] = {0.5, 0.5, 0.5, 1.0};
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
