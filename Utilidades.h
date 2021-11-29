#ifndef UTILIDADES
#define UTILIDADES V1.0
/* 
	Refactory general ..... R.Vivo',2013
	Ampliacion colores .... R.Vivo',2014
	Bugs rad() y deg() .... R.Vivo',2014
	Texto en WCS y DCS .... R.Vivo',2015
*/

#include <iostream>
#include <cmath>
#include <GL/freeglut.h>
#include <GL/glext.h>

using namespace std;

#define PI 3.1415926
#define signo(a) (a<0?-1:1)
#define deg(a) (a*180/PI)
#define rad(a) (a*PI/180)

const GLfloat BLANCO[] = {1,1,1,1};
const GLfloat NEGRO[]  = {0,0,0,1};
const GLfloat GRISCLARO[]  = {0.7,0.7,0.7,1};
const GLfloat GRISOSCURO[]  = {0.2,0.2,0.2,1};
const GLfloat ROJO[]   = {1,0,0,1};
const GLfloat VERDE[]  = {0,1,0,1};
const GLfloat AZUL[]   = {0,0,1,1};
const GLfloat AMARILLO[] = {1,1,0,1};
const GLfloat BRONZE[] = {140.0/255,120.0/255,83.0/255,1};
const GLfloat BRONCE[] = {140.0/255,120.0/255,83.0/255,1};
const GLfloat MARINO[] = {0,0,0.5,1};
const GLfloat ORO[] = {218.0/255,165.0/255,32.0/255,1};


void quad(GLfloat v0[3], GLfloat v1[3], GLfloat v2[3], GLfloat v3[3], int M = 10, int N = 10);
/* v0,v1,v2,v3: vertices del quad
   NxM: resolucion opcional (por defecto 10x10)
   Dibuja el cuadrilatero de entrada con resolucion MxN y normales. 
   Se asume counterclock en la entrada                            */

void ejes();
/* Dibuja unos ejes de longitud 1 y una esferita en el origen */

/********** IMPLEMENTACION ************************************************************************************************/

void quad(GLfloat v0[3], GLfloat v1[3], GLfloat v2[3], GLfloat v3[3], int M, int N)
// Dibuja un cuadrilatero con resolucion MxN y fija la normal 
{
	if(M<1) M=1; if(N<1) N=1;	// Resolucion minima
	GLfloat ai[3], ci[3], bj[3], dj[3], p0[3], p1[3];
	// calculo de la normal (v1-v0)x(v3-v0) unitaria 
	GLfloat v01[] = { v1[0]-v0[0], v1[1]-v0[1], v1[2]-v0[2] };
	GLfloat v03[] = { v3[0]-v0[0], v3[1]-v0[1], v3[2]-v0[2] };
	GLfloat normal[] = { v01[1]*v03[2] - v01[2]*v03[1] ,
						 v01[2]*v03[0] - v01[0]*v03[2] ,
						 v01[0]*v03[1] - v01[1]*v03[0] };
	float norma = sqrt( normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2] );
	glNormal3f( normal[0]/norma, normal[1]/norma, normal[2]/norma );
	// ai: punto sobre el segmento v0v1, bj: v1v2, ci: v3v2, dj: v0v3
	for(int i=0; i<M; i++){
		// puntos sobre segmentos a y c
		for(int k=0; k<3; k++){ 
			ai[k] = v0[k] + i*(v1[k]-v0[k])/M;
			ci[k] = v3[k] + i*(v2[k]-v3[k])/M;
		}
		// strip vertical. i=s, j=t
		glBegin(GL_QUAD_STRIP);
		for(int j=0; j<=N; j++){
			for(int k=0; k<3; k++){
				// puntos sobre los segmentos b y d
				bj[k] = v1[k] + j*(v2[k]-v1[k])/N;
				dj[k] = v0[k] + j*(v3[k]-v0[k])/N;

				// p0= ai + j/N (ci-ai)
				p0[k] = ai[k] + j*(ci[k]-ai[k])/N;
				// p1= p0 + 1/M (bj-dj)
				p1[k] = p0[k] + (bj[k]-dj[k])/M;
			}
			// punto izquierdo
			glTexCoord2f(i*1.0f/M, j*1.0f/N);  // s,t
			glVertex3f(p0[0],p0[1],p0[2]);
			// punto derecho
			glTexCoord2f((i+1)*1.0f/M, j*1.0f/N);
			glVertex3f(p1[0],p1[1],p1[2]);
		}
		glEnd();
	}
}
void ejes()
{
    //Construye la Display List compilada de una flecha vertical
    GLuint id = glGenLists(1);
    glNewList(id,GL_COMPILE);			
        //Brazo de la flecha
        glBegin(GL_LINES);
            glVertex3f(0,0,0);
            glVertex3f(0,1,0);
        glEnd();
        //Punta de la flecha
        glPushMatrix();
        glTranslatef(0,1,0);
        glRotatef(-90,1,0,0);
        glTranslatef(0.0,0.0,-1/10.0);
		glutSolidCone(1/50.0,1/10.0,10,1);
        glPopMatrix();
    glEndList();						

    //Ahora construye los ejes
	glPushAttrib(GL_ENABLE_BIT|GL_CURRENT_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
    //Eje X en rojo
    glColor3fv(ROJO);
    glPushMatrix();
    glRotatef(-90,0,0,1);
    glCallList(id);
    glPopMatrix();
    //Eje Y en verde
    glColor3fv(VERDE);
    glPushMatrix();
    glCallList(id);
    glPopMatrix();
    //Eje Z en azul
    glColor3fv(AZUL);
    glPushMatrix();
    glRotatef(90,1,0,0);
    glCallList(id);
    glPopMatrix();
    //Esferita en el origen
    glColor3f(0.5,0.5,0.5);
	glutSolidSphere(0.05,8,8);
	glPopAttrib();
	//Limpieza
	glDeleteLists(id,1);
}

#endif
