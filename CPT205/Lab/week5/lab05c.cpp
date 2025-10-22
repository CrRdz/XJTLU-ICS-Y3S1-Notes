// Zoom and Pan 

#define FREEGLUT_STATIC 
#include <GL/freeglut.h> 

//Hide the terminal windows 
#pragma comment (linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"") 

// XYZ position of the camera 
float x = 0.0f, y = 1.0f, z = 5.0f;

void changeSize(int w, int h)
{
	// Prevent a divide by zero, when window is too short 
	if (h == 0)
		h = 1;
	float ratio = w * 1.0 / h;

	// Use the Projection Matrix 
	glMatrixMode(GL_PROJECTION);

	// Reset Matrix 
	glLoadIdentity();

	// Set the viewport to be the entire window 
	glViewport(0, 0, w, h);

	// Set the correct perspective. 
	gluPerspective(45.0f, ratio, 0.1f, 100.0f);

	// Get Back to the Modelview 
	glMatrixMode(GL_MODELVIEW);
}

// Draw a simple man body 
void drawMan()
{
	glColor3f(1.0f, 1.0f, 1.0f);
	// Draw Body 
	glTranslatef(0.0f, 0.75f, 0.0f);
	glutSolidSphere(0.75f, 20, 20); // radius, lines of longitude and lines of latitude 

	// Draw Head 
	glTranslatef(0.0f, 1.0f, 0.0f);
	glutSolidSphere(0.25f, 20, 20);
}

void renderScene(void)
{
	// Clear Color and Depth Buffers 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// Reset transformations 
	glLoadIdentity();
	// Set the camera 
	gluLookAt(x, y, z, x, y, z - 1.0, 0.0f, 1.0f, 0.0f);

	glPushMatrix();
	glTranslatef(0.0, 0.0, 0.0);
	drawMan();
	glPopMatrix();

	glutSwapBuffers();
}

void processNormalKeys(unsigned char key, int x, int y)
{

	//Press ESC quit 
	if (key == 27)
		exit(0);
}

void processSpecialKeys(int key, int xx, int yy)
{
	float fraction = 0.2f;

		switch (key)
		{
			//Camera move left 
		case GLUT_KEY_LEFT:
			x -= fraction;
			break;
			//Camera move right 
		case GLUT_KEY_RIGHT:
			x += fraction;
			break;
			// Camera move up 
		case GLUT_KEY_UP:
			y += fraction;
			break;
			// Camera move down 
		case GLUT_KEY_DOWN:
			y -= fraction;
			break;
			// Zoom out 
		case GLUT_KEY_PAGE_UP:
			z += fraction;
			break;
			// Zoom in 
		case GLUT_KEY_PAGE_DOWN:
			z -= fraction;
			break;
		}
}

int main(int argc, char** argv)
{
	// init GLUT and create window 
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(320, 320);
	glutCreateWindow("Movement");

	// register callbacks 
	glutDisplayFunc(renderScene);
	glutReshapeFunc(changeSize);
	glutIdleFunc(renderScene);  // draw content when there is no window event  
	glutKeyboardFunc(processNormalKeys);
	glutSpecialFunc(processSpecialKeys);

	// OpenGL init 
	glEnable(GL_DEPTH_TEST);

	// enter GLUT event processing cycle 
	glutMainLoop();

	return 1;
}