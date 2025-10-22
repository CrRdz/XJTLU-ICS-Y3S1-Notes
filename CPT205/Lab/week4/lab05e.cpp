// Lab05_Projection_Functions.cpp
// A teapot for viewing
#define FREEGLUT_STATIC
#include <math.h>
#include <iostream>
#include <gl/freeglut.h>
void myDisplay(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(1.0, 1.0, 1.0);
	glutWireTeapot(2); // draw a wireframe teapot
	glFlush();
}
void myinit(void)
{
	glShadeModel(GL_FLAT); // Flat shading specified
}
void myReshape(GLsizei w, GLsizei h)
{
	glViewport(0, 0, w, h); // define viewport
	glMatrixMode(GL_PROJECTION); // projection transformations
	glLoadIdentity(); // clear the projection matrix
	glFrustum(-1.0, 1.0, -1.0, 1.0, 1.5, 20.0); // set the perspective frustum
	
	// Task 5
	//glFrustum(0, 1.0, 0, 1.0, 1.5, 20.0);

	glMatrixMode(GL_MODELVIEW); // back to model-view matrix to define camera
	glLoadIdentity(); // clear the model-view matrix
	gluLookAt(0, 0, 5, 0, 0, 0, 0, 1, 0); // set up camera / viewing coordinate system

	// Task 2
	//gluLookAt(0, 0, 5, 0, 0, 0, 0, -1, 0); 
	
    // Task 3
	//gluLookAt(-5, 5, 0, 0, 0, 0, 0, 1, 0);

	//Task 4
	//gluLookAt(0, 5, 0, 0, 0, 0, 0, 0, -1);

	//Task 6
	//glRotatef(90, 1, 0, 0);
}
int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
	glutInitWindowSize(500, 500);
	glutCreateWindow("Teapot");
	myinit();
	glutReshapeFunc(myReshape);
	glutDisplayFunc(myDisplay);
	glutMainLoop();
}
