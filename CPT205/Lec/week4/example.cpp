#define FREEGLUT_STATIC
#include <gl/freeglut.h>

GLint winWidth = 600, winHeight = 600; // initial display window size

GLfloat x0 = 100.0, y0 = 50.0, z0 = 50.0; // viewing co-ordinate origin
GLfloat xref = 50.0, yref = 50.0, zref = 0.0; // look-at point
GLfloat Vx = 0.0, Vy = 1.0, Vz = 0.0; // view-up vector

// co-ordinate limits for clipping window
GLfloat xwMin = -40.0, ywMin = -60.0, xwMax = 40.0, ywMax = 60.0;

GLfloat dnear = 25.0, dfar = 125.0; // positions for near and far clipping planes

void init(void) {
	glClearColor(1.0, 1.0, 1.0, 0.0);

	// decides which matrix is to be affected by subsequent transformation functions
	glMatrixMode(GL_MODELVIEW);
	// define camera/viewing coordinate system
	gluLookAt(x0, y0, z0, xref, yref, zref, Vx, Vy, Vz);

	glMatrixMode(GL_PROJECTION); // projection transformations
	glFrustum(xwMin, xwMax, ywMin, ywMax, dnear, dfar); // define perspective frustum
}

void displayFcn(void) {
	glClear(GL_COLOR_BUFFER_BIT);

	// parameters for a square fill area
	glColor3f(1.0, 0.0, 0.0);
	glPolygonMode(GL_FRONT, GL_FILL); // fill in front face of polygon
	glPolygonMode(GL_BACK, GL_LINE); // keep back face of polygon as wireframe/line

	// square in the x-y plane
	glBegin(GL_QUADS);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(100.0, 0.0, 0.0);
	glVertex3f(100.0, 100.0, 0.0);
	glVertex3f(0.0, 100.0, 0.0);
	glEnd();

	glFlush();
}
void reshapeFcn(GLint newWidth, GLint newHeight) {
	glViewport(0, 0, newWidth, newHeight); // Set viewport to window dimensions
}

int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	glutInitWindowPosition(50, 50);
	glutInitWindowSize(winWidth, winHeight);
	glutCreateWindow("Perspective View of a Square");
	init();
	glutDisplayFunc(displayFcn);
	glutReshapeFunc(reshapeFcn);
	glutMainLoop();
}