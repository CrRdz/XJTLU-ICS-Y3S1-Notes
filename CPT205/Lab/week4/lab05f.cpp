// Lab05_Text_and_Transformations.cpp 

#define FREEGLUT_STATIC 
#include <math.h> 
#include <GL/freeglut.h> 
#include <iostream> 
#include "windows.h" 
#define MAX_CHAR 128 

GLdouble x_p = 0;  // parameter to define camera position in x-direction 
GLdouble z_p = 6;  // parameter to define camera position in z-direction 
int flag = 1;  // flag for value ranges of x_p and z_p 

void drawString(const char* str)
{
    static int isFirstCall = 1;  // lazy init flag 
    static GLuint lists;   //base id of the generated display lists 

    if (isFirstCall)
    {
        isFirstCall = 0;
        lists = glGenLists(MAX_CHAR);  // reserve MAX_CHAR consecutive list IDs 
        // Build bitmap glyph lists for the font currently selected into the DC. The parameters represent: 
        // 1) Device Context, 2) first glyph index, 3) number of glyph to build, 4) base list id.
        wglUseFontBitmaps(wglGetCurrentDC(), 0, MAX_CHAR, lists);
    }

    // render each character by calling its display list 
    for (; *str != '\0'; ++str)
    {
        glCallList(lists + *str);
    }
}

// Create and select a GDI font into the current Device Context (used by wglUseFontBitmap). 
// Note: selectFont should be called before the first drawString so that glyph lists are built 
//        for the intended typeface/size/charset. 
void selectFont(int size, int charset, const char* face)
{
    // Create a logical GDI font and select it into the current device context 
    HFONT hFont = CreateFontA(size, 0, 0, 0, FW_MEDIUM, 0, 0, 0, charset, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, face);
    HFONT hOldFont = (HFONT)SelectObject(wglGetCurrentDC(), hFont);  // activate new font in DC 
    DeleteObject(hOldFont);  // discard the previous font object 
}

void myDisplay(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1.0, 1.0, 1.0);

    glMatrixMode(GL_PROJECTION);  //projection transformation 
    glLoadIdentity();// clear the matrix 
    glFrustum(-1.0, 1.0, -1.0, 1.0, 1.5, 20.0);
    gluLookAt(x_p, 0, z_p, 0, 0, 0, 0, 1, 0);

    glMatrixMode(GL_MODELVIEW);  // back to modelview matrix 
    glLoadIdentity();  // clear the matrix 

    glColor3f(1.0, 1.0, 1.0);
    //draw Torus 
    glPushMatrix();
    glTranslated(1.5, -1.0, 0.0);
    glutWireTorus(0.5, 1, 12, 12);  // inner radius, out radius, number of sides for each  
    glPopMatrix();       // radial section, number of radial divisions 

    glPushMatrix();
    glTranslated(-1.5, -1.0, 0.0);
    glutSolidTorus(0.5, 1, 12, 12);
    glPopMatrix();

    //Draw 2D text 
    selectFont(48, ANSI_CHARSET, "Comic Sans MS");
    glRasterPos2f(0.0, 1.0);
    drawString("Torus");
   

    glFlush();
}

void myinit(void)
{
    glShadeModel(GL_FLAT);
}

void myReshape(GLsizei w, GLsizei h)
{
    glViewport(0, 0, w, h);  // define the viewport 
}

void mouse_input(int button, int state, int x, int y)  // mouse interaction 
{
    /*
         * Mouse handler:
         * On each LEFT click, move the camera to the next point (x_p, z_p) on the
         * circle x_p^2 + z_p^2 = 36 in the XZ plane (R = 6). We keep x_p in steps of 1
         * and recompute z_p = ±sqrt(36 - x_p^2). The sign is controlled by 'flag'
         * to traverse the full circle in four arcs:
         *   flag = 1 : z >= 0,  x : 0 to +6  (upper-right arc)
         *   flag = 2 : z <= 0,  x : +6 to 0   (lower-right arc)
         *   flag = 3 : z <= 0,  x : 0 to -6  (lower-left  arc)
         *   flag = 4 : z >= 0,  x : -6 to 0   (upper-left  arc)
         */
    if (state == GLUT_DOWN && button == GLUT_LEFT_BUTTON)
    {
        if (x_p < 6 && x_p >= 0 && flag == 1)  // Quadrant I:  x: 0 to +6,  z ≥ 0 
        {
            x_p = x_p + 1;
            z_p = sqrt(36 - x_p * x_p);
        }
        else if (x_p == 6 && flag == 1)  // Hit (+6,0): switch to lower arc 
        {
            x_p = x_p - 1;
            z_p = -sqrt(36 - x_p * x_p);
            flag = 2;
        }
        else if (x_p < 6 && x_p>0 && flag == 2)   // Quadrant IV: x: +6 to 0,  z ≤ 0  
        {
            x_p = x_p - 1;
            z_p = -sqrt(36 - x_p * x_p);
        }
        else if (x_p == 0 && flag == 2)  // Cross x=0: continue to negative side 
        {
            x_p = x_p - 1;
            z_p = -sqrt(36 - x_p * x_p);
            flag = 3;
        }
        else if (x_p > -6 && x_p < 0 && flag == 3)  // Quadrant III: x: 0 to −6, z ≤ 0  
        {
            x_p = x_p - 1;
            z_p = -sqrt(36 - x_p * x_p);
        }
        else if (x_p == -6 && flag == 3)  // Hit (−6,0): switch back to upper arc 
        {
            x_p = x_p + 1;
            z_p = sqrt(36 - x_p * x_p);
            flag = 4;
        }
        else if (x_p > -6 && x_p < 0 && flag == 4)   // Quadrant II: x: −6 to 0, z ≥ 0 
        {
            x_p = x_p + 1;
            z_p = sqrt(36 - x_p * x_p);
        }
        else if (x_p == 0 && flag == 4)   // Completed a loop: reset to state 1s 
        {
            x_p = x_p + 1;
            z_p = sqrt(36 - x_p * x_p);
            flag = 1;
        }
        glutPostRedisplay();
    }
}
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
    glutInitWindowSize(500, 500);
    glutCreateWindow("Text and Transformations");
    myinit();
    glutReshapeFunc(myReshape);
    glutDisplayFunc(myDisplay);
    glutMouseFunc(mouse_input);
    glutMainLoop();
}