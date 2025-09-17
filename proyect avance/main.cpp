#include <GL/glut.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <sstream>
#include <fstream>

using namespace std;

// Estructuras de datos
struct Point {
    int x, y;
    Point(int x = 0, int y = 0) : x(x), y(y) {}
};

struct Figure {
    int type; // 0: línea directa, 1: línea DDA, 2: círculo incremental, 3: círculo punto medio, 4: elipse
    vector<Point> points;
    float color[3];
    int thickness;
};

// Variables globales
extern int windowWidth;
extern int windowHeight;
extern bool showGrid;
extern bool showAxes;
extern bool showCoords;
extern bool showHelp;
extern bool showAbout;
extern int gridSpacing;
extern float currentColor[3];
extern int currentThickness;
extern int drawingMode;
extern vector<Point> currentPoints;
extern vector<Figure> figures;
extern vector<vector<Figure>> history;
extern int historyIndex;

// Prototipos de funciones
void drawGrid();
void drawAxes();
void drawPixel(int x, int y, float r, float g, float b);
void drawLineDirect(int x1, int y1, int x2, int y2);
void drawLineDDA(int x1, int y1, int x2, int y2);
void drawCircleIncremental(int xc, int yc, int radius);
void drawCircleMidPoint(int xc, int yc, int radius);
void drawEllipseMidPoint(int xc, int yc, int rx, int ry);
void display();
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void keyboard(unsigned char key, int x, int y);
void createMenu();
void menu(int option);
void colorMenu(int option);
void thicknessMenu(int option);
void viewMenu(int option);
void toolsMenu(int option);
void helpMenu(int option);
void saveToHistory();
void drawText(float x, float y, const string& text);
void exportPPM(const string& filename);
void showHelpDialog();
void showAboutDialog();

// Función principal
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("CAD 2D Basico - Proyecto DMV");

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, 0, windowHeight);
    glMatrixMode(GL_MODELVIEW);

    createMenu();
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(keyboard);

    glClearColor(1.0, 1.0, 1.0, 1.0); // Fondo blanco
    glutMainLoop();

    return 0;
}
