#define _USE_MATH_DEFINES

#include <iostream> 
#include <iomanip>
#include <cmath>
#include <random>
#include <GL/freeglut.h>
#include <sstream>
#include "Utilidades.h"

/********************************* CONSTANTS *********************************/
// Window
#define PROJECT_NAME "OpenGL Motorbike"
#define WINDOW_WIDTH 854
#define WINDOW_HEIGHT 480

// General game behaviour
#define FPS 60
#define SPEED_INCREMENT 0.4f
#define MAX_SPEED 30
#define ANGLE_INCREMENT 0.5f
#define MAX_ANGLE 45

// Road
#define ROAD_AMPLITUDE 10
#define ROAD_PERIOD 300
#define RENDER_DISTANCE 150
#define ROAD_WIDTH 8
#define ROAD_BORDER_HEIGHT 0.4f
#define ROAD_TUNNEL_HEIGHT 4
#define DISTANCE_BETWEEN_TUNNELS 800
#define TUNNEL_LENGTH 400 
#define NUM_STREETLAMPS 4
#define LAMP_CYLINDER_RADIUS 0.2f
#define NUM_LAMPS_BETWEEN_SIGNS 5
#define SIGN_HEIGHT 4
#define Z_BETWEEN_TREES 15
#define X_BETWEEN_TREES 4.5f 
#define NUM_TREES_X 4
#define TREE_TRUNK_RADIUS 0.2f
#define TREE_TRUNK_HEIGHT 3
#define TREE_CONE_BASE 1
#define TREE_CONE_HEIGHT 5

#define QUAD_DENSITY 4

// Lighting
#define LAMP_HEIGHT ROAD_TUNNEL_HEIGHT
#define DISTANCE_BETWEEN_LAMPS RENDER_DISTANCE / 4
#define VEHICLE_PASSING_LAMP_DISTANCE DISTANCE_BETWEEN_LAMPS / 2

// Camera
#define FOV_Y 45
#define Z_NEAR 1
#define Z_FAR 200
#define PLAYER_Y 1.0f
#define THIRD_PERSON_Y 2.0f
#define BIRDS_EYE_Y 50.0f

// Rain
#define NUM_RAINDROPS 3000
#define MIN_RAINDROP_SPEED 50
#define MAX_RAINDROP_SPEED 500

// Others
#define HIGH_DETAIL_VIEW_DISTANCE 50
#define SECOND_IN_MILLIS 1000.0f
#define X 0
#define Y 1
#define Z 2

/********************************* TYPEDEFS **********************************/
typedef struct {
    float position[3];
    float* velocity[3];
    float speed;
    float length;
} raindrop_t;

/******************************** PROTOTYPES *********************************/
// Rain 
void initializeRaindrop(raindrop_t*);
void createRaindrops(void);
void updateAndRenderRain(void);

// Tunnel
bool outsideTunnel(int);

// Materials & Textures
void loadTextures(void);
void setSupportMaterialAndTexture(void);
void setArrowMaterialAndTexture(void);
void setSignMaterialAndTexture(int);
void setLampMaterialAndTexture(void);
void setGroundMaterialAndTexture(void);
void setRoadMaterialAndTexture(void);
void setRoadBorderMaterialAndTexture(void);
void setTunnelWallMaterialAndTexture(void); 
void setTunnelCeilingMaterialAndTexture(void);

// Road
float road_tracing(float);
bool insideRoadBorder(float, float);
void drawCylindricalSupport(GLfloat*, GLfloat, GLfloat, GLfloat);
void displayRoad(int);

// Rendering of elements
void renderSignSupports(float, float);
void renderSign(float);
void renderLamp(float, float, float);
void renderRoad(int, int, int);
void renderRoadWall(int, int, float);
void renderRoadCeiling(int, float);
void renderSkyline(int);
void renderGround(float);
void renderWindArrow(void);
void renderArrow(void);
bool atHighQualityRoadPosition(int);

// Configuration of scene
void configureRoad(void);
void configureMoonlight(void);
void configureHeadlight(void);
void setupLighting(void);

// Trees
void renderTrees(float);
void renderTree(GLfloat*);
bool atTreePosition(int);

// Showing of elements
void showControls(void);
void showHUD(void);
void showBike(void);

/***************************** GLOBAL VARIABLES ******************************/
// Modes
static int draw_mode; // GL_LINE or GL_FILL
static enum {PLAYER_VIEW, THIRD_PERSON_VIEW, BIRDS_EYE_VIEW} camera_mode;
static enum {CLEAR, RAINFALL} weather_mode;
static enum {AXIS_ON, AXIS_OFF} axis_mode;
static enum {COLLISIONS, NO_COLLISIONS} collision_mode;
static enum {HUD_ON, HUD_OFF} hud_mode;

// Vehicle physics
static float speed = 0.0;
static float velocity[3] = { 0.0, 1.0, 1.0 };
static float position[3] = { 0.0, 1.0, 0.0 };
static float turn_angle = 0;

// Rain particles
static raindrop_t raindrops[NUM_RAINDROPS];
static float rain_velocity[3] = { 0.0, -1.0, 0.0 };

// Textures
GLuint tex_road, tex_road_border;
GLuint tex_ground;
GLuint tex_sign1, tex_sign2, tex_sign3, tex_sign4, tex_sign5, tex_sign6;
GLuint tex_support, tex_lamp;
GLuint tex_tunnel_wall, tex_tunnel_ceiling;
GLuint tex_skyline;
GLuint tex_bike_pov, tex_bike_bev, tex_bike_tpv;
GLuint tex_arrow;

// Random numbers
// source: https://stackoverflow.com/questions/288739/generate-random-numbers-uniformly-over-an-entire-range
static std::random_device rd;     // only used once to initialise (seed) engine
static std::mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)

// Other
static int lamps[] = { GL_LIGHT2, GL_LIGHT3, GL_LIGHT4, GL_LIGHT5 };
static int num_sidelengths_passed = 1;

/***************************** HELPER FUNCTIONS ******************************/

bool atTreePosition(int z) {
    return outsideTunnel(z) && 
        z % Z_BETWEEN_TREES == 0 && 
        z - position[Z] < 100; 
}

void renderTree(GLfloat* tree_position) {
    glPushMatrix();
    setSupportMaterialAndTexture();
    drawCylindricalSupport(
            tree_position,
            TREE_TRUNK_RADIUS,
            TREE_TRUNK_HEIGHT, 
            20
         );
    glPopMatrix();
    glPushMatrix();
    glPushAttrib(GL_CURRENT_BIT);
    glColor3f(0.2, 1.0, 0.2);
    glTranslatef(tree_position[X], tree_position[Y]+TREE_TRUNK_HEIGHT, tree_position[Z]);
    glRotatef(-90, 1, 0, 0);
    glutSolidCone(TREE_CONE_BASE, TREE_CONE_HEIGHT, 10, 10);
    glPopAttrib();
    glPopMatrix();
}

void renderTrees(float z) {
    for (int i = 1; i < NUM_TREES_X; i++) {
        GLfloat left_tree[]  = { road_tracing(z) + ROAD_WIDTH + X_BETWEEN_TREES * i, -2, z };
        GLfloat right_tree[] = { road_tracing(z) - ROAD_WIDTH - X_BETWEEN_TREES * i, -2, z };
        renderTree(left_tree);
        renderTree(right_tree);
    }
}

bool outsideTunnel(int z) {
    return (z > 0 && 
            z % (DISTANCE_BETWEEN_TUNNELS + TUNNEL_LENGTH) < DISTANCE_BETWEEN_TUNNELS) 
        || z <= 0;
}

void initializeRaindrop(raindrop_t* raindrop) {
    static std::uniform_int_distribution<int> speed_uni(MIN_RAINDROP_SPEED, MAX_RAINDROP_SPEED);

    static std::uniform_int_distribution<int> X_uni(-20, 20);
    static std::uniform_int_distribution<int> Y_uni(3, 7);
    static std::uniform_int_distribution<int> Z_uni(1.5, RENDER_DISTANCE / 3);

    raindrop->position[X] = X_uni(rng);
    raindrop->position[Y] = Y_uni(rng); 
    raindrop->position[Z] = Z_uni(rng);

    raindrop->speed = speed_uni(rng);
    raindrop->length = (raindrop->speed / (MAX_RAINDROP_SPEED * 3 )) ;
}

raindrop_t createNewRaindrop() {
    raindrop_t raindrop;
    initializeRaindrop(&raindrop);

    return raindrop;
}

void createRaindrops() {
    for (int i = 0; i < NUM_RAINDROPS; i++) {
        raindrops[i] = createNewRaindrop();
    }
}

void changeRainVelocity() {
    static std::uniform_int_distribution<> X_uni(0, 360);
    static std::uniform_int_distribution<> Z_uni(0, 360);
    
    rain_velocity[X] = sin(X_uni(rng));
    rain_velocity[Y] = -1.0; // it wouldn't make sense for the drops to not drop :)
    rain_velocity[Z] = cos(Z_uni(rng));
}

void createRain() {
    changeRainVelocity();
    createRaindrops();
}

void updateAndRenderRain() {
    for (int i = 0; i < NUM_RAINDROPS; i++) {
        // Update
        raindrops[i].position[X] += raindrops[i].speed*rain_velocity[X] / SECOND_IN_MILLIS;
        raindrops[i].position[Y] += raindrops[i].speed*rain_velocity[Y] / SECOND_IN_MILLIS;
        raindrops[i].position[Z] += raindrops[i].speed*rain_velocity[Z] / SECOND_IN_MILLIS;

        if (!outsideTunnel(raindrops[i].position[Z]+position[Z]))
            continue;

        if (raindrops[i].position[Y] <= 0) {
            initializeRaindrop(raindrops + i);
        }

        // Draw raindrop
        glPushMatrix();
        glPushAttrib(GL_CURRENT_BIT);
        glColor3f(0.1, 0.1, 1.0);
        glBegin(GL_LINES);
        glVertex3f(
                raindrops[i].position[X], 
                raindrops[i].position[Y], 
                raindrops[i].position[Z] + position[Z]
            );
        glVertex3f(
                raindrops[i].position[X] + raindrops[i].length*rain_velocity[X],
                raindrops[i].position[Y] + raindrops[i].length*rain_velocity[Y],
                raindrops[i].position[Z] + position[Z] + raindrops[i].length*rain_velocity[Z]
            );
        glEnd();
        glPopAttrib();
        glPopMatrix();
     
    }
}
void loadTextures() {

    glGenTextures(1, &tex_road);
	glBindTexture(GL_TEXTURE_2D, tex_road);
	loadImageFile((char*)"assets/road.jpg");
    
    glGenTextures(1, &tex_bike_pov);
	glBindTexture(GL_TEXTURE_2D, tex_bike_pov);
	loadImageFile((char*)"assets/bike_pov.png");

    glGenTextures(1, &tex_bike_bev);
	glBindTexture(GL_TEXTURE_2D, tex_bike_bev);
	loadImageFile((char*)"assets/bike_bev.png");

    glGenTextures(1, &tex_bike_tpv);
	glBindTexture(GL_TEXTURE_2D, tex_bike_tpv);
	loadImageFile((char*)"assets/bike_tpv.png");

    glGenTextures(1, &tex_ground);
	glBindTexture(GL_TEXTURE_2D, tex_ground);
	loadImageFile((char*)"assets/grass.jpg");

    glGenTextures(1, &tex_road_border);
	glBindTexture(GL_TEXTURE_2D, tex_road_border);
	loadImageFile((char*)"assets/road_border.jpg");

    glGenTextures(1, &tex_support);
	glBindTexture(GL_TEXTURE_2D, tex_support);
	loadImageFile((char*)"assets/wood.jpg");

    glGenTextures(1, &tex_lamp);
	glBindTexture(GL_TEXTURE_2D, tex_lamp);
	loadImageFile((char*)"assets/cream_white.jpg");

    glGenTextures(1, &tex_sign1);
	glBindTexture(GL_TEXTURE_2D, tex_sign1);
	loadImageFile((char*)"assets/welcome_to_paradise.jpg");

    glGenTextures(1, &tex_sign2);
	glBindTexture(GL_TEXTURE_2D, tex_sign2);
	loadImageFile((char*)"assets/pain_natural.jpg");

    glGenTextures(1, &tex_sign3);
	glBindTexture(GL_TEXTURE_2D, tex_sign3);
	loadImageFile((char*)"assets/no_indep.jpg");

    glGenTextures(1, &tex_sign4);
	glBindTexture(GL_TEXTURE_2D, tex_sign4);
	loadImageFile((char*)"assets/consume.jpg");

    glGenTextures(1, &tex_sign5);
	glBindTexture(GL_TEXTURE_2D, tex_sign5);
	loadImageFile((char*)"assets/marry_reproduce.jpg");

    glGenTextures(1, &tex_sign6);
	glBindTexture(GL_TEXTURE_2D, tex_sign6);
	loadImageFile((char*)"assets/obey.jpg");

    glGenTextures(1, &tex_lamp);
	glBindTexture(GL_TEXTURE_2D, tex_lamp);
	loadImageFile((char*)"assets/cream_white.jpg");

    glGenTextures(1, &tex_tunnel_wall);
	glBindTexture(GL_TEXTURE_2D, tex_tunnel_wall);
	loadImageFile((char*)"assets/tunnel_wall.jpg");
    
    glGenTextures(1, &tex_tunnel_ceiling);
	glBindTexture(GL_TEXTURE_2D, tex_tunnel_ceiling);
	loadImageFile((char*)"assets/tunnel_ceiling.jpg");

    glGenTextures(1, &tex_skyline);
	glBindTexture(GL_TEXTURE_2D, tex_skyline);
	loadImageFile((char*)"assets/background_skyline_long.jpg");

    glGenTextures(1, &tex_arrow);
	glBindTexture(GL_TEXTURE_2D, tex_arrow);
	loadImageFile((char*)"assets/arrow.png");
}

float road_tracing(float u) {
    return ROAD_AMPLITUDE + ROAD_AMPLITUDE * sin(2 * M_PI * (u - ROAD_PERIOD / 4) / ROAD_PERIOD);
}

bool insideRoadBorder(float nextX, float nextZ) {
    bool left_of_right_border = (nextX <= road_tracing(nextZ) + ROAD_WIDTH - 0.7);
    bool right_of_left_border = (nextX >= road_tracing(nextZ) - ROAD_WIDTH + 0.7);
    return (left_of_right_border && right_of_left_border);
}

void drawCylindricalSupport(GLfloat* pos, GLfloat radius, GLfloat height, GLfloat slices) {
    GLfloat h0, h1, x, z;

    for (int i = 0; i < slices; i++) {
        h0 = ((float)i)     * 2.0 * M_PI / slices;
        h1 = ((float)i + 1) * 2.0 * M_PI / slices;
        
        GLfloat v0[3] = { pos[0] + radius * cos(h0), pos[1], pos[2] + radius * sin(h0) }; // bottom left
        GLfloat v1[3] = { pos[0] + radius * cos(h1), pos[1], pos[2] + radius * sin(h1) }; // bottom right
        GLfloat v2[3] = { pos[0] + radius * cos(h0), pos[1] + height, pos[2] + radius * sin(h0) }; // top left
        GLfloat v3[3] = { pos[0] + radius * cos(h1), pos[1] + height, pos[2] + radius * sin(h1) }; // top right
        quadtex(v3, v2, v0, v1, 0, 1, 0, 1, 1, 1);
    }
}

void renderSignSupports(float z, float height) {
    drawCylindricalSupport(
            new GLfloat[] { road_tracing(z) + ROAD_WIDTH, 0, z }, 
            LAMP_CYLINDER_RADIUS,
            height, 
            20
         );
    drawCylindricalSupport(
            new GLfloat[] { road_tracing(z) - ROAD_WIDTH, 0, z }, 
            LAMP_CYLINDER_RADIUS,
            height, 
            20
         );
}

void setArrowMaterialAndTexture() {
    static GLfloat D[] = { 0.8, 0.8, 0.8 };
    static GLfloat S[] = { 0.3, 0.3, 0.3 };
    static float BE = 2;

    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, D);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, S);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, BE);

    glBindTexture(GL_TEXTURE_2D, tex_arrow);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

void setSupportMaterialAndTexture() {
    static GLfloat D[] = { 0.8, 0.8, 0.8 };
    static GLfloat S[] = { 0.3, 0.3, 0.3 };
    static float BE = 2;

    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, D);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, S);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, BE);

    glBindTexture(GL_TEXTURE_2D, tex_support);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void renderSign(float z) {
    GLfloat top_right[] = {
        road_tracing(z) + ROAD_WIDTH, 
        LAMP_HEIGHT + SIGN_HEIGHT, 
        z
    };
    GLfloat top_left[]  =  { 
        road_tracing(z) - ROAD_WIDTH, 
        LAMP_HEIGHT + SIGN_HEIGHT, 
        z
    };
    GLfloat bottom_left[] = { 
        road_tracing(z) - ROAD_WIDTH, 
        LAMP_HEIGHT - 0.1, 
        z
    };
    GLfloat bottom_right[] = { 
        road_tracing(z) + ROAD_WIDTH, 
        LAMP_HEIGHT - 0.1, 
        z
    };

    quadtex(top_right, top_left, bottom_left, bottom_right, 0, 1, 1, 0, 1, 1);
}

void setSignMaterialAndTexture(int signs_passed) {

    static GLfloat D[] = { 0.8, 0.8, 0.8 };
    static GLfloat S[] = { 0.3, 0.3, 0.3 };
    static float BE = 2;

    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, D);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, S);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, BE);

    // Choose sign texture
    if (signs_passed == 0) {
        glBindTexture(GL_TEXTURE_2D, tex_sign1);
    }
    else if (signs_passed == 1) {
        glBindTexture(GL_TEXTURE_2D, tex_sign2);
    }
    else if (signs_passed % 4 == 0){
        glBindTexture(GL_TEXTURE_2D, tex_sign3);
    }
    else if (signs_passed % 4 == 1) {
        glBindTexture(GL_TEXTURE_2D, tex_sign4);
    }
    else if (signs_passed % 4 == 2){
        glBindTexture(GL_TEXTURE_2D, tex_sign5);
    }
    else if (signs_passed % 4 == 3) {
        glBindTexture(GL_TEXTURE_2D, tex_sign6);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void renderLamp(float x, float y, float z) {
    glTranslatef(x, y, z);
    glutSolidSphere(0.4, 10, 10);
}

void setLampMaterialAndTexture() {
    static GLfloat D[] = { 0.8, 0.8, 0.8 };
    static GLfloat S[] = { 0.3, 0.3, 0.3 };
    static float BE = 10;

    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, D);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, S);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, BE);

    glBindTexture(GL_TEXTURE_2D, tex_lamp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

// Configures lighting, adds geometry to lighting and controls where signs appear 
void configureRoad() {
    // TODO: fix spheres with no light
    static float SL_z[NUM_STREETLAMPS] = { DISTANCE_BETWEEN_LAMPS, 
                                           2 * DISTANCE_BETWEEN_LAMPS, 
                                           3 * DISTANCE_BETWEEN_LAMPS,
                                           4 * DISTANCE_BETWEEN_LAMPS
                                    }; 
    static int first_SL = 0;
    static int count_SL_since_last_sign = 0;
    static int sign_index = -1; // index in SL_z of z position of the sign 
    static int signs_passed = 0;

    // Rotate lamp positions to render next lamp in correct position
    if (position[Z] > (SL_z[first_SL] + VEHICLE_PASSING_LAMP_DISTANCE)) {
        SL_z[first_SL] += NUM_STREETLAMPS * DISTANCE_BETWEEN_LAMPS;
        first_SL = (first_SL + 1) % 4;
        count_SL_since_last_sign++;
        
        // The sign must not be rendered after we passed enough lamps
        if (sign_index != -1 && count_SL_since_last_sign > NUM_STREETLAMPS - 1) {
            sign_index = -1;
            count_SL_since_last_sign = 0;
            signs_passed++;
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
            glPushMatrix();
            setSupportMaterialAndTexture();
            renderSignSupports(SL_z[sign_index], LAMP_HEIGHT + SIGN_HEIGHT);
            glPopMatrix();

            // Rectangle that will contain the texture
            glPushMatrix();
            setSignMaterialAndTexture(signs_passed);
            renderSign(SL_z[sign_index]);
            glPopMatrix();

            positions_SL[i][X] = road_tracing(SL_z[i]); // sign lamp goes on middle
            directions_SL[i][X] = 0.0; // pointing down
        }
        // Render lamp supports for outside tunnel
        else if (outsideTunnel(SL_z[i])) {
            glPushMatrix();
            setSupportMaterialAndTexture();

            drawCylindricalSupport(
                    new GLfloat[] { positions_SL[i][X], 0, positions_SL[i][Z] }, 
                    LAMP_CYLINDER_RADIUS,
                    LAMP_HEIGHT, 
                    20
                );
            glPopMatrix();
        }
        // Do not render anything else, tunnel geometry supports lamps
        else {
            positions_SL[i][X] = road_tracing(SL_z[i]); // lamps in tunnel go on middle
            directions_SL[i][X] = 0.0; // and they point straight down
        }
    }
    
    // Lamps themselves
    for (int i = 0; i < NUM_STREETLAMPS; i++) {
        glLightfv(lamps[i], GL_SPOT_DIRECTION, directions_SL[i]);
	    glLightfv(lamps[i], GL_POSITION, positions_SL[i]);
        glPushMatrix();
        setLampMaterialAndTexture();
        renderLamp(positions_SL[i][X], positions_SL[i][Y], positions_SL[i][Z]);
        glPopMatrix();
    }
}


void configureMoonlight() {
    static GLfloat ML_position[] = { 0.0, 10.0, 0.0, 0.0 };
	glLightfv(GL_LIGHT0, GL_POSITION, ML_position);
}

void configureHeadlight() {
    static GLfloat HL_position[] = { 0.0, 0.7, 0.0, 1.0 };
    static GLfloat HL_direction[] = { 0.0, -0.3, -0.6 };
	glLightfv(GL_LIGHT1, GL_POSITION, HL_position);
    glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, HL_direction);
}

void showControls() {
    std::cout << "Game controls:" << "\n";
    std::cout << "\tArrows: control vehicle movement." << "\n";
    std::cout << "\t'S' or 's': toggle between solid and wire drawing modes." << "\n";
    std::cout << "\t'P' or 'p': cycle between player view, third person view and birds-eye view." << "\n";
    std::cout << "\t'Y' or 'y': randomly change the wind." << "\n";
    std::cout << "\t'L' or 'l': toggle between night and day." << "\n";
    std::cout << "\t'D' or 'd': toggle between active and inactive collision for the road." << "\n";
    std::cout << "\t'W' or 'w': toggle between clear and rainy weather." << "\n";
    std::cout << "\t'N' or 'n': toggle between fog and no fog." << "\n";
    std::cout << "\t'C' or 'c': show/hide HUD." << "\n";
    std::cout << "\t'E' or 'e': show/hide axis vectors." << "\n";
    std::cout << "\tESC: exit." << "\n";
}

void renderGround(float sidelength) {
    if (position[Z] + RENDER_DISTANCE > sidelength*num_sidelengths_passed) {
        num_sidelengths_passed++;
    }
    setGroundMaterialAndTexture();
    GLfloat top_right[] = {
        sidelength * num_sidelengths_passed, 
        -1, 
        sidelength * num_sidelengths_passed
    };
    GLfloat top_left[]  =  { 
        -sidelength * num_sidelengths_passed,
        -1,
        sidelength * num_sidelengths_passed
    };
    GLfloat bottom_left[] = { 
        -sidelength * num_sidelengths_passed,
        -1,
        -sidelength * num_sidelengths_passed
    };
    GLfloat bottom_right[] = { 
        sidelength * num_sidelengths_passed,
        -1,
        -sidelength * num_sidelengths_passed
    };

    quadtex(top_right, top_left, bottom_left, bottom_right, 20, 0, 20, 0, 1, 1);
}

void renderSkyline(int radius) {
    static GLUquadric *quadric;
    quadric = gluNewQuadric();

    glPushMatrix();
	glBindTexture(GL_TEXTURE_2D, tex_skyline);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	gluQuadricTexture(quadric, 1);

	glTranslatef(position[X], -30, position[Z]);
    glRotatef(91, 0, 1, 0);
	glRotatef(-90, 1, 0, 0);
	gluCylinder(quadric, radius, radius, 110, 50, 50);
	glPopMatrix();
}

void setGroundMaterialAndTexture() {
    static GLfloat D[] = { 0.8, 0.8, 0.8 };
    static GLfloat S[] = { 0.3, 0.3, 0.3 };
    static float BE = 2;

    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, D);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, S);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, BE);

    glBindTexture(GL_TEXTURE_2D, tex_ground);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void setRoadMaterialAndTexture() {
    static GLfloat D[] = { 0.8, 0.8, 0.8 };
    static GLfloat S[] = { 0.3, 0.3, 0.3 };
    static float BE = 2;

    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, D);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, S);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, BE);

    glBindTexture(GL_TEXTURE_2D, tex_road);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void setRoadBorderMaterialAndTexture() {
    static GLfloat D[] = { 0.8, 0.8, 0.8 };
    static GLfloat S[] = { 0.3, 0.3, 0.3 };
    static float BE = 5;

    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, D);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, S);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, BE);

    glBindTexture(GL_TEXTURE_2D, tex_road_border);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void setTunnelWallMaterialAndTexture() {
    static GLfloat D[] = { 0.6, 0.6, 0.6 };
    static GLfloat S[] = { 0.5, 0.5, 0.5 };
    static float BE = 4;

    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, D);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, S);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, BE);

    glBindTexture(GL_TEXTURE_2D, tex_tunnel_wall);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

}

void setBikeTexture() {
    if (camera_mode == PLAYER_VIEW)
        glBindTexture(GL_TEXTURE_2D, tex_bike_pov);
    else if (camera_mode == BIRDS_EYE_VIEW)
        glBindTexture(GL_TEXTURE_2D, tex_bike_bev);
    else
        glBindTexture(GL_TEXTURE_2D, tex_bike_tpv);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

}

void setTunnelCeilingMaterialAndTexture() {
    static GLfloat D[] = { 0.6, 0.6, 0.6 };
    static GLfloat S[] = { 0.5, 0.5, 0.5 };
    static float BE = 2;

    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, D);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, S);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, BE);

    glBindTexture(GL_TEXTURE_2D, tex_tunnel_ceiling);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void renderRoad(int z, int horizontal_slices, int vertical_slices) {
    GLfloat next_left[3]  = { road_tracing(z + 1) + ROAD_WIDTH, 0, (float)z + 1 };
    GLfloat next_right[3] = { road_tracing(z + 1) - ROAD_WIDTH, 0, (float)z + 1 };
    GLfloat this_right[3] = { road_tracing(z    ) - ROAD_WIDTH, 0, (float)z };
    GLfloat this_left[3]  = { road_tracing(z    ) + ROAD_WIDTH, 0, (float)z };

    quadtex(next_right, next_left, this_left, this_right, 0, 1, 0, 1, horizontal_slices, vertical_slices);
}

void renderRoadWall(int z, int side, float height) {
    // side is 1 => left, -1 => right
    GLfloat next_down[3] = { road_tracing(z + 1) + side * ROAD_WIDTH, 0, (float)z + 1 }; 
    GLfloat this_up[3]   = { road_tracing(z    ) + side * ROAD_WIDTH, height, (float)z };
    GLfloat next_up[3]   = { road_tracing(z + 1) + side * ROAD_WIDTH, height, (float)z + 1 };
    GLfloat this_down[3] = { road_tracing(z    ) + side * ROAD_WIDTH, 0, (float)z }; 
   
    // Order matters (so the border is facing us)
    if (side == -1) {
        quadtex(next_up, next_down, this_down, this_up, 0, 1, 0, 1, 1, 1);
    }
    else {
        quadtex(next_down, next_up, this_up, this_down, 0, 1, 0, 1, 1, 1);
    }
}

void renderRoadCeiling(int z, float height) {
    GLfloat next_right[3] = { road_tracing(z + 1) - ROAD_WIDTH, height, (float)z + 1 };
    GLfloat this_right[3] = { road_tracing(z    ) - ROAD_WIDTH, height, (float)z };
    GLfloat next_left[3]  = { road_tracing(z + 1) + ROAD_WIDTH, height, (float)z + 1 };
    GLfloat this_left[3]  = { road_tracing(z    ) + ROAD_WIDTH, height, (float)z };
    
    quadtex(next_right, next_left, this_left, this_right, 0, 1, 0, 1, 1, 1);
}

bool atHighQualityRoadPosition(int z) {
    return (z - position[Z] < HIGH_DETAIL_VIEW_DISTANCE);
}

void displayRoad(int length) {
    glPolygonMode(GL_FRONT_AND_BACK, draw_mode);

    for (int i = position[Z] - TUNNEL_LENGTH; i < position[Z] + length; i++) {
        glPushMatrix();
        setRoadMaterialAndTexture(); 

        // Road
        if (atHighQualityRoadPosition(i)) { 
            renderRoad(i, 3*QUAD_DENSITY, QUAD_DENSITY);
        }
        else {
            renderRoad(i, 3*QUAD_DENSITY/4, QUAD_DENSITY/4);
        }
        glPopMatrix(); 

        glPushMatrix();
        setRoadBorderMaterialAndTexture();
        renderRoadWall(i, -1, ROAD_BORDER_HEIGHT);
        renderRoadWall(i,  1, ROAD_BORDER_HEIGHT);
        glPopMatrix();

        // Trees
        if (atTreePosition(i)) {
            renderTrees(i);
        }

        // Tunnel
        if (!outsideTunnel(i)) {
            glPushMatrix();
            setTunnelWallMaterialAndTexture();

            renderRoadWall(i, -1, ROAD_TUNNEL_HEIGHT);
            renderRoadWall(i,  1, ROAD_TUNNEL_HEIGHT);
            glPopMatrix(); 

            glPushMatrix();
            setTunnelCeilingMaterialAndTexture();
            renderRoadCeiling(i, ROAD_TUNNEL_HEIGHT);
            glPopMatrix();
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
    GLfloat A1[] = { 0.9, 0.9, 0.9, 1.0 };
    GLfloat D1[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat S1[] = { 1.0, 1.0, 1.0, 1.0 };
    float cutoff = 40.0; // degrees
    float exponent = 20.0;
    
    glLightfv(GL_LIGHT1, GL_AMBIENT, A1);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, D1);
    glLightfv(GL_LIGHT1, GL_SPECULAR, S1);
    glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, cutoff);
    glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, exponent);

    // Streetlamps
    GLfloat A[] = {0.7, 0.7, 0.7, 1.0};
    GLfloat D[] = {0.8, 0.8, 0.8, 1.0};
    GLfloat S[] = {0.3, 0.3, 0.3, 1.0};
    cutoff = 90.0; // degrees
    exponent = 3.0;
    
    for (int i = 0; i < NUM_STREETLAMPS; i++) {
        glLightfv(lamps[i], GL_AMBIENT, A);
        glLightfv(lamps[i], GL_DIFFUSE, D);
        glLightfv(lamps[i], GL_SPECULAR, S);
        glLightf(lamps[i], GL_SPOT_CUTOFF, cutoff);
        glLightf(lamps[i], GL_SPOT_EXPONENT, exponent);
    }
}


void renderArrow() {
    quadtex(
            new GLfloat[] {  0,  0, 0}, 
            new GLfloat[] { -1,  0, 0},
            new GLfloat[] { -1, -2, 0}, 
            new GLfloat[] {  0, -2, 0},
            1, 0, 1, 0, 1, 1);
}

void renderWindArrow() {
    glPushMatrix();
    setArrowMaterialAndTexture();
    // Rotate so as to always point towards wind (rain) velocity x
    // This means we must get the angle between our LOOK AT vector and the rain velocity
    float dot = rain_velocity[X]*velocity[X] + rain_velocity[Z]*velocity[Z];
    float mag1 = std::sqrt(rain_velocity[X]*rain_velocity[X] + rain_velocity[Z]*rain_velocity[Z]);
    float mag2 = std::sqrt(velocity[X]*velocity[X]  + velocity[Z]*velocity[Z]);

    float angle_arrow_rot = std::acos(dot / (mag1 * mag2)) / M_PI * 180;

    if (velocity[X] > rain_velocity[X]) angle_arrow_rot = 360-angle_arrow_rot;

    glTranslatef(0.8, -0.55, 0);
    glScalef(0.07, 0.12, 1);
    
    glTranslatef(0, -1, 0);
    glRotatef(angle_arrow_rot, 0, 0, 1);
    glTranslatef(0, 1, 0);

    renderArrow();
    glPopMatrix();
}

void showHUD() {
    static int starting_time = glutGet(GLUT_ELAPSED_TIME);
    static int previous = starting_time; 
    static int frames = 0;
    static int fps = FPS;
    
    int current = glutGet(GLUT_ELAPSED_TIME);
    frames++; // each time this function is called a frame is presented to the user

    std::stringstream speed_ss, fps_ss, time_ss, distance_ss;

    speed_ss << std::setprecision(3) << speed << "\t m/s";
    time_ss << (int)((current - starting_time) / SECOND_IN_MILLIS + 0.5)<< "\t s";
    distance_ss << (int) position[Z] << "\t m";

    fps_ss << std::setprecision(3) << fps << "\t fps";

    if (current - previous >= SECOND_IN_MILLIS) {
        fps = frames;
        previous = current;
        frames = 0;

    }
    

    // Background of HUD text and arrow in dark for better readability
    glPushMatrix();
    glPushAttrib(GL_CURRENT_BIT);
    glColor4f(0.2, 0.0, 0.7, 0.8);
    setSupportMaterialAndTexture(); // any texture just for blending

    glTranslatef(1, 1, 0);
    glBegin(GL_TRIANGLE_STRIP);
    glVertex3f( -0.2, -0.3, 0);
    glVertex3f(  0.2, -0.3, 0);
    glVertex3f( -0.2,    0, 0);
    glVertex3f(  0.2,    0, 0);
    glEnd();
    glPopAttrib();
    glPopMatrix();
    
    // Text
    glPushMatrix();
    glTranslatef(0.85, 0.92, 0);
    texto(0, 0, (char *) speed_ss.str().c_str(), BLANCO);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.85, 0.87, 0);
    texto(0, 0, (char *) time_ss.str().c_str(), BLANCO);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.85, 0.82, 0);
    texto(0, 0, (char *) distance_ss.str().c_str(), BLANCO);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(0.85, 0.77, 0);
    texto(0, 0, (char *) fps_ss.str().c_str(), BLANCO);
    glPopMatrix();
}

void showBike() {
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    gluLookAt(0, 0, 0, 0, 0, -1, 0, 1, 0);
    
    renderWindArrow();

    setBikeTexture();
    if (camera_mode == PLAYER_VIEW) {
        float v0[3] = { -0.7, -1.05, 0 };
        float v1[3] = {  0.7, -1.05, 0 };
        float v2[3] = {  0.5,     0, 0 };
        float v3[3] = { -0.5,     0, 0 };
        
        quadtex(v0, v1, v2, v3);
    }
    else if (camera_mode == THIRD_PERSON_VIEW) {
        float v0[3] = { -0.3, -1.2, 0 };
        float v1[3] = {  0.3, -1.2, 0 };
        float v2[3] = {  0.3, -0.2, 0 };
        float v3[3] = { -0.3, -0.2, 0 };
        
        quadtex(v0, v1, v2, v3);

    }
    else if (camera_mode == BIRDS_EYE_VIEW && outsideTunnel(position[Z])) {
        float pov_to_bev_factor = 13;
        float v0[3] = { -0.7f / pov_to_bev_factor, -2 / pov_to_bev_factor, 0 };
        float v1[3] = {  0.7f / pov_to_bev_factor, -2 / pov_to_bev_factor, 0 };
        float v2[3] = {  0.5f / pov_to_bev_factor,  0 / pov_to_bev_factor, 0 };
        float v3[3] = { -0.5f / pov_to_bev_factor,  0 / pov_to_bev_factor, 0 };

        quadtex(v0, v1, v2, v3);
    }
    
    showHUD();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

}

/********************************* CALLBACKS *********************************/
void init() {

    // Mode initialization
    draw_mode = GL_FILL;
    camera_mode = PLAYER_VIEW;

    loadTextures();

    setupLighting();

    createRain();

	glClearColor(0, 0, 0, 1);

    GLfloat fog_color[]={ 0.4, 0.4, 0.4, 0.6}; // Color de la niebla
    glFogfv(GL_FOG_COLOR, fog_color);
    glFogf(GL_FOG_DENSITY, 0.05);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT2);
    glEnable(GL_LIGHT3);
    glEnable(GL_LIGHT4);
    glEnable(GL_LIGHT5);

    showControls();
}

void display() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

    // Camera-dependent elements
    configureMoonlight();
    configureHeadlight();
    
    if (camera_mode == THIRD_PERSON_VIEW) {
        gluLookAt(
               position[X], THIRD_PERSON_Y, position[Z], 
               position[X] + velocity[X], THIRD_PERSON_Y/2 + velocity[Y], position[Z] + velocity[Z], 
               0, 1, 0
            );

    }
    else {
        gluLookAt(
               position[X], position[Y], position[Z], 
               position[X] + velocity[X], velocity[Y], position[Z] + velocity[Z], 
               0, 1, 0
            );
    }
   
    // Camera-independent elements
    displayRoad(RENDER_DISTANCE);
    renderSkyline(RENDER_DISTANCE);
    renderGround(RENDER_DISTANCE);
    if (weather_mode == RAINFALL) {
        updateAndRenderRain();
    }
    
    if (axis_mode == AXIS_ON)
        ejes();

    if (hud_mode == HUD_ON) {
        showBike();
    }


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
    
    float nextX = position[X] + displacement * velocity[X]; 
    float nextZ = position[Z] + displacement * velocity[Z]; 


    if (collision_mode == COLLISIONS) {
        // Only check if inside road (will be used for points)
        if (insideRoadBorder(nextX, nextZ)) {

            position[X] += displacement * velocity[X];
            position[Z] += displacement * velocity[Z];
        }
        else {
            if (speed){
                if (speed > 2)
                    speed /= 2;
                else
                    speed = 0;
            }

	        displacement = elapsed * speed;

            if (position[X] < road_tracing(position[Z])){
                position[X] += displacement * (road_tracing(position[Z]))/5;
            }
            else {
                position[X] += displacement * -(road_tracing(position[Z]))/5;
            }
        }
    }
    else {
        position[X] += displacement * velocity[X];
        position[Z] += displacement * velocity[Z];
    }


	glutPostRedisplay();
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

}

void onKey(unsigned char key, int x, int y) {
	switch (key) {
        case 's':
        case 'S':
            draw_mode = (draw_mode == GL_LINE) ? GL_FILL : GL_LINE;
            if (draw_mode == GL_LINE) {
                glDisable(GL_TEXTURE_2D);
            }
            else {
                glEnable(GL_TEXTURE_2D);
            }

            break;

        case 'c':
        case 'C':
            hud_mode = (hud_mode == HUD_ON) ? HUD_OFF : HUD_ON;
            break;

        case 'e':
        case 'E':
            axis_mode = (axis_mode == AXIS_ON) ? AXIS_OFF : AXIS_ON;
            break;

        case 'n':
        case 'N':
            if (glIsEnabled(GL_FOG)){ 
                glDisable(GL_FOG);
            }
            else {
                glEnable(GL_FOG);
            }
            break;

        case 'l':
        case 'L':
            if (glIsEnabled(GL_LIGHTING)){
                glDisable(GL_LIGHTING);
            }
            else {
                glEnable(GL_LIGHTING);
            }
            break;

        case 'd':
        case 'D':
            collision_mode = (collision_mode == COLLISIONS) ? NO_COLLISIONS : COLLISIONS;
            break;

        case 'y':
        case 'Y':
            changeRainVelocity();    
            break;

        case 'w':
        case 'W':
            weather_mode = (weather_mode == RAINFALL) ? CLEAR : RAINFALL;
            break;

        case 'p':
        case 'P':
            switch (camera_mode) {
                case PLAYER_VIEW:
                    camera_mode = THIRD_PERSON_VIEW;
                    if (!glIsEnabled(GL_LIGHT1))
                        glEnable(GL_LIGHT1);
                    break;
                case THIRD_PERSON_VIEW:
                    camera_mode = BIRDS_EYE_VIEW;
                    position[Y] = BIRDS_EYE_Y;
                    if (glIsEnabled(GL_LIGHT1))
                        glDisable(GL_LIGHT1);
                    break; 
                default:
                    camera_mode = PLAYER_VIEW;
                    position[Y] = PLAYER_Y;
                    if (!glIsEnabled(GL_LIGHT1))
                        glEnable(GL_LIGHT1);
                    break;
            }

            break;

        case 27: // esc
            exit(0);
	}
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
