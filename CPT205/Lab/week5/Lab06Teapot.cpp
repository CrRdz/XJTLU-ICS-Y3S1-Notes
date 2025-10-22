#define FREEGLUT_STATIC
#include <GL/freeglut.h>
#include <Windows.h>

#include "Lab06Teapot.h"

GLfloat rotx = 0, roty = 0, rotz = 0;

static void teapot(GLint grid, GLfloat scale, GLenum type)
{
	float p[4][4][3], q[4][4][3], r[4][4][3], s[4][4][3];
	long i, j, k, l;

	glPushAttrib(GL_ENABLE_BIT | GL_EVAL_BIT);
	glEnable(GL_AUTO_NORMAL);
	glEnable(GL_NORMALIZE);
	glEnable(GL_MAP2_VERTEX_3);
	glEnable(GL_MAP2_TEXTURE_COORD_2);
	
	glPushMatrix();
	glRotatef(270.0, 1.0, 0.0, 0.0);
	glScalef(0.5f * scale, 0.5f * scale, 0.5f * scale);
	glTranslatef(0.0, 0.0, -1.5);
	for (i = 0; i < 10; i++)
	{
		for (j = 0; j < 4; j++)
		{
			for (k = 0; k < 4; k++)
			{
				for (l = 0; l < 3; l++)
				{
					p[j][k][l] = cpdata[patchdata[i][j * 4 + k]][l];
					q[j][k][l] = cpdata[patchdata[i][j * 4 + (3 - k)]][l];
					if (l == 1)
					{
						q[j][k][l] *= -1.0;
					}
					if (i < 6)
					{
						r[j][k][l] = cpdata[patchdata[i][j * 4 + (3 - k)]][l];
						if (l == 0)
						{
							r[j][k][l] *= -1.0;
						}
						s[j][k][l] = cpdata[patchdata[i][j * 4 + k]][l];
						if (l == 0)
						{
							s[j][k][l] *= -1.0;
						}
						if (l == 1)
						{
							s[j][k][l] *= -1.0;
						}
					}
				}
			}
		}
		glMap2f(GL_MAP2_TEXTURE_COORD_2, 0, 1, 2, 2, 0, 1, 4, 2, &tex[0][0][0]);
		glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4, &p[0][0][0]);
		glMapGrid2f(grid, 0.0, 1.0, grid, 0.0, 1.0);
		glEvalMesh2(type, 0, grid, 0, grid);
		glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4, &q[0][0][0]);
		glEvalMesh2(type, 0, grid, 0, grid);
		if (i < 6)
		{
			glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4, &r[0][0][0]);
			glEvalMesh2(type, 0, grid, 0, grid);
			glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4, &s[0][0][0]);
			glEvalMesh2(type, 0, grid, 0, grid);
		}
	}
	glPopMatrix();
	glPopAttrib();
}

void solidTeapot(GLfloat scale)
{
	teapot(14, scale, GL_FILL);
}

void wireTeapot(GLfloat scale)
{
	teapot(10, scale, GL_LINE);
}

void init()
{
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glShadeModel(GL_FLAT);
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT);
	glPushMatrix();
		glColor3f(1.0, 0.0f, 0.0f);
		glTranslatef(-1.0f, -1.0, 0.0);
		glRotatef(rotx, -1.0, 0.0, 0.0);
		glRotatef(roty, 0.0, -1.0, 0.0);
		glRotatef(rotz, 0.0, 0.0, -1.0);
		wireTeapot(1.0);
	glPopMatrix();
	glPushMatrix();
		glColor3f(0.0, 1.0f, 0.0f);
		glTranslatef(1.0f, 1.0, 0.0);
		glRotatef(360 - rotx, 1.0, 0.0, 0.0);
		glRotatef(360 - roty, 0.0, 1.0, 0.0);
		glRotatef(360 - rotz, 0.0, 0.0, 1.0);
		solidTeapot(1.0);
	glPopMatrix();

	glutSwapBuffers();
}
void reshape(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, (GLfloat)w / (GLfloat)h, 1.0, 20.0);
	gluLookAt(0.0, 0.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}
void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'x':
		rotx += 1.0;
		if (rotx >= 360)
		{
			rotx -= 360;
		}
		glutPostRedisplay();
		break;
	case 'X':
		rotx -= 1.0;
		if (rotx <= -360)
		{
			rotx += 360;
		}
		glutPostRedisplay();
		break;
	case 'y':
		roty += 1.0;
		if (roty >= 360)
		{
			roty -= 360;
		}
		glutPostRedisplay();
		break;
	case 'Y':
		roty -= 1.0;
		if (roty <= -360)
		{
			roty += 360;
		}
		glutPostRedisplay();
		break;
	case 'z':
		rotz += 1.0;
		if (rotz >= 360)
		{
			rotz -= 360;
		}
		glutPostRedisplay();
		break;
	case 'Z':
		rotz -= 1.0;
		if (rotz <= 360)
		{
			rotz += 360;
		}
		glutPostRedisplay();
		break;
	default:
		break;
	}
}
int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(500, 500);
	glutInitWindowPosition(100, 100);
	glutCreateWindow(argv[0]);
	init();
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMainLoop();
	return 0;
}