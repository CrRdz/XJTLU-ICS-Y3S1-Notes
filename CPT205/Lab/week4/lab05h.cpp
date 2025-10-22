// Drawing a house with wireframe polygons and with filled polygons 
// Lab05_Drawing_a_house.cpp 
#define FREEGLUT_STATIC 
#include <stdlib.h> 
#include <stdio.h> 
#include <math.h> 
#include <GL/freeglut.h> 
void define_to_OpenGL();
bool flag;  // flag for wireframe or filled house 
void when_in_mainloop()  // idle callback function 
{
	glutPostRedisplay();  // force OpenGL to redraw the current window 
}
void keyboard_input(unsigned char key, int x, int y)  // keyboard interaction 
{
	if (key == 'q' || key == 'Q')
	{
		exit(0);
	}
	else if (key == 'f' || key == 'F')
	{
		//press ¡®f¡¯ to toggle polygons drawn in between ¡®line¡¯ and ¡®fill¡¯ modes 
		if (flag == false)
			flag = true;
		else
			flag = false;
	}
}

void define_to_OpenGL()
{
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	if (flag == false)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else if (flag == true)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//This draws reference axes 
	glColor3f(0.0, 0.0, 0.0);
	glLineStipple(2, 0x5555);
	glEnable(GL_LINE_STIPPLE);

	glBegin(GL_LINES);
	glVertex2f(-1.0, 0.0);
	glVertex2f(1.0, 0.0);
	glEnd();
	glBegin(GL_LINES);
	glVertex2f(0.0, -1.0);
	glVertex2f(0.0, 1.0);
	glEnd();
	glDisable(GL_LINE_STIPPLE);

	//The wall 
	glColor3f(153.0 / 255.0, 51.0 / 255.0, 102.0 / 255.0);
	glBegin(GL_POLYGON);
	glVertex2f(-0.5, 0);
	glVertex2f(-0.5, 0.5);
	glVertex2f(-0.25, 0.5);
	glVertex2f(-0.25, 0);
	glEnd();

	//The roof 
	glColor3f(255.0 / 255.0, 66.0 / 255.0, 14.0 / 255.0);
	glBegin(GL_POLYGON);
	glVertex2f(-0.9, 0);
	glVertex2f(0, 0.5);
	glVertex2f(0.9, 0);
	glEnd();

	//The chimney 
	glColor3f(153.0 / 255.0, 204.0 / 255.0, 255.0 / 255.0);
	glBegin(GL_POLYGON);
	glVertex2f(-0.5, -0.9);
	glVertex2f(-0.5, 0);
	glVertex2f(0.5, 0);
	glVertex2f(0.5, -0.9);
	glEnd();

	//The window glass 
	float width = 0.1;
	float height = 0.1;

	glColor3f(255.0 / 255.0, 204.0 / 255.0, 153.0 / 255.0);
	float x_shift = -0.3;
	float y_shift = -0.3;
	glBegin(GL_POLYGON);
	glVertex2f(-width + x_shift, -height + y_shift);
	glVertex2f(-width + x_shift, height + y_shift);
	glVertex2f(width + x_shift, height + y_shift);
	glVertex2f(width + x_shift, -height + y_shift);
	glEnd();

	x_shift = -0.1;
	y_shift = -0.3;
	glBegin(GL_POLYGON);
	glVertex2f(-width + x_shift, -height + y_shift);
	glVertex2f(-width + x_shift, height + y_shift);
	glVertex2f(width + x_shift, height + y_shift);
	glVertex2f(width + x_shift, -height + y_shift);
	glEnd();

	x_shift = -0.3;
	y_shift = -0.5;
	glBegin(GL_POLYGON);
	glVertex2f(-width + x_shift, -height + y_shift);
	glVertex2f(-width + x_shift, height + y_shift);
	glVertex2f(width + x_shift, height + y_shift);
	glVertex2f(width + x_shift, -height + y_shift);
	glEnd();

	x_shift = -0.1;
	y_shift = -0.5;
	glBegin(GL_POLYGON);
	glVertex2f(-width + x_shift, -height + y_shift);
	glVertex2f(-width + x_shift, height + y_shift);
	glVertex2f(width + x_shift, height + y_shift);
	glVertex2f(width + x_shift, -height + y_shift);
	glEnd();

	//The windows edge 
	glColor3f(0.0, 0.0, 0.0);
	x_shift = 0.3;
	y_shift = -0.3;
	glBegin(GL_LINES);
	glVertex2f(-width - x_shift, -height + y_shift);
	glVertex2f(0.0, -height + y_shift);
	glEnd();
	glBegin(GL_LINES);
	glVertex2f(width - x_shift, height + y_shift);
	glVertex2f(width - x_shift, -height * 3 + y_shift);
	glEnd();
	//The door 
	glColor3f(102.0 / 255.0, 51.0 / 255.0, 0.0 / 255.0);
	glBegin(GL_POLYGON);
	glVertex2f(0.1, -0.9);
	glVertex2f(0.1, -0.2);
	glVertex2f(0.4, -0.2);
	glVertex2f(0.4, -0.9);
	glEnd();
	glFlush();
}
int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutCreateWindow("My House");
	flag = false;
	glutIdleFunc(when_in_mainloop);
	glutDisplayFunc(define_to_OpenGL);
	glutKeyboardFunc(keyboard_input);
	glutMainLoop();
}