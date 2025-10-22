// Lab05_Further_transformations.cpp 

#define FREEGLUT_STATIC 
#include <stdlib.h> 
#include <stdio.h> 
#include <math.h> 
#include <GL/freeglut.h> 
#include <iostream> 

using namespace std;
float r = 0;

//Task 2 
GLfloat step = 1;  // declare step 
int time_interval = 16;  // declare refresh interval in ms 

void when_in_mainloop()  // idle callback function 
{
	glutPostRedisplay();  // force OpenGL to redraw the current window 
}

void OnTimer(int value)
{
	r += step;
	if (r >= 360)
		r = 0;
	else if (r <= 0)
		r = 359;

	glutTimerFunc(time_interval, OnTimer, 1);
}

//Task 3 
void keyboard_input(unsigned char key, int x, int y) // keyboard interaction 
{
	if (key == 'q' || key == 'Q')
		exit(0);
	else if (key == 'f' || key == 'F')// change direction of movement 
		step = -step;
	else if (key == 's' || key == 'S')// stop 
		step = 0;
	else if (key == 'r' || key == 'R')// set step 
		step = 10;
}

//Task 4 
void mouse_input(int button, int state, int x, int y)  // mouse interaction 
{
	if (state == GLUT_DOWN && button == GLUT_LEFT_BUTTON && step >= -15)
		step -= 1;  // decrement step 
	else if (state == GLUT_DOWN && button == GLUT_RIGHT_BUTTON && step <= 15)
		step += 1;  // increment step 
}

void display(void)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 600, 0, 400);

	glClearColor(0, 0, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	// draw a vertical line in white 
	glPushMatrix();
	glTranslatef(300, 0, 0);
	glColor3f(1, 1, 1);
	glBegin(GL_LINES);
	glVertex2f(0, 50);
	glVertex2f(0, 200);
	glEnd();
	glPopMatrix();


	// draw a rectangle in green 
	glPushMatrix();
	glTranslatef(0, 50, 0);
	glScalef(1, 2, 1);
	glColor3f(0, 1, 0);
	glBegin(GL_POLYGON);
	glVertex2f(0, 0);
	glVertex2f(0, -25);
	glVertex2f(600, -25);
	glVertex2f(600, 0);
	glEnd();
	glPopMatrix();

	// draw a windmill (4 triangles spaced 90бу apart from each other) in red 
	glPushMatrix();  // save the current model-view matrix (before 1st triangle) 
	glColor3f(1, 0, 0);
	glTranslatef(300, 200, 0);
	glRotatef(r, 0, 0, 1);
	glBegin(GL_TRIANGLES);
	glVertex2f(0, 0);
	glVertex2f(0, 30);
	glVertex2f(10, 18);
	glEnd();
	glPushMatrix();  // save the current model-view matrix (1st triangle) 
	glRotatef(90, 0, 0, 1);
	glBegin(GL_TRIANGLES);
	glVertex2f(0, 0);
	glVertex2f(0, 30);
	glVertex2f(10, 18);
	glEnd();
	glPushMatrix();  // save the current model-view matrix (2nd triangle) 
	glRotatef(90, 0, 0, 1);
	glBegin(GL_TRIANGLES);
	glVertex2f(0, 0);
	glVertex2f(0, 30);
	glVertex2f(10, 18);
	glEnd();
	glPushMatrix();  // save the current model-view matrix (3rd triangle) 
	glRotatef(90, 0, 0, 1);
	glBegin(GL_TRIANGLES);
	glVertex2f(0, 0);
	glVertex2f(0, 30);
	glVertex2f(10, 18);
	glEnd();
	glPopMatrix();  // discard the current model-view matrix (return to 3rd triangle) 
	glPopMatrix();  // discard the current model-view matrix (return to 2nd triangle) 
	glPopMatrix();  // discard the current model-view matrix (return to 1st triangle) 
	glPopMatrix();  // discard the current model-view matrix (return to before 1st triangle) 

	glutSwapBuffers();
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(600, 400);
	glutCreateWindow("My Interactive Window");

	glutDisplayFunc(display);

	//Task 2 
	glutIdleFunc(when_in_mainloop);
	glutTimerFunc(time_interval, OnTimer, 1);

	//Task 3 
	glutKeyboardFunc(keyboard_input);

	//Task 4 
	glutMouseFunc(mouse_input);

	glutMainLoop();
}